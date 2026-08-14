//  Copyright (c) 2015 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/reduce_by_key.hpp
/// \page hpx::reduce_by_key
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace experimental {
    // clang-format off
    /// Reduce by Key performs an inclusive scan reduction operation on elements
    /// supplied in key/value pairs. The algorithm produces a single output
    /// value for each set of equal consecutive keys in [key_first, key_last).
    /// the value being the
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, init, *first, ..., *(first + (i - result))).
    /// for the run of consecutive matching keys.
    /// The number of keys supplied must match the number of values.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam RanIter     The type of the key iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam RanIter2    The type of the value iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam FwdIter1    The type of the iterator representing the
    ///                     destination key range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination value range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Compare     The type of the optional function/function object to use
    ///                     to compare keys (deduced).
    ///                     Assumed to be std::equal_to otherwise.
    /// \tparam Func        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a reduce_by_key requires \a Func to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param key_first    Refers to the beginning of the sequence of key elements
    ///                     the algorithm will be applied to.
    /// \param key_last     Refers to the end of the sequence of key elements the
    ///                     algorithm will be applied to.
    /// \param values_first Refers to the beginning of the sequence of value elements
    ///                     the algorithm will be applied to.
    /// \param keys_output  Refers to the start output location for the keys
    ///                     produced by the algorithm.
    /// \param values_output Refers to the start output location for the values
    ///                     produced by the algorithm.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    /// \param func         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 \a Ret must be
    ///                     such that an object of type \a FwdIter can be
    ///                     dereferenced and then implicitly converted to any
    ///                     of those types.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a reduce_by_key algorithm returns a
    ///           \a hpx::future<pair<Iter1,Iter2>> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a pair<Iter1,Iter2>
    ///           otherwise.
    ///
    template <typename ExPolicy, typename RanIter, typename RanIter2,
        typename FwdIter1, typename FwdIter2,
        typename Compare =
            std::equal_to<typename std::iterator_traits<RanIter>::value_type>,
        typename Func =
            std::plus<typename std::iterator_traits<RanIter2>::value_type>>
    typename util::detail::algorithm_result<ExPolicy,
        util::in_out_result<FwdIter1, FwdIter2>>::type
    reduce_by_key(ExPolicy&& policy, RanIter key_first, RanIter key_last,
        RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
        Compare&& comp = Compare(), Func&& func = Func());
    // clang-format on
}}    // namespace hpx::experimental

