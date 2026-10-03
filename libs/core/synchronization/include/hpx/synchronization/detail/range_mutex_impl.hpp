//  Copyright (c) 2023 Johan511
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is based on https://github.com/Johan511/ByteLock

#pragma once

#include <hpx/datastructures/detail/flat_map.hpp>
#include <hpx/execution_base/this_thread.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::synchronization::detail {

    template <typename Mtx, template <typename> typename Guard>
    class range_mutex
    {
        template <typename Key, typename Value>
        using map_ty = hpx::detail::flat_map<Key, Value>;

        Mtx mtx;
        std::size_t counter = 0;
        map_ty<std::size_t, std::pair<std::size_t, std::size_t>> range_map;
        map_ty<std::size_t, std::shared_ptr<std::atomic_bool>> waiting;

    public:
        std::size_t lock(std::size_t begin, std::size_t end);
        std::size_t try_lock(std::size_t begin, std::size_t end);
        void unlock(std::size_t lock_id);
    };

    template <class Mtx, template <class> class Guard>
    std::size_t range_mutex<Mtx, Guard>::lock(
        std::size_t begin, std::size_t end)
    {
        bool localFlag = false;
        std::size_t blocker_id;

        std::shared_ptr<std::atomic_bool> wait_flag;

        while (true)
        {
            {
                Guard<Mtx> const lock_guard(mtx);
                for (auto const& it : range_map)
                {
                    auto [b, e] = it.second;

                    if ((!(e < begin)) & (!(end < b)))
                    {
                        blocker_id = it.first;
                        localFlag = true;
                        wait_flag = waiting[blocker_id];
                        break;
                    }
                }
                if (!localFlag)
                {
                    ++counter;
                    range_map[counter] = {begin, end};
                    waiting[counter] = std::shared_ptr<std::atomic_bool>(
                        new std::atomic_bool(false));
                    return counter;
                }
                localFlag = false;
            }
            auto pred = [&wait_flag]() noexcept { return wait_flag->load(); };
            util::yield_while<true>(pred, "hpx::range_mutex::lock");
        }
        HPX_UNREACHABLE;
    }

    template <class Mtx, template <class> class Guard>
    void range_mutex<Mtx, Guard>::unlock(std::size_t lock_id)
    {
        if (lock_id == 0)
            return;
        Guard const lock_guard(mtx);

        range_map.erase(lock_id);

        waiting[lock_id]->store(true);

        waiting.erase(lock_id);
        return;
    }

    template <class Mtx, template <class> class Guard>
    std::size_t range_mutex<Mtx, Guard>::try_lock(
        std::size_t begin, std::size_t end)
    {
        Guard const lock_guard(mtx);
        for (auto const& it : range_map)
        {
            auto [b, e] = it.second;

            if (!(e < begin) && !(end < b))
            {
                return 0;
            }
        }
        range_map[++counter] = {begin, end};
        return counter;
    }
}    // namespace hpx::synchronization::detail
