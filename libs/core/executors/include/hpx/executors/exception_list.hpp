//  Copyright (c) 2007-2025 Hartmut Kaiser
//                2017-2018 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/execution_policy_fwd.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Result = void,
            typename Enable = void>
        struct handle_exception_impl;

        template <typename ExPolicy, typename Result>
        struct handle_exception_impl<ExPolicy, Result,
            std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy> &&
                !hpx::is_unsequenced_execution_policy_v<ExPolicy>>>
        {
            using type = Result;

            [[noreturn]] static Result call()
            {
                try
                {
                    throw;    //-V667
                }
                catch (std::bad_alloc const&)
                {
                    throw;
                }
                catch (hpx::exception_list const&)
                {
                    throw;
                }
                catch (...)
                {
                    throw hpx::exception_list(std::current_exception());
                }
            }

            static Result call(hpx::future<Result> f)
            {
                HPX_ASSERT(f.has_exception());
                return f.get();
            }

            [[noreturn]] static Result call(std::exception_ptr const& e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (std::bad_alloc const&)
                {
                    throw;
                }
                catch (hpx::exception_list const&)
                {
                    throw;
                }
                catch (...)
                {
                    throw hpx::exception_list(std::current_exception());
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename ExPolicy, typename Result>
        struct handle_exception_impl<ExPolicy, Result,
            std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy> &&
                !hpx::is_unsequenced_execution_policy_v<ExPolicy>>>
        {
            using type = future<Result>;

            static future<Result> call()
            {
                try
                {
                    throw;    //-V667
                }
                catch (std::bad_alloc const& e)
                {
                    return hpx::make_exceptional_future<Result>(e);
                }
                catch (hpx::exception_list const& el)
                {
                    return hpx::make_exceptional_future<Result>(el);
                }
                catch (...)
                {
                    return hpx::make_exceptional_future<Result>(
                        hpx::exception_list(std::current_exception()));
                }
            }

            static future<Result> call(future<Result> f)
            {
                HPX_ASSERT(f.has_exception());
                // Intel complains if this is not explicitly moved
#if defined(HPX_INTEL_VERSION)
                return HPX_MOVE(f);
#else
                return f;
#endif
            }

            static future<Result> call(std::exception_ptr const& e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (std::bad_alloc const&)
                {
                    return hpx::make_exceptional_future<Result>(
                        std::current_exception());
                }
                catch (hpx::exception_list const& el)
                {
                    return hpx::make_exceptional_future<Result>(el);
                }
                catch (...)
                {
                    return hpx::make_exceptional_future<Result>(
                        hpx::exception_list(std::current_exception()));
                }
            }
        };

        HPX_CXX_CORE_EXPORT using exception_list_termination_handler_type =
            hpx::function<void()>;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
        set_exception_list_termination_handler(
            exception_list_termination_handler_type f);

        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void
        exception_list_termination_handler();

        ///////////////////////////////////////////////////////////////////////
        // any exceptions thrown by algorithms executed with an unsequenced
        // policy are to call terminate.
        template <typename ExPolicy, typename Result>
        struct handle_exception_impl<ExPolicy, Result,
            std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy> &&
                hpx::is_unsequenced_execution_policy_v<ExPolicy>>>
        {
            using type = Result;

            [[noreturn]] static Result call()
            {
                exception_list_termination_handler();
            }

            [[noreturn]] static hpx::future<Result> call(hpx::future<Result>&&)
            {
                exception_list_termination_handler();
            }

            [[noreturn]] static hpx::future<Result> call(
                std::exception_ptr const&)
            {
                exception_list_termination_handler();
            }
        };

        template <typename ExPolicy, typename Result>
        struct handle_exception_impl<ExPolicy, Result,
            std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy> &&
                hpx::is_unsequenced_execution_policy_v<ExPolicy>>>
        {
            using type = future<Result>;

            [[noreturn]] static future<Result> call()
            {
                exception_list_termination_handler();
            }

            [[noreturn]] static future<Result> call(future<Result>)
            {
                exception_list_termination_handler();
            }

            [[noreturn]] static future<Result> call(std::exception_ptr const&)
            {
                exception_list_termination_handler();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename ExPolicy, typename Result>
        struct handle_exception_impl<ExPolicy, Result,
            std::enable_if_t<
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
        {
            using type = Result;

            [[noreturn]] static Result call()
            {
                try
                {
                    throw;    //-V667
                }
                catch (std::bad_alloc const&)
                {
                    throw;
                }
                catch (hpx::exception_list const&)
                {
                    throw;
                }
                catch (...)
                {
                    throw hpx::exception_list(std::current_exception());
                }
            }

            [[noreturn]] static Result call(std::exception_ptr const& e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (std::bad_alloc const&)
                {
                    throw;
                }
                catch (hpx::exception_list const&)
                {
                    throw;
                }
                catch (...)
                {
                    throw hpx::exception_list(std::current_exception());
                }
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Result = void>
        struct handle_exception
          : handle_exception_impl<std::decay_t<ExPolicy>, Result>
        {
        };
        /// \endcond
    }    // namespace detail

    // we're just reusing our existing implementation
    HPX_CXX_CORE_EXPORT using hpx::exception_list;
}    // namespace hpx::parallel
