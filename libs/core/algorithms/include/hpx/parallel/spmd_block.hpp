//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/util/adapt_sharing_mode.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::lcos::local {

    /// The class spmd_block defines an interface for launching multiple images
    /// while giving handles to each image to interact with the remaining
    /// images. The \a define_spmd_block function templates create multiple
    /// images of a user-defined function (or lambda) and launches them in a
    /// possibly separate thread. A temporary spmd block object is created and
    /// diffused to each image. The constraint for the function (or lambda)
    /// given to the define_spmd_block function is to accept a spmd_block as
    /// first parameter.
    HPX_CXX_CORE_EXPORT struct spmd_block
    {
    private:
        using barrier_type = hpx::barrier<>;
        using table_type =
            std::map<std::set<std::size_t>, std::shared_ptr<barrier_type>>;
        using mutex_type = hpx::mutex;

    public:
        explicit spmd_block(std::size_t num_images, std::size_t image_id,
            barrier_type& barrier, table_type& barriers, mutex_type& mtx)
          : num_images_(num_images)
          , image_id_(image_id)
          , barrier_(barrier)
          , barriers_(barriers)
          , mtx_(mtx)
        {
        }

        // Note: spmd_block class is movable/move-assignable but not
        // copyable/copy-assignable

        spmd_block(spmd_block&&) = default;
        spmd_block(spmd_block const&) = delete;

        spmd_block& operator=(spmd_block&&) = default;
        spmd_block& operator=(spmd_block const&) = delete;

        ~spmd_block() = default;

        [[nodiscard]] std::size_t get_num_images() const
        {
            return num_images_;
        }

        [[nodiscard]] std::size_t this_image() const
        {
            return image_id_;
        }

        void sync_all() const
        {
            barrier_.get().arrive_and_wait();
        }

        void sync_images(std::set<std::size_t> const& images) const
        {
            using lock_type = std::lock_guard<mutex_type>;

            table_type& brs = barriers_.get();
            table_type::iterator it;

            // Critical section
            {
                lock_type lk(mtx_);
                it = brs.find(images);

                if (it == brs.end())
                {
                    it = brs
                             .insert({images,
                                 std::make_shared<barrier_type>(images.size())})
                             .first;
                }
            }

            if (images.find(image_id_) != images.end())
            {
                it->second->arrive_and_wait();
            }
        }

        void sync_images(std::vector<std::size_t> const& input_images) const
        {
            std::set<std::size_t> const images(
                input_images.begin(), input_images.end());
            sync_images(images);
        }

        template <typename Iterator>
        std::enable_if_t<std::input_iterator<Iterator>> sync_images(
            Iterator begin, Iterator end) const
        {
            std::set<std::size_t> const images(begin, end);
            sync_images(images);
        }

        template <typename... I>
        std::enable_if_t<util::all_of_v<std::is_integral<I>...>> sync_images(
            I... i) const
        {
            std::set<std::size_t> const images = {
                static_cast<std::size_t>(i)...};
            sync_images(images);
        }

    private:
        std::size_t num_images_;
        std::size_t image_id_;
        mutable std::reference_wrapper<barrier_type> barrier_;
        mutable std::reference_wrapper<table_type> barriers_;
        mutable std::reference_wrapper<mutex_type> mtx_;
    };

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename F>
        struct spmd_block_helper
        {
        private:
            using barrier_type = hpx::barrier<>;
            using table_type =
                std::map<std::set<std::size_t>, std::shared_ptr<barrier_type>>;
            using mutex_type = hpx::mutex;

        public:
            std::shared_ptr<barrier_type> barrier_;
            std::shared_ptr<table_type> barriers_;
            std::shared_ptr<mutex_type> mtx_;
            std::decay_t<F> f_;
            std::size_t num_images_;

            template <typename... Ts>
            void operator()(std::size_t image_id, Ts&&... ts) const
            {
                spmd_block block(
                    num_images_, image_id, *barrier_, *barriers_, *mtx_);
                HPX_INVOKE(f_, HPX_MOVE(block), HPX_FORWARD(Ts, ts)...);
            }
        };
    }    // namespace detail

    // Asynchronous version
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
        typename... Args,
        typename = std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy>>>
    decltype(auto) define_spmd_block(
        ExPolicy&& policy, std::size_t num_images, F&& f, Args&&... args)
    {
        static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
            "hpx::is_async_execution_policy<ExPolicy>");

        using ftype = std::decay_t<F>;
        using first_type = hpx::util::first_argument_t<ftype>;

        using barrier_type = hpx::barrier<>;
        using table_type =
            std::map<std::set<std::size_t>, std::shared_ptr<barrier_type>>;
        using mutex_type = hpx::mutex;

        static_assert(std::is_same_v<spmd_block, first_type>,
            "define_spmd_block() needs a function or lambda that "
            "has at least a local spmd_block as 1st argument");

        std::shared_ptr<barrier_type> barrier =
            std::make_shared<barrier_type>(num_images);
        std::shared_ptr<table_type> barriers = std::make_shared<table_type>();
        std::shared_ptr<mutex_type> mtx = std::make_shared<mutex_type>();

        // The tasks launched here may synchronize between each other. This may
        // lead to deadlocks if the tasks are combined to run on the same
        // thread.
        decltype(auto) hinted_policy =
            hpx::execution::experimental::adapt_sharing_mode(policy,
                hpx::threads::thread_sharing_hint::do_not_combine_tasks);

        return hpx::parallel::execution::bulk_async_execute(
            hpx::execution::to_hierarchical_spawning(hinted_policy.executor()),
            detail::spmd_block_helper<F>{
                barrier, barriers, mtx, HPX_FORWARD(F, f), num_images},
            hpx::util::counting_shape(num_images), HPX_FORWARD(Args, args)...);
    }

    // Synchronous version
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
        typename... Args,
        typename =
            std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy>>>
    void define_spmd_block(
        ExPolicy&& policy, std::size_t num_images, F&& f, Args&&... args)
    {
        static_assert(hpx::is_execution_policy_v<ExPolicy>,
            "hpx::is_execution_policy_v<ExPolicy>");

        using ftype = std::decay_t<F>;
        using first_type = hpx::util::first_argument_t<ftype>;

        using barrier_type = hpx::barrier<>;
        using table_type =
            std::map<std::set<std::size_t>, std::shared_ptr<barrier_type>>;
        using mutex_type = hpx::mutex;

        static_assert(std::is_same_v<spmd_block, first_type>,
            "define_spmd_block() needs a lambda that "
            "has at least a spmd_block as 1st argument");

        std::shared_ptr<barrier_type> barrier =
            std::make_shared<barrier_type>(num_images);
        std::shared_ptr<table_type> barriers = std::make_shared<table_type>();
        std::shared_ptr<mutex_type> mtx = std::make_shared<mutex_type>();

        // The tasks launched here may synchronize between each other. This may
        // lead to deadlocks if the tasks are combined to run on the same
        // thread.
        decltype(auto) hinted_policy =
            hpx::execution::experimental::adapt_sharing_mode(policy,
                hpx::threads::thread_sharing_hint::do_not_combine_tasks);

        hpx::parallel::execution::bulk_sync_execute(
            hpx::execution::to_hierarchical_spawning(hinted_policy.executor()),
            detail::spmd_block_helper<F>{
                barrier, barriers, mtx, HPX_FORWARD(F, f), num_images},
            hpx::util::counting_shape(num_images), HPX_FORWARD(Args, args)...);
    }

    HPX_CXX_CORE_EXPORT template <typename F, typename... Args>
    void define_spmd_block(std::size_t num_images, F&& f, Args&&... args)
    {
        define_spmd_block(hpx::execution::par, num_images, HPX_FORWARD(F, f),
            HPX_FORWARD(Args, args)...);
    }
}    // namespace hpx::lcos::local

