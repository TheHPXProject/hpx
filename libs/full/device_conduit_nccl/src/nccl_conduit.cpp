//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_DEVICE_CONDUIT_NCCL)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>

#include <hpx/device_conduit_nccl/nccl_conduit.hpp>

// cuda* names for HIP in HIP builds
#include <hpx/async_cuda/custom_gpu_api.hpp>

#if defined(HPX_HAVE_HIP)
#include <rccl/rccl.h>
#else
#include <nccl.h>
#endif

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace hpx::parcelset::device_conduits {

    // completions of operations whose buffers a call of a released channel's
    // thread may still use (RCCL): called by the conduit's background work
    // once the call has returned (not on that thread, which isn't one of
    // HPX's)
    struct nccl_conduit::deferred
    {
        std::atomic<bool> returned{false};
        std::vector<hpx::move_only_function<void(std::exception_ptr)>>
            completions;
        std::exception_ptr error;
    };

    namespace {

        // the functions the errors name
        constexpr char const conduit_send[] =
            "hpx::parcelset::device_conduits::nccl_conduit::send_impl";
        constexpr char const conduit_recv[] =
            "hpx::parcelset::device_conduits::nccl_conduit::recv_impl";
        constexpr char const conduit_progress[] =
            "hpx::parcelset::device_conduits::nccl_conduit::progress";
        constexpr char const conduit_round[] =
            "hpx::parcelset::device_conduits::nccl_conduit::round_ready";

        // a duration in seconds, with one decimal
        std::string seconds_text(std::chrono::steady_clock::duration const d)
        {
            std::ostringstream os;
            os << std::fixed << std::setprecision(1)
               << std::chrono::duration<double>(d).count();
            return os.str();
        }

        std::exception_ptr make_error(char const* function, std::string msg)
        {
            return std::make_exception_ptr(hpx::exception(
                hpx::error::network_error, std::string(function) + ": " + msg));
        }

        std::exception_ptr nccl_error(char const* function, char const* what,
            ncclResult_t const result, ncclComm_t const comm = nullptr)
        {
            std::string msg =
                std::string(what) + " failed: " + ncclGetErrorString(result);
            if (char const* last = ncclGetLastError(comm);
                last != nullptr && *last != '\0')
            {
                msg += " (";
                msg += last;
                msg += ")";
            }
            return make_error(function, HPX_MOVE(msg));
        }

        std::exception_ptr cuda_error(
            char const* function, char const* what, cudaError_t const error)
        {
            return make_error(function,
                std::string(what) + " failed: " + cudaGetErrorString(error));
        }

        // the GPU of the device memory at 'p', -1 if it isn't device memory
        int device_of(void const* p) noexcept
        {
            cudaPointerAttributes attributes;
            std::memset(&attributes, 0, sizeof(attributes));
            if (cudaPointerGetAttributes(&attributes, p) != cudaSuccess)
            {
                (void) cudaGetLastError();
                return -1;
            }
#if defined(HPX_HAVE_HIP) && HIP_VERSION_MAJOR < 6
            // HIP before ROCm 6 names the field memoryType
            auto const type = attributes.memoryType;
#else
            auto const type = attributes.type;
#endif
            return type == cudaMemoryTypeDevice ? attributes.device : -1;
        }

        // makes 'device' current on this OS thread, restores the previous
        // one at the end of the scope (HPX threads of other tasks may rely
        // on it)
        class device_guard
        {
        public:
            explicit device_guard(int const device) noexcept
            {
                if (device >= 0 && cudaGetDevice(&previous_) == cudaSuccess &&
                    previous_ != device)
                {
                    if (cudaSetDevice(device) == cudaSuccess)
                    {
                        restore_ = true;
                    }
                    else
                    {
                        (void) cudaGetLastError();
                    }
                }
            }

            device_guard(device_guard const&) = delete;
            device_guard& operator=(device_guard const&) = delete;

            ~device_guard()
            {
                if (restore_ && cudaSetDevice(previous_) != cudaSuccess)
                {
                    (void) cudaGetLastError();
                }
            }

        private:
            int previous_ = -1;
            bool restore_ = false;
        };

#if defined(__linux__)
        // The CPUs of the process (of its main thread, which HPX doesn't
        // bind), read once.
        cpu_set_t const* process_cpus() noexcept
        {
            static cpu_set_t cpus;
            static bool const valid = [] {
                CPU_ZERO(&cpus);
                return sched_getaffinity(getpid(), sizeof(cpus), &cpus) == 0;
            }();
            return valid ? &cpus : nullptr;
        }
#endif

        // NCCL creates threads in some calls (its proxy threads when a
        // communicator is created, a thread launching each group of a
        // non-blocking communicator), which inherit the CPUs of the calling
        // thread. An HPX worker thread is bound to one core, which it keeps
        // busy: NCCL's threads would run only when the scheduler preempts it,
        // after milliseconds. The calling thread uses all CPUs of the process
        // while it makes these calls (NCCL binds its proxy threads to the
        // CPUs near the GPU among these).
        template <typename F>
        decltype(auto) with_process_cpus(F&& f)
        {
#if defined(__linux__)
            cpu_set_t const* const cpus = process_cpus();
            cpu_set_t own;
            CPU_ZERO(&own);
            pthread_t const self = pthread_self();
            bool const widened = cpus != nullptr &&
                pthread_getaffinity_np(self, sizeof(own), &own) == 0 &&
                !CPU_EQUAL(&own, cpus) &&
                pthread_setaffinity_np(self, sizeof(*cpus), cpus) == 0;
            auto const restore = hpx::experimental::scope_exit([&] {
                if (widened)
                {
                    (void) pthread_setaffinity_np(self, sizeof(own), &own);
                }
            });
#endif
            return HPX_FORWARD(F, f)();
        }

#if defined(HPX_HAVE_HIP)
        // RCCL (2.22) fails when the non-blocking initializations of two
        // communicators overlap in a process (it hangs or crashes), which
        // happens here with a communicator per direction. With RCCL, the
        // communicators are blocking, and their calls that may wait for the
        // peer (the initialization, issuing a group) run on a thread of the
        // channel, one at a time; the progress polls their result as it polls
        // a non-blocking communicator.
        class blocking_calls
        {
            struct state
            {
                std::mutex mtx;
                std::condition_variable cv;
                std::function<ncclResult_t(ncclComm_t&)> task;
                bool stop = false;
                std::atomic<ncclResult_t> result{ncclSuccess};

                // the communicator an initialization has created (until the
                // channel takes it), or the one handed back by release
                ncclComm_t comm = nullptr;

                // handed back by release
                cudaStream_t stream = nullptr;
                int device = -1;

                // marked once the last call has returned (see release)
                std::shared_ptr<nccl_conduit::deferred> deferred;
            };

        public:
            blocking_calls() = default;
            blocking_calls(blocking_calls const&) = delete;
            blocking_calls& operator=(blocking_calls const&) = delete;

            ~blocking_calls()
            {
                (void) release(nullptr, nullptr, -1);
            }

            // runs 'task' on the thread (no other one is running); the task
            // gets the communicator to create (for an initialization)
            void start(std::function<ncclResult_t(ncclComm_t&)> task)
            {
                if (!thread_.joinable())
                {
                    // the thread, and the threads RCCL creates from it, use
                    // all CPUs of the process (see with_process_cpus)
                    try
                    {
                        thread_ = with_process_cpus(
                            [s = state_] { return std::thread(&run, s); });
                    }
                    catch (...)
                    {
                        // the channel fails as with an error of RCCL
                        state_->result.store(
                            ncclSystemError, std::memory_order_release);
                        return;
                    }
                }
                {
                    std::lock_guard<std::mutex> l(state_->mtx);
                    state_->result.store(
                        ncclInProgress, std::memory_order_relaxed);
                    state_->task = HPX_MOVE(task);
                }
                state_->cv.notify_one();
            }

            // the result of the last task, ncclInProgress while it runs
            [[nodiscard]] ncclResult_t result() const noexcept
            {
                return state_->result.load(std::memory_order_acquire);
            }

            // the communicator the last initialization has created (once
            // its result is there)
            [[nodiscard]] ncclComm_t take_comm() noexcept
            {
                std::lock_guard<std::mutex> l(state_->mtx);
                return std::exchange(state_->comm, nullptr);
            }

            // stops the thread: it aborts 'comm' and destroys 'stream' (and
            // a communicator an initialization still running creates) once
            // its current call has returned, which may wait for the peer;
            // false if no thread runs (it couldn't be created): the caller
            // keeps 'comm' and 'stream' and releases them itself
            [[nodiscard]] bool release(ncclComm_t const comm,
                cudaStream_t const stream, int device,
                std::shared_ptr<nccl_conduit::deferred> deferred = nullptr)
            {
                if (!thread_.joinable())
                {
                    if (deferred != nullptr)
                    {
                        deferred->returned.store(
                            true, std::memory_order_release);
                    }
                    return false;
                }
                {
                    std::lock_guard<std::mutex> l(state_->mtx);
                    if (comm != nullptr)
                    {
                        state_->comm = comm;
                    }
                    state_->stream = stream;
                    state_->device = device;
                    state_->deferred = HPX_MOVE(deferred);
                    state_->stop = true;
                }
                state_->cv.notify_one();
                if (result() == ncclInProgress)
                {
                    thread_.detach();
                }
                else
                {
                    thread_.join();
                }
                return true;
            }

        private:
            static void run(std::shared_ptr<state> const s)
            {
                std::unique_lock<std::mutex> l(s->mtx);
                for (;;)
                {
                    s->cv.wait(l, [&] { return s->task || s->stop; });
                    if (s->stop)
                    {
                        // released: a task not started yet is dropped
                        break;
                    }
                    auto task = HPX_MOVE(s->task);
                    s->task = nullptr;
                    l.unlock();

                    // only the channel's initialization sets the
                    // communicator; the channel reads it after the result
                    ncclComm_t comm = nullptr;
                    ncclResult_t const result = task(comm);

                    l.lock();
                    if (comm != nullptr)
                    {
                        s->comm = comm;
                    }
                    s->result.store(result, std::memory_order_release);
                }

                // released
                if (s->comm != nullptr)
                {
                    (void) ncclCommAbort(s->comm);
                    s->comm = nullptr;
                }
                if (s->stream != nullptr)
                {
                    if (s->device >= 0)
                    {
                        (void) cudaSetDevice(s->device);
                    }
                    (void) cudaStreamDestroy(s->stream);
                    s->stream = nullptr;
                }
                if (s->deferred != nullptr)
                {
                    s->deferred->returned.store(
                        true, std::memory_order_release);
                    s->deferred.reset();
                }
            }

            std::shared_ptr<state> state_ = std::make_shared<state>();
            std::thread thread_;
        };
#endif

        using completion_type =
            hpx::move_only_function<void(std::exception_ptr)>;

        // the conduit of this locality, if it is the NCCL conduit
        nccl_conduit* this_conduit()
        {
            device_conduit* conduit = get_device_conduit();
            if (conduit == nullptr ||
                conduit->name() != nccl_conduit::name_impl())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status, conduit_round,
                    "the NCCL device conduit isn't used here (see "
                    "hpx.parcel.device_conduit)");
            }
            return static_cast<nccl_conduit*>(conduit);
        }
    }    // namespace

    // the receiver 'receiver' has formed a round of chunks this locality
    // sent it
    void nccl_conduit_round_ready(
        std::uint32_t const receiver, nccl_conduit_round round)
    {
        this_conduit()->round_ready(receiver, HPX_MOVE(round));
    }

    // the receiver 'receiver' won't ask for the chunks 'tags'
    void nccl_conduit_chunks_dropped(std::uint32_t const receiver,
        std::vector<std::uint64_t> const& tags, std::string const& why)
    {
        this_conduit()->chunks_dropped(receiver, tags, why);
    }
}    // namespace hpx::parcelset::device_conduits

