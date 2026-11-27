//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//  Copyright (c)      2022 Patrick Diehl
//  Copyright (c)      2023 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/config_registry.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/util.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

//
// AM functions
//
typedef enum
{
    SIGNAL = 136,          // ack to a done_t via gasnet_AMReplyShortM()
    SIGNAL_LONG,           // ack to a done_t via gasnet_AMReplyLongM()
    DO_REPLY_PUT = 143,    // do a PUT here from another locale
    DO_COPY_PAYLOAD        // copy AM payload to another address
} AM_handler_function_idx_t;

typedef struct
{
    void* ack;      // acknowledgement object
    void* tgt;      // target memory address
    void* src;      // source memory address
    size_t size;    // number of bytes.
} xfer_info_t;

// Gasnet AM handler arguments are only 32 bits, so here we have
// functions to get the 2 arguments for a 64-bit pointer,
// and a function to reconstitute the pointer from the 2 arguments.
//
static inline gasnet_handlerarg_t get_arg_from_ptr0(uintptr_t addr)
{
    // This one returns the bottom 32 bits.
    // clang-format off
    return ((gasnet_handlerarg_t) ((((uint64_t) (addr)) << 32UL) >> 32UL));
    // clang-format on
}
static inline gasnet_handlerarg_t get_arg_from_ptr1(uintptr_t addr)
{
    // this one returns the top 32 bits.
    // clang-format off
    return ((gasnet_handlerarg_t) (((uint64_t) (addr)) >> 32UL));
    // clang-format on
}
static inline uintptr_t get_uintptr_from_args(
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1)
{
    // clang-format off
    return (uintptr_t) (((uint64_t) (uint32_t) a0) |
        (((uint64_t) (uint32_t) a1) << 32UL));
    // clang-format on
}
static inline void* get_ptr_from_args(
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1)
{
    return (void*) get_uintptr_from_args(a0, a1);
}

// Build acknowledgement address arguments for gasnetAMRequest*() calls.
//
#define Arg0(addr) get_arg_from_ptr0((uintptr_t) addr)
#define Arg1(addr) get_arg_from_ptr1((uintptr_t) addr)

// The following macro is from the GASNet test.h distribution
//
#define GASNET_Safe(fncall)                                                    \
    do                                                                         \
    {                                                                          \
        int _retval;                                                           \
        if ((_retval = fncall) != GASNET_OK)                                   \
        {                                                                      \
            fprintf(stderr,                                                    \
                "ERROR calling: %s\n"                                          \
                " at: %s:%i\n"                                                 \
                " error: %s (%s)\n",                                           \
                #fncall, __FILE__, __LINE__, gasnet_ErrorName(_retval),        \
                gasnet_ErrorDesc(_retval));                                    \
            fflush(stderr);                                                    \
            gasnet_exit(_retval);                                              \
        }                                                                      \
    } while (0)

// This is the type of object we use to manage GASNet acknowledgements.
//
// Initialize the count to 0, the target to the number of return signal
// events you expect, and the flag to 0.  Fire the request, then do a
// BLOCKUNTIL(flag).  When all the return signals have occurred, the AM
// handler will set the flag to 1 and your BLOCKUNTIL will complete.
// (Note that the GASNet documentation says that GASNet code assumes
// the condition for a BLOCKUNTIL can only be changed by the execution
// of an AM handler.)
//
typedef struct
{
    std::atomic<std::uint32_t> count;
    std::uint32_t target;
    int volatile flag;
} done_t;

static void AM_signal([[maybe_unused]] gasnet_token_t token,
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1)
{
    done_t* done = reinterpret_cast<done_t*>(get_ptr_from_args(a0, a1));
    uint_least32_t prev;
    prev = done->count.fetch_add(1, std::memory_order_seq_cst);
    if (prev + 1 == done->target)
        done->flag = 1;
}

static void AM_signal_long([[maybe_unused]] gasnet_token_t token,
    [[maybe_unused]] void* buf, [[maybe_unused]] size_t nbytes,
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1)
{
    done_t* done = reinterpret_cast<done_t*>(get_ptr_from_args(a0, a1));
    uint_least32_t prev;
    prev = done->count.fetch_add(1, std::memory_order_seq_cst);
    if (prev + 1 == done->target)
        done->flag = 1;
}

