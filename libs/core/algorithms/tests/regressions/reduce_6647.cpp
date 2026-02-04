//  Copyright (c) 2026 Bhoomish Gupta
//
<<<<<<< HEAD
//  SPDX - License - Identifier : BSL - 1.0
=======
>>>>>>> 7cd0cb47ab (Fix bug #6647: Correct type handling in reduce)
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #6647:Incorrect reduce implementation

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <climits>
#include <utility>
#include <vector>

struct minmax
{
    std::pair<int, int> operator()(
        std::pair<int, int> lhs, std::pair<int, int> rhs) const
    {
        return {
            lhs.first < rhs.first ? lhs.first : rhs.first,
            lhs.second < rhs.second ? rhs.second : lhs.second,
        };
    }

    std::pair<int, int> operator()(std::pair<int, int> lhs, int rhs) const
    {
        return (*this)(lhs, std::pair<int, int>{rhs, rhs});
    }

    std::pair<int, int> operator()(int lhs, std::pair<int, int> rhs) const
    {
        return (*this)(std::pair<int, int>{lhs, lhs}, rhs);
    }

    std::pair<int, int> operator()(int lhs, int rhs) const
    {
        return (*this)(
            std::pair<int, int>{lhs, lhs}, std::pair<int, int>{rhs, rhs});
    }
};

int hpx_main()
{
    std::vector<int> c = {3, 1, 4, 1, 5, 9, 2, 6};

    auto result = hpx::reduce(hpx::execution::seq, c.begin(), c.end(),
        std::pair<int, int>{INT_MAX, INT_MIN}, minmax{});

    HPX_TEST_EQ(result.first, 1);
    HPX_TEST_EQ(result.second, 9);

    result = hpx::reduce(hpx::execution::par, c.begin(), c.end(),
        std::pair<int, int>{INT_MAX, INT_MIN}, minmax{});

    HPX_TEST_EQ(result.first, 1);
    HPX_TEST_EQ(result.second, 9);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
