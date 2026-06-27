//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//  Copyright (c) 2021 Chuanqiu He
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// make sure inspect doesn't get upset about the unproteced min()/max() below
// hpxinspect:nominmax

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I1, typename I2>
    struct in_in_result
    {
        HPX_NO_UNIQUE_ADDRESS I1 in1;
        HPX_NO_UNIQUE_ADDRESS I2 in2;

        template <typename II1, typename II2>
            requires(std::convertible_to<I1 const&, II1> &&
                std::convertible_to<I2 const&, II2>)
        constexpr operator in_in_result<II1, II2>() const&
        {
            return {in1, in2};
        }

        template <typename II1, typename II2>
            requires(
                std::convertible_to<I1, II1> && std::convertible_to<I2, II2>)
        constexpr operator in_in_result<II1, II2>() &&
        {
            return {HPX_MOVE(in1), HPX_MOVE(in2)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & in1 & in2;
            // clang-format on
        }
    };

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2>
    I2 get_in2_element(util::in_in_result<I1, I2>&& p)
    {
        return p.in2;
    }

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2>
    hpx::future<I2> get_in2_element(hpx::future<util::in_in_result<I1, I2>>&& f)
    {
        return hpx::make_future<I2>(
            HPX_MOVE(f), [](util::in_in_result<I1, I2>&& p) { return p.in2; });
    }

    HPX_CXX_CORE_EXPORT template <typename InInResultSender>
        requires(hpx::execution::experimental::is_sender_v<InInResultSender>)
    decltype(auto) get_in2_element(InInResultSender&& result_sender)
    {
        return hpx::execution::experimental::then(
            HPX_FORWARD(InInResultSender, result_sender), [](auto&& result) {
                return util::get_in2_element(
                    HPX_FORWARD(decltype(result), result));
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    struct in_out_result
    {
        HPX_NO_UNIQUE_ADDRESS I in;
        HPX_NO_UNIQUE_ADDRESS O out;

        template <typename I2, typename O2>
            requires(std::convertible_to<I const&, I2> &&
                std::convertible_to<O const&, O2>)
        constexpr operator in_out_result<I2, O2>() const&
        {
            return {in, out};
        }

        template <typename I2, typename O2>
            requires(std::convertible_to<I, I2> && std::convertible_to<O, O2>)
        constexpr operator in_out_result<I2, O2>() &&
        {
            return {HPX_MOVE(in), HPX_MOVE(out)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & in & out;
            // clang-format on
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    std::pair<I, O> get_pair(util::in_out_result<I, O>&& p)
    {
        return std::pair<I, O>{p.in, p.out};
    }

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    hpx::future<std::pair<I, O>> get_pair(
        hpx::future<util::in_out_result<I, O>>&& f)
    {
        return hpx::make_future<std::pair<I, O>>(
            HPX_MOVE(f), [](util::in_out_result<I, O>&& p) {
                return std::pair<I, O>{p.in, p.out};
            });
    }

    HPX_CXX_CORE_EXPORT template <typename Sender>
        requires(hpx::execution::experimental::is_sender_v<Sender>)
    decltype(auto) get_pair(Sender&& sender)
    {
        return hpx::execution::experimental::then(
            HPX_FORWARD(Sender, sender), [](auto&& in_out_result) {
                return get_pair(
                    HPX_FORWARD(decltype(in_out_result), in_out_result));
            });
    }

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    I get_first_element(util::in_out_result<I, O>&& p)
    {
        return p.in;
    }

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    hpx::future<I> get_first_element(hpx::future<util::in_out_result<I, O>>&& f)
    {
        return hpx::make_future<I>(
            HPX_MOVE(f), [](util::in_out_result<I, O>&& p) { return p.in; });
    }

    namespace functional {

        HPX_CXX_CORE_EXPORT struct get_first_element
        {
            template <typename T>
            decltype(auto) operator()(T&& val) const
            {
                return hpx::parallel::util::get_first_element(
                    HPX_FORWARD(T, val));
            }
        };
    }    // namespace functional

    HPX_CXX_CORE_EXPORT template <typename Sender>
        requires(hpx::execution::experimental::is_sender_v<Sender>)
    decltype(auto) get_first_element(Sender&& sender)
    {
        return hpx::execution::experimental::then(
            HPX_FORWARD(Sender, sender), functional::get_first_element{});
    }

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    O get_second_element(util::in_out_result<I, O>&& p)
    {
        return p.out;
    }

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    hpx::future<O> get_second_element(
        hpx::future<util::in_out_result<I, O>>&& f)
    {
        return hpx::make_future<O>(
            HPX_MOVE(f), [](util::in_out_result<I, O>&& p) { return p.out; });
    }

    namespace functional {

        HPX_CXX_CORE_EXPORT struct get_second_element
        {
            template <typename T>
            decltype(auto) operator()(T&& val) const
            {
                return hpx::parallel::util::get_second_element(
                    HPX_FORWARD(T, val));
            }
        };
    }    // namespace functional

    HPX_CXX_CORE_EXPORT template <typename Sender>
        requires(hpx::execution::experimental::is_sender_v<Sender>)
    decltype(auto) get_second_element(Sender&& sender)
    {
        return hpx::execution::experimental::then(
            HPX_FORWARD(Sender, sender), functional::get_second_element{});
    }

    // converts a in_out_result into a iterator_range
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    hpx::util::iterator_range<I, O> get_subrange(in_out_result<I, O> const& ior)
    {
        return hpx::util::iterator_range<I, O>(ior.in, ior.out);
    }

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    hpx::future<hpx::util::iterator_range<I, O>> get_subrange(
        hpx::future<in_out_result<I, O>>&& ior)
    {
        return hpx::make_future<hpx::util::iterator_range<I, O>>(
            HPX_MOVE(ior), [](in_out_result<I, O>&& ior) {
                return hpx::util::iterator_range<I, O>(ior.in, ior.out);
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct min_max_result
    {
        HPX_NO_UNIQUE_ADDRESS T min;
        HPX_NO_UNIQUE_ADDRESS T max;

        template <typename T2>
            requires(std::convertible_to<T const&, T2>)
        constexpr operator min_max_result<T2>() const&
        {
            return {min, max};
        }

        template <typename T2>
            requires(std::is_convertible_v<T, T2>)
        constexpr operator min_max_result<T2>() &&
        {
            return {HPX_MOVE(min), HPX_MOVE(max)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & min & max;
            // clang-format on
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    struct in_in_out_result
    {
        HPX_NO_UNIQUE_ADDRESS I1 in1;
        HPX_NO_UNIQUE_ADDRESS I2 in2;
        HPX_NO_UNIQUE_ADDRESS O out;

        template <typename II1, typename II2, typename O1>
            requires(std::convertible_to<I1 const&, II1> &&
                std::convertible_to<I2 const&, II2> &&
                std::convertible_to<O const&, O1>)
        constexpr operator in_in_out_result<II1, II2, O1>() const&
        {
            return {in1, in2, out};
        }

        template <typename II2, typename II1, typename O1>
            requires(std::convertible_to<I1, II1> &&
                std::convertible_to<I2, II2> && std::convertible_to<O, O1>)
        constexpr operator in_in_out_result<II1, II2, O1>() &&
        {
            return {HPX_MOVE(in1), HPX_MOVE(in2), HPX_MOVE(out)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & in1 & in2 & out;
            // clang-format on
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    O get_third_element(util::in_in_out_result<I1, I2, O>&& p)
    {
        return p.out;
    }

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    hpx::future<O> get_third_element(
        hpx::future<util::in_in_out_result<I1, I2, O>>&& f)
    {
        return hpx::make_future<O>(
            HPX_MOVE(f), [](in_in_out_result<I1, I2, O>&& p) { return p.out; });
    }

    HPX_CXX_CORE_EXPORT template <typename InInOutResultSender>
        requires(hpx::execution::experimental::is_sender_v<InInOutResultSender>)
    decltype(auto) get_third_element(InInOutResultSender&& result_sender)
    {
        return hpx::execution::experimental::then(
            HPX_FORWARD(InInOutResultSender, result_sender), [](auto&& result) {
                return util::get_third_element(
                    HPX_FORWARD(decltype(result), result));
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename O1, typename O2>
    struct in_out_out_result
    {
        HPX_NO_UNIQUE_ADDRESS I in;
        HPX_NO_UNIQUE_ADDRESS O1 out1;
        HPX_NO_UNIQUE_ADDRESS O2 out2;

        template <typename II, typename OO1, typename OO2>
            requires(std::convertible_to<I const&, II> &&
                std::convertible_to<O1 const&, OO1> &&
                std::convertible_to<O2 const&, OO2>)
        constexpr operator in_out_out_result<II, OO1, OO2>() const&
        {
            return {in, out1, out2};
        }

        template <typename II, typename OO1, typename OO2>
            requires(std::convertible_to<I, II> &&
                std::convertible_to<O1, OO1> && std::convertible_to<O2, OO2>)
        constexpr operator in_out_out_result<II, OO1, OO2>() &&
        {
            return {HPX_MOVE(in), HPX_MOVE(out1), HPX_MOVE(out2)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & in & out1 & out2;
            // clang-format on
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    constexpr HPX_FORCEINLINE in_out_out_result<Ts...> make_in_out_out_result(
        hpx::tuple<Ts...>&& t)
    {
        static_assert(hpx::tuple_size<hpx::tuple<Ts...>>::value == 3,
            "size of tuple should be 3 to convert to in_out_out_result");

        using result_type = in_out_out_result<Ts...>;

        return result_type{hpx::get<0>(t), hpx::get<1>(t), hpx::get<2>(t)};
    }

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    hpx::future<in_out_out_result<Ts...>> make_in_out_out_result(
        hpx::future<hpx::tuple<Ts...>>&& f)
    {
        static_assert(hpx::tuple_size<hpx::tuple<Ts...>>::value == 3,
            "size of tuple should be 3 to convert to in_out_out_result");

        using result_type = in_out_out_result<Ts...>;

        return hpx::make_future<result_type>(
            HPX_MOVE(f), [](hpx::tuple<Ts...>&& t) -> result_type {
                return make_in_out_out_result(HPX_MOVE(t));
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename F>
    struct in_fun_result
    {
        HPX_NO_UNIQUE_ADDRESS I in;
        HPX_NO_UNIQUE_ADDRESS F fun;

        template <typename I2, typename F2>
            requires(std::convertible_to<I const&, I2> &&
                std::convertible_to<F const&, F2>)
        constexpr operator in_fun_result<I2, F2>() const&
        {
            return {in, fun};
        }

        template <typename I2, typename F2>
            requires(
                std::is_convertible_v<I, I2> && std::is_convertible_v<F, F2>)
        constexpr operator in_fun_result<I2, F2>() &&
        {
            return {HPX_MOVE(in), HPX_MOVE(fun)};
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & in & fun;
            // clang-format on
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iterator,
        typename Sentinel = Iterator>
    hpx::util::iterator_range<Iterator, Sentinel> make_subrange(
        Iterator iterator, Sentinel sentinel)
    {
        return hpx::util::iterator_range(iterator, sentinel);
    }

    HPX_CXX_CORE_EXPORT template <typename Iterator,
        typename Sentinel = Iterator>
    hpx::future<hpx::util::iterator_range<Iterator, Sentinel>> make_subrange(
        hpx::future<Iterator>&& iterator, Sentinel sentinel)
    {
        return hpx::make_future<hpx::util::iterator_range<Iterator, Sentinel>>(
            HPX_MOVE(iterator), [sentinel](Iterator&& it) {
                return hpx::util::iterator_range(it, sentinel);
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename I, typename T>
    struct in_value_result
    {
        HPX_NO_UNIQUE_ADDRESS I in;
        HPX_NO_UNIQUE_ADDRESS T value;

        template <typename I2, typename T2>
            requires(std::convertible_to<I const&, I2> &&
                std::convertible_to<T const&, T2>)
        constexpr operator in_value_result<I2, T2>() const&
        {
            return {in, value};
        }

        template <typename I2, typename T2>
            requires(std::convertible_to<I, I2> && std::convertible_to<T, T2>)
        constexpr operator in_value_result<I2, T2>() &&
        {
            return {HPX_MOVE(in), HPX_MOVE(value)};
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename O, typename T>
    struct out_value_result
    {
        HPX_NO_UNIQUE_ADDRESS O out;
        HPX_NO_UNIQUE_ADDRESS T value;

        template <typename O2, typename T2>
            requires(std::convertible_to<O const&, O2> &&
                std::convertible_to<T const&, T2>)
        constexpr operator out_value_result<O2, T2>() const&
        {
            return {out, value};
        }

        template <typename O2, typename T2>
            requires(std::convertible_to<O, O2> && std::convertible_to<T, T2>)
        constexpr operator out_value_result<O2, T2>() &&
        {
            return {HPX_MOVE(out), HPX_MOVE(value)};
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        in_out_result<
            typename hpx::tuple_element<0,
                typename std::decay_t<ZipIter>::iterator_tuple_type>::type,
            typename hpx::tuple_element<1,
                typename std::decay_t<ZipIter>::iterator_tuple_type>::type>
        get_in_out_result(ZipIter&& zipiter)
        {
            using iterator_tuple_type =
                typename std::decay_t<ZipIter>::iterator_tuple_type;

            using result_type = in_out_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type,
                typename hpx::tuple_element<1, iterator_tuple_type>::type>;

            iterator_tuple_type t = zipiter.get_iterator_tuple();
            return result_type{hpx::get<0>(t), hpx::get<1>(t)};
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIterSender>
            requires(hpx::execution::experimental::is_sender_v<ZipIterSender>)
        decltype(auto) get_in_out_result(ZipIterSender&& zipiter_sender)
        {
            return hpx::execution::experimental::then(
                HPX_FORWARD(ZipIterSender, zipiter_sender), [](auto&& zipiter) {
                    return get_in_out_result(
                        HPX_FORWARD(decltype(zipiter), zipiter));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        hpx::future<
            in_out_result<typename hpx::tuple_element<0,
                              typename ZipIter::iterator_tuple_type>::type,
                typename hpx::tuple_element<1,
                    typename ZipIter::iterator_tuple_type>::type>>
        get_in_out_result(hpx::future<ZipIter>&& zipiter)
        {
            using iterator_tuple_type = typename ZipIter::iterator_tuple_type;

            using result_type = in_out_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type,
                typename hpx::tuple_element<1, iterator_tuple_type>::type>;

            return hpx::make_future<result_type>(
                HPX_MOVE(zipiter), [](ZipIter zipiter) {
                    return get_in_out_result(HPX_MOVE(zipiter));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        min_max_result<typename hpx::tuple_element<0,
            typename ZipIter::iterator_tuple_type>::type>
        get_min_max_result(ZipIter&& zipiter)
        {
            using iterator_tuple_type = typename ZipIter::iterator_tuple_type;

            using result_type = min_max_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type>;

            iterator_tuple_type t = zipiter.get_iterator_tuple();
            return result_type{hpx::get<0>(t), hpx::get<1>(t)};
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        hpx::future<min_max_result<typename hpx::tuple_element<0,
            typename ZipIter::iterator_tuple_type>::type>>
        get_min_max_result(hpx::future<ZipIter>&& zipiter)
        {
            using iterator_tuple_type = typename ZipIter::iterator_tuple_type;

            using result_type = min_max_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type>;

            return hpx::make_future<result_type>(
                HPX_MOVE(zipiter), [](ZipIter zipiter) {
                    return get_min_max_result(HPX_MOVE(zipiter));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        in_in_out_result<
            typename hpx::tuple_element<0,
                typename std::decay_t<ZipIter>::iterator_tuple_type>::type,
            typename hpx::tuple_element<1,
                typename std::decay_t<ZipIter>::iterator_tuple_type>::type,
            typename hpx::tuple_element<2,
                typename std::decay_t<ZipIter>::iterator_tuple_type>::type>
        get_in_in_out_result(ZipIter&& zipiter)
        {
            using iterator_tuple_type =
                typename std::decay_t<ZipIter>::iterator_tuple_type;

            using result_type = in_in_out_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type,
                typename hpx::tuple_element<1, iterator_tuple_type>::type,
                typename hpx::tuple_element<2, iterator_tuple_type>::type>;

            iterator_tuple_type t = zipiter.get_iterator_tuple();
            return result_type{hpx::get<0>(t), hpx::get<1>(t), hpx::get<2>(t)};
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIterSender>
            requires(hpx::execution::experimental::is_sender_v<ZipIterSender>)
        decltype(auto) get_in_in_out_result(ZipIterSender&& zipiter_sender)
        {
            return hpx::execution::experimental::then(
                HPX_FORWARD(ZipIterSender, zipiter_sender), [](auto&& zipiter) {
                    return get_in_in_out_result(
                        HPX_FORWARD(decltype(zipiter), zipiter));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename ZipIter>
        hpx::future<
            in_in_out_result<typename hpx::tuple_element<0,
                                 typename ZipIter::iterator_tuple_type>::type,
                typename hpx::tuple_element<1,
                    typename ZipIter::iterator_tuple_type>::type,
                typename hpx::tuple_element<2,
                    typename ZipIter::iterator_tuple_type>::type>>
        get_in_in_out_result(hpx::future<ZipIter>&& zipiter)
        {
            using iterator_tuple_type = typename ZipIter::iterator_tuple_type;

            using result_type = in_in_out_result<
                typename hpx::tuple_element<0, iterator_tuple_type>::type,
                typename hpx::tuple_element<1, iterator_tuple_type>::type,
                typename hpx::tuple_element<2, iterator_tuple_type>::type>;

            return hpx::make_future<result_type>(
                HPX_MOVE(zipiter), [](ZipIter zipiter) {
                    return get_in_in_out_result(HPX_MOVE(zipiter));
                });
        }
    }    // namespace detail
}    // namespace hpx::parallel::util

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_fun_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_in_out_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_in_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_out_out_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_out_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::min_max_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::in_value_result;
    HPX_CXX_CORE_EXPORT using hpx::parallel::util::out_value_result;
}    // namespace hpx::ranges
// namespace hpx::ranges
