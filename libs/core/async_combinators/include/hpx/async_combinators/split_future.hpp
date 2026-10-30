//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/async_combinators/split_future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a split_future is an operator allowing to split a given
    /// future of a sequence of values (any tuple, std::pair, or std::array)
    /// into an equivalent container of futures where each future represents
    /// one of the values from the original future. In some sense this function
    /// provides the inverse operation of \a when_all.
    ///
    /// \param f    [in] A future holding an arbitrary sequence of values stored
    ///             in a tuple-like container. This facility supports
    ///             \a hpx::tuple<>, \a std::pair<T1, T2>, and
    ///             \a std::array<T, N>
    ///
    /// \return     Returns an equivalent container (same container type as
    ///             passed as the argument) of futures, where each future refers
    ///             to the corresponding value in the input parameter. All the
    ///             returned futures become ready once the input future has
    ///             become ready. If the input future is exceptional, all output
    ///             futures will be exceptional as well.
    ///
    /// \note       The following cases are special:
    /// \code
    ///     tuple<future<void> > split_future(future<tuple<> > && f);
    ///     array<future<void>, 1> split_future(future<array<T, 0> > && f);
    /// \endcode
    ///             here the returned futures are directly representing the
    ///             futures which were passed to the function.
    ///
    /// \note       Passing an invalid future to split_future does not throw
    ///             immediately. Instead, it returns futures that carry an
    ///             `hpx::error::no_state` exception, which will throw upon calling
    ///             `.get()`.
    ///
    template <typename... Ts>
    inline tuple<future<Ts>...> split_future(future<tuple<Ts...>>&& f);

    /// The function \a split_future is an operator allowing to split a given
    /// future of a sequence of values (any std::vector)
    /// into a std::vector of futures where each future represents
    /// one of the values from the original std::vector. In some sense this
    /// function provides the inverse operation of \a when_all.
    ///
    /// \param f    [in] A future holding an arbitrary sequence of values stored
    ///             in a std::vector.
    /// \param size [in] The number of elements the vector will hold once the
    ///             input future has become ready
    ///
    /// \return     Returns a std::vector of futures, where each future refers
    ///             to the corresponding value in the input parameter. All
    ///             the returned futures become ready once the input future has
    ///             become ready. If the input future is exceptional, all output
    ///             futures will be exceptional as well.
    ///
    template <typename T>
    inline std::vector<future<T>> split_future(
        future<std::vector<T>>&& f, std::size_t size);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/type_support.hpp>

#include <array>
#include <cstddef>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename ContResult>
        class split_nth_continuation
          : public lcos::detail::future_data<ContResult>
        {
            using base_type = lcos::detail::future_data<ContResult>;

        private:
            template <std::size_t I, typename T>
            void on_ready(
                traits::detail::shared_state_ptr_for_t<T> const& state)
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        using result_type = traits::future_traits_t<T>;
                        result_type* result = state->get_result();
                        this->base_type::set_value(
                            HPX_MOVE(hpx::get<I>(*result)));
                    },
                    [&](std::exception_ptr ep) {
                        this->base_type::set_exception(HPX_MOVE(ep));
                    });
            }

        public:
            template <std::size_t I, typename Future>
            void attach(Future& future)
            {
                using shared_state_ptr =
                    traits::detail::shared_state_ptr_for_t<Future>;

                // Bind an on_completed handler to this future which will wait
                // for the future and will transfer its result to the new
                // future.
                hpx::intrusive_ptr<split_nth_continuation> this_(this);
                shared_state_ptr const& state =
                    hpx::traits::detail::get_shared_state(future);

                if (!state)
                {
                    this->base_type::set_exception(HPX_GET_EXCEPTION(
                        hpx::error::no_state, "split_nth_continuation::attach",
                        "the future has no valid shared state"));
                    return;
                }

                state->execute_deferred();
                state->set_on_completed(util::deferred_call(
                    &split_nth_continuation::on_ready<I, Future>,
                    HPX_MOVE(this_), state));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, typename Tuple, std::size_t I,
            typename Future>
        inline hpx::traits::detail::shared_state_ptr_t<
            typename hpx::tuple_element<I, Tuple>::type>
        extract_nth_continuation(Future& future)
        {
            using shared_state = split_nth_continuation<Result>;

            hpx::traits::detail::shared_state_ptr_t<Result> p(
                new shared_state());

            static_cast<shared_state*>(p.get())->template attach<I>(future);
            return p;
        }

        ///////////////////////////////////////////////////////////////////////
        template <std::size_t I, typename Tuple>
        HPX_FORCEINLINE hpx::future<typename hpx::tuple_element<I, Tuple>::type>
        extract_nth_future(hpx::future<Tuple>& future)
        {
            using result_type = hpx::tuple_element_t<I, Tuple>;

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                extract_nth_continuation<result_type, Tuple, I>(future));
        }

        template <std::size_t I, typename Tuple>
        HPX_FORCEINLINE hpx::future<typename hpx::tuple_element<I, Tuple>::type>
        extract_nth_future(hpx::shared_future<Tuple>& future)
        {
            using result_type = hpx::tuple_element_t<I, Tuple>;

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                extract_nth_continuation<result_type, Tuple, I>(future));
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename... Ts, std::size_t... Is>
        HPX_FORCEINLINE hpx::tuple<hpx::future<Ts>...> split_future_helper(
            hpx::future<hpx::tuple<Ts...>>&& f, hpx::util::index_pack<Is...>)
        {
            return hpx::make_tuple(extract_nth_future<Is>(f)...);
        }

        template <typename... Ts, std::size_t... Is>
        HPX_FORCEINLINE hpx::tuple<hpx::future<Ts>...> split_future_helper(
            hpx::shared_future<hpx::tuple<Ts...>>&& f,
            hpx::util::index_pack<Is...>)
        {
            return hpx::make_tuple(extract_nth_future<Is>(f)...);
        }

        ///////////////////////////////////////////////////////////////////////