// Put from arg->src (which is local to the AM handler) back to
// arg->dst (which is local to the caller of this AM).
// nbytes is < gasnet_AMMaxLongReply here (see chpl_comm_get).
//
static void AM_reply_put(
    gasnet_token_t token, void* buf, [[maybe_unused]] size_t nbytes)
{
    xfer_info_t* x = static_cast<xfer_info_t*>(buf);

    HPX_ASSERT(nbytes == sizeof(xfer_info_t));

    GASNET_Safe(gasnet_AMReplyLong2(token, SIGNAL_LONG, x->src, x->size, x->tgt,
        Arg0(x->ack), Arg1(x->ack)));
}

// Copy from the payload in this active message to dst.
//
static void AM_copy_payload(gasnet_token_t token, void* buf, size_t nbytes,
    gasnet_handlerarg_t ack0, gasnet_handlerarg_t ack1,
    gasnet_handlerarg_t dst0, gasnet_handlerarg_t dst1)
{
    void* dst = get_ptr_from_args(dst0, dst1);
    {
        // would prefer to protect the memory segments
        // associated with each node (n-node mutex)
        // will require future work
        //
        std::lock_guard<hpx::mutex> lk(hpx::util::gasnet_environment::dshm_mut);
        std::memcpy(dst, buf, nbytes);
    }

    GASNET_Safe(gasnet_AMReplyShort2(token, SIGNAL, ack0, ack1));
}

[[maybe_unused]] static gasnet_handlerentry_t ftable[] = {
    {SIGNAL, (void (*)()) &AM_signal},
    {SIGNAL_LONG, (void (*)()) &AM_signal_long},
    {DO_REPLY_PUT, (void (*)()) &AM_reply_put},
    {DO_COPY_PAYLOAD, (void (*)()) &AM_copy_payload}};

//
// Initialize one of the above.
//
static inline void init_done_obj(done_t* done, int target)
{
    done->count.store(0, std::memory_order_seq_cst);
    done->target = target;
    done->flag = 0;
}

static inline void am_poll_try()
{
    // Serialize polling for IBV, UCX, Aries, and OFI. Concurrent polling causes
    // contention in these configurations. For other configurations that are
    // AM-based (udp/amudp, mpi/ammpi) serializing can hurt performance.
    //
#if defined(GASNET_CONDUIT_IBV) || defined(GASNET_CONDUIT_UCX) ||              \
    defined(GASNET_CONDUIT_ARIES) || defined(GASNET_CONDUIT_OFI)
    std::lock_guard<hpx::spinlock> lk(
        hpx::util::gasnet_environment::pollingLock);
    (void) gasnet_AMPoll();
#else
    (void) gasnet_AMPoll();
#endif
}

static inline void wait_done_obj(done_t* done, bool do_yield)
{
    while (!done->flag)
    {
        am_poll_try();
        if (do_yield)
        {
            hpx::this_thread::suspend(
                hpx::threads::thread_schedule_state::pending,
                "gasnet::wait_done_obj");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)) ||   \
    defined(HPX_HAVE_MODULE_GASNET_BASE)
namespace hpx::util {

    // the GASNet library, for hpx::complete_version()
    namespace {

        std::string gasnet_library_version()
        {
            return "GASNET_VERSION:" +
                std::to_string(GASNET_RELEASE_VERSION_MAJOR) + ':' +
                std::to_string(GASNET_RELEASE_VERSION_MINOR) + ':' +
                std::to_string(GASNET_RELEASE_VERSION_PATCH) + '-' +
                "GASNET_CONDUIT:" + GASNET_CONDUIT_NAME_STR;
        }

        config_registry::add_library_version_helper register_gasnet_version{
            {"GASNET", &gasnet_library_version}};
    }    // namespace
}    // namespace hpx::util
#endif

namespace hpx::util {

    namespace detail {

        bool detect_gasnet_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
            std::string gasnet_environment_strings =
                cfg.get_entry("hpx.parcel.gasnet.env", default_env);

            hpx::string_util::char_separator<char> sep(";,: ");
            hpx::string_util::tokenizer tokens(gasnet_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug)
                        << "Found GASNET environment variable: " << tok << "="
                        << std::string(env) << ", enabling GASNET support\n";
                    return true;
                }
            }

            LBT_(info)
                << "No known GASNET environment variable found, disabling "
                   "GASNET support\n";
            return false;
        }
    }    // namespace detail

    bool gasnet_environment::check_gasnet_environment(
        util::runtime_configuration const& cfg)
    {
#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_MODULE_GASNET_BASE)
        // We disable the GASNET parcelport if any of these hold:
        //
        // - The parcelport is explicitly disabled
        // - The application is not run in an GASNET environment
        // - The TCP parcelport is enabled and has higher priority
        //
        if (get_entry_as(cfg, "hpx.parcel.gasnet.enable", 1) == 0 ||
            (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.gasnet.priority", 0))) ||
            (get_entry_as(cfg, "hpx.parcel.gasnet.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.mpi.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.gasnet.priority", 0))))
        {
            LBT_(info)
                << "GASNET support disabled via configuration settings\n";
            return false;
        }

        return true;
#else
        return false;
#endif
    }
}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_MODULE_GASNET_BASE))