#else

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>
#include <hpx/parallel/algorithms/inclusive_scan.hpp>
#include <hpx/parallel/container_algorithms/copy.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/// \cond NOINTERNAL
#ifdef EXTRA_DEBUG
#include <iostream>
#include <iterator>
#define DEBUG_REDUCE_BY_KEY(a) std::cout << a
#else
#define DEBUG_REDUCE_BY_KEY(a)
#endif
/// \endcond

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    // -------------------------------------------------------------------
    // simple iterator helper object for access to prev/next items
    // -------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct reduce_stencil_transformer
    {
        // declare result type as a template
        template <typename T>
        struct result;

        // specialize result for iterator type
        template <typename This, typename Iterator>
        struct result<This(Iterator)>
        {
            using element_type =
                typename std::iterator_traits<Iterator>::reference;
            using type = hpx::tuple<element_type, element_type, element_type>;
        };

        // call operator for stencil transform
        // it will dereference tuple(it-1, it, it+1)
        template <typename Iterator>
        typename result<reduce_stencil_transformer(Iterator)>::type operator()(
            Iterator const& it) const
        {
            using type =
                typename result<reduce_stencil_transformer(Iterator)>::type;
            return type(*std::prev(it), *it, *std::next(it));
        }
    };

    // -------------------------------------------------------------------
    // transform iterator using reduce_stencil_transformer helper
    // -------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename Iterator,
        typename Transformer = detail::reduce_stencil_transformer>
    class reduce_stencil_iterator
      : public hpx::util::transform_iterator<Iterator, Transformer>
    {
    private:
        using base_type = hpx::util::transform_iterator<Iterator, Transformer>;

    public:
        reduce_stencil_iterator() = default;

        explicit reduce_stencil_iterator(Iterator const& it)
          : base_type(it, Transformer())
        {
        }

        reduce_stencil_iterator(Iterator const& it, Transformer const& t)
          : base_type(it, t)
        {
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Transformer>
    reduce_stencil_iterator<Iterator, Transformer> make_reduce_stencil_iterator(
        Iterator const& it, Transformer const& t)
    {
        return reduce_stencil_iterator<Iterator, Transformer>(it, t);
    }

    // -------------------------------------------------------------------
    // state of a reduce by key step
    // -------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct reduce_key_series_states
    {
        bool start;    // START of a segment
        bool end;      // END of a segment

        explicit reduce_key_series_states(bool s = false, bool e = false)
          : start(s)
          , end(e)
        {
        }
    };

    // -------------------------------------------------------------------
    // callable that actually computes the state using the stencil iterator
    // -------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename Transformer,
        typename StencilIterType, typename KeyStateIterType, typename Compare>
    struct reduce_stencil_generate
    {
        using element_type = typename Transformer::template result<Transformer(
            StencilIterType)>::element_type;
        using tuple_type = typename Transformer::template result<Transformer(
            StencilIterType)>::type;
        using KeyStateType =
            typename std::iterator_traits<KeyStateIterType>::reference;

        reduce_stencil_generate() {}

        void operator()(tuple_type const& value, KeyStateType& kiter,
            Compare const& comp) const
        {
            // resolves to a tuple of values for *(it-1), *it, *(it+1)
            element_type left = hpx::get<0>(value);
            element_type mid = hpx::get<1>(value);
            element_type right = hpx::get<2>(value);

            // we need to determine which of three states this
            // index is. It can be:
            // 1. Middle of a set of equivalent keys.
            // 2. Start of a set of equivalent keys.
            // 3. End of a set of equivalent keys.
            // 4. Both the start and end of a set of keys

            bool const left_matches(comp(left, mid));
            bool const right_matches(comp(mid, right));
            kiter = reduce_key_series_states(!left_matches, !right_matches);
        }
    };

    // -------------------------------------------------------------------
    // helper that extracts the final output iterators from copy_if
    // -------------------------------------------------------------------
    // Zip iterator has 3 iterators inside
    // Iter1, key type : Iter2, value type : Iter3, state type
    HPX_CXX_CORE_EXPORT template <typename ZIter, typename iKey, typename iVal>
    constexpr util::in_out_result<iKey, iVal> make_pair_result(
        ZIter zipiter, iKey key_start, iVal val_start)
    {
        // the iterator we want is 'second' part of pair type (from copy_if)
        auto t = zipiter.out.get_iterator_tuple();
        iKey key_end = hpx::get<0>(t);
        return util::in_out_result<iKey, iVal>{key_end,
            std::next(val_start,
                hpx::parallel::detail::distance(key_start, key_end))};
    }

    // async version that returns future<pair> from future<zip_iterator<blah>>
    HPX_CXX_CORE_EXPORT template <typename ZIter, typename iKey, typename iVal>
    hpx::future<util::in_out_result<iKey, iVal>> make_pair_result(
        hpx::future<ZIter>&& ziter, iKey key_start, iVal val_start)
    {
        using result_type = util::in_out_result<iKey, iVal>;

        return hpx::make_future<result_type>(
            HPX_MOVE(ziter), [=](ZIter zipiter) {
                auto t = zipiter.second.get_iterator_tuple();
                iKey key_end = hpx::get<0>(t);
                return result_type{key_end,
                    std::next(val_start,
                        hpx::parallel::detail::distance(key_start, key_end))};
            });
    }

    // -------------------------------------------------------------------
    // The main algorithm is implemented here, it replaces any async execution
    // policy with a non async one so that no waits are necessary on the
    // internal algorithms. Async execution is handled by the wrapper layer that
    // calls this.
    // -------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RanIter,
        typename RanIter2, typename FwdIter1, typename FwdIter2,
        typename Compare, typename Func>
    util::in_out_result<FwdIter1, FwdIter2> reduce_by_key_impl(
        ExPolicy&& policy, RanIter key_first, RanIter key_last,
        RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
        Compare&& comp, Func&& func)
    {
        using namespace hpx::parallel::detail;
        using namespace hpx::util;

        // we need to determine based on the keys what is the key state for each
        // key. The states are start, middle, end of a series and the special
        // state start and end of the sequence
        std::vector<reduce_key_series_states> key_state;
        using keystate_iter_type =
            std::vector<reduce_key_series_states>::iterator;
        using reducebykey_iter = detail::reduce_stencil_iterator<RanIter,
            reduce_stencil_transformer>;
        using element_type = typename std::iterator_traits<RanIter>::reference;
        using zip_ref = typename zip_iterator<reducebykey_iter,
            keystate_iter_type>::reference;
        //
        std::uint64_t const number_of_keys =
            hpx::parallel::detail::distance(key_first, key_last);
        //
        key_state.assign(number_of_keys, reduce_key_series_states());
        {
            reduce_stencil_transformer r_s_t;
            reducebykey_iter reduce_begin =
                make_reduce_stencil_iterator(key_first, r_s_t);
            reducebykey_iter reduce_end =
                make_reduce_stencil_iterator(key_last, r_s_t);

            if (number_of_keys == 2)
            {
                // for two entries, one is a start, the other an end,
                // if they are different, then they are both start/end
                element_type left = *key_first;
                element_type right = *std::next(key_first);
                key_state[0] =
                    reduce_key_series_states(true, !comp(left, right));
                key_state[1] =
                    reduce_key_series_states(!comp(left, right), true);
            }
            else
            {
                // do the first and last elements by hand to simplify the iterator
                // traversal as there is no prev/next for first/last
                element_type elem0 = *key_first;
                element_type elem1 = *std::next(key_first);
                key_state[0] =
                    reduce_key_series_states(true, !comp(elem0, elem1));
                // middle elements
                reduce_stencil_generate<reduce_stencil_transformer, RanIter,
                    keystate_iter_type, Compare>
                    kernel;
                hpx::for_each(policy(hpx::execution::non_task),
                    zip_iterator(reduce_begin + 1, key_state.begin() + 1),
                    zip_iterator(reduce_end - 1, key_state.end() - 1),
                    [&kernel, &comp](zip_ref ref) {
                        kernel(hpx::get<0>(ref), hpx::get<1>(ref), comp);
                    });
                // Last element
                element_type elem_n = *std::prev(key_last);
                element_type elemn = *std::prev(std::prev(key_last));
                key_state.back() =
                    reduce_key_series_states(elem_n != elemn, true);
            }
        }
        {
            using zip_iterator_in = zip_iterator<RanIter2,
                std::vector<reduce_key_series_states>::iterator>;
            using zip_type_in = typename zip_iterator_in::value_type;

            using zip_iterator_vout = zip_iterator<FwdIter2,
                std::vector<reduce_key_series_states>::iterator>;

            using value_type =
                typename std::iterator_traits<RanIter2>::value_type;

            zip_iterator_in states_begin =
                zip_iterator(values_first, hpx::util::begin(key_state));
            zip_iterator_in states_end = zip_iterator(
                values_first + number_of_keys, hpx::util::end(key_state));
            zip_iterator_vout states_out_begin =
                zip_iterator(values_output, hpx::util::begin(key_state));

            zip_type_in initial;
            //
            using lambda_type =
                hpx::tuple<value_type, reduce_key_series_states>;
            hpx::inclusive_scan(
                policy(hpx::execution::non_task), states_begin, states_end,
                states_out_begin,
                // B is the current entry, A is the one passed in from 'previous'
                [&func](zip_type_in a, zip_type_in b) -> lambda_type {
                    value_type a_val = hpx::get<0>(a);
                    reduce_key_series_states const a_state = hpx::get<1>(a);
                    value_type b_val = hpx::get<0>(b);
                    reduce_key_series_states const b_state = hpx::get<1>(b);
                    DEBUG_REDUCE_BY_KEY("{ " << a_val << "+" << b_val << " },\t"
                                             << a_state << b_state);
                    // if carrying a start flag, then copy - don't add
                    if (b_state.start)
                    {
                        DEBUG_REDUCE_BY_KEY(" = " << b_val << std::endl);
                        return hpx::make_tuple(b_val,
                            reduce_key_series_states(
                                a_state.start || b_state.start, b_state.end));
                    }

                    // normal add of previous + this
                    DEBUG_REDUCE_BY_KEY(
                        " = " << func(a_val, b_val) << std::endl);

                    value_type temp = func(a_val, b_val);
                    return hpx::make_tuple(temp,
                        reduce_key_series_states(
                            a_state.start || b_state.start, b_state.end));
                },
                initial);

            // now copy the values and keys for each element that is marked by
            // an 'END' state to the final output
            using zip2_ref = typename hpx::util::zip_iterator<RanIter, FwdIter2,
                std::vector<reduce_key_series_states>::iterator>::reference;

            std::vector<value_type> temp(number_of_keys);
            hpx::copy(policy(hpx::execution::non_task), values_output,
                values_output + number_of_keys, temp.begin());

            return make_pair_result(
                hpx::ranges::copy_if(policy(hpx::execution::non_task),
                    zip_iterator(
                        key_first, temp.begin(), hpx::util::begin(key_state)),
                    zip_iterator(key_last, temp.begin() + number_of_keys,
                        hpx::util::end(key_state)),
                    zip_iterator(keys_output, values_output,
                        hpx::util::begin(key_state)),
                    // copies to dest only when 'end' state is true
                    [](zip2_ref it) { return hpx::get<2>(it).end; }),
                keys_output, values_output);
        }
    }

    ///////////////////////////////////////////////////////////////////////
    // reduce_by_key wrapper struct
    HPX_CXX_CORE_EXPORT template <typename FwdIter1, typename FwdIter2>
    struct reduce_by_key
      : public algorithm<reduce_by_key<FwdIter1, FwdIter2>,
            util::in_out_result<FwdIter1, FwdIter2>>
    {
        constexpr reduce_by_key() noexcept
          : algorithm<reduce_by_key, util::in_out_result<FwdIter1, FwdIter2>>(
                "reduce_by_key")
        {
        }

        template <typename ExPolicy, typename RanIter, typename RanIter2,
            typename Compare, typename Func>
        static constexpr util::in_out_result<FwdIter1, FwdIter2> sequential(
            ExPolicy&& policy, RanIter key_first, RanIter key_last,
            RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
            Compare&& comp, Func&& func)
        {
            return reduce_by_key_impl(HPX_FORWARD(ExPolicy, policy), key_first,
                key_last, values_first, keys_output, values_output,
                HPX_FORWARD(Compare, comp), HPX_FORWARD(Func, func));
        }

        template <typename ExPolicy, typename RanIter, typename RanIter2,
            typename Compare, typename Func>
        static util::detail::algorithm_result_t<ExPolicy,
            util::in_out_result<FwdIter1, FwdIter2>>
        parallel(ExPolicy&& policy, RanIter key_first, RanIter key_last,
            RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
            Compare&& comp, Func&& func)
        {
            return util::detail::algorithm_result<ExPolicy,
                util::in_out_result<FwdIter1,
                    FwdIter2>>::get(execution::async_execute(policy.executor(),
                hpx::util::deferred_call(
                    &hpx::parallel::detail::reduce_by_key_impl<ExPolicy&&,
                        RanIter, RanIter2, FwdIter1, FwdIter2, Compare&&,
                        Func&&>,
                    policy, key_first, key_last, values_first, keys_output,
                    values_output, HPX_FORWARD(Compare, comp),
                    HPX_FORWARD(Func, func))));
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::experimental {

#ifdef EXTRA_DEBUG
    std::ostream& operator<<(
        std::ostream& os, detail::reduce_key_series_states const& rs)
    {
        os << "{ start=" << rs.start << ",end=" << rs.end << "} ";
        return os;
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RanIter,
        typename RanIter2, typename FwdIter1, typename FwdIter2,
        typename Compare = std::equal_to<>, typename Func = std::plus<>>
    // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<RanIter> &&
            hpx::traits::is_iterator_v<RanIter2> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2>
        )
    // clang-format on
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>
    reduce_by_key(ExPolicy&& policy, RanIter key_first, RanIter key_last,
        RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
        Compare comp = Compare(), Func func = Func())
    {
        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>;

        static_assert(std::random_access_iterator<RanIter> &&
                std::random_access_iterator<RanIter2> &&
                std::forward_iterator<FwdIter1> &&
                std::forward_iterator<FwdIter2>,
            "iterators : Random_access for inputs and forward for outputs.");

        std::uint64_t const number_of_keys =
            hpx::parallel::detail::distance(key_first, key_last);

        if (number_of_keys == 0)
        {
            return result::get(
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>{
                    keys_output, values_output});
        }

        if (number_of_keys == 1)
        {
            // we only have a single key/value so that is our output
            *keys_output = *key_first;
            *values_output = *values_first;
            return result::get(
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>{
                    ++keys_output, ++values_output});
        }

        return hpx::parallel::detail::reduce_by_key<FwdIter1, FwdIter2>().call(
            HPX_FORWARD(ExPolicy, policy), key_first, key_last, values_first,
            keys_output, values_output, HPX_MOVE(comp), HPX_MOVE(func));
    }
}    // namespace hpx::experimental

namespace hpx::parallel {

    template <typename ExPolicy, typename RanIter, typename RanIter2,
        typename FwdIter1, typename FwdIter2,
        typename Compare =
            std::equal_to<typename std::iterator_traits<RanIter>::value_type>,
        typename Func =
            std::plus<typename std::iterator_traits<RanIter2>::value_type>>
    HPX_DEPRECATED_V(1, 9,
        "hpx::parallel::reduce_by_key is deprecated. Please use "
        "hpx::experimental::reduce_by_key instead.")
    constexpr decltype(auto)
        reduce_by_key(ExPolicy&& policy, RanIter key_first, RanIter key_last,
            RanIter2 values_first, FwdIter1 keys_output, FwdIter2 values_output,
            Compare&& comp = Compare(), Func&& func = Func())
    {
        return hpx::experimental::reduce_by_key(policy, key_first, key_last,
            values_first, keys_output, values_output, comp, func);
    }
}    // namespace hpx::parallel

#endif
