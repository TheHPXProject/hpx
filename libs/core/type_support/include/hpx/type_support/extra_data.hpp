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
        HPX_CORE_EXPORT void reset() const;

    private:
        // a new table, not yet installed, and the storage of the object it
        // is created for (in the same block)
        struct new_table
        {
            table* t;
            void* member;
        };

        // A new table with room for the id 'id' (and for those assigned so
        // far), allocated together with the object of type T it is created
        // for. It takes over the objects of the current table.
        template <typename T>
        T& grow_with(extra_data_id_type const id)
        {
            using member_type = extra_data_member<T>;

            auto const [t, storage] =
                prepare_table(id, sizeof(member_type), alignof(member_type));
            member_type* member = nullptr;
            try
            {
                member = ::new (storage) member_type();
            }
            catch (...)
            {
                discard_table(t);
                throw;
            }

            install_table(t, id, member);
            return member->t_;
        }

        // Allocates a table with room for the id 'id' and an object of the
        // given size and alignment, with the objects of the current table.
        [[nodiscard]] HPX_CORE_EXPORT new_table prepare_table(
            extra_data_id_type id, std::size_t member_size,
            std::size_t member_alignment) const;

        // makes 't' the current table, with 'member' (in its block) for the
        // id 'id' (the current table must be the one 't' was prepared for)
        HPX_CORE_EXPORT void install_table(table* t, extra_data_id_type id,
            extra_data_member_base* member) noexcept;

        // frees a table that hasn't been installed
        HPX_CORE_EXPORT static void discard_table(table* t) noexcept;

        // destroys the objects in the reverse order of their ids, then frees
        // the tables
        HPX_CORE_EXPORT void clear() noexcept;

        table* table_ = nullptr;
    };
}    // namespace hpx::util
