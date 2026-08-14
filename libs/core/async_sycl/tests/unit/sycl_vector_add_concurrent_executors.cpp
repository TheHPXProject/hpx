//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

#if defined(HPX_HAVE_SYCL)
#include <hpx/async_sycl/sycl_executor.hpp>
#include <hpx/async_sycl/sycl_future.hpp>

#include <atomic>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <sycl/sycl.hpp>

#include "common/sycl_vector_add_test_utils.hpp"

constexpr size_t vector_size = 80000000;
constexpr size_t number_executors = 8;
constexpr size_t number_repetitions = 10;
constexpr int slice_size_per_executor =
    vector_size / (number_executors * number_repetitions);
std::atomic<size_t> number_executors_finished = 0;

// Dividing the previous vectorAdd example into one that uses multiple chunks
// worked on by multiple executors concurrently Intended to uncover racing and
// overheads.  To this end, there is nothing outside the executors to keep the
// SYCL runtime alive (effectively also testing if the default queue in our
// sycl_event_callback file keeps the runtime alive)
void VectorAdd(std::vector<size_t> const& a_vector,
    std::vector<size_t> const& b_vector, std::vector<size_t>& add_parallel)
{
    std::vector<hpx::shared_future<void>> futs(number_executors);
    for (size_t exec_id = 0; exec_id < number_executors; exec_id++)
    {
        auto const a_data = a_vector.data();
        auto const b_data = b_vector.data();
        auto c_data = add_parallel.data();
        futs[exec_id] = hpx::async([exec_id, a_data, b_data, c_data]() {
            hpx::sycl::experimental::sycl_executor exec(
                sycl::default_selector_v);
            sycl::range<1> num_items{slice_size_per_executor};
            for (size_t repetition = 0; repetition < number_repetitions - 1;
                repetition++)
            {
                size_t const current_chunk_id = slice_size_per_executor *
                    (exec_id * number_repetitions + repetition);
                sycl::buffer a_buf(a_data + current_chunk_id, num_items);
                sycl::buffer b_buf(b_data + current_chunk_id, num_items);
                sycl::buffer add_buf(c_data + current_chunk_id, num_items);

                // Testing post
                hpx::apply(exec, &sycl::queue::submit, [&](sycl::handler& h) {
                    sycl::accessor a(a_buf, h, sycl::read_only);
                    sycl::accessor b(b_buf, h, sycl::read_only);
                    sycl::accessor add(
                        add_buf, h, sycl::write_only, sycl::no_init);
                    h.parallel_for(
                        num_items, [=](auto i) { add[i] = a[i] + b[i]; });
                });
            }
            size_t const last_chunk_id = slice_size_per_executor *
                (exec_id * number_repetitions + number_repetitions - 1);
            sycl::buffer a_buf(a_data + last_chunk_id, num_items);
            sycl::buffer b_buf(b_data + last_chunk_id, num_items);
            sycl::buffer add_buf(c_data + last_chunk_id, num_items);

            // Testing async_exec
            auto kernel_fut =
                hpx::async(exec, &sycl::queue::submit, [&](sycl::handler& h) {
                    sycl::accessor a(a_buf, h, sycl::read_only);
                    sycl::accessor b(b_buf, h, sycl::read_only);
                    sycl::accessor add(
                        add_buf, h, sycl::write_only, sycl::no_init);
                    h.parallel_for(
                        num_items, [=](auto i) { add[i] = a[i] + b[i]; });
                });
            auto final_fut = kernel_fut.then([exec_id](auto&& fut) {
                std::cout << "OKAY: SYCL executor " << exec_id + 1
                          << " is done! " << std::endl;
                fut.get();
            });
            final_fut.get();
            number_executors_finished++;
        });
    }
    auto when = hpx::when_all(futs);
    when.get();

    // Check results 1: Counting the number of called continuations Must match
    // the number of executors or something is seriously wrong
    if (number_executors_finished == number_executors)
    {
        std::cout << "OKAY: All executors done!" << std::endl;
    }
    else
    {
        std::cerr << "ERROR: Number of finished continuations ("
                  << number_executors_finished
                  << ") do not match overall number of executors ("
                  << number_executors << ")!" << std::endl;
        std::terminate();
    }
}

int hpx_main(int, char*[])
{
    // Enable polling for the future
    hpx::sycl::experimental::detail::register_polling(
        hpx::resource::get_thread_pool(0));
    std::cout << "SYCL Future polling enabled!\n";
    std::cout << "SYCL language version: " << SYCL_LANGUAGE_VERSION << "\n";

    // input vectors
    std::vector<size_t> a(vector_size), b(vector_size),
        add_parallel(vector_size);

    fill_vector_add_input(a, b, add_parallel);
    // Run vector add using multiple executors and multiple cpu tasks
    VectorAdd(a, b, add_parallel);
    // Compare results to sequentiell version
    check_vector_add_results(a, b, add_parallel);
    print_vector_results(a, b, add_parallel);

    // Disable polling
    std::cout << "Disabling SYCL future polling.\n";
    hpx::sycl::experimental::detail::unregister_polling(
        hpx::resource::get_thread_pool(0));
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
#else
#include <iostream>

// Handle none-sycl builds
int main()
{
    std::cerr << "SYCL Support was not given at compile time! " << std::endl;
    std::cerr << "Please check your build configuration!" << std::endl;
    std::cerr << "Exiting..." << std::endl;
    return 1;    // Fail test, as it was meant to test SYCL
}
#endif
