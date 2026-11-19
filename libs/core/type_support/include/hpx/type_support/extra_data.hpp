//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2019-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace hpx::util {

    // The id of a type of extra data: a small number, the same in all shared
    // libraries, assigned when the type is used first.
    HPX_CXX_CORE_EXPORT using extra_data_id_type = std::size_t;

    namespace detail {

        // A new id of a type of extra data (called once for each type, by
        // its extra_data_helper<T>::id()).
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extra_data_id_type
        next_extra_data_id() noexcept;

        // the number of ids assigned so far
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extra_data_id_type
        extra_data_id_count() noexcept;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct extra_data_helper
    {
        // This is intentionally left unimplemented, will lead to linker errors
        // if used with unknown data type. It is defined once (in one shared
        // library) for each type, as
        //
        //     static extra_data_id_type const id =
        //         detail::next_extra_data_id();
        //     return id;
        static extra_data_id_type id() noexcept;

        // this is a function that should be implemented in order to reset the
        // extra archive data item
        static void reset(T* data) noexcept;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] extra_data_id_type extra_data_id() noexcept
    {
        // asked once per shared library (id() is exported by the library
        // that defines it)
        static extra_data_id_type const id = extra_data_helper<T>::id();
        return id;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr void reset_extra_data(T* data) noexcept(
        noexcept(extra_data_helper<T>::reset(data)))
    {
        extra_data_helper<T>::reset(data);
    }

    HPX_CXX_CORE_EXPORT struct extra_data_member_base
    {
        extra_data_member_base() = default;

        extra_data_member_base(extra_data_member_base const&) = delete;
        extra_data_member_base(extra_data_member_base&&) = delete;
        extra_data_member_base& operator=(
            extra_data_member_base const&) = delete;
        extra_data_member_base& operator=(extra_data_member_base&&) = delete;

        virtual ~extra_data_member_base() = default;

        virtual void reset() = 0;

        // constructed in the block of a slot table (destroyed, not deleted)
        bool in_table_block = false;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct extra_data_member final : extra_data_member_base
    {
        void reset() override
        {
            reset_extra_data(std::addressof(t_));
        }

        T t_;
    };

    // At most one object of each type of extra data (e.g. of an archive),
    // found by the id of the type in constant time. The objects are created
    // on first use and keep their addresses until the extra data is
    // destroyed (also when it is moved). Without objects it allocates
    // nothing; the first object is allocated together with the table of the
    // objects (one slot for each id assigned so far, so the table isn't likely
    // to grow).
    HPX_CXX_CORE_EXPORT class extra_data
    {
        // A table of the objects, by id, in a block that also holds the
        // object the table was created for. A table replaced by a larger one
        // stays allocated (it holds an object) until the extra data is
        // destroyed.
        struct table
        {
            std::size_t size;
            std::size_t alignment;    // of the block
            table* previous;          // the table this one replaced

            [[nodiscard]] extra_data_member_base** slots() noexcept
            {
                return reinterpret_cast<extra_data_member_base**>(this + 1);
            }
        };

    public:
        extra_data() noexcept = default;

        extra_data(extra_data const&) = delete;
        extra_data& operator=(extra_data const&) = delete;

        extra_data(extra_data&& rhs) noexcept
          : table_(std::exchange(rhs.table_, nullptr))
        {
        }

        extra_data& operator=(extra_data&& rhs) noexcept
        {
            if (this != &rhs)
            {
                clear();
                table_ = std::exchange(rhs.table_, nullptr);
            }
            return *this;
        }

        ~extra_data()
        {
            clear();
        }

        // the object of type T, created if there's none yet
        template <typename T>
        T& get()
        {
            extra_data_id_type const id = extra_data_id<T>();
            if (table_ == nullptr || id >= table_->size)
            {
                return grow_with<T>(id);
            }

            extra_data_member_base*& member = table_->slots()[id];
            if (member == nullptr)
            {
                member = new extra_data_member<T>();
            }
            return static_cast<extra_data_member<T>*>(member)->t_;
        }

        // the object of type T, nullptr if there's none
        template <typename T>
        [[nodiscard]] T* try_get() const noexcept
        {
            extra_data_id_type const id = extra_data_id<T>();
            if (table_ == nullptr || id >= table_->size)
            {
                return nullptr;
            }
            extra_data_member_base* member = table_->slots()[id];
            return member == nullptr ?
                nullptr :
                std::addressof(static_cast<extra_data_member<T>*>(member)->t_);
        }

        // resets all objects (in the reverse order of their ids)
        void reset() const
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

    private:
        // A new table with room for the id 'id' (and for those assigned so
        // far), allocated together with the object of type T it is created
        // for. It takes over the objects of the current table.
        template <typename T>
        T& grow_with(extra_data_id_type const id)
        {
            using member_type = extra_data_member<T>;

            std::size_t size = detail::extra_data_id_count();
            if (size <= id)
            {
                size = id + 1;
            }

            constexpr std::size_t alignment =
                alignof(member_type) > alignof(table) ? alignof(member_type) :
                                                        alignof(table);
            std::size_t const slots_end =
                sizeof(table) + size * sizeof(extra_data_member_base*);
            std::size_t const member_offset =
                (slots_end + alignof(member_type) - 1) &
                ~(alignof(member_type) - 1);

            void* block =
                allocate_block(member_offset + sizeof(member_type), alignment);
            member_type* member = nullptr;
            try
            {
                member = ::new (static_cast<char*>(block) + member_offset)
                    member_type();
            }
            catch (...)
            {
                free_block(block, alignment);
                throw;
            }
            member->in_table_block = true;

            table* t = ::new (block) table{size, alignment, table_};
            extra_data_member_base** slots = t->slots();
            for (std::size_t i = 0; i != size; ++i)
            {
                slots[i] = nullptr;
            }
            if (table_ != nullptr)
            {
                extra_data_member_base** old_slots = table_->slots();
                for (std::size_t i = 0; i != table_->size; ++i)
                {
                    slots[i] = old_slots[i];
                }
            }
            slots[id] = member;
            table_ = t;
            return member->t_;
        }

        // destroys the objects in the reverse order of their ids, then frees
        // the tables
        void clear() noexcept
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
                std::size_t const alignment = t->alignment;
                t->~table();
                free_block(t, alignment);
                t = previous;
            }
        }

        // the aligned operator new only for over-aligned objects: it can be
        // considerably slower (e.g. with MSVC)
        [[nodiscard]] static void* allocate_block(
            std::size_t const size, std::size_t const alignment)
        {
            if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            {
                return ::operator new(size, std::align_val_t(alignment));
            }
            return ::operator new(size);
        }

        static void free_block(
            void* block, std::size_t const alignment) noexcept
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

        table* table_ = nullptr;
    };
}    // namespace hpx::util