HPX_PLAIN_ACTION(hpx::parcelset::device_conduits::nccl_conduit_round_ready,
    nccl_conduit_round_ready_action)
HPX_PLAIN_ACTION(hpx::parcelset::device_conduits::nccl_conduit_chunks_dropped,
    nccl_conduit_chunks_dropped_action)

namespace hpx::parcelset::device_conduits {

    // sending the conduit's own parcels
    namespace {

        // runs 'f' (which sends a parcel) on an HPX thread: the conduit's
        // background work may run on an OS thread
        template <typename F>
        void on_hpx_thread(F&& f)
        {
            if (threads::get_self_ptr() != nullptr)
            {
                f();
            }
            else
            {
                hpx::post(HPX_FORWARD(F, f));
            }
        }

        // tells the sender 'peer' that the chunks 'tags' won't be asked for
        // (called without locks held: this sends a parcel); if that fails
        // too, the sender's chunks stay held until the locality is removed
        void tell_dropped(std::uint32_t const peer,
            std::vector<std::uint64_t> tags,
            std::exception_ptr const& error) noexcept
        {
            if (tags.empty())
            {
                return;
            }
            try
            {
                hpx::id_type const dest = naming::get_id_from_locality_id(peer);
                std::uint32_t const here = hpx::get_locality_id();
                std::string why = hpx::get_error_what(error);
                on_hpx_thread([peer, dest, here, tags = HPX_MOVE(tags),
                                  why = HPX_MOVE(why)]() mutable {
                    try
                    {
                        hpx::post(nccl_conduit_chunks_dropped_action(), dest,
                            here, HPX_MOVE(tags), HPX_MOVE(why));
                    }
                    catch (...)
                    {
                        LPT_(warning).format("nccl_conduit: couldn't tell "
                                             "locality {} about chunks not "
                                             "received: {}",
                            peer,
                            hpx::get_error_what(std::current_exception()));
                    }
                });
            }
            catch (...)
            {
                LPT_(warning).format("nccl_conduit: couldn't tell locality "
                                     "{} about chunks not received: {}",
                    peer, hpx::get_error_what(std::current_exception()));
            }
        }
    }    // namespace
}    // namespace hpx::parcelset::device_conduits

namespace hpx::parcelset::device_conduits {

    ///////////////////////////////////////////////////////////////////////////
    // One direction between this locality and another one: a communicator of
    // two ranks (the receiver is rank 0), a stream, and the chunks on their
    // way through the rounds.
    struct nccl_conduit::channel
    {
        // one chunk: a send or a receive
        struct operation
        {
            std::uint64_t tag = 0;
            void* p = nullptr;
            std::size_t size = 0;
            int device = -1;
            completion_type done;
        };

        // the operations of one round, issued as one NCCL group, finished
        // when 'event' (recorded on the stream after them) has completed
        struct round_type
        {
            std::vector<operation> operations;
            cudaEvent_t event = nullptr;

            // the round's number (of its channel), when it was formed, and
            // when and as which launch of this process it was issued (for
            // the report of a stalled round)
            std::uint64_t number = 0;
            std::chrono::steady_clock::time_point formed_at;
            std::chrono::steady_clock::time_point issued_at;
            std::uint64_t launch = 0;
            bool reported = false;
        };

