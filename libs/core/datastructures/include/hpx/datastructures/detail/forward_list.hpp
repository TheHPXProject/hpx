//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <stdexcept>
#include <utility>

namespace hpx::detail {

    HPX_CXX_CORE_EXPORT template <typename T>
    class forward_list
    {
    private:
        struct node
        {
            T data;
            node* next = nullptr;

            explicit node(T const& value)
              : data(value)
            {
            }
            explicit node(T&& value) noexcept
              : data(HPX_MOVE(value))
            {
            }
        };

        node* head_ = nullptr;

    public:
        // Constructor
        constexpr forward_list() = default;

        // Destructor
        ~forward_list()
        {
            clear();
        }

        // Delete copy operations (optional: remove these if you want to allow copying)
        forward_list(forward_list const&) = delete;
        forward_list(forward_list&& other) = delete;
        forward_list& operator=(forward_list const&) = delete;
        forward_list& operator=(forward_list&& other) = delete;

        // Return reference to the first element
        T& front()
        {
            if (!head_)
            {
                throw std::runtime_error(
                    "forward_list::front() called on empty list");
            }
            return head_->data;
        }

        T const& front() const
        {
            if (!head_)
            {
                throw std::runtime_error(
                    "forward_list::front() called on empty list");
            }
            return head_->data;
        }

        // Insert element at the beginning
        void push_front(T const& value)
        {
            node* newnode = new node(value);
            newnode->next = head_;
            head_ = newnode;
        }

        void push_front(T&& value)
        {
            node* newnode = new node(HPX_MOVE(value));
            newnode->next = head_;
            head_ = newnode;
        }

        // Remove first element
        void pop_front()
        {
            if (!head_)
            {
                throw std::runtime_error(
                    "forward_list::pop_front() called on empty list");
            }
            node* temp = head_;
            head_ = head_->next;
            delete temp;
        }

        // Insert element at the end
        // Time complexity: O(n) where n is the number of elements
        void push_back(T const& value)
        {
            node* newnode = new node(value);

            if (!head_)
            {
                head_ = newnode;
                return;
            }

            node* current = head_;
            while (current->next)
            {
                current = current->next;
            }
            current->next = newnode;
        }

        void push_back(T&& value)
        {
            node* newnode = new node(HPX_MOVE(value));

            if (!head_)
            {
                head_ = newnode;
                return;
            }

            node* current = head_;
            while (current->next)
            {
                current = current->next;
            }
            current->next = newnode;
        }

        // Reverse the list in-place
        void reverse() noexcept
        {
            node* prev = nullptr;
            node* current = head_;

            while (current)
            {
                node* next = current->next;    // Save next node
                current->next = prev;          // Reverse the link
                prev = current;                // Move prev forward
                current = next;                // Move current forward
            }

            head_ = prev;    // Update head_ to the last node
        }

        // Check if list is empty
        constexpr bool empty() const noexcept
        {
            return head_ == nullptr;
        }

        // Clear the list
        void clear()
        {
            while (!empty())
            {
                pop_front();
            }
        }
    };
}    // namespace hpx::detail
