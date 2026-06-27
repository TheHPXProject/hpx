//  Copyright (c) 2026 Mo'men Samir
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    struct local_result
    {
        bool local_res;     // local is_partitioned result
        bool first_elem;    // predicate value of first element in segment
        bool last_elem;     // predicate value of last element in segment
    };

    template <typename Iter>
    struct seg_is_partitioned
      : algorithm<seg_is_partitioned<Iter>, local_result>
    {
        constexpr seg_is_partitioned() noexcept
          : algorithm<seg_is_partitioned<Iter>, local_result>("is_partitioned")
        {
        }

        template <typename ExPolicy, typename FwdIter, typename Pred,
            typename Proj>
        static local_result sequential(ExPolicy&& /* policy */, FwdIter first,
            FwdIter last, Pred&& pred, Proj&& proj)
        {
            util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

            bool first_elem = HPX_INVOKE(pred_projected, *first);
            bool last_elem = HPX_INVOKE(pred_projected, *std::prev(last));

            bool result = is_partitioned<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, last, HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Proj, proj));

            return {result, first_elem, last_elem};
        }

        template <typename ExPolicy, typename FwdIter, typename Pred,
            typename Proj>
        static util::detail::algorithm_result_t<ExPolicy, local_result>
        parallel(ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred,
            Proj&& proj)
        {
            util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

            bool first_elem = HPX_INVOKE(pred_projected, *first);
            bool last_elem = HPX_INVOKE(pred_projected, *std::prev(last));

            auto result = is_partitioned<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));

            if constexpr (hpx::traits::is_future_v<decltype(result)>)
            {
                return result.then([first_elem, last_elem](
                                       hpx::future<bool>&& f) -> local_result {
                    return {f.get(), first_elem, last_elem};
                });
            }
            else
            {
                return util::detail::algorithm_result<ExPolicy,
                    local_result>::get(local_result{
                    result, first_elem, last_elem});
            }
        }
    };
}    // namespace hpx::parallel::detail