        channel(std::uint32_t const peer, bool const sending,
            int const local_device, int const remote_device,
            std::shared_ptr<outbox> box) noexcept
          : peer(peer)
          , sending(sending)
          , remote_device(remote_device)
          , box(HPX_MOVE(box))
          , device(local_device)
        {
        }

        channel(channel const&) = delete;
        channel& operator=(channel const&) = delete;

        ~channel()
        {
            // the conduit is going away (after draining it) or the channel
            // is idle: anything still here can't finish any more
            std::exception_ptr const error = make_error(conduit_progress,
                "the NCCL device conduit was destroyed with transfers to or "
                "from locality " +
                    std::to_string(peer) + " in flight");
            for (auto& completion : take_completions())
            {
                completion(error);
            }
            release_nccl();
        }

        // all completions of operations not finished yet (with mtx and
        // comm_mtx held, or when no other thread uses the channel)
        std::vector<completion_type> take_completions();

        // aborts the communicator, destroys the stream (with comm_mtx held)
        void release_nccl(
            std::shared_ptr<nccl_conduit::deferred> deferred = nullptr) noexcept
        {
#if defined(HPX_HAVE_HIP)
            // RCCL: a call of the channel's thread may still use them (and
            // wait for the peer), so they can't be released here as NCCL's
            // non-blocking communicator is (ncclCommAbort stops what is in
            // progress); the thread releases them when the call has returned,
            // and marks 'deferred' then
            if (calls.release(comm, stream, device, HPX_MOVE(deferred)))
            {
                comm = nullptr;
                stream = nullptr;
            }
            initializing = false;
#endif
            device_guard const g(device);
            if (comm != nullptr)
            {
                // nothing waits for the peer: the operations have finished,
                // or they fail
                (void) ncclCommAbort(comm);
                comm = nullptr;
            }
            comm_ready = false;
            if (stream != nullptr)
            {
                (void) cudaStreamDestroy(stream);
                stream = nullptr;
            }
        }

        // whether nothing is left to do (with mtx and comm_mtx held)
        [[nodiscard]] bool idle() const noexcept
        {
            return pending.empty() && ready.empty() && formed.empty() &&
                in_flight.empty() && !group_pending;
        }

        std::uint32_t const peer;
        bool const sending;

        // the GPU of the other end (the one here is 'device')
        int const remote_device;

        // sending: the chunks held for the peer (shared by its channels)
        std::shared_ptr<outbox> const box;

        // guards the queues (held for short times only)
        hpx::spinlock mtx;

        // receiving: the receives not in a round yet
        std::vector<operation> pending;

        // sending: the rounds the receiver has formed, by number
        std::map<std::uint64_t, nccl_conduit_round> ready;

        // the rounds formed (receiving) or ready with their chunks (sending)
        // but not issued yet, in order
        std::deque<round_type> formed;

        // receiving: the number of the next round to form; sending: of the
        // next round to issue
        std::uint64_t next_round = 0;

        // the ncclUniqueId of the communicator (receiving: created with the
        // first round; sending: the one of the first round)
        std::vector<char> id;

        // the channel has failed: all its transfers fail
        std::exception_ptr error;

        // guards the NCCL and CUDA state below; NCCL communicators aren't
        // thread safe (one issuer at a time, others leave the work to it)
        hpx::spinlock comm_mtx;

        ncclComm_t comm = nullptr;

        // the communicator has been initialized (non-blocking)
        bool comm_ready = false;

#if defined(HPX_HAVE_HIP)
        // RCCL: the thread making the blocking calls (NCCL's non-blocking
        // communicator needs none), and whether it is creating the
        // communicator
        blocking_calls calls;
        bool initializing = false;
#endif

        // the last group returned ncclInProgress: the communicator must
        // become ready again before the next NCCL call and the event of the
        // round can be recorded
        bool group_pending = false;

        int device = -1;
        cudaStream_t stream = nullptr;

        // the rounds issued, in order
        std::deque<round_type> in_flight;
    };

    ///////////////////////////////////////////////////////////////////////////
    // The chunks sent to one locality that wait for their round, by tag. They
    // are shared by the channels to that locality (one per pair of GPUs):
    // the round a receiver forms names the channel, and its tags the chunks.
    struct nccl_conduit::outbox
    {
        explicit outbox(std::uint32_t const peer) noexcept
          : peer(peer)
        {
        }

        outbox(outbox const&) = delete;
        outbox& operator=(outbox const&) = delete;

        ~outbox()
        {
            // the conduit is going away (after draining it): anything still
            // here can't be sent any more
            std::exception_ptr const error = make_error(conduit_progress,
                "the NCCL device conduit was destroyed with chunks for "
                "locality " +
                    std::to_string(peer) + " not sent");
            for (auto& done : take(nullptr))
            {
                done(error);
            }
        }

        // the completions of the chunks 'tags' (of all if null), which are
        // removed (with mtx held, or when no other thread uses the outbox)
        std::vector<completion_type> take(
            std::vector<std::uint64_t> const* tags)
        {
            std::vector<completion_type> result;
            auto const take_one = [&](auto it) {
                if (it->second.done)
                {
                    result.push_back(HPX_MOVE(it->second.done));
                }
                return held.erase(it);
            };
            if (tags == nullptr)
            {
                for (auto it = held.begin(); it != held.end();)
                {
                    it = take_one(it);
                }
            }
            else
            {
                for (std::uint64_t const tag : *tags)
                {
                    if (auto const it = held.find(tag); it != held.end())
                    {
                        take_one(it);
                    }
                }
            }
            return result;
        }

        std::uint32_t const peer;

        // guards 'held' and 'error' (after a channel's mtx, if both)
        hpx::spinlock mtx;
        std::map<std::uint64_t, channel::operation> held;

        // the locality was disconnected: new chunks for it fail
        std::exception_ptr error;
    };

    std::vector<completion_type> nccl_conduit::channel::take_completions()
    {
        std::vector<completion_type> result;
        auto const take = [&](std::vector<operation>& operations) {
            for (auto& op : operations)
            {
                if (op.done)
                {
                    result.push_back(HPX_MOVE(op.done));
                }
            }
            operations.clear();
        };
        take(pending);
        if (box != nullptr && !ready.empty())
        {
            // the chunks of the rounds asked for but not formed yet
            std::lock_guard<hpx::spinlock> lb(box->mtx);
            for (auto& [number, r] : ready)
            {
                for (auto& done : box->take(&r.tags))
                {
                    result.push_back(HPX_MOVE(done));
                }
            }
        }
        ready.clear();
        for (auto& r : formed)
        {
            take(r.operations);
        }
        formed.clear();
        for (auto& r : in_flight)
        {
            take(r.operations);
            if (r.event != nullptr)
            {
                device_guard const g(device);
                (void) cudaEventDestroy(r.event);
                r.event = nullptr;
            }
        }
        in_flight.clear();
        group_pending = false;
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    void nccl_conduit::config_defaults(std::vector<std::string>& ini)
    {
        ini.emplace_back("[hpx.parcel.conduit.nccl]");
        // a round in flight for longer than this many seconds is reported
        // (0: never): rounds of several communicators on one GPU can
        // deadlock (see the module's documentation)
        ini.emplace_back(
            "stall_timeout = ${HPX_PARCEL_CONDUIT_NCCL_STALL_TIMEOUT:30}");
        // what happens then: 'report' (a warning in the parcel log) or
        // 'fail' (the channel's transfers fail with an error)
        ini.emplace_back(
            "stall_action = ${HPX_PARCEL_CONDUIT_NCCL_STALL_ACTION:report}");
        // for tests only: the sender holds each round back this many
        // milliseconds before issuing it
        ini.emplace_back("test_hold_rounds = 0");
    }

    nccl_conduit::nccl_conduit(util::runtime_configuration const& cfg)
    {
        double const timeout = std::stod(
            cfg.get_entry("hpx.parcel.conduit.nccl.stall_timeout", "30"));
        if (timeout < 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "nccl_conduit",
                "hpx.parcel.conduit.nccl.stall_timeout must not be negative, "
                "not {}",
                timeout);
        }
        stall_timeout_ =
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(timeout));

