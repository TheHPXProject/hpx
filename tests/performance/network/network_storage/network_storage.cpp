//  Copyright (c) 2014 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/components/iostreams/standard_streams.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <simple_profiler.hpp>

//
// This is a test program which reads and writes chunks of memory to storage
// distributed across localities.
//
// The principal problem can be summarized as follows:
//
// Put data into remote memory
//    1 Pass user data pointer into serialize_buffer (0 copy)
//    2 Copy data into parcelport for transmission. (1 copy)
//    3 Transmit data into remote parcelport buffer
//    4 Copy data from parcelport into serialize_buffer (2 copy)
//    5 Copy data from serialize buffer into remote host storage (3 copy ?)
//
// Get data from remote memory
//    1 Allocate temporary buffer and copy from host storage into it (1 copy)
//    2 Wrap storage in serialize_buffer and give to parcelport for return
//    3 Copy data from serialize_buffer into parcelport for transmission (2 copy)
//    4 Receive data into local parcelport buffer
//    5 Copy parcelport buffer to serialize_buffer(user data pointer) (3 copy)
//
// The ideal situation would be as follows
//
// Put into remote memory
//    1 Pass user data pointer into serialize_buffer (0 copy)
//    2 Copy data into parcelport for transmission. (1 copy)
//    3 Transmit data into remote parcelport buffer
//    4 Copy data from parcelport into serialize_buffer(user data pointer) (2 copy)
//
// Get data from remote memory
//    1 Request serialize buffer from parcelport (0 copy)
//    2 Copy from storage into serialize_buffer (1 copy)
//    4 Receive data into local parcelport buffer
//    5 Copy parcelport buffer to serialize_buffer(user data pointer) (2 copy)
//
// To make each process run a main function and participate in the test,
// use a command line of the kind (no mpiexec assumed)
//
//     network_storage.exe -l%1 -%%x --hpx:run-hpx-main --hpx:threads=4
//
// (+or more) where %l is num_ranks, and %%x is rank of current process

//----------------------------------------------------------------------------
// @TODO : add support for custom network executor
// local_priority_queue_os_executor exec(4, "thread:0-3=core:12-15.pu:0");
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// #defines to control program
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// These are used to track how many requests are pending for each locality
// Many requests for read/write of memory may be issued at a time, resulting
// in thousands of futures that need to be waited on. To reduce this number
// a background thread can be spawned to check for ready futures and remove
// them from the waiting list. The vars are used for this bookkeeping task.
//
//#define USE_CLEANING_THREAD
//#define USE_PARCELPORT_THREAD

//----------------------------------------------------------------------------
// Array allocation on start assumes a certain maximum number of localities will be used
#define MAX_RANKS 16384

//----------------------------------------------------------------------------
// Define this to make memory access asynchronous and return a future
//#define ASYNC_MEMORY
#ifdef ASYNC_MEMORY
typedef hpx::future<int> async_mem_result_type;
#define ASYNC_MEM_RESULT(x) (hpx::make_ready_future<int>(TEST_SUCCESS));
#else
typedef int async_mem_result_type;
#define ASYNC_MEM_RESULT(x) (x);
#endif

//----------------------------------------------------------------------------
// control the amount of debug messaging that is output
#define DEBUG_LEVEL 0

//----------------------------------------------------------------------------
#if DEBUG_LEVEL > 0
#define LOG_DEBUG_MSG(x) std::cout << "Network storage " << x << std::endl
#define DEBUG_OUTPUT(level, x)                                                 \
    if (DEBUG_LEVEL >= level)                                                  \
    {                                                                          \
        LOG_DEBUG_MSG(x);                                                      \
    }
#define DEBUG_ONLY(x) x
#else
#define DEBUG_OUTPUT(level, x)
#define DEBUG_ONLY(x)
#endif

//----------------------------------------------------------------------------
#define TEST_FAIL 0
#define TEST_SUCCESS 1

//----------------------------------------------------------------------------
// global vars
//----------------------------------------------------------------------------
std::vector<std::vector<hpx::future<int>>> ActiveFutures;
std::array<std::atomic<int>, MAX_RANKS> FuturesWaiting;

#if defined(USE_CLEANING_THREAD) || defined(USE_PARCELPORT_THREAD)
std::atomic<bool> FuturesActive;
hpx::spinlock FuturesMutex;
#endif

//----------------------------------------------------------------------------
//
// Each locality allocates a buffer of memory which is used to host transfers
//
char* local_storage = nullptr;
hpx::spinlock storage_mutex;