#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
        template <typename... Ts, std::size_t... Is>
        HPX_FORCEINLINE std::tuple<hpx::future<Ts>...> split_future_helper(
            hpx::future<std::tuple<Ts...>>&& f, hpx::util::index_pack<Is...>)
        {
            return std::make_tuple(extract_nth_future<Is>(f)...);
        }

        template <typename... Ts, std::size_t... Is>
        HPX_FORCEINLINE std::tuple<hpx::future<Ts>...> split_future_helper(
            hpx::shared_future<std::tuple<Ts...>>&& f,
            hpx::util::index_pack<Is...>)
        {
            return std::make_tuple(extract_nth_future<Is>(f)...);
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        template <typename T1, typename T2>
        HPX_FORCEINLINE std::pair<hpx::future<T1>, hpx::future<T2>>
        split_future_helper(hpx::future<std::pair<T1, T2>>&& f)
        {
            return std::make_pair(
                extract_nth_future<0>(f), extract_nth_future<1>(f));
        }

        template <typename T1, typename T2>
        HPX_FORCEINLINE std::pair<hpx::future<T1>, hpx::future<T2>>
        split_future_helper(hpx::shared_future<std::pair<T1, T2>>&& f)
        {
            return std::make_pair(
                extract_nth_future<0>(f), extract_nth_future<1>(f));
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename ContResult>
        class split_continuation : public lcos::detail::future_data<ContResult>
        {
            using base_type = lcos::detail::future_data<ContResult>;

        private:
            template <typename T>
            void on_ready(std::size_t i,
                traits::detail::shared_state_ptr_for_t<T> const& state)
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        using result_type = traits::future_traits_t<T>;
                        result_type* result = state->get_result();
                        if (i >= result->size())
                        {
                            HPX_THROW_EXCEPTION(hpx::error::length_error,
                                "split_continuation::on_ready",
                                "index out of bounds");
                        }
                        this->base_type::set_value(HPX_MOVE((*result)[i]));
                    },
                    [&](std::exception_ptr ep) {
                        this->base_type::set_exception(HPX_MOVE(ep));
                    });
            }

        public:
            template <typename Future>
            void attach(std::size_t i, Future& future)
            {
                using shared_state_ptr =
                    traits::detail::shared_state_ptr_for_t<Future>;

                // Bind an on_completed handler to this future which will wait
                // for the future and will transfer its result to the new
                // future.
                hpx::intrusive_ptr<split_continuation> this_(this);
                shared_state_ptr const& state =
                    hpx::traits::detail::get_shared_state(future);

                if (!state)
                {
                    this->base_type::set_exception(HPX_GET_EXCEPTION(
                        hpx::error::no_state, "split_continuation::attach",
                        "the future has no valid shared state"));
                    return;
                }

                state->execute_deferred();
                state->set_on_completed(
                    util::deferred_call(&split_continuation::on_ready<Future>,
                        HPX_MOVE(this_), i, state));
            }
        };

        template <typename T, typename Future>
        inline hpx::future<T> extract_future_array(
            std::size_t i, Future& future)
        {
            using shared_state = split_continuation<T>;

            hpx::traits::detail::shared_state_ptr_t<T> p(new shared_state());

            static_cast<shared_state*>(p.get())->attach(i, future);
            return hpx::traits::future_access<hpx::future<T>>::create(p);
        }

        template <std::size_t N, typename T, typename Future>
        inline std::array<hpx::future<T>, N> split_future_helper_array(
            Future&& f)
        {
            std::array<hpx::future<T>, N> result;

            for (std::size_t i = 0; i != N; ++i)
            {
                result[i] = extract_future_array<T>(i, f);
            }

            return result;
        }

        template <typename T, typename Future>
        inline std::vector<hpx::future<T>> split_future_helper_vector(
            Future&& f, std::size_t size)
        {
            std::vector<hpx::future<T>> result;
            result.reserve(size);

            for (std::size_t i = 0; i != size; ++i)
            {
                result.push_back(extract_future_array<T>(i, f));
            }

            return result;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Note: Passing an invalid future to split_future does not throw
    /// immediately. Instead, it returns futures that carry an
    /// `hpx::error::no_state` exception, which will throw upon calling `.get()`.
    HPX_CXX_CORE_EXPORT template <typename... Ts>
    HPX_FORCEINLINE hpx::tuple<hpx::future<Ts>...> split_future(
        hpx::future<hpx::tuple<Ts...>>&& f)
    {
        return detail::split_future_helper(
            HPX_MOVE(f), hpx::util::make_index_pack_t<sizeof...(Ts)>());
    }

    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE hpx::tuple<hpx::future<void>>
    split_future(hpx::future<hpx::tuple<>>&& f)
    {
        return hpx::make_tuple(hpx::future<void>(HPX_MOVE(f)));
    }

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    HPX_FORCEINLINE hpx::tuple<hpx::future<Ts>...> split_future(
        hpx::shared_future<hpx::tuple<Ts...>>&& f)
    {
        return detail::split_future_helper(
            HPX_MOVE(f), hpx::util::make_index_pack_t<sizeof...(Ts)>());
    }

    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE hpx::tuple<hpx::future<void>>
    split_future(hpx::shared_future<hpx::tuple<>>&& f)
    {
        return hpx::make_tuple(hpx::make_future<void>(HPX_MOVE(f)));
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
    HPX_CXX_CORE_EXPORT template <typename... Ts>
    HPX_FORCEINLINE std::tuple<hpx::future<Ts>...> split_future(
        hpx::future<std::tuple<Ts...>>&& f)
    {
        return detail::split_future_helper(
            HPX_MOVE(f), hpx::util::make_index_pack_t<sizeof...(Ts)>());
    }

    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE std::tuple<hpx::future<void>>
    split_future(hpx::future<std::tuple<>>&& f)
    {
        return std::make_tuple(hpx::future<void>(HPX_MOVE(f)));
    }

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    HPX_FORCEINLINE std::tuple<hpx::future<Ts>...> split_future(
        hpx::shared_future<std::tuple<Ts...>>&& f)
    {
        return detail::split_future_helper(
            HPX_MOVE(f), hpx::util::make_index_pack_t<sizeof...(Ts)>());
    }

    HPX_FORCEINLINE std::tuple<hpx::future<void>> split_future(
        hpx::shared_future<std::tuple<>>&& f)
    {
        return std::make_tuple(hpx::make_future<void>(HPX_MOVE(f)));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T1, typename T2>
    HPX_FORCEINLINE std::pair<hpx::future<T1>, hpx::future<T2>> split_future(
        hpx::future<std::pair<T1, T2>>&& f)
    {
        return detail::split_future_helper(HPX_MOVE(f));
    }

    HPX_CXX_CORE_EXPORT template <typename T1, typename T2>
    HPX_FORCEINLINE std::pair<hpx::future<T1>, hpx::future<T2>> split_future(
        hpx::shared_future<std::pair<T1, T2>>&& f)
    {
        return detail::split_future_helper(HPX_MOVE(f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <std::size_t N, typename T>
    HPX_FORCEINLINE std::array<hpx::future<T>, N> split_future(
        hpx::future<std::array<T, N>>&& f)
    {
        return detail::split_future_helper_array<N, T>(HPX_MOVE(f));
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE std::array<hpx::future<void>, 1> split_future(
        hpx::future<std::array<T, 0>>&& f)
    {
        std::array<hpx::future<void>, 1> result;
        result[0] = hpx::future<void>(HPX_MOVE(f));
        return result;
    }

    HPX_CXX_CORE_EXPORT template <std::size_t N, typename T>
    HPX_FORCEINLINE std::array<hpx::future<T>, N> split_future(
        hpx::shared_future<std::array<T, N>>&& f)
    {
        return detail::split_future_helper_array<N, T>(HPX_MOVE(f));
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE std::array<hpx::future<void>, 1> split_future(
        hpx::shared_future<std::array<T, 0>>&& f)
    {
        std::array<hpx::future<void>, 1> result;
        result[0] = hpx::make_future<void>(HPX_MOVE(f));
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE std::vector<hpx::future<T>> split_future(
        hpx::future<std::vector<T>>&& f, std::size_t size)
    {
        return detail::split_future_helper_vector<T>(HPX_MOVE(f), size);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE std::vector<hpx::future<T>> split_future(
        hpx::shared_future<std::vector<T>>&& f, std::size_t size)
    {
        return detail::split_future_helper_vector<T>(HPX_MOVE(f), size);
    }
}    // namespace hpx

#endif