        std::string const action =
            cfg.get_entry("hpx.parcel.conduit.nccl.stall_action", "report");
        if (action != "report" && action != "fail")
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "nccl_conduit",
                "hpx.parcel.conduit.nccl.stall_action must be 'report' or "
                "'fail', not '{}'",
                action);
        }
        fail_stalled_ = action == "fail";

        hold_rounds_ = std::chrono::milliseconds(std::stoll(
            cfg.get_entry("hpx.parcel.conduit.nccl.test_hold_rounds", "0")));
    }

    nccl_conduit::~nccl_conduit()
    {
        // destroyed after drain(): the channels abort their communicators
        // (the peers may have shut down already); anything still in flight
        // or held fails (the base logs it)
        std::map<channel_key, std::shared_ptr<channel>> senders;
        std::map<channel_key, std::shared_ptr<channel>> receivers;
        std::map<std::uint32_t, std::shared_ptr<outbox>> outboxes;
        std::vector<std::shared_ptr<channel>> removed;
        std::vector<std::shared_ptr<outbox>> removed_outboxes;
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            senders.swap(senders_);
            receivers.swap(receivers_);
            outboxes.swap(outboxes_);
            removed.swap(removed_);
            removed_outboxes.swap(removed_outboxes_);
        }
    }

    std::shared_ptr<nccl_conduit::channel> nccl_conduit::get_receiver(
        channel_key const& key)
    {
        std::lock_guard<hpx::spinlock> l(channels_mtx_);
        auto it = receivers_.find(key);
        if (it == receivers_.end())
        {
            auto const& [peer, here, there] = key;
            it = receivers_
                     .emplace(key,
                         std::make_shared<channel>(
                             peer, false, here, there, nullptr))
                     .first;
        }
        return it->second;
    }

    std::shared_ptr<nccl_conduit::outbox> nccl_conduit::get_outbox(
        std::uint32_t const locality_id)
    {
        std::lock_guard<hpx::spinlock> l(channels_mtx_);
        auto it = outboxes_.find(locality_id);
        if (it == outboxes_.end())
        {
            auto box = std::make_shared<outbox>(locality_id);
            if (auto const d = disconnected_.find(locality_id);
                d != disconnected_.end())
            {
                // no one will ask for its chunks
                box->error = d->second;
            }
            it = outboxes_.emplace(locality_id, HPX_MOVE(box)).first;
        }
        return it->second;
    }

    std::shared_ptr<nccl_conduit::channel> nccl_conduit::find_sender(
        channel_key const& key)
    {
        auto const& [peer, here, there] = key;
        std::lock_guard<hpx::spinlock> l(channels_mtx_);
        if (auto const it = senders_.find(key); it != senders_.end())
        {
            return it->second;
        }

        // a locality that has left still asks for what was sent to it
        for (auto const& c : removed_)
        {
            if (c->sending && c->peer == peer && c->device == here &&
                c->remote_device == there)
            {
                return c;
            }
        }

        // the first round of this pair of GPUs
        if (auto const it = outboxes_.find(peer); it != outboxes_.end())
        {
            return senders_
                .emplace(key,
                    std::make_shared<channel>(
                        peer, true, here, there, it->second))
                .first->second;
        }
        for (auto const& box : removed_outboxes_)
        {
            if (box->peer == peer)
            {
                auto c =
                    std::make_shared<channel>(peer, true, here, there, box);
                removed_.push_back(c);
                return c;
            }
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////
    void nccl_conduit::send_impl(std::uint32_t const locality_id,
        serialization::detail::device_chunk_descriptor& d, void const* p,
        std::size_t const size,
        hpx::move_only_function<void(std::exception_ptr)> sent)
    {
        int const device = device_of(p);
        if (device < 0)
        {
            // thrown, so that the parcel fails while it is serialized: its
            // descriptor is never written, and the receiver never waits for
            // a chunk this side can't send (with node faults tolerated the
            // error passed to 'sent' would only be logged)
            std::rethrow_exception(make_error(conduit_send,
                "the NCCL device conduit moves device memory only"));
        }

        // the receiver uses the channel from this GPU
        d.extra = static_cast<std::uint64_t>(device);

        std::shared_ptr<outbox> const box = get_outbox(locality_id);
        std::exception_ptr error;
        {
            std::lock_guard<hpx::spinlock> l(box->mtx);
            if (box->error)
            {
                error = box->error;
            }
            else
            {
                // held until the receiver has formed its round
                box->held.emplace(d.tag,
                    channel::operation{d.tag, const_cast<void*>(p), size,
                        device, HPX_MOVE(sent)});
            }
        }
        if (error)
        {
            sent(HPX_MOVE(error));
        }
    }

    void nccl_conduit::recv_impl(
        serialization::detail::device_chunk_descriptor const& d, void* p,
        std::size_t const size, std::function<void(std::exception_ptr)> done)
    {
        int const device = device_of(p);
        if (device < 0)
        {
            done(make_error(conduit_recv,
                "the NCCL device conduit moves device memory only"));
            return;
        }

        // the channel from the sender's GPU (d.extra, see send_impl) to the
        // one of the memory here
        std::shared_ptr<channel> const c =
            get_receiver({static_cast<std::uint32_t>(d.source), device,
                static_cast<int>(d.extra)});
        std::exception_ptr error;
        {
            std::lock_guard<hpx::spinlock> l(c->mtx);
            if (c->error)
            {
                error = c->error;
            }
            else
            {
                c->pending.push_back(
                    channel::operation{d.tag, p, size, device, HPX_MOVE(done)});
            }
        }
        if (error)
        {
            // the sender holds the chunk until it is asked for it
            tell_dropped(static_cast<std::uint32_t>(d.source), {d.tag}, error);
            done(HPX_MOVE(error));
            return;
        }

        // forms the round now (a short wait for the next receives would make
        // larger rounds, but delay this one)
        progress(*c);
    }

    void nccl_conduit::round_ready(
        std::uint32_t const receiver, nccl_conduit_round round)
    {
        std::shared_ptr<channel> const c =
            find_sender({receiver, static_cast<int>(round.sender_device),
                static_cast<int>(round.receiver_device)});
        if (c == nullptr)
        {
            // nothing was sent to it (or it was removed and is gone)
            LPT_(warning).format("nccl_conduit: locality {} asks for round {} "
                                 "of chunks this locality doesn't hold",
                receiver, round.round);
            return;
        }

        std::exception_ptr error;
        std::vector<completion_type> failed;
        {
            std::lock_guard<hpx::spinlock> l(c->mtx);
            if (c->error)
            {
                // the round's chunks can't be sent any more
                error = c->error;
                std::lock_guard<hpx::spinlock> lb(c->box->mtx);
                failed = c->box->take(&round.tags);
            }
            else
            {
                if (round.round == 0)
                {
                    c->id = HPX_MOVE(round.id);
                }
                std::uint64_t const number = round.round;
                c->ready.emplace(number, HPX_MOVE(round));
            }
        }
        if (error)
        {
            for (auto& done : failed)
            {
                done(error);
            }
            return;
        }
        progress(*c);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace {

        // fails the channel (with mtx not held, comm_mtx held): returns the
        // completions to call with the error; 'not_asked_for' gets the tags
        // of the receives the sender wasn't asked for yet (receiving), for
        // tell_dropped
        std::vector<completion_type> fail_channel(nccl_conduit::channel& c,
            std::exception_ptr const& error,
            std::vector<std::uint64_t>* not_asked_for = nullptr,
            std::shared_ptr<nccl_conduit::deferred>* deferred = nullptr)
        {
            std::vector<completion_type> completions;
            std::shared_ptr<nccl_conduit::deferred> later;
            {
                std::lock_guard<hpx::spinlock> l(c.mtx);
                if (!c.error)
                {
                    c.error = error;
                }
#if defined(HPX_HAVE_HIP)
                // a group the channel's thread is still issuing uses the
                // buffers of the rounds in flight: their completions wait
                // for it to return
                if (deferred != nullptr && !c.in_flight.empty() &&
                    c.calls.result() == ncclInProgress)
                {
                    later = std::make_shared<nccl_conduit::deferred>();
                    later->error = error;
                    for (auto& r : c.in_flight)
                    {
                        for (auto& op : r.operations)
                        {
                            if (op.done)
                            {
                                later->completions.push_back(HPX_MOVE(op.done));
                                op.done = nullptr;
                            }
                        }
                    }
                    *deferred = later;
                }
#else
                (void) deferred;
#endif
                if (not_asked_for != nullptr && !c.sending)
                {
                    for (auto const& op : c.pending)
                    {
                        not_asked_for->push_back(op.tag);
                    }
                }
                completions = c.take_completions();
            }
            c.release_nccl(HPX_MOVE(later));
            return completions;
        }
    }    // namespace

    void nccl_conduit::round_not_sent(channel_key const& key,
        std::vector<std::uint64_t> tags, std::exception_ptr const& error)
    {
        std::shared_ptr<channel> c;
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            if (auto const it = receivers_.find(key); it != receivers_.end())
            {
                c = it->second;
            }
            else
            {
                for (auto const& r : removed_)
                {
                    if (!r->sending && r->peer == std::get<0>(key) &&
                        r->device == std::get<1>(key) &&
                        r->remote_device == std::get<2>(key))
                    {
                        c = r;
                        break;
                    }
                }
            }
        }

        if (c != nullptr)
        {
            std::vector<completion_type> failed;
            {
                std::shared_ptr<deferred> later;
                {
                    std::lock_guard<hpx::spinlock> cl(c->comm_mtx);
                    failed = fail_channel(*c, error, &tags, &later);
                }
                keep_deferred(HPX_MOVE(later));
            }
            for (auto& done : failed)
            {
                done(error);
            }
        }

        // the sender may still be reachable another way (the request was
        // lost, not the locality)
        tell_dropped(std::get<0>(key), HPX_MOVE(tags), error);
    }

    void nccl_conduit::chunks_dropped(std::uint32_t const receiver,
        std::vector<std::uint64_t> const& tags, std::string const& why)
    {
        // the chunks are in the outbox of the receiver (or of the receiver
        // that has left)
        std::vector<std::shared_ptr<outbox>> boxes;
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            if (auto const it = outboxes_.find(receiver); it != outboxes_.end())
            {
                boxes.push_back(it->second);
            }
            for (auto const& box : removed_outboxes_)
            {
                if (box->peer == receiver)
                {
                    boxes.push_back(box);
                }
            }
        }

        std::exception_ptr const error = make_error(conduit_round,
            "locality " + std::to_string(receiver) +
                " won't receive the chunk: " + why);
        std::vector<completion_type> failed;
        for (auto const& box : boxes)
        {
            std::lock_guard<hpx::spinlock> lb(box->mtx);
            for (auto& done : box->take(&tags))
            {
                failed.push_back(HPX_MOVE(done));
            }
        }
        for (auto& done : failed)
        {
            done(error);
        }
    }

    bool nccl_conduit::progress(channel& c)
    {
        bool did_work = false;

        // what to do once no lock is held: the rounds to tell the sender
        // about, the completions to call (and their error)
        std::vector<nccl_conduit_round> rounds;
        std::vector<completion_type> completions;
        std::vector<completion_type> failed;
        std::exception_ptr error;

        // form the rounds
        {
            std::lock_guard<hpx::spinlock> l(c.mtx);
            if (c.error)
            {
                return false;
            }

            if (!c.sending && !c.pending.empty())
            {
                nccl_conduit_round r;
                r.round = c.next_round++;
                r.sender_device = c.remote_device;
                r.receiver_device = c.device;
                if (r.round == 0)
                {
                    // the communicator's id, for the sender (rank 1)
                    ncclUniqueId id;
                    if (ncclResult_t const result = ncclGetUniqueId(&id);
                        result != ncclSuccess)
                    {
                        error = nccl_error(
                            conduit_progress, "ncclGetUniqueId", result);
                    }
                    else
                    {
                        auto const* begin = reinterpret_cast<char const*>(&id);
                        c.id.assign(begin, begin + sizeof(id));
                        r.id = c.id;
                    }
                }
                if (!error)
                {
                    channel::round_type formed;
                    formed.operations.swap(c.pending);
                    r.tags.reserve(formed.operations.size());
                    r.sizes.reserve(formed.operations.size());
                    for (auto const& op : formed.operations)
                    {
                        r.tags.push_back(op.tag);
                        r.sizes.push_back(op.size);
                    }
                    formed.number = r.round;
                    formed.formed_at = std::chrono::steady_clock::now();
                    c.formed.push_back(HPX_MOVE(formed));
                    rounds.push_back(HPX_MOVE(r));
                    did_work = true;
                }
            }

            // the sender issues the rounds strictly by number, the chunks in
            // the order the receiver gave
            while (c.sending && !error)
            {
                auto const it = c.ready.find(c.next_round);
                if (it == c.ready.end())
                {
                    break;
                }
                nccl_conduit_round const& r = it->second;
                channel::round_type formed;
                formed.operations.reserve(r.tags.size());
                {
                    // the chunks of the round, from the ones held for the
                    // peer (on this channel's GPU)
                    std::lock_guard<hpx::spinlock> lb(c.box->mtx);
                    for (std::size_t i = 0; i != r.tags.size(); ++i)
                    {
                        auto const op = c.box->held.find(r.tags[i]);
                        if (op == c.box->held.end() ||
                            op->second.size != r.sizes[i] ||
                            op->second.device != c.device)
                        {
                            // the order of the channel is lost
                            error = make_error(conduit_progress,
                                "locality " + std::to_string(c.peer) +
                                    " asks for a chunk this locality doesn't "
                                    "hold on GPU " +
                                    std::to_string(c.device) + " (tag " +
                                    std::to_string(r.tags[i]) + ")");
                            break;
                        }
                        formed.operations.push_back(HPX_MOVE(op->second));
                        c.box->held.erase(op);
                    }
                }
                if (error)
                {
                    // the chunks taken already fail with the channel
                    for (auto& op : formed.operations)
                    {
                        failed.push_back(HPX_MOVE(op.done));
                    }
                    break;
                }
                formed.number = c.next_round;
                formed.formed_at = std::chrono::steady_clock::now();
                c.ready.erase(it);
                ++c.next_round;
                c.formed.push_back(HPX_MOVE(formed));
                did_work = true;
            }
        }

        // tell the senders about the new rounds (outside the locks: this
        // sends a parcel)
        for (auto& r : rounds)
        {
            try
            {
                hpx::id_type const dest =
                    naming::get_id_from_locality_id(c.peer);
                std::uint32_t const here = hpx::get_locality_id();
                channel_key const key{c.peer, c.device, c.remote_device};
                std::vector<std::uint64_t> tags = r.tags;

                // a request that can't be sent fails the channel: nothing
                // else would end the wait for its receives (with node
                // faults tolerated the default handler only logs a write
                // error). The conduit is looked up when that happens (it is
                // gone once the runtime has reset it).
                auto not_sent = [key](std::vector<std::uint64_t> tags,
                                    std::exception_ptr const& e) {
                    device_conduit* const conduit = get_device_conduit();
                    if (conduit != nullptr &&
                        conduit->name() == nccl_conduit::name_impl())
                    {
                        static_cast<nccl_conduit*>(conduit)->round_not_sent(
                            key, HPX_MOVE(tags), e);
                    }
                };
                on_hpx_thread([dest, here, not_sent, r = HPX_MOVE(r),
                                  tags = HPX_MOVE(tags)]() mutable {
                    try
                    {
                        hpx::post_cb(
                            nccl_conduit_round_ready_action(), dest,
                            [not_sent, tags](std::error_code const& ec,
                                parcelset::parcel const&) mutable {
                                if (ec)
                                {
                                    not_sent(HPX_MOVE(tags),
                                        make_error(conduit_progress,
                                            "the request for a round "
                                            "couldn't be sent: " +
                                                ec.message()));
                                }
                            },
                            here, HPX_MOVE(r));
                    }
                    catch (...)
                    {
                        not_sent(HPX_MOVE(tags), std::current_exception());
                    }
                });
            }
            catch (...)
            {
                if (!error)
                {
                    error = std::current_exception();
                }
            }
        }

        // issue the rounds, poll the communicator and the rounds in flight;
        // the thread holding the lock does it for all
        std::unique_lock<hpx::spinlock> cl(c.comm_mtx, std::try_to_lock);
        if (!cl.owns_lock())
        {
            if (error)
            {
                // fail it once the lock is free (rare)
                cl.lock();
            }
            else
            {
                return did_work;
            }
        }

        while (!error)
        {
            // create the communicator once the id is there and the GPU is
            // known (from the first round)
#if defined(HPX_HAVE_HIP)
            // RCCL: the communicator exists only once the channel's thread
            // has created it (NCCL's non-blocking initialization sets it
            // right away and finishes in the background)
            if (c.comm == nullptr && !c.initializing)
#else
            if (c.comm == nullptr)
#endif
            {
                {
                    std::lock_guard<hpx::spinlock> l(c.mtx);
                    if (c.id.size() != sizeof(ncclUniqueId) || c.formed.empty())
                    {
                        break;
                    }
                }

                ncclUniqueId id;
                std::memcpy(&id, c.id.data(), sizeof(id));

                // the GPU of the channel (from its key)
                int const device = c.device;
                device_guard const g(device);
                if (cudaError_t const e = cudaStreamCreateWithFlags(
                        &c.stream, cudaStreamNonBlocking);
                    e != cudaSuccess)
                {
                    (void) cudaGetLastError();
                    error = cuda_error(
                        conduit_progress, "cudaStreamCreateWithFlags", e);
                    break;
                }

#if defined(HPX_HAVE_HIP)
                // RCCL: a blocking initialization (it waits for the peer to
                // initialize its side), made by the channel's thread instead
                // of a non-blocking one made here, since RCCL fails when the
                // non-blocking initializations of two communicators overlap
                // (see blocking_calls). The thread was created with the CPUs
                // of the process, so with_process_cpus isn't needed.
                c.calls.start([id, device, rank = c.sending ? 1 : 0](
                                  ncclComm_t& comm) {
                    (void) cudaSetDevice(device);
                    ncclConfig_t config = NCCL_CONFIG_INITIALIZER;
                    config.blocking = 1;
                    return ncclCommInitRankConfig(&comm, 2, id, rank, &config);
                });
                c.initializing = true;
#else
                ncclConfig_t config = NCCL_CONFIG_INITIALIZER;
                config.blocking = 0;
                ncclResult_t const result = with_process_cpus([&] {
                    return ncclCommInitRankConfig(
                        &c.comm, 2, id, c.sending ? 1 : 0, &config);
                });
                if (result != ncclSuccess && result != ncclInProgress)
                {
                    error = nccl_error(
                        conduit_progress, "ncclCommInitRankConfig", result);
                    break;
                }
#endif
                did_work = true;
            }

            // a non-blocking communicator must be ready before the next
            // call (after its creation and after a group in progress)
            if (!c.comm_ready || c.group_pending)
            {
                ncclResult_t state = ncclSuccess;
#if defined(HPX_HAVE_HIP)
                // RCCL: the communicator is blocking, there is nothing to
                // poll in it; the initialization or group is done once the
                // call of the channel's thread has returned, its result is
                // what ncclCommGetAsyncError reports for NCCL
                state = c.calls.result();
                if (state == ncclInProgress)
                {
                    break;
                }
                if (c.initializing)
                {
                    c.initializing = false;
                    c.comm = c.calls.take_comm();
                    if (state != ncclSuccess)
                    {
                        error = nccl_error(conduit_progress,
                            "ncclCommInitRankConfig", state, c.comm);
                        break;
                    }
                }
#else
                if (ncclResult_t const result =
                        ncclCommGetAsyncError(c.comm, &state);
                    result != ncclSuccess)
                {
                    error = nccl_error(conduit_progress,
                        "ncclCommGetAsyncError", result, c.comm);
                    break;
                }
                if (state == ncclInProgress)
                {
                    break;
                }
#endif
                if (state != ncclSuccess)
                {
                    error = nccl_error(
                        conduit_progress, "the communicator", state, c.comm);
                    break;
                }
                did_work = true;
                c.comm_ready = true;
                if (c.group_pending)
                {
                    // the round's operations are on the stream now
                    c.group_pending = false;
                    device_guard const g(c.device);
                    auto& r = c.in_flight.back();
                    if (cudaError_t const e = cudaEventCreateWithFlags(
                            &r.event, cudaEventDisableTiming);
                        e != cudaSuccess)
                    {
                        (void) cudaGetLastError();
                        r.event = nullptr;
                        error = cuda_error(
                            conduit_progress, "cudaEventCreateWithFlags", e);
                        break;
                    }
                    if (cudaError_t const e =
                            cudaEventRecord(r.event, c.stream);
                        e != cudaSuccess)
                    {
                        (void) cudaGetLastError();
                        error =
                            cuda_error(conduit_progress, "cudaEventRecord", e);
                        break;
                    }
                }
            }

            // issue the next round as one group
            channel::round_type r;
            {
                std::lock_guard<hpx::spinlock> l(c.mtx);
                if (c.formed.empty())
                {
                    break;
                }
                if (c.sending && hold_rounds_.count() != 0 &&
                    std::chrono::steady_clock::now() -
                            c.formed.front().formed_at <
                        hold_rounds_)
                {
                    // held back (hpx.parcel.conduit.nccl.test_hold_rounds)
                    break;
                }
                r = HPX_MOVE(c.formed.front());
                c.formed.pop_front();
            }

            device_guard const g(c.device);
#if defined(HPX_DEBUG)
            // the channels are per GPU (see recv_impl and the outbox)
            for (auto const& op : r.operations)
            {
                HPX_ASSERT(op.device == c.device);
            }
#endif

#if defined(HPX_HAVE_HIP)
            // RCCL: the communicator is blocking, so ncclGroupEnd may wait
            // (for the peer, when the connection is set up with the first
            // group). Unlike NCCL's non-blocking group, which returns
            // ncclInProgress and finishes on a thread of NCCL, the group is
            // issued by the channel's thread, and the result is polled the
            // same way (as if ncclGroupEnd had returned ncclInProgress).
            std::vector<std::pair<void*, std::size_t>> chunks;
            chunks.reserve(r.operations.size());
            for (auto const& op : r.operations)
            {
                chunks.emplace_back(op.p, op.size);
            }
            c.calls.start(
                [chunks = HPX_MOVE(chunks), sending = c.sending, comm = c.comm,
                    stream = c.stream, device = c.device](ncclComm_t&) {
                    (void) cudaSetDevice(device);
                    ncclResult_t result = ncclGroupStart();
                    if (result != ncclSuccess)
                    {
                        return result;
                    }
                    for (auto const& [p, size] : chunks)
                    {
                        // the receiver is rank 0, the sender rank 1
                        result = sending ?
                            ncclSend(p, size, ncclInt8, 0, comm, stream) :
                            ncclRecv(p, size, ncclInt8, 1, comm, stream);
                        if (result != ncclSuccess)
                        {
                            break;
                        }
                    }
                    ncclResult_t const end = ncclGroupEnd();
                    return result == ncclSuccess ? end : result;
                });
            ncclResult_t const result = ncclInProgress;
#else
            ncclResult_t result = ncclGroupStart();
            if (result == ncclSuccess)
            {
                for (auto const& op : r.operations)
                {
                    // the receiver is rank 0, the sender rank 1
                    result = c.sending ?
                        ncclSend(op.p, op.size, ncclInt8, 0, c.comm, c.stream) :
                        ncclRecv(op.p, op.size, ncclInt8, 1, c.comm, c.stream);
                    if (result != ncclSuccess && result != ncclInProgress)
                    {
                        break;
                    }
                }
                ncclResult_t const end =
                    with_process_cpus([] { return ncclGroupEnd(); });
                if (result == ncclSuccess || result == ncclInProgress)
                {
                    result = end;
                }
            }
#endif
            if (result != ncclSuccess && result != ncclInProgress)
            {
                for (auto& op : r.operations)
                {
                    failed.push_back(HPX_MOVE(op.done));
                }
                error = nccl_error(conduit_progress,
                    c.sending ? "ncclSend" : "ncclRecv", result, c.comm);
                break;
            }
            did_work = true;

            r.issued_at = std::chrono::steady_clock::now();
            r.launch = launches_.fetch_add(1, std::memory_order_relaxed);
            c.in_flight.push_back(HPX_MOVE(r));
            if (result == ncclInProgress)
            {
                // the event is recorded once the communicator is ready
                c.group_pending = true;
                c.comm_ready = false;
                continue;
            }

            auto& issued = c.in_flight.back();
            if (cudaError_t const e = cudaEventCreateWithFlags(
                    &issued.event, cudaEventDisableTiming);
                e != cudaSuccess)
            {
                (void) cudaGetLastError();
                issued.event = nullptr;
                error =
                    cuda_error(conduit_progress, "cudaEventCreateWithFlags", e);
                break;
            }
            if (cudaError_t const e = cudaEventRecord(issued.event, c.stream);
                e != cudaSuccess)
            {
                (void) cudaGetLastError();
                error = cuda_error(conduit_progress, "cudaEventRecord", e);
                break;
            }
        }

        // the rounds that have finished, in order
        while (!error && !c.in_flight.empty() &&
            c.in_flight.front().event != nullptr)
        {
            auto& r = c.in_flight.front();
            device_guard const g(c.device);
            cudaError_t const e = cudaEventQuery(r.event);
            if (e == cudaErrorNotReady)
            {
                (void) cudaGetLastError();
                break;
            }
            if (e != cudaSuccess)
            {
                (void) cudaGetLastError();
                error = cuda_error(conduit_progress, "cudaEventQuery", e);
                break;
            }
            (void) cudaEventDestroy(r.event);
            for (auto& op : r.operations)
            {
                completions.push_back(HPX_MOVE(op.done));
            }
            c.in_flight.pop_front();
            did_work = true;
        }

        // the oldest round in flight has stalled (it is reported once)
        std::optional<stalled_round> stalled;
        if (!error && stall_timeout_.count() != 0 && !c.in_flight.empty())
        {
            auto& r = c.in_flight.front();
            auto const age = std::chrono::steady_clock::now() - r.issued_at;
            if (!r.reported && age > stall_timeout_)
            {
                r.reported = true;
                stalled = stalled_round{c.peer, c.sending, c.device,
                    c.remote_device, r.number, r.launch, age, {}, {}};
                for (auto const& op : r.operations)
                {
                    stalled->tags.push_back(op.tag);
                    stalled->sizes.push_back(op.size);
                }
                if (fail_stalled_)
                {
                    error = make_error(conduit_progress,
                        "round " + std::to_string(r.number) +
                            " to/from "
                            "locality " +
                            std::to_string(c.peer) + " has not finished in " +
                            seconds_text(age) +
                            " s (hpx.parcel.conduit.nccl.stall_timeout), "
                            "possibly a deadlock of NCCL launches on GPU " +
                            std::to_string(c.device) +
                            " (see the device_conduit_nccl documentation)");
                }
            }
        }

        // an NCCL error is reported by the communicator too (the peer
        // failed or left)
        if (!error && c.comm != nullptr && c.comm_ready)
        {
            ncclResult_t state = ncclSuccess;
            if (ncclCommGetAsyncError(c.comm, &state) == ncclSuccess &&
                state != ncclSuccess && state != ncclInProgress)
            {
                error = nccl_error(
                    conduit_progress, "the communicator", state, c.comm);
            }
        }

        std::vector<std::uint64_t> not_asked_for;
        std::shared_ptr<deferred> later;
        if (error)
        {
            auto rest = fail_channel(c, error, &not_asked_for, &later);
            for (auto& done : rest)
            {
                failed.push_back(HPX_MOVE(done));
            }
            did_work = true;
        }
        cl.unlock();
        keep_deferred(HPX_MOVE(later));
        if (stalled)
        {
            report_stall(*stalled, &c);
        }

        // the sender holds the chunks of the receives it wasn't asked for
        // (a failure before or with the first round included)
        if (error)
        {
            tell_dropped(c.peer, HPX_MOVE(not_asked_for), error);
        }

        for (auto& done : completions)
        {
            done(nullptr);
        }
        for (auto& done : failed)
        {
            done(error);
        }
        return did_work;
    }

    void nccl_conduit::report_stall(stalled_round const& r, channel const* c)
    {
        stalled_rounds_.fetch_add(1, std::memory_order_relaxed);

        auto const list = [](std::vector<std::uint64_t> const& v) {
            std::string text;
            for (std::size_t i = 0; i != v.size() && i != 8; ++i)
            {
                text += (i == 0 ? "" : ",") + std::to_string(v[i]);
            }
            return v.size() > 8 ? text + ",..." : text;
        };

        // the other rounds in flight on the same GPU, in the order this
        // process launched them (the channels busy elsewhere are left out)
        std::vector<std::shared_ptr<channel>> channels;
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            for (auto const* map : {&senders_, &receivers_})
            {
                for (auto const& [key, other] : *map)
                {
                    channels.push_back(other);
                }
            }
            channels.insert(channels.end(), removed_.begin(), removed_.end());
        }
        auto const now = std::chrono::steady_clock::now();
        std::map<std::uint64_t, std::string> others;
        std::size_t busy = 0;
        for (auto const& other : channels)
        {
            if (other.get() == c || other->device != r.device)
            {
                continue;
            }
            std::unique_lock<hpx::spinlock> ol(
                other->comm_mtx, std::try_to_lock);
            if (!ol.owns_lock())
            {
                ++busy;
                continue;
            }
            for (auto const& f : other->in_flight)
            {
                others[f.launch] = std::string(other->sending ? "to" : "from") +
                    " locality " + std::to_string(other->peer) + " GPU " +
                    std::to_string(other->remote_device) + " round " +
                    std::to_string(f.number) + " (launch " +
                    std::to_string(f.launch) + ", " +
                    seconds_text(now - f.issued_at) + " s)";
            }
        }

        std::string text;
        for (auto const& [launch, what] : others)
        {
            text += "; " + what;
        }
        LPT_(warning).format(
            "nccl_conduit: round {} {} locality {} (GPU {} here, {} there; "
            "launch {}; tags {}; sizes {}) has not finished after {} s. "
            "Other rounds in flight on GPU {}: {}{}{}. {}",
            r.number, r.sending ? "to" : "from", r.peer, r.device,
            r.remote_device, r.launch, list(r.tags), list(r.sizes),
            seconds_text(r.age), r.device, others.size(), text,
            busy != 0 ? " (" + std::to_string(busy) + " channels not seen)" :
                        std::string(),
            others.empty() ?
                "The peer may be slow, or may have stopped taking part." :
                "Rounds of several communicators in flight on one GPU can "
                "deadlock if the processes launched them in different orders "
                "(see the device_conduit_nccl documentation); compare with "
                "the peer's report.");
        if (fail_stalled_)
        {
            LPT_(warning).format("nccl_conduit: the channel {} locality {} "
                                 "fails (hpx.parcel.conduit.nccl.stall_action"
                                 " = fail)",
                r.sending ? "to" : "from", r.peer);
        }
    }

    void nccl_conduit::keep_deferred(std::shared_ptr<deferred> d)
    {
        if (d != nullptr)
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            deferred_.push_back(HPX_MOVE(d));
        }
    }

    bool nccl_conduit::background_work_impl()
    {
        std::vector<std::shared_ptr<channel>> channels;
        {
            std::unique_lock<hpx::spinlock> l(channels_mtx_, std::try_to_lock);
            if (!l.owns_lock())
            {
                return false;
            }
            channels.reserve(
                senders_.size() + receivers_.size() + removed_.size());
            for (auto const& [key, c] : senders_)
            {
                channels.push_back(c);
            }
            for (auto const& [key, c] : receivers_)
            {
                channels.push_back(c);
            }
            for (auto const& c : removed_)
            {
                channels.push_back(c);
            }
        }

        bool did_work = false;
        for (auto const& c : channels)
        {
            did_work = progress(*c) || did_work;
        }

        // destroy the channels of removed localities once idle
        std::unique_lock<hpx::spinlock> l(channels_mtx_, std::try_to_lock);
        if (l.owns_lock())
        {
            std::vector<std::shared_ptr<channel>> idle;
            std::erase_if(removed_, [&](std::shared_ptr<channel> const& c) {
                std::unique_lock<hpx::spinlock> cl(
                    c->comm_mtx, std::try_to_lock);
                if (!cl.owns_lock())
                {
                    return false;
                }
                std::lock_guard<hpx::spinlock> ll(c->mtx);
                if (!c->idle())
                {
                    return false;
                }
                // a sending channel stays while chunks are held for the
                // peer: a later round asks for them on this channel (its
                // round numbers), a new one would wait for round 0
                if (c->box != nullptr)
                {
                    std::lock_guard<hpx::spinlock> lb(c->box->mtx);
                    if (!c->box->held.empty())
                    {
                        return false;
                    }
                }
                idle.push_back(c);
                return true;
            });

            // and their outboxes once empty (nothing left to ask for)
            std::vector<std::shared_ptr<outbox>> empty;
            std::erase_if(
                removed_outboxes_, [&](std::shared_ptr<outbox> const& box) {
                    std::unique_lock<hpx::spinlock> lb(
                        box->mtx, std::try_to_lock);
                    if (!lb.owns_lock() || !box->held.empty())
                    {
                        return false;
                    }
                    empty.push_back(box);
                    return true;
                });
            // and the completions whose call has returned
            std::vector<std::shared_ptr<deferred>> returned;
            std::erase_if(deferred_, [&](std::shared_ptr<deferred> const& d) {
                if (!d->returned.load(std::memory_order_acquire))
                {
                    return false;
                }
                returned.push_back(d);
                return true;
            });
            l.unlock();

            for (auto const& d : returned)
            {
                for (auto& done : d->completions)
                {
                    done(d->error);
                }
            }

            // the last references (destroyed without locks held)
            did_work = !idle.empty() || !empty.empty() || !returned.empty() ||
                did_work;
        }
        return did_work;
    }

    void nccl_conduit::locality_removed_impl(
        std::uint32_t const locality_id, bool const forced) noexcept
    {
        // new transfers to the locality use new channels (and fail like
        // parcels to it); the old ones finish what the locality has asked
        // for, unless it was disconnected
        std::vector<std::shared_ptr<channel>> channels;
        std::shared_ptr<outbox> box;
        std::exception_ptr const disconnected = forced ?
            make_error(conduit_progress,
                "locality " + std::to_string(locality_id) +
                    " was disconnected") :
            std::exception_ptr();
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            if (forced)
            {
                disconnected_.insert_or_assign(locality_id, disconnected);
            }
            for (auto* map : {&senders_, &receivers_})
            {
                // all pairs of GPUs
                for (auto it = map->begin(); it != map->end();)
                {
                    if (std::get<0>(it->first) == locality_id)
                    {
                        channels.push_back(HPX_MOVE(it->second));
                        it = map->erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            for (auto const& c : channels)
            {
                removed_.push_back(c);
            }
            if (auto const it = outboxes_.find(locality_id);
                it != outboxes_.end())
            {
                box = HPX_MOVE(it->second);
                outboxes_.erase(it);
                removed_outboxes_.push_back(box);
            }
        }

        if (!forced)
        {
            return;
        }

        // the chunks not asked for yet fail too
        if (box != nullptr)
        {
            std::exception_ptr const& error = disconnected;
            std::vector<completion_type> completions;
            {
                std::lock_guard<hpx::spinlock> lb(box->mtx);
                box->error = error;
                completions = box->take(nullptr);
            }
            for (auto& done : completions)
            {
                done(error);
            }
        }
        for (auto const& c : channels)
        {
            std::exception_ptr const error = make_error(conduit_progress,
                "locality " + std::to_string(locality_id) +
                    " was disconnected");
            std::vector<completion_type> completions;
            std::shared_ptr<deferred> later;
            {
                std::lock_guard<hpx::spinlock> cl(c->comm_mtx);
                completions = fail_channel(*c, error, nullptr, &later);
            }
            keep_deferred(HPX_MOVE(later));
            for (auto& done : completions)
            {
                done(error);
            }
        }
    }
}    // namespace hpx::parcelset::device_conduits

HPX_REGISTER_DEVICE_CONDUIT(hpx::parcelset::device_conduits::nccl_conduit, nccl)

#endif
