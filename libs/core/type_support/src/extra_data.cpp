//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/type_support/extra_data.hpp>

#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

namespace hpx::util::detail {

    namespace {

        std::atomic<extra_data_id_type> id_count(0);
    }    // namespace

    extra_data_id_type next_extra_data_id() noexcept
    {
        return id_count.fetch_add(1, std::memory_order_relaxed);
    }

    extra_data_id_type extra_data_id_count() noexcept
    {
        return id_count.load(std::memory_order_relaxed);
    }
}    // namespace hpx::util::detail

namespace hpx::util {

    namespace {

        // The smallest number of slots of a table: the ids are assigned when
        // a type is used first, a table created early would otherwise grow
        // when other types are used later. Covers the types of extra data of
        // HPX and a few of the application.
        constexpr std::size_t minimum_table_size = 16;

        // the aligned operator new only for over-aligned objects: it can be
        // considerably slower (e.g. with MSVC)
        void* allocate_block(
            std::size_t const size, std::size_t const alignment)
        {
            if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                return ::operator new(size, std::align_val_t(alignment));
            }
            return ::operator new(size);
        }

        void free_block(void* block, std::size_t const alignment) noexcept
        {
            if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                ::operator delete(block, std::align_val_t(alignment));
            }
            else
            {
                ::operator delete(block);
            }
        }
    }    // namespace

    void extra_data::reset() const
    {
        if (table_ == nullptr)
        {
            return;
        }
        extra_data_member_base** slots = table_->slots();
        for (std::size_t i = table_->size; i != 0; --i)
        {
            if (slots[i - 1] != nullptr)
            {
                slots[i - 1]->reset();
            }
        }
    }

    extra_data::new_table extra_data::prepare_table(extra_data_id_type const id,
        std::size_t const member_size, std::size_t const member_alignment) const
    {
        std::size_t size = detail::extra_data_id_count();
        if (size <= id)
        {
            size = id + 1;
        }
        if (size < minimum_table_size)
        {
            size = minimum_table_size;
        }

        std::size_t const alignment = member_alignment > alignof(table) ?
            member_alignment :
            alignof(table);
        std::size_t const slots_end =
            sizeof(table) + size * sizeof(extra_data_member_base*);
        std::size_t const member_offset =
            (slots_end + member_alignment - 1) & ~(member_alignment - 1);

        void* block = allocate_block(member_offset + member_size, alignment);

        table* t = ::new (block) table{size, alignment, table_};
        extra_data_member_base** slots = t->slots();
        std::size_t i = 0;
        if (table_ != nullptr)
        {
            extra_data_member_base** old_slots = table_->slots();
            for (/**/; i != table_->size; ++i)
            {
                slots[i] = old_slots[i];
            }
        }
        for (/**/; i != size; ++i)
        {
            slots[i] = nullptr;
        }

        return new_table{t, static_cast<char*>(block) + member_offset};
    }

    void extra_data::install_table(table* t, extra_data_id_type const id,
        extra_data_member_base* member) noexcept
    {
        // the objects are default constructed, without access to this extra
        // data: constructing one doesn't create others
        HPX_ASSERT(t->previous == table_);

        member->in_table_block = true;
        t->slots()[id] = member;
        table_ = t;
    }

    void extra_data::discard_table(table* t) noexcept
    {
        std::size_t const alignment = t->alignment;
        t->~table();
        free_block(t, alignment);
    }

    void extra_data::clear() noexcept
    {
        if (table_ == nullptr)
        {
            return;
        }

        extra_data_member_base** slots = table_->slots();
        for (std::size_t i = table_->size; i != 0; --i)
        {
            extra_data_member_base* member = slots[i - 1];
            if (member == nullptr)
            {
                continue;
            }
            if (member->in_table_block)
            {
                member->~extra_data_member_base();
            }
            else
            {
                delete member;
            }
        }

        for (table* t = std::exchange(table_, nullptr); t != nullptr;)
        {
            table* previous = t->previous;
            discard_table(t);
            t = previous;
        }
    }
}    // namespace hpx::util
