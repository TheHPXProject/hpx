////////////////////////////////////////////////////////////////////////////////
//  Algorithms from "CAS-Based Lock-Free Algorithm for Shared Deques"
//  by M. M. Michael
//  Link: http://www.research.ibm.com/people/m/michael/europar-2003.pdf
//
//  C++ implementation - Copyright (C) 2011 Bryce Lelbach
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  Disclaimer: Not a Boost library.
//
//  The complexity of each operation is defined in terms of total work (see
//  section 2.5 of the paper) under contention from P processes. Without
//  contention, all operations have a constant complexity, except for the dtor.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/concurrency/detail/freelist.hpp>
#include <hpx/concurrency/detail/tagged_ptr.hpp>
#include <hpx/concurrency/detail/tagged_ptr_pair.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace hpx::lockfree {

    // The "left" and "right" terminology is used instead of top and bottom to
    // stay consistent with the paper that this code is based on..
    HPX_CXX_CORE_EXPORT enum class deque_status_type : std::int8_t {
        stable,
        rpush,
        lpush
    };

    HPX_CXX_CORE_EXPORT constexpr bool operator==(
        int lhs, deque_status_type rhs) noexcept
    {
        return lhs == static_cast<int>(rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator==(
        deque_status_type lhs, int rhs) noexcept
    {
        return static_cast<int>(lhs) == rhs;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    struct deque_node    //-V690
    {
        using pointer = hpx::lockfree::detail::tagged_ptr<deque_node>;
        using atomic_pointer = std::atomic<pointer>;

        using tag_t = typename pointer::tag_t;

        atomic_pointer left;
        atomic_pointer right;
        T data;

        constexpr deque_node() noexcept
          : left(nullptr)
          , right(nullptr)
          , data()
        {
        }

        deque_node(deque_node const& p) noexcept    //-V730
          : left(p.left.load(std::memory_order_relaxed))
          , right(p.right.load(std::memory_order_relaxed))
        {
        }

        deque_node(deque_node* lptr, deque_node* rptr, T const& v,
            tag_t ltag = 0, tag_t rtag = 0) noexcept
          : left(pointer(lptr, ltag))
          , right(pointer(rptr, rtag))
          , data(v)
        {
        }

        deque_node(deque_node* lptr, deque_node* rptr, T&& v, tag_t ltag = 0,
            tag_t rtag = 0) noexcept
          : left(pointer(lptr, ltag))
          , right(pointer(rptr, rtag))
          , data(HPX_MOVE(v))
        {
        }
    };

    // FIXME: A lot of these methods can be dropped; in fact, it may make sense
    // to re-structure this class like deque_node.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct deque_anchor    //-V690
    {
        using node = deque_node<T>;
        using node_pointer = typename node::pointer;
        using atomic_node_pointer = typename node::atomic_pointer;

        using tag_t = typename node::tag_t;

        using anchor = deque_anchor<T>;
        using pair = tagged_ptr_pair<node, node>;
        using atomic_pair = std::atomic<pair>;

    private:
        atomic_pair pair_;

    public:
        constexpr deque_anchor() noexcept
          : pair_(pair(nullptr, nullptr, deque_status_type::stable, 0))
        {
        }

        deque_anchor(deque_anchor const& p) noexcept
          : pair_(p.pair_.load(std::memory_order_relaxed))
        {
        }

        explicit deque_anchor(pair const& p) noexcept
          : pair_(p)
        {
        }

        deque_anchor(node* lptr, node* rptr,
            tag_t status = deque_status_type::stable, tag_t tag = 0) noexcept
          : pair_(pair(lptr, rptr, status, tag))
        {
        }

        pair lrs(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return pair_.load(mo);
        }

        node* left(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return pair_.load(mo).get_left_ptr();
        }

        node* right(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return pair_.load(mo).get_right_ptr();
        }

        tag_t status(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return pair_.load(mo).get_left_tag();
        }

        tag_t tag(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return pair_.load(mo).get_right_tag();
        }

        bool cas(deque_anchor const& expected, deque_anchor const& desired,
            std::memory_order mo = std::memory_order_acq_rel) noexcept
        {
            return pair_.compare_exchange_strong(
                expected.lrs(std::memory_order_acquire),
                desired.lrs(std::memory_order_acquire), mo);
        }

        bool cas(pair& expected, deque_anchor const& desired,
            std::memory_order mo = std::memory_order_acq_rel) noexcept
        {
            return pair_.compare_exchange_strong(
                expected, desired.lrs(std::memory_order_acquire), mo);
        }

        bool cas(deque_anchor const& expected, pair const& desired,
            std::memory_order mo = std::memory_order_acq_rel) noexcept
        {
            return pair_.compare_exchange_strong(
                expected.lrs(std::memory_order_acquire), desired, mo);
        }

        bool cas(pair& expected, pair const& desired,
            std::memory_order mo = std::memory_order_acq_rel) noexcept
        {
            return pair_.compare_exchange_strong(expected, desired, mo);
        }

        bool operator==(deque_anchor const& rhs) const noexcept
        {
            return pair_.load(std::memory_order_acquire) ==
                rhs.pair_.load(std::memory_order_acquire);
        }

        bool operator!=(deque_anchor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        friend bool operator==(
            deque_anchor const& lhs, pair const& rhs) noexcept
        {
            return lhs.pair_.load(std::memory_order_acquire) == rhs;
        }

        friend bool operator!=(
            deque_anchor const& lhs, pair const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend bool operator==(
            pair const& lhs, deque_anchor const& rhs) noexcept
        {
            return lhs == rhs.pair_.load(std::memory_order_acquire);
        }

        friend bool operator!=(
            pair const& lhs, deque_anchor const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        [[nodiscard]] bool is_lock_free() const noexcept
        {
            return pair_.is_lock_free();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T,
        typename freelist_t = caching_freelist_t,
        typename Alloc = std::allocator<T>>
    struct deque
    {
    public:
        HPX_NON_COPYABLE(deque);

    public:
        using node = deque_node<T>;

        using node_pointer = typename node::pointer;
        using atomic_node_pointer = typename node::atomic_pointer;

        using tag_t = typename node::tag_t;

        using anchor = deque_anchor<T>;
        using anchor_pair = typename anchor::pair;
        using atomic_anchor_pair = typename anchor::atomic_pair;

        using node_allocator =
            typename std::allocator_traits<Alloc>::template rebind_alloc<node>;

        using pool =
            std::conditional_t<std::is_same_v<freelist_t, caching_freelist_t>,
                caching_freelist<node, node_allocator>,
                static_freelist<node, node_allocator>>;

    private:
        anchor anchor_;
        pool pool_;

        static constexpr std::size_t padding_size =
            hpx::threads::get_cache_line_size() - sizeof(anchor);    //-V103
        char padding[padding_size];

        node* alloc_node(
            node* lptr, node* rptr, T const& v, tag_t ltag = 0, tag_t rtag = 0)
        {
            node* chunk = pool_.allocate();
            if (chunk == nullptr)
            {
                throw std::bad_alloc();
            }
            hpx::construct_at(chunk, lptr, rptr, v, ltag, rtag);
            return chunk;
        }

        node* alloc_node(
            node* lptr, node* rptr, T&& v, tag_t ltag = 0, tag_t rtag = 0)
        {
            node* chunk = pool_.allocate();
            if (chunk == nullptr)
            {
                throw std::bad_alloc();
            }
            hpx::construct_at(chunk, lptr, rptr, HPX_MOVE(v), ltag, rtag);
            return chunk;
        }

        void dealloc_node(node* n) noexcept
        {
            if (n != nullptr)
            {
                std::destroy_at(n);
                pool_.deallocate(n);
            }
        }

        void stabilize_left(anchor_pair& lrs) noexcept
        {
            // Get the right node of the leftmost pointer held by lrs and its
            // ABA tag (tagged_ptr).
            node_pointer prev =
                lrs.get_left_ptr()->right.load(std::memory_order_acquire);

            if (anchor_ != lrs)
                return;

            // Get the left node of prev and its tag (again, a tuple represented
            // by a tagged_ptr).
            node_pointer prevnext =
                prev.get_ptr()->left.load(std::memory_order_acquire);

            // Check if prevnext is equal to r.
            if (prevnext.get_ptr() != lrs.get_left_ptr())
            {
                if (anchor_ != lrs)
                    return;

                // Attempt the CAS, incrementing the tag to protect from the ABA
                // problem.
                if (!prev.get_ptr()->left.compare_exchange_strong(prevnext,
                        node_pointer(
                            lrs.get_left_ptr(), prevnext.get_tag() + 1)))
                    return;
            }
            // Try to update the anchor, modifying the status and ABA tag.
            anchor_.cas(lrs,
                anchor_pair(lrs.get_left_ptr(), lrs.get_right_ptr(),
                    deque_status_type::stable, lrs.get_right_tag() + 1));
        }

        void stabilize_right(anchor_pair& lrs) noexcept
        {
            // Get the left node of the rightmost pointer held by lrs and its
            // ABA tag (tagged_ptr).
            node_pointer prev =
                lrs.get_right_ptr()->left.load(std::memory_order_acquire);

            if (anchor_ != lrs)
                return;

            // Get the right node of prev and its tag (again, a tuple
            // represented by a tagged_ptr).
            node_pointer prevnext =
                prev.get_ptr()->right.load(std::memory_order_acquire);

            // Check if prevnext is equal to r.
            if (prevnext.get_ptr() != lrs.get_right_ptr())
            {
                if (anchor_ != lrs)
                    return;

                // Attempt the CAS, incrementing the tag to protect from the ABA
                // problem.
                if (!prev.get_ptr()->right.compare_exchange_strong(prevnext,
                        node_pointer(
                            lrs.get_right_ptr(), prevnext.get_tag() + 1)))
                    return;
            }
            // Try to update the anchor, modifying the status and ABA tag.
            anchor_.cas(lrs,
                anchor_pair(lrs.get_left_ptr(), lrs.get_right_ptr(),
                    deque_status_type::stable, lrs.get_right_tag() + 1));
        }

        void stabilize(anchor_pair& lrs) noexcept
        {
            // The left tag stores the status.
            if (lrs.get_left_tag() == deque_status_type::rpush)
                stabilize_right(lrs);
            else    // lrs.s() == lpush
                stabilize_left(lrs);
        }

    public:
        //  Variable 'padding' is uninitialized
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

        explicit deque(std::size_t initial_nodes = 128)
          : anchor_()
          , pool_(initial_nodes)
        {
        }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

        // Not thread-safe.
        // Complexity: O(N*Processes)
        ~deque()
        {
            if (!empty())
            {
                T dummy = T();
                while (true)
                {
                    if (!pop_left(dummy))
                        break;
                }
            }
        }

        // Thread-safe and non-blocking.
        // Complexity: O(Processes)
        [[nodiscard]] bool empty(
            std::memory_order mo = std::memory_order_relaxed) const noexcept
        {
            anchor_pair lrs = anchor_.lrs(mo);
            return lrs.get_left_ptr() == nullptr &&
                lrs.get_right_ptr() == nullptr;
        }

        // Thread-safe and non-blocking.
        // Complexity: O(1)
        [[nodiscard]] bool is_lock_free() const noexcept
        {
            return anchor_.is_lock_free();
        }

        // Thread-safe and non-blocking (may block if node needs to be allocated
        // from the operating system). Returns false if the freelist is not able
        // to allocate a new deque node. Complexity: O(Processes)
        bool push_left(T data)
        {
            // Allocate the new node which we will be inserting.
            node* n = alloc_node(nullptr, nullptr, HPX_MOVE(data));

            if (n == nullptr)
                return false;

            // Loop until we insert successfully.
            while (true)
            {
                // Load the anchor.
                anchor_pair lrs = anchor_.lrs(std::memory_order_relaxed);

                // Check if the deque is empty.
                if (lrs.get_left_ptr() == nullptr &&
                    lrs.get_right_ptr() == nullptr)
                {
                    // If the deque is empty, we simply install a new anchor
                    // which points to the new node as both its leftmost and
                    // rightmost element.
                    if (anchor_.cas(lrs,
                            anchor_pair(n, n, lrs.get_left_tag(),
                                lrs.get_right_tag() + 1)))
                        return true;
                }

                // Check if the deque is stable.
                else if (lrs.get_left_tag() == deque_status_type::stable)
                {
                    // Make the right pointer on our new node refer to the current
                    // leftmost node.
                    n->right.store(node_pointer(lrs.get_left_ptr()));

                    // Now we want to make the anchor point to our new node as
                    // the leftmost node. We change the state to lpush as the
                    // deque will become unstable if this operation succeeds.
                    anchor_pair new_anchor(n, lrs.get_right_ptr(),
                        deque_status_type::lpush, lrs.get_right_tag() + 1);

                    if (anchor_.cas(lrs, new_anchor))
                    {
                        stabilize_left(new_anchor);
                        return true;
                    }
                }

                // The deque must be unstable, so we have to stabilize it before
                // we can continue.
                else    // lrs.s() != stable
                    stabilize(lrs);
            }
        }

        // Thread-safe and non-blocking (may block if node needs to be allocated
        // from the operating system). Returns false if the freelist is not able
        // to allocate a new deque node. Complexity: O(Processes)
        bool push_right(T data)
        {
            // Allocate the new node which we will be inserting.
            node* n = alloc_node(nullptr, nullptr, HPX_MOVE(data));

            if (n == nullptr)
                return false;

            // Loop until we insert successfully.
            while (true)
            {
                // Load the anchor.
                anchor_pair lrs = anchor_.lrs(std::memory_order_relaxed);

                // Check if the deque is empty.
                if (lrs.get_right_ptr() == nullptr &&
                    lrs.get_left_ptr() == nullptr)
                {
                    // If the deque is empty, we simply install a new anchor
                    // which points to the new node as both its leftmost and
                    // rightmost element.
                    if (anchor_.cas(lrs,
                            anchor_pair(n, n, lrs.get_left_tag(),
                                lrs.get_right_tag() + 1)))
                        return true;
                }

                // Check if the deque is stable.
                else if (lrs.get_left_tag() == deque_status_type::stable)
                {
                    // Make the left pointer on our new node refer to the
                    // current rightmost node.
                    n->left.store(node_pointer(lrs.get_right_ptr()));

                    // Now we want to make the anchor point to our new node as
                    // the rightmost node. We change the state to rpush as the
                    // deque will become unstable if this operation succeeds.
                    anchor_pair new_anchor(lrs.get_left_ptr(), n,
                        deque_status_type::rpush, lrs.get_right_tag() + 1);

                    if (anchor_.cas(lrs, new_anchor))
                    {
                        stabilize_right(new_anchor);
                        return true;
                    }
                }

                // The deque must be unstable, so we have to stabilize it before
                // we can continue.
                else    // lrs.s() != stable
                    stabilize(lrs);
            }
        }

        // Thread-safe and non-blocking. Returns false if the deque is empty.
        // Complexity: O(Processes)
        bool pop_left(T& r) noexcept
        {
            // Loop until we either pop an element or learn that the deque is
            // empty.
            while (true)
            {
                // Load the anchor.
                anchor_pair lrs = anchor_.lrs(std::memory_order_relaxed);

                // Check if the deque is empty.
                if (lrs.get_left_ptr() == nullptr &&
                    lrs.get_right_ptr() == nullptr)
                    return false;

                // Check if the deque has 1 element.
                if (lrs.get_left_ptr() == lrs.get_right_ptr())
                {
                    // Try to set both anchor pointer
                    if (anchor_.cas(lrs,
                            anchor_pair(nullptr, nullptr, lrs.get_left_tag(),
                                lrs.get_right_tag() + 1)))
                    {
                        // Set the result, deallocate the popped node, and
                        // return.
                        r = HPX_MOVE(lrs.get_left_ptr()->data);
                        dealloc_node(lrs.get_left_ptr());
                        return true;
                    }
                }

                // Check if the deque is stable.
                else if (lrs.get_left_tag() == deque_status_type::stable)
                {
                    // Make sure the anchor hasn't changed since we loaded it.
                    if (anchor_ != lrs)
                        continue;

                    // Get the leftmost nodes' right node.
                    node_pointer prev = lrs.get_left_ptr()->right.load(
                        std::memory_order_acquire);

                    // Try to update the anchor to point to prev as the leftmost
                    // node.
                    if (anchor_.cas(lrs,
                            anchor_pair(prev.get_ptr(), lrs.get_right_ptr(),
                                lrs.get_left_tag(), lrs.get_right_tag() + 1)))
                    {
                        // Set the result, deallocate the popped node, and return.
                        r = HPX_MOVE(lrs.get_left_ptr()->data);
                        dealloc_node(lrs.get_left_ptr());
                        return true;
                    }
                }

                // The deque must be unstable, so we have to stabilize it before
                // we can continue.
                else    // lrs.s() != stable
                    stabilize(lrs);
            }
        }

        bool pop_left(T* r) noexcept
        {
            return pop_left(*r);
        }

        // Thread-safe and non-blocking. Returns false if the deque is empty.
        // Complexity: O(Processes)
        bool pop_right(T& r) noexcept
        {
            // Loop until we either pop an element or learn that the deque is
            // empty.
            while (true)
            {
                // Load the anchor.
                anchor_pair lrs = anchor_.lrs(std::memory_order_relaxed);

                // Check if the deque is empty.
                if (lrs.get_right_ptr() == nullptr &&
                    lrs.get_left_ptr() == nullptr)
                    return false;

                // Check if the deque has 1 element.
                if (lrs.get_right_ptr() == lrs.get_left_ptr())
                {
                    // Try to set both anchor pointer
                    if (anchor_.cas(lrs,
                            anchor_pair(nullptr, nullptr, lrs.get_left_tag(),
                                lrs.get_right_tag() + 1)))
                    {
                        // Set the result, deallocate the popped node, and
                        // return.
                        r = HPX_MOVE(lrs.get_right_ptr()->data);
                        dealloc_node(lrs.get_right_ptr());
                        return true;
                    }
                }

                // Check if the deque is stable.
                else if (lrs.get_left_tag() == deque_status_type::stable)
                {
                    // Make sure the anchor hasn't changed since we loaded it.
                    if (anchor_ != lrs)
                        continue;

                    // Get the rightmost nodes' left node.
                    node_pointer prev = lrs.get_right_ptr()->left.load(
                        std::memory_order_acquire);

                    // Try to update the anchor to point to prev as the
                    // rightmost node.
                    if (anchor_.cas(lrs,
                            anchor_pair(lrs.get_left_ptr(), prev.get_ptr(),
                                lrs.get_left_tag(), lrs.get_right_tag() + 1)))
                    {
                        // Set the result, deallocate the popped node, and
                        // return.
                        r = HPX_MOVE(lrs.get_right_ptr()->data);
                        dealloc_node(lrs.get_right_ptr());
                        return true;
                    }
                }

                // The deque must be unstable, so we have to stabilize it before
                // we can continue.
                else    // lrs.s() != stable
                    stabilize(lrs);
            }
        }

        bool pop_right(T* r) noexcept
        {
            return pop_right(*r);
        }
    };
}    // namespace hpx::lockfree