namespace hpx::util {

    hpx::spinlock gasnet_environment::pollingLock{};
    hpx::mutex gasnet_environment::dshm_mut{};
    hpx::mutex gasnet_environment::mtx_{};
    bool gasnet_environment::enabled_ = false;
    bool gasnet_environment::has_called_init_ = false;
    int gasnet_environment::provided_threading_flag_ = GASNET_PAR;
    int gasnet_environment::is_initialized_ = -1;
    int gasnet_environment::init_val_ = GASNET_ERR_RESOURCE;
    hpx::mutex* gasnet_environment::segment_mutex = nullptr;
    gasnet_seginfo_t* gasnet_environment::segments = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    int gasnet_environment::init(int* argc, char*** argv, int const minimal,
        [[maybe_unused]] int const required, int& provided)
    {
        if (!has_called_init_)
        {
            gasnet_environment::init_val_ = gasnet_init(argc, argv);
            has_called_init_ = true;
        }

        if (gasnet_environment::init_val_ == GASNET_ERR_NOT_INIT)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init",
                "GASNET initialization error");
        }
        else if (gasnet_environment::init_val_ == GASNET_ERR_RESOURCE)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init", "GASNET resource error");
        }
        else if (gasnet_environment::init_val_ == GASNET_ERR_BAD_ARG)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init",
                "GASNET bad argument error");
        }
        else if (gasnet_environment::init_val_ == GASNET_ERR_NOT_READY)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init",
                "GASNET not ready error");
        }

        if (provided < minimal)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init",
                "GASNET doesn't provide minimal requested thread level");
        }

        if (gasnet_attach(nullptr, 0, gasnet_getMaxLocalSegmentSize(), 0) !=
            GASNET_OK)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::init",
                "GASNET failed to attach to memory");
        }

        // create a number of segments equal to the number of hardware
        // threads per machine (locality)
        //
        //segments.resize(hpx::threads::hardware_concurrency() * size());
        //
        gasnet_environment::segments = new gasnet_seginfo_t[size()];
        gasnet_environment::segment_mutex = new hpx::mutex[size()];

        GASNET_Safe(gasnet_getSegmentInfo(segments, size()));

        gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
        gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

        gasnet_set_waitmode(GASNET_WAIT_BLOCK);

        return gasnet_environment::init_val_;
    }

    ///////////////////////////////////////////////////////////////////////////
    void gasnet_environment::init(
        int* argc, char*** argv, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        int this_rank = -1;
        has_called_init_ = false;

        // We assume to use the GASNET parcelport if it is not explicitly disabled
        enabled_ = check_gasnet_environment(rtcfg);
        if (!enabled_)
        {
            rtcfg.add_entry("hpx.parcel.gasnet.enable", "0");
            return;
        }

        rtcfg.add_entry("hpx.parcel.bootstrap", "gasnet");

        int required = GASNET_PAR;
        int retval =
            init(argc, argv, required, required, provided_threading_flag_);
        if (GASNET_OK != retval)
        {
            // explicitly disable gasnet if not run by gasnetrun
            rtcfg.add_entry("hpx.parcel.gasnet.enable", "0");

            enabled_ = false;

            char message[1024 + 1];
            std::snprintf(message, 1024 + 1, "%s\n", gasnet_ErrorDesc(retval));
            std::string msg("gasnet_environment::init: gasnet_init failed: ");
            msg = msg + message + ".";
            throw std::runtime_error(msg.c_str());
        }

        if (provided_threading_flag_ != GASNET_PAR)
        {
            // explicitly disable gasnet if not run by gasnetrun
            rtcfg.add_entry("hpx.parcel.gasnet.multithreaded", "0");
        }

        this_rank = rank();

#if defined(HPX_HAVE_NETWORKING)
        if (this_rank == 0)
        {
            rtcfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = hpx::runtime_mode::worker;
        }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        rtcfg.mode_ = hpx::runtime_mode::console;
#else
        rtcfg.mode_ = hpx::runtime_mode::local;
#endif

        rtcfg.add_entry("hpx.parcel.gasnet.rank", std::to_string(this_rank));
        rtcfg.add_entry(
            "hpx.parcel.gasnet.processorname", get_processor_name());
    }

    std::string gasnet_environment::get_processor_name()
    {
        char name[1024 + 1] = {'\0'};
        std::string const rnkstr = std::to_string(rank());
        int const len = rnkstr.size();
        if (1025 < len)
        {
            HPX_THROW_EXCEPTION(error::invalid_status,
                "hpx::util::gasnet_environment::get_processor_name",
                "GASNET processor name is larger than 1025");
        }
        std::copy(std::begin(rnkstr), std::end(rnkstr), name);
        return name;
    }

    bool gasnet_environment::gettable(
        int const node, void* start, size_t const len)
    {
        uintptr_t const segstart =
            (uintptr_t) gasnet_environment::segments[node].addr;
        uintptr_t const segend =
            segstart + gasnet_environment::segments[node].size;
        uintptr_t const reqstart = (uintptr_t) start;
        uintptr_t const reqend = reqstart + len;

        return (segstart <= reqstart && reqstart <= segend &&
            segstart <= reqend && reqend <= segend);
    }

    void gasnet_environment::put(std::uint8_t* addr, int const node,
        std::uint8_t* raddr, std::size_t const size)
    {
        bool const in_remote_seg = gettable(node, raddr, size);
        if (in_remote_seg)
        {
            std::lock_guard<hpx::mutex> const lk(segment_mutex[node]);
            gasnet_put(node, static_cast<void*>(raddr),
                static_cast<void*>(addr), size);
        }
        else
        {
            // tell the remote node to copy the data being sent
            //
            size_t max_chunk = gasnet_AMMaxMedium();
            size_t start = 0;

            // AMRequestMedium will send put; the active message handler
            // will memcpy on the remote host
            //
            for (start = 0; start < size; start += max_chunk)
            {
                size_t this_size;
                done_t done;

                this_size = size - start;
                if (this_size > max_chunk)
                {
                    this_size = max_chunk;
                }

                void* addr_chunk = addr + start;
                void* raddr_chunk = raddr + start;

                init_done_obj(&done, 1);

                // Send an AM over to ask for a them to copy the data
                // passed in the active message (addr_chunk) to raddr_chunk.
                GASNET_Safe(gasnet_AMRequestMedium4(node, DO_COPY_PAYLOAD,
                    addr_chunk, this_size, Arg0(&done), Arg1(&done),
                    Arg0(raddr_chunk), Arg1(raddr_chunk)));

                // Wait for the PUT to complete.
                wait_done_obj(&done, false);
            }
        }
    }

    void gasnet_environment::get(std::uint8_t* addr, int const node,
        std::uint8_t* raddr, std::size_t const size)
    {
        if (rank() == node)
        {
            std::memmove(addr, raddr, size);
        }
        else
        {
            // Handle remote address not in remote segment.
            // The GASNet Spec says:
            //   The source memory address for all gets and the target memory address
            //   for all puts must fall within the memory area registered for remote
            //   access by the remote node (see gasnet_attach()), or the results are
            //   undefined
            //
            // In other words, it is OK if the local side of a GET or PUT
            // is not in the registered memory region.
            //
            bool remote_in_segment = gettable(node, raddr, size);

            if (remote_in_segment)
            {
                // If raddr is in the remote segment, do a normal gasnet_get.
                // GASNet will handle the local portion not being in the segment.
                //
                gasnet_get(addr, node, raddr, size);    // dest, node, src, size
            }
            else
            {
                // If raddr is not in the remote segment, we need to send an
                // active message; the other node will PUT back to us.
                // The local side has to be in the registered memory segment.
                //
                bool local_in_segment = false;
                void* local_buf = nullptr;
                std::size_t max_chunk = gasnet_AMMaxLongReply();
                std::size_t start = 0;

                local_in_segment = gettable(rank(), addr, size);

                // If the local address isn't in a registered segment,
                // do the GET into a temporary buffer instead, and then
                // copy the result back.
                //
                if (!local_in_segment)
                {
                    size_t buf_sz = size;
                    if (buf_sz > max_chunk)
                    {
                        buf_sz = max_chunk;
                    }

                    local_buf = calloc(1, buf_sz);
                    HPX_ASSERT(gettable(node, local_buf, buf_sz));
                }

                // do a PUT on the remote locale back to here.
                // But do it in chunks of size gasnet_AMMaxLongReply()
                // since we use gasnet_AMReplyLong to do the PUT.
                for (start = 0; start < size; start += max_chunk)
                {
                    size_t this_size;
                    void* addr_chunk;
                    xfer_info_t info;
                    done_t done;

                    this_size = size - start;
                    if (this_size > max_chunk)
                    {
                        this_size = max_chunk;
                    }

                    addr_chunk = addr + start;

                    init_done_obj(&done, 1);

                    info.ack = &done;
                    info.tgt = local_buf ? local_buf : addr_chunk;
                    info.src = raddr + start;
                    info.size = this_size;

                    // Send an AM over to ask for a PUT back to us
                    GASNET_Safe(gasnet_AMRequestMedium0(
                        node, DO_REPLY_PUT, &info, sizeof(info)));

                    // Wait for the PUT to complete.
                    wait_done_obj(&done, false);

                    // Now copy from local_buf back to addr if necessary.
                    if (local_buf)
                    {
                        std::memcpy(addr_chunk, local_buf, this_size);
                    }
                }

                // If we were using a temporary local buffer free it
                if (local_buf)
                {
                    free(local_buf);
                }
            }
        }
    }

    void gasnet_environment::finalize()
    {
        if (enabled() && has_called_init())
        {
            gasnet_exit(1);
            if (gasnet_environment::segments != nullptr)
            {
                delete gasnet_environment::segments;
            }
            if (gasnet_environment::segment_mutex != nullptr)
            {
                delete gasnet_environment::segment_mutex;
            }
        }
    }

    bool gasnet_environment::enabled()
    {
        return enabled_;
    }

    bool gasnet_environment::multi_threaded()
    {
        return provided_threading_flag_ != GASNET_PAR;
    }

    bool gasnet_environment::has_called_init()
    {
        return has_called_init_;
    }

    int gasnet_environment::size()
    {
        int res(-1);
        if (enabled())
            res = static_cast<int>(gasnet_nodes());
        return res;
    }

    int gasnet_environment::rank()
    {
        int res(-1);
        if (enabled())
            res = static_cast<int>(gasnet_mynode());
        return res;
    }

    gasnet_environment::scoped_lock::scoped_lock()
    {
        if (!multi_threaded())
            mtx_.lock();
    }

    gasnet_environment::scoped_lock::~scoped_lock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    void gasnet_environment::scoped_lock::unlock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    gasnet_environment::scoped_try_lock::scoped_try_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            locked = mtx_.try_lock();
        }
    }

    gasnet_environment::scoped_try_lock::~scoped_try_lock()
    {
        if (!multi_threaded() && locked)
            mtx_.unlock();
    }

    void gasnet_environment::scoped_try_lock::unlock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }
}    // namespace hpx::util

#endif