namespace hpx::parallel {

    /// The class spmd_block defines an interface for launching multiple images
    /// while giving handles to each image to interact with the remaining
    /// images. The \a define_spmd_block function templates create multiple
    /// images of a user-defined function (or lambda) and launches them in a
    /// possibly separate thread. A temporary spmd block object is created and
    /// diffused to each image. The constraint for the function (or lambda)
    /// given to the define_spmd_block function is to accept a spmd_block as
    /// first parameter.
    HPX_CXX_CORE_EXPORT using spmd_block = hpx::lcos::local::spmd_block;

    // Asynchronous version
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
        typename... Args,
        typename = std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy>>>
    decltype(auto) define_spmd_block(
        ExPolicy&& policy, std::size_t num_images, F&& f, Args&&... args)
    {
        return hpx::lcos::local::define_spmd_block(
            HPX_FORWARD(ExPolicy, policy), num_images, HPX_FORWARD(F, f),
            HPX_FORWARD(Args, args)...);
    }

    // Synchronous version
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
        typename... Args,
        typename =
            std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy>>>
    void define_spmd_block(
        ExPolicy&& policy, std::size_t num_images, F&& f, Args&&... args)
    {
        hpx::lcos::local::define_spmd_block(HPX_FORWARD(ExPolicy, policy),
            num_images, HPX_FORWARD(F, f), HPX_FORWARD(Args, args)...);
    }

    HPX_CXX_CORE_EXPORT template <typename F, typename... Args>
    void define_spmd_block(std::size_t num_images, F&& f, Args&&... args)
    {
        hpx::lcos::local::define_spmd_block(hpx::execution::par, num_images,
            HPX_FORWARD(F, f), HPX_FORWARD(Args, args)...);
    }
}    // namespace hpx::parallel