//
struct test_options
{
    std::uint64_t iterations;
    std::uint64_t local_storage_MB;
    std::uint64_t global_storage_MB;
    std::uint64_t transfer_size_B;
    std::uint64_t threads;
    std::uint64_t semaphore;
    std::string network;
    bool warmup;
    bool all2all;
    bool distribution;
    bool nolocal;
};

//----------------------------------------------------------------------------
void allocate_local_storage(uint64_t local_storage_bytes)
{
    local_storage = new char[local_storage_bytes];
}

//----------------------------------------------------------------------------
void delete_local_storage()
{
    delete[] local_storage;
}

//----------------------------------------------------------------------------
void release_storage_lock(char* p)
{
    DEBUG_OUTPUT(6, "Release lock and delete memory");
    delete[] p;
    //  storage_mutex.unlock();
}

//----------------------------------------------------------------------------
// This routine simply copies from the source buffer into the local memory
// at the address offset.
//
// The function does not need to be asynchronous as it completes immediately,
// but we return a future as this test needs to mimic "asynchronous" storage
async_mem_result_type copy_to_local_storage(
    char const* src, uint32_t offset, uint64_t length)
{
    char* dest = &local_storage[offset];
    std::copy(src, src + length, dest);
    //  memcpy(dest, src, length);
    return ASYNC_MEM_RESULT(TEST_SUCCESS);
}

//----------------------------------------------------------------------------
// This routine simply copies from local memory at the address offset
// into a provided buffer
//
// The function does not need to be asynchronous as it completes immediately,
// but we return a future as this test needs to mimic "asynchronous" storage
async_mem_result_type copy_from_local_storage(
    char* dest, uint32_t offset, uint64_t length)
{
    char const* src = &local_storage[offset];
    std::copy(src, src + length, dest);
    //  memcpy(dest, src, length);
    return ASYNC_MEM_RESULT(TEST_SUCCESS);
}

//----------------------------------------------------------------------------
// A custom allocator which takes a pointer in its constructor and then returns
// this pointer in response to any allocate request. It is here to try to fool
// the hpx serialization into copying directly into a user provided buffer
// without copying from a result into another buffer.
//
template <typename T>
class pointer_allocator
{
public:
    typedef T value_type;
    typedef T* pointer;
    typedef T const* const_pointer;
    typedef T& reference;
    typedef T const& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    pointer_allocator() noexcept
      : pointer_(nullptr)
      , size_(0)
    {
    }

    pointer_allocator(pointer p, size_type size) noexcept
      : pointer_(p)
      , size_(size)
    {
    }

    pointer address(reference value) const
    {
        return &value;
    }
    const_pointer address(const_reference value) const
    {
        return &value;
    }

    pointer allocate(size_type n, void const* /* hint */ = nullptr)
    {
        HPX_TEST_EQ(n, size_);
        return static_cast<T*>(pointer_);
    }

    void deallocate(pointer p, size_type n) noexcept
    {
        HPX_TEST(p == pointer_);
        HPX_TEST_EQ(n, size_);
    }

private:
    // serialization support
    friend class hpx::serialization::access;

    template <typename Archive>
    void load(Archive& ar, unsigned int const)
    {
        std::size_t t = 0;
        ar >> size_ >> t;
        pointer_ = reinterpret_cast<pointer>(t);
    }

    template <typename Archive>
    void save(Archive& ar, unsigned int const) const
    {
        std::size_t t = reinterpret_cast<std::size_t>(pointer_);
        ar << size_ << t;
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()

private:
    pointer pointer_;
    size_type size_;
};

//----------------------------------------------------------------------------
// A simple Buffer for sending data, it does not need any special allocator
// user data may be sent to another locality using zero copy by wrapping
// it in one of these buffers
typedef hpx::serialization::serialize_buffer<char> general_buffer_type;

// When receiving data, we receive a hpx::serialize_buffer, we try to minimize
// copying of data by providing a receive buffer with a fixed data pointer
// so that data is placed directly into it.
typedef pointer_allocator<char> PointerAllocator;
typedef hpx::serialization::serialize_buffer<char, PointerAllocator>
    transfer_buffer_type;

typedef std::map<uint64_t, std::shared_ptr<general_buffer_type>> alive_map;
typedef hpx::spinlock mutex_type;
typedef std::lock_guard<mutex_type> scoped_lock;
//
mutex_type keep_alive_mutex;
alive_map keep_alive_buffers;

//
void async_callback(
    uint64_t const index, std::error_code const&, hpx::parcelset::parcel const&)
{
    scoped_lock lock(keep_alive_mutex);
    DEBUG_OUTPUT(7, "Async callback triggered for index " << index);
    alive_map::iterator it = keep_alive_buffers.find(index);
    if (it != keep_alive_buffers.end())
    {
        keep_alive_buffers.erase(it);
    }
    else
    {
        std::cout << "async_callback failed on index " << index << std::endl;
    }
}

//----------------------------------------------------------------------------
//
// Two actions which are called from remote or local localities
//
// Copy to storage, just invokes the local copy function and returns the future
// from it
//
// Copy from storage allocates a buffer for the return memory and then wraps
// it into a serialize buffer which is returned and passed into the local
// process.
//
// This unfortunately means memory is copied from the storage, into a buffer
// and not zero copied as we would like.
// I have not been successful in removing this copy.
namespace Storage {
    //------------------------------------------------------------------------
    // A PUT into memory on this locality from a requester sending a
    // general_buffer_type
    async_mem_result_type CopyToStorage(
        general_buffer_type const& srcbuffer, uint32_t address, uint64_t length)
    {
        std::shared_ptr<char[]> src = srcbuffer.data_array();

        return copy_to_local_storage(src.get(), address, length);
    }

