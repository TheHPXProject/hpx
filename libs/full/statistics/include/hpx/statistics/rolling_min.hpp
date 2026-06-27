//  Copyright (c) 2017-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/statistics/rolling_window.hpp>
#include <boost/accumulators/statistics_fwd.hpp>

namespace hpx::util::detail {

    template <typename Sample>
    struct rolling_min_impl : boost::accumulators::accumulator_base
    {
        using float_type = Sample;

        // for boost::result_of
        using result_type = float_type;

        template <typename Args>
        explicit rolling_min_impl(Args const& /* args */)
          : min_(boost::numeric::as_max(Sample()))
          , is_dirty_(false)
        {
        }

        template <typename Args>
        void operator()(Args const& args)
        {
            boost::accumulators::find_accumulator<
                boost::accumulators::tag::rolling_window>(
                args[boost::accumulators::accumulator])(args);
            is_dirty_ = true;
        }

        template <typename Args>
        result_type result(Args const& args) const
        {
            if (is_dirty_)
            {
                using namespace boost::accumulators;

                min_ = boost::numeric::as_max(Sample());

                // work around problem in older Boost versions
                auto r = rolling_window_plus1(args);
                bool const full = impl::is_rolling_window_plus1_full(args);

                for (auto const& s : r.advance_begin(full ? 1 : 0))
                {
                    boost::numeric::min_assign(min_, s);
                }

                is_dirty_ = false;
            }
            return min_;
        }

    private:
        mutable Sample min_;
        mutable bool is_dirty_;
    };
}    // namespace hpx::util::detail
// namespace hpx::util::detail

///////////////////////////////////////////////////////////////////////////////
// tag::rolling_min
namespace boost::accumulators {

    namespace tag {

        struct rolling_min : depends_on<rolling_window>
        {
            struct impl
            {
                template <typename Sample, typename Weight>
                struct apply
                {
                    using type = hpx::util::detail::rolling_min_impl<Sample>;
                };
            };
        };
    }    // namespace tag

    ///////////////////////////////////////////////////////////////////////////////
    // extract::rolling_min
    namespace extract {

        inline constexpr extractor<tag::rolling_min> rolling_min = {};
    }    // namespace extract
}    // namespace boost::accumulators
// namespace boost::accumulators

namespace hpx::util {

    namespace tag {

        using boost::accumulators::tag::rolling_min;
    }    // namespace tag

    using boost::accumulators::extract::rolling_min;
}    // namespace hpx::util
// namespace hpx::util
