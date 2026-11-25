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
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hpx::parcelset::device_conduits {

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
            return attributes.type == cudaMemoryTypeDevice ? attributes.device :
                                                             -1;
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
            };

        public:
            blocking_calls() = default;
            blocking_calls(blocking_calls const&) = delete;
            blocking_calls& operator=(blocking_calls const&) = delete;

            ~blocking_calls()
            {
                release(nullptr, nullptr, -1);
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
            // its current call has returned, which may wait for the peer
            void release(
                ncclComm_t const comm, cudaStream_t const stream, int device)
            {
                if (!thread_.joinable())
                {
                    return;
                }
                {
                    std::lock_guard<std::mutex> l(state_->mtx);
                    if (comm != nullptr)
                    {
                        state_->comm = comm;
                    }
                    state_->stream = stream;
                    state_->device = device;
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
}    // namespace hpx::parcelset::device_conduits

HPX_PLAIN_ACTION(hpx::parcelset::device_conduits::nccl_conduit_round_ready,
    nccl_conduit_round_ready_action)

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
        };

        channel(std::uint32_t const peer, bool const sending) noexcept
          : peer(peer)
          , sending(sending)
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
        std::vector<completion_type> take_completions()
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
            for (auto& [tag, op] : held)
            {
                if (op.done)
                {
                    result.push_back(HPX_MOVE(op.done));
                }
            }
            held.clear();
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

        // aborts the communicator, destroys the stream (with comm_mtx held)
        void release_nccl() noexcept
        {
#if defined(HPX_HAVE_HIP)
            // RCCL: a call of the channel's thread may still use them (and
            // wait for the peer), so they can't be released here as NCCL's
            // non-blocking communicator is (ncclCommAbort stops what is in
            // progress); the thread releases them when the call has returned
            calls.release(comm, stream, device);
            comm = nullptr;
            stream = nullptr;
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
            return pending.empty() && held.empty() && ready.empty() &&
                formed.empty() && in_flight.empty() && !group_pending;
        }

        std::uint32_t const peer;
        bool const sending;

        // guards the queues (held for short times only)
        hpx::spinlock mtx;

        // receiving: the receives not in a round yet
        std::vector<operation> pending;

        // sending: the chunks waiting for their round, by tag
        std::map<std::uint64_t, operation> held;

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
    nccl_conduit::nccl_conduit(util::runtime_configuration const&) {}

    nccl_conduit::~nccl_conduit()
    {
        // destroyed after drain(): the channels abort their communicators
        // (the peers may have shut down already); anything still in flight
        // fails (the base logs it)
        std::map<std::uint32_t, std::shared_ptr<channel>> senders;
        std::map<std::uint32_t, std::shared_ptr<channel>> receivers;
        std::vector<std::shared_ptr<channel>> removed;
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            senders.swap(senders_);
            receivers.swap(receivers_);
            removed.swap(removed_);
        }
    }

    std::shared_ptr<nccl_conduit::channel> nccl_conduit::get_channel(
        std::uint32_t const locality_id, bool const sending)
    {
        std::lock_guard<hpx::spinlock> l(channels_mtx_);
        auto& channels = sending ? senders_ : receivers_;
        auto it = channels.find(locality_id);
        if (it == channels.end())
        {
            it = channels
                     .emplace(locality_id,
                         std::make_shared<channel>(locality_id, sending))
                     .first;
        }
        return it->second;
    }

    std::shared_ptr<nccl_conduit::channel> nccl_conduit::find_sender(
        std::uint32_t const locality_id)
    {
        std::lock_guard<hpx::spinlock> l(channels_mtx_);
        if (auto const it = senders_.find(locality_id); it != senders_.end())
        {
            return it->second;
        }

        // a locality that has left still asks for what was sent to it
        for (auto const& c : removed_)
        {
            if (c->sending && c->peer == locality_id)
            {
                return c;
            }
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////
    void nccl_conduit::send_impl(std::uint32_t const locality_id,
        serialization::detail::device_chunk_descriptor const& d, void const* p,
        std::size_t const size,
        hpx::move_only_function<void(std::exception_ptr)> sent)
    {
        int const device = device_of(p);
        if (device < 0)
        {
            // the receiver will ask for the chunk; the error stops the
            // runtime (the parcel has left)
            sent(make_error(conduit_send,
                "the NCCL device conduit moves device memory only"));
            return;
        }

        std::shared_ptr<channel> const c = get_channel(locality_id, true);
        std::exception_ptr error;
        {
            std::lock_guard<hpx::spinlock> l(c->mtx);
            if (c->error)
            {
                error = c->error;
            }
            else
            {
                // held until the receiver has formed its round
                c->held.emplace(d.tag,
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

        std::shared_ptr<channel> const c =
            get_channel(static_cast<std::uint32_t>(d.source), false);
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
        std::shared_ptr<channel> const c = find_sender(receiver);
        if (c == nullptr)
        {
            // nothing was sent to it (or it was removed and is gone)
            LPT_(warning).format("nccl_conduit: locality {} asks for round {} "
                                 "of chunks this locality doesn't hold",
                receiver, round.round);
            return;
        }

        {
            std::lock_guard<hpx::spinlock> l(c->mtx);
            if (c->error)
            {
                return;
            }
            if (round.round == 0)
            {
                c->id = HPX_MOVE(round.id);
            }
            std::uint64_t const number = round.round;
            c->ready.emplace(number, HPX_MOVE(round));
        }
        progress(*c);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace {

        // fails the channel (with mtx not held, comm_mtx held): returns the
        // completions to call with the error
        std::vector<completion_type> fail_channel(
            nccl_conduit::channel& c, std::exception_ptr const& error)
        {
            std::vector<completion_type> completions;
            {
                std::lock_guard<hpx::spinlock> l(c.mtx);
                if (!c.error)
                {
                    c.error = error;
                }
                completions = c.take_completions();
            }
            c.release_nccl();
            return completions;
        }
    }    // namespace

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
                for (std::size_t i = 0; i != r.tags.size(); ++i)
                {
                    auto const op = c.held.find(r.tags[i]);
                    if (op == c.held.end() || op->second.size != r.sizes[i])
                    {
                        // the order of the channel is lost
                        error = make_error(conduit_progress,
                            "locality " + std::to_string(c.peer) +
                                " asks for a chunk this locality doesn't "
                                "hold (tag " +
                                std::to_string(r.tags[i]) + ")");
                        break;
                    }
                    formed.operations.push_back(HPX_MOVE(op->second));
                    c.held.erase(op);
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
                if (threads::get_self_ptr() != nullptr)
                {
                    hpx::post(nccl_conduit_round_ready_action(), dest, here,
                        HPX_MOVE(r));
                }
                else
                {
                    // the background work of an OS thread
                    hpx::post([dest, here, r = HPX_MOVE(r)]() mutable {
                        hpx::post(nccl_conduit_round_ready_action(), dest, here,
                            HPX_MOVE(r));
                    });
                }
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
                int device = -1;
                {
                    std::lock_guard<hpx::spinlock> l(c.mtx);
                    if (c.id.size() != sizeof(ncclUniqueId) ||
                        c.formed.empty() || c.formed.front().operations.empty())
                    {
                        break;
                    }
                    device = c.formed.front().operations.front().device;
                }

                ncclUniqueId id;
                std::memcpy(&id, c.id.data(), sizeof(id));

                c.device = device;
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
                r = HPX_MOVE(c.formed.front());
                c.formed.pop_front();
            }

            device_guard const g(c.device);
            for (auto const& op : r.operations)
            {
                if (op.device != c.device)
                {
                    error = make_error(conduit_progress,
                        "the NCCL device conduit moves the chunks of one GPU "
                        "per locality (the channel to or from locality " +
                            std::to_string(c.peer) + " uses GPU " +
                            std::to_string(c.device) + ", a chunk is on GPU " +
                            std::to_string(op.device) + ")");
                    break;
                }
            }
            if (error)
            {
                for (auto& op : r.operations)
                {
                    failed.push_back(HPX_MOVE(op.done));
                }
                break;
            }

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

        if (error)
        {
            auto rest = fail_channel(c, error);
            for (auto& done : rest)
            {
                failed.push_back(HPX_MOVE(done));
            }
            did_work = true;
        }
        cl.unlock();

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
            for (auto const& [locality_id, c] : senders_)
            {
                channels.push_back(c);
            }
            for (auto const& [locality_id, c] : receivers_)
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
                idle.push_back(c);
                return true;
            });
            l.unlock();

            // the last references (destroyed without locks held)
            did_work = !idle.empty() || did_work;
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
        {
            std::lock_guard<hpx::spinlock> l(channels_mtx_);
            for (auto* map : {&senders_, &receivers_})
            {
                if (auto const it = map->find(locality_id); it != map->end())
                {
                    channels.push_back(HPX_MOVE(it->second));
                    map->erase(it);
                }
            }
            for (auto const& c : channels)
            {
                removed_.push_back(c);
            }
        }

        if (!forced)
        {
            return;
        }
        for (auto const& c : channels)
        {
            std::exception_ptr const error = make_error(conduit_progress,
                "locality " + std::to_string(locality_id) +
                    " was disconnected");
            std::vector<completion_type> completions;
            {
                std::lock_guard<hpx::spinlock> cl(c->comm_mtx);
                completions = fail_channel(*c, error);
            }
            for (auto& done : completions)
            {
                done(error);
            }
        }
    }
}    // namespace hpx::parcelset::device_conduits

HPX_REGISTER_DEVICE_CONDUIT(hpx::parcelset::device_conduits::nccl_conduit, nccl)

#endif