    //------------------------------------------------------------------------
    // A GET from memory on this locality is returned to the requester in the
    // transfer_buffer_type
    hpx::future<transfer_buffer_type> CopyFromStorage(
        uint32_t address, uint64_t length, std::size_t remote_buffer)
    {
        // we must allocate a temporary buffer to copy from storage into
        // we can't use the remote buffer supplied because it is a handle to memory on
        // the (possibly) remote node. We allocate here using
        // a nullptr deleter so the array will
        // not be released by the shared_pointer.
        //
        // The memory must be freed after final use.
        std::allocator<char> local_allocator;
        std::shared_ptr<char[]> local_buffer(local_allocator.allocate(length),
            [](char*) { DEBUG_OUTPUT(6, "Not deleting memory"); });

        // allow the storage class to asynchronously copy the data into buffer
#ifdef ASYNC_MEMORY
        async_mem_result_type fut =
#endif
            copy_from_local_storage(local_buffer.get(), address, length);
        DEBUG_OUTPUT(
            6, "create local buffer count " << local_buffer.use_count());

        // wrap the remote buffer pointer in an allocator for return
        pointer_allocator<char> return_allocator(
            reinterpret_cast<char*>(remote_buffer), length);

        // lock the mutex, will be unlocked by the transfer buffer's deleter
//        storage_mutex.lock();
#ifdef ASYNC_MEMORY
        return fut.then(hpx::launch::sync,
            // return the data in a transfer buffer
            [=](hpx::future<int> fut) -> transfer_buffer_type {
                DEBUG_OUTPUT(
                    6, ".then local buffer count " << local_buffer.use_count());
                return transfer_buffer_type(
                    local_buffer.get(), length, transfer_buffer_type::take,
                    [](char* p) { release_storage_lock(p); }, return_allocator);
            });
#else
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif
        return hpx::make_ready_future<transfer_buffer_type>(
            transfer_buffer_type(
                local_buffer.get(), length, transfer_buffer_type::take,
                [](char* p) { release_storage_lock(p); }, return_allocator));
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
#endif
    }
}    // namespace Storage

//----------------------------------------------------------------------------
// normally these are in a header
HPX_DEFINE_PLAIN_ACTION(Storage::CopyToStorage, CopyToStorage_action);
HPX_REGISTER_ACTION_DECLARATION(CopyToStorage_action)

HPX_DEFINE_PLAIN_ACTION(Storage::CopyFromStorage, CopyFromStorage_action);
//HPX_REGISTER_ACTION_DECLARATION(CopyFromStorage_action)

// and these in a cpp
HPX_REGISTER_ACTION(CopyToStorage_action)
HPX_REGISTER_ACTION(CopyFromStorage_action)

//----------------------------------------------------------------------------
#ifdef USE_PARCELPORT_THREAD
// the main message sending loop may generate many thousands of send requests
// and each is associated with a future. To reduce the number we must wait on
// this loop runs in a background thread and simply removes any completed futures
// from the main list of active futures.
int background_work()
{
    while (FuturesActive)
    {
        hpx::parcelset::do_background_work(
            0, hpx::parcelset::parcelport_background_mode_all);
        hpx::this_thread::suspend(std::chrono::microseconds(10));
    }
    return 1;
}
#endif

//----------------------------------------------------------------------------
#ifdef USE_CLEANING_THREAD
// the main message sending loop may generate many thousands of send requests
// and each is associated with a future. To reduce the number we must wait on
// this loop runs in a background thread and simply removes any completed futures
// from the main list of active futures.
int RemoveCompletions()
{
    int num_removed = 0;
    while (FuturesActive)
    {
        {
            std::lock_guard<hpx::spinlock> lk(FuturesMutex);
            for (std::vector<hpx::future<int>>& futvec : ActiveFutures)
            {
                for (std::vector<hpx::future<int>>::iterator fut =
                         futvec.begin();
                    fut != futvec.end();)
                {
                    if (fut->is_ready())
                    {
                        int ret = fut->get();
                        if (ret != TEST_SUCCESS)
                        {
                            throw std::runtime_error("Remote put/get failed");
                        }
                        num_removed++;
                        fut = futvec.erase(fut);
                    }
                    else
                    {
                        ++fut;
                    }
                }
            }
        }
        hpx::this_thread::suspend(std::chrono::microseconds(10));
    }
    return num_removed;
}
#endif

//----------------------------------------------------------------------------
// Take a vector of futures representing pass/fail and reduce to a single pass fail
int reduce(hpx::future<std::vector<hpx::future<int>>> futvec)
{
    int res = TEST_SUCCESS;
    std::vector<hpx::future<int>> vfs = futvec.get();
    for (hpx::future<int>& f : vfs)
    {
        if (f.get() == TEST_FAIL)
            return TEST_FAIL;
    }
    return res;
}

//----------------------------------------------------------------------------
// Test speed of write/put
void test_write(uint64_t rank, uint64_t nranks, uint64_t num_transfer_slots,
    std::mt19937& gen, std::uniform_int_distribution<>& random_rank,
    std::uniform_int_distribution<>& random_slot, test_options& options)
{
    CopyToStorage_action actWrite;
    //
    DEBUG_OUTPUT(1, "Entering Barrier at start of write on rank " << rank);
    //
    hpx::distributed::barrier::synchronize();
    //
    DEBUG_OUTPUT(1, "Passed Barrier at start of write on rank " << rank);
    //
    hpx::chrono::high_resolution_timer timerWrite;
    hpx::util::simple_profiler level1(
        "Write function", rank == 0 && !options.warmup);
    //
    bool active = (rank == 0) || (rank > 0 && options.all2all);
    if (rank == 0)
        std::cout << "Iteration ";
    for (std::uint64_t i = 0; active && i < options.iterations; i++)
    {
        hpx::util::simple_profiler iteration(level1, "Iteration");
        if (rank == 0)
        {
            if (i % 10 == 0)
            {
                std::cout << "x" << std::flush;
            }
            else
            {
                std::cout << "." << std::flush;
            }
        }

        DEBUG_OUTPUT(1, "Starting iteration " << i << " on rank " << rank);

#ifdef USE_CLEANING_THREAD
        //
        // start a thread which will clear any completed futures from our list.
        //
        FuturesActive = true;
        hpx::future<int> cleaner = hpx::async(RemoveCompletions);
#endif

#ifdef USE_PARCELPORT_THREAD
        //
        // start a thread which will keep the parcelport active.
        //
        FuturesActive = true;
        hpx::future<int> background = hpx::async(background_work);
#endif

        // used to track callbacks to free buffers for async_cb
        uint64_t buffer_index = 0;

        //
        // Start main message sending loop
        //
        for (uint64_t i = 0; i < num_transfer_slots; i++)
        {
            hpx::util::simple_profiler prof_setup(iteration, "Setup slots");
            uint64_t send_rank;
            if (options.distribution == 0)
            {
                // pick a random locality to send to
                send_rank = random_rank(gen);
                while (options.nolocal && send_rank == rank)
                {
                    send_rank = random_rank(gen);
                }
            }
            else
            {
                send_rank = static_cast<uint64_t>((rank + i) % nranks);
                while (options.nolocal && send_rank == rank)
                {
                    send_rank = random_rank(gen);
                }
            }

            // get the pointer to the current packet send buffer
            char* buffer = &local_storage[i * options.transfer_size_B];
            // Get the HPX locality from the dest rank
            hpx::id_type locality =
                hpx::naming::get_id_from_locality_id(send_rank);
            // pick a random slot to write our data into
            int memory_slot = random_slot(gen);
            uint32_t memory_offset =
                static_cast<uint32_t>(memory_slot * options.transfer_size_B);
            DEBUG_OUTPUT(5,
                "Rank " << rank << " sending block " << i << " to rank "
                        << send_rank);
            prof_setup.done();

            // Execute a PUT on whatever locality we chose
            // Create a serializable memory buffer ready for sending.
            // Do not copy any data. Protect this with a mutex to ensure the
            // background thread removing completed futures doesn't collide
            {
                hpx::util::simple_profiler prof_put(iteration, "Put");
                DEBUG_OUTPUT(
                    5, "Put from rank " << rank << " on rank " << send_rank);
#ifdef USE_CLEANING_THREAD
                ++FuturesWaiting[send_rank];
                std::lock_guard<hpx::spinlock> lk(FuturesMutex);
#endif

                std::shared_ptr<general_buffer_type> temp_buffer =
                    std::make_shared<general_buffer_type>(
                        static_cast<char*>(buffer), options.transfer_size_B,
                        general_buffer_type::reference);
                using hpx::placeholders::_1;
                using hpx::placeholders::_2;
                {
                    scoped_lock lock(keep_alive_mutex);
                    keep_alive_buffers[buffer_index] = temp_buffer;
                }
                auto temp_future =
                    hpx::async_cb(hpx::launch::fork, actWrite, locality,
                        hpx::bind(&async_callback, buffer_index, _1, _2),
                        *temp_buffer, memory_offset, options.transfer_size_B)
                        .then(hpx::launch::sync,
                            [send_rank](hpx::future<int>&& fut) -> int {
                                int result = fut.get();
                                --FuturesWaiting[send_rank];
                                return result;
                            });
                buffer_index++;

                ActiveFutures[send_rank].push_back(std::move(temp_future));
            }
        }
        DEBUG_OUTPUT(2, "Exited transfer loop on rank " << rank);

#ifdef USE_CLEANING_THREAD
        int removed = 0;
        {
            hpx::util::simple_profiler prof_clean(iteration, "Cleaning Wait");
            // tell the cleaning thread it's time to stop
            FuturesActive = false;
            // wait for cleanup thread to terminate before we reduce any remaining futures
            removed = cleaner.get();
            DEBUG_OUTPUT(
                2, "Cleaning thread rank " << rank << " removed " << removed);
        }
#endif

#ifdef USE_PARCELPORT_THREAD
        // tell the parcelport thread it's time to stop
        FuturesActive = false;
        background.get();
#endif
        //
        hpx::util::simple_profiler prof_move(iteration, "Moving futures");
        std::vector<hpx::future<int>> final_list;
        for (uint64_t i = 0; i < nranks; i++)
        {
            // move the contents of intermediate vector into final list
            final_list.reserve(final_list.size() + ActiveFutures[i].size());
            std::move(ActiveFutures[i].begin(), ActiveFutures[i].end(),
                std::back_inserter(final_list));
            ActiveFutures[i].clear();
        }
        prof_move.done();
        //
        hpx::util::simple_profiler fwait(iteration, "Future wait");

        DEBUG_ONLY(int numwait = static_cast<int>(final_list.size());)

        DEBUG_OUTPUT(1, "Waiting for when_all future on rank " << rank);
        hpx::future<int> result =
            hpx::when_all(final_list).then(hpx::launch::sync, reduce);
        result.get();
#ifdef USE_CLEANING_THREAD
        int total = numwait + removed;
#endif
        DEBUG_OUTPUT(
            3, "Future wait, rank " << rank << " waiting on " << numwait);
        fwait.done();
    }
    if (rank == 0)
        std::cout << std::endl;
    DEBUG_OUTPUT(2, "Exited iterations loop on rank " << rank);

    hpx::util::simple_profiler prof_barrier(level1, "Final Barrier");
    hpx::distributed::barrier::synchronize();
    //
    uint64_t active_ranks = options.all2all ? nranks : 1;
    double writeMB = static_cast<double>(
        active_ranks * options.local_storage_MB * options.iterations);
    double writeTime = timerWrite.elapsed();
    double writeBW = writeMB / writeTime;
    double IOPS = static_cast<double>(options.iterations * num_transfer_slots);
    double IOPs_s = IOPS / writeTime;
    if (rank == 0)
    {
        std::cout << "Total time           : " << writeTime << "\n";
        std::cout << "Memory Transferred   : " << writeMB << " MB\n";
        std::cout << "Number of local IOPs : " << IOPS << "\n";
        std::cout << "IOPs/s (local)       : " << IOPs_s << "\n";
        std::cout << "Aggregate BW Write   : " << writeBW << " MB/s"
                  << std::endl;
        // a complete set of results that our python matplotlib script will ingest
        char const* msg =
            "CSVData, write, network, "
            "{1}, ranks, {2}, threads, {3}, Memory, {4}, IOPsize, {5}, "
            "IOPS/s, {6}, BW(MB/s), {7}, ";
        if (!options.warmup)
        {
            hpx::util::format_to(std::cout, msg, options.network, nranks,
                options.threads, writeMB, options.transfer_size_B, IOPs_s,
                writeBW)
                << std::endl;
        }
        std::cout << std::endl;
    }
}

//----------------------------------------------------------------------------
// Copy the data once into the destination buffer if the get() operation was
// entirely local (no data copies have been made so far).
static void transfer_data(
    general_buffer_type recv, hpx::future<transfer_buffer_type>&& f)
{
    transfer_buffer_type buffer(f.get());
    //  if (buffer.data() != recv.data())
    {
        std::copy(buffer.data(), buffer.data() + buffer.size(), recv.data());
    }
    /*
  else {
    DEBUG_OUTPUT(2, "Skipped copy due to matching pointers");
  }
  */
}

//----------------------------------------------------------------------------
// Test speed of read/get
void test_read(uint64_t rank, uint64_t nranks, uint64_t num_transfer_slots,
    std::mt19937& gen, std::uniform_int_distribution<>& random_rank,
    std::uniform_int_distribution<>& random_slot, test_options& options)
{
    CopyFromStorage_action actRead;
    //
    DEBUG_OUTPUT(1, "Entering Barrier at start of read on rank " << rank);
    //
    hpx::distributed::barrier::synchronize();
    //
    DEBUG_OUTPUT(1, "Passed Barrier at start of read on rank " << rank);
    //
    // this is mostly the same as the put loop, except that the received future
    // is not an int, but a transfer buffer which we have to copy out of.
    //
    hpx::chrono::high_resolution_timer timerRead;
    //
    if (rank == 0)
        std::cout << "Iteration ";
    bool active = (rank == 0) || (rank > 0 && options.all2all);
    for (std::uint64_t i = 0; active && i < options.iterations; i++)
    {
        if (rank == 0)
        {
            if (i % 10 == 0)
            {
                std::cout << "x" << std::flush;
            }
            else
            {
                std::cout << "." << std::flush;
            }
        }

        DEBUG_OUTPUT(1, "Starting iteration " << i << " on rank " << rank);
#ifdef USE_CLEANING_THREAD
        //
        // start a thread which will clear any completed futures from our list.
        //
        FuturesActive = true;
        hpx::future<int> cleaner = hpx::async(RemoveCompletions);
#endif

#ifdef USE_PARCELPORT_THREAD
        //
        // start a thread which will keep the parcelport active.
        //
        FuturesActive = true;
        hpx::future<int> background = hpx::async(background_work);
#endif
        //
        // Start main message sending loop
        //
        //
        for (uint64_t i = 0; i < num_transfer_slots; i++)
        {
            hpx::chrono::high_resolution_timer looptimer;
            uint64_t send_rank;
            if (options.distribution == 0)
            {
                // pick a random locality to send to
                send_rank = random_rank(gen);
                while (options.nolocal && send_rank == rank)
                {
                    send_rank = random_rank(gen);
                }
            }
            else
            {
                send_rank = static_cast<uint64_t>((rank + i) % nranks);
                while (options.nolocal && send_rank == rank)
                {
                    send_rank = random_rank(gen);
                }
            }

            // get the pointer to the current packet send buffer
            //            char *buffer = &local_storage[i*options.transfer_size_B];

            // Get the HPX locality from the dest rank
            hpx::id_type locality =
                hpx::naming::get_id_from_locality_id(send_rank);

            // pick a random slot to write our data into
            int memory_slot = random_slot(gen);
            uint32_t memory_offset =
                static_cast<uint32_t>(memory_slot * options.transfer_size_B);

            // create a transfer buffer object to receive the data being returned to us
            general_buffer_type general_buffer(&local_storage[memory_offset],
                options.transfer_size_B, general_buffer_type::reference);

            // Execute a GET on whatever locality we chose
            // We pass the pointer to our local memory in the PUT, and it is used
            // by the serialization routines so that the copy from parcelport memory
            // is performed directly into our user memory. This avoids the need
            // to copy the data from a serialization buffer into our memory
            {
#ifdef USE_CLEANING_THREAD
                ++FuturesWaiting[send_rank];
                std::lock_guard<hpx::spinlock> lk(FuturesMutex);
#endif
                std::size_t buffer_address =
                    reinterpret_cast<std::size_t>(general_buffer.data());
                //
                auto temp_future =
                    hpx::async(hpx::launch::fork, actRead, locality,
                        memory_offset, options.transfer_size_B, buffer_address)
                        .then(hpx::launch::sync,
                            [=](hpx::future<transfer_buffer_type>&& buffer)
                                -> void {
                                return transfer_data(
                                    general_buffer, std::move(buffer));
                            })
                        .then(hpx::launch::sync,
                            [=](hpx::future<void> fut) -> int {
                                // Retrieve the serialized data buffer that was
                                // returned from the action
                                // try to minimize copies by receiving into our
                                // custom buffer
                                fut.get();
                                --FuturesWaiting[send_rank];
                                return TEST_SUCCESS;
                            });

                ActiveFutures[send_rank].push_back(std::move(temp_future));
            }
        }
#ifdef USE_CLEANING_THREAD
        // tell the cleaning thread it's time to stop
        FuturesActive = false;
        // wait for cleanup thread to terminate before we reduce any remaining
        // futures
        int removed = cleaner.get();
        DEBUG_OUTPUT(
            2, "Cleaning thread rank " << rank << " removed " << removed);
#endif

#ifdef USE_PARCELPORT_THREAD
        // tell the parcelport thread it's time to stop
        FuturesActive = false;
        background.get();
#endif
        //
        hpx::chrono::high_resolution_timer movetimer;
        std::vector<hpx::future<int>> final_list;
        for (uint64_t i = 0; i < nranks; i++)
        {
            // move the contents of intermediate vector into final list
            final_list.reserve(final_list.size() + ActiveFutures[i].size());
            std::move(ActiveFutures[i].begin(), ActiveFutures[i].end(),
                std::back_inserter(final_list));
            ActiveFutures[i].clear();
        }
        DEBUG_ONLY(double movetime = movetimer.elapsed();
            int numwait = static_cast<int>(final_list.size());)
        hpx::chrono::high_resolution_timer futuretimer;

        DEBUG_OUTPUT(1, "Waiting for whena_all future on rank " << rank);
        hpx::future<int> result =
            hpx::when_all(final_list).then(hpx::launch::sync, reduce);
        result.get();
#ifdef USE_CLEANING_THREAD
        int total = numwait + removed;
#else
        DEBUG_ONLY(int total = numwait;)
#endif
        DEBUG_OUTPUT(3,
            "Future timer, rank "
                << rank << " waiting on " << numwait << " total " << total
                << " " << futuretimer.elapsed() << " Move time " << movetime);
    }
    hpx::distributed::barrier::synchronize();
    //
    if (rank == 0)
        std::cout << std::endl;
    //
    uint64_t active_ranks = options.all2all ? nranks : 1;
    double readMB = static_cast<double>(
        active_ranks * options.local_storage_MB * options.iterations);
    double readTime = timerRead.elapsed();
    double readBW = readMB / readTime;
    double IOPS = static_cast<double>(options.iterations * num_transfer_slots);
    double IOPs_s = IOPS / readTime;
    if (rank == 0)
    {
        std::cout << "Total time           : " << readTime << "\n";
        std::cout << "Memory Transferred   : " << readMB << " MB \n";
        std::cout << "Number of local IOPs : " << IOPS << "\n";
        std::cout << "IOPs/s (local)       : " << IOPs_s << "\n";
        std::cout << "Aggregate BW Read    : " << readBW << " MB/s"
                  << std::endl;
        // a complete set of results that our python matplotlib script will ingest
        char const* msg =
            "CSVData, read, network, {1}, ranks, "
            "{2}, threads, {3}, Memory, {4}, IOPsize, {5}, IOPS/s, {6}, "
            "BW(MB/s), {7}, ";
        hpx::util::format_to(std::cout, msg, options.network, nranks,
            options.threads, readMB, options.transfer_size_B, IOPs_s, readBW)
            << std::endl;
    }
}

//----------------------------------------------------------------------------
// Main test loop which randomly sends packets of data from one locality to
// another looping over the entire buffer address space and timing the total
// transmit/receive time to see how well we're doing.
int hpx_main(hpx::program_options::variables_map& vm)
{
    DEBUG_OUTPUT(3, "HPX main");
    //
    hpx::id_type here = hpx::find_here();
    uint64_t rank = hpx::naming::get_locality_id_from_id(here);
    std::string name = hpx::get_locality_name();
    uint64_t nranks = hpx::get_num_localities().get();
    std::size_t current = hpx::get_worker_thread_num();
    std::vector<hpx::id_type> remotes = hpx::find_remote_localities();
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    //
    if (nranks > MAX_RANKS)
    {
        std::cerr << "This test can only be run using " << MAX_RANKS
                  << " nodes, please recompile this test with the"
                     " MAX_RANKS set to a higher number "
                  << std::endl;
        return 1;
    }

    char const* msg = "hello world from OS-thread {1} on locality "
                      "{2} rank {3} hostname {4}";
    hpx::util::format_to(
        std::cout, msg, current, hpx::get_locality_id(), rank, name.c_str())
        << std::endl;
    //
    // extract command line argument
    test_options options;
    options.transfer_size_B = vm["transferKB"].as<std::uint64_t>() * 1024;
    options.local_storage_MB = vm["localMB"].as<std::uint64_t>();
    options.global_storage_MB = vm["globalMB"].as<std::uint64_t>();
    options.iterations = vm["iterations"].as<std::uint64_t>();
    options.threads = hpx::get_os_thread_count();
    options.network = vm["parceltype"].as<std::string>();
    options.all2all = vm["all-to-all"].as<bool>();
    options.distribution =
        vm["distribution"].as<std::uint64_t>() ? true : false;
    options.nolocal = vm["no-local"].as<bool>();
    options.semaphore = vm["semaphore"].as<std::uint64_t>();
    options.warmup = false;

    //
    if (options.global_storage_MB > 0)
    {
        options.local_storage_MB = options.global_storage_MB / nranks;
    }

    DEBUG_OUTPUT(2, "Allocating local storage on rank " << rank);
    allocate_local_storage(options.local_storage_MB * 1024 * 1024);
    //
    std::uint64_t num_transfer_slots = static_cast<std::uint64_t>(1024) *
        static_cast<std::uint64_t>(1024) * options.local_storage_MB /
        options.transfer_size_B;
    DEBUG_OUTPUT(1,
        "num ranks " << nranks << ", num_transfer_slots " << num_transfer_slots
                     << " on rank " << rank);
    //
    if (options.nolocal && nranks == 1)
    {
        std::cout << "Fatal error, cannot use nolocal with a single rank"
                  << std::endl;
        std::terminate();
    }
    //
    std::mt19937 gen;
    std::uniform_int_distribution<> random_rank(0, (int) nranks - 1);
    std::uniform_int_distribution<> random_slot(
        0, (int) num_transfer_slots - 1);
    //
    ActiveFutures.reserve(nranks);
    for (uint64_t i = 0; i < nranks; i++)
    {
        FuturesWaiting[i].store(0);
        ActiveFutures.push_back(std::vector<hpx::future<int>>());
    }

    test_options warmup = options;
    warmup.iterations = 1;
    warmup.warmup = true;
    test_write(rank, nranks, num_transfer_slots, gen, random_rank, random_slot,
        warmup);
    //
    test_write(rank, nranks, num_transfer_slots, gen, random_rank, random_slot,
        options);
    test_read(rank, nranks, num_transfer_slots, gen, random_rank, random_slot,
        options);
    //
    delete_local_storage();

    DEBUG_OUTPUT(3, "Calling finalize " << rank);
    if (rank == 0)
        return hpx::finalize();
    else
        return 0;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Configure application-specific options
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("localMB",
        hpx::program_options::value<std::uint64_t>()->default_value(256),
        "Sets the storage capacity (in MB) on each node.\n"
        "The total storage will be num_ranks * localMB");

    desc_commandline.add_options()("globalMB",
        hpx::program_options::value<std::uint64_t>()->default_value(0),
        "Sets the storage capacity (in MB) for the entire job.\n"
        "The storage per node will be globalMB / num_ranks\n"
        "By default, localMB is used, setting this overrides localMB value.");

    desc_commandline.add_options()("transferKB",
        hpx::program_options::value<std::uint64_t>()->default_value(64),
        "Sets the default block transfer size (in KB).\n"
        "Each put/get IOP will be this size");

    desc_commandline.add_options()("semaphore",
        hpx::program_options::value<std::uint64_t>()->default_value(16),
        "The max amount of simultaneous put/get operations to allow.\n");

    desc_commandline.add_options()("iterations",
        hpx::program_options::value<std::uint64_t>()->default_value(5),
        "The number of iterations over the global memory.\n");

    desc_commandline.add_options()("all-to-all",
        hpx::program_options::value<bool>()->default_value(true),
        "When set, all ranks send to all others, when off, "
        "only rank 0 sends to the others.\n");

    desc_commandline.add_options()("no-local",
        hpx::program_options::value<bool>()->default_value(false),
        "When set, non local transfers are made, "
        "ranks send to the others but not to themselves.\n");

    // if the user does not set parceltype on the command line,
    // we use a default of unknowm so we don't mistake plots
    desc_commandline.add_options()("parceltype",
        hpx::program_options::value<std::string>()->default_value("unknown"),
        "Pass in the parcelport network type being tested,"
        "this value has no effect on the code and is used only in output "
        "so that plotting scripts know what network type was active during \
            the test.\n");

    desc_commandline.add_options()("distribution",
        hpx::program_options::value<std::uint64_t>()->default_value(1),
        "Specify the distribution of data blocks to send/receive,\n"
        "in random mode, blocks of data are sent from one rank to \
            any other rank (including itself),"
        "in block-cyclic mode, blocks of data are sent from one rank to \
            other ranks in block-cyclic order,"
        "0 : random \n"
        "1 : block cyclic");

    // Initialize and run HPX, this test requires to run hpx_main on all localities
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    DEBUG_OUTPUT(3, "Calling hpx::init");
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
#endif
