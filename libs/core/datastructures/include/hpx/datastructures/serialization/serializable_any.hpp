/*=============================================================================
    Copyright (c) 2013 Shuangyang Yang
    Copyright (c) 2007-2025 Hartmut Kaiser
    Copyright (c) Christopher Diggins 2005
    Copyright (c) Pablo Aguilar 2005
    Copyright (c) Kevlin Henney 2001

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    The class hpx::any is built based on boost::spirit::hold_any class.
    It adds support for HPX serialization, move assignment, == operator.
==============================================================================*/

/// \file serializable_any.hpp
/// \page hpx::any, hpx::make_any
/// \headerfile hpx/any.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/datastructures/any.hpp>
#include <hpx/datastructures/traits/supports_streaming_with_any.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <type_traits>
#include <utility>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
#if defined(HPX_MSVC) && HPX_MSVC >= 1400
#pragma warning(push)
#pragma warning(disable : 4100)    // 'x': unreferenced formal parameter
#pragma warning(disable : 4127)    // conditional expression is constant
#endif

////////////////////////////////////////////////////////////////////////////////
namespace hpx::util::detail::any {

    ////////////////////////////////////////////////////////////////////////////
    // serializable function pointer table
    template <typename IArch, typename OArch, typename Char>
        requires(!std::is_void_v<IArch> && !std::is_void_v<OArch>)
    struct fxn_ptr_table<IArch, OArch, Char, std::true_type>
    {
        fxn_ptr_table() = default;

        virtual ~fxn_ptr_table() = default;
        virtual fxn_ptr_table* get_ptr() = 0;

        fxn_ptr_table(fxn_ptr_table const&) = delete;
        fxn_ptr_table(fxn_ptr_table&&) = delete;

        fxn_ptr_table& operator=(fxn_ptr_table const&) = delete;
        fxn_ptr_table& operator=(fxn_ptr_table&&) = delete;

        std::type_info const& (*get_type)() = nullptr;
        void (*static_delete)(void**) = nullptr;
        void (*destruct)(void**) = nullptr;
        void (*clone)(void* const*, void**) = nullptr;
        void (*copy)(void* const*, void**) = nullptr;
        bool (*equal_to)(void* const*, void* const*) = nullptr;
        std::basic_istream<Char>& (*stream_in)(
            std::basic_istream<Char>&, void**) = nullptr;
        std::basic_ostream<Char>& (*stream_out)(
            std::basic_ostream<Char>&, void* const*) = nullptr;

        virtual void save_object(void* const*, OArch& ar, unsigned) = 0;
        virtual void load_object(void**, IArch& ar, unsigned) = 0;

        template <typename Arch>
        static void serialize(Arch&, unsigned) noexcept
        {
        }

        HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT(fxn_ptr_table)
    };

    ////////////////////////////////////////////////////////////////////////////
    template <typename IArch, typename OArch, typename Vtable, typename Char>
    struct fxn_ptr<IArch, OArch, Vtable, Char, std::true_type>
      : fxn_ptr_table<IArch, OArch, Char, std::true_type>
    {
        using base_type = fxn_ptr_table<IArch, OArch, Char, std::true_type>;

        fxn_ptr()
        {
            base_type::get_type = Vtable::get_type;
            base_type::static_delete = Vtable::static_delete;
            base_type::destruct = Vtable::destruct;
            base_type::clone = Vtable::clone;
            base_type::copy = Vtable::copy;
            base_type::equal_to = Vtable::equal_to;
            base_type::stream_in = Vtable::stream_in;
            base_type::stream_out = Vtable::stream_out;
        }

        base_type* get_ptr() override
        {
            return Vtable::get_ptr();
        }

        void save_object(void* const* object, OArch& ar, unsigned) override
        {
            // clang-format off
            ar & Vtable::get(object);
            // clang-format on
        }
        void load_object(void** object, IArch& ar, unsigned) override
        {
            // clang-format off
            ar & Vtable::construct(object);
            // clang-format on
        }

        template <typename Arch>
        void serialize(Arch& ar, unsigned)
        {
            // clang-format off
            ar & hpx::serialization::base_object<base_type>(*this);
            // clang-format on
        }
        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE(fxn_ptr, override)
    };
}    // namespace hpx::util::detail::any

namespace hpx::util {

    ////////////////////////////////////////////////////////////////////////////
    template <typename IArch, typename OArch, typename Char>
    class basic_any<IArch, OArch, Char, std::true_type>
    {
    public:
        // constructors
        constexpr basic_any() noexcept
          : table(detail::any::get_table<detail::any::empty>::get<IArch, OArch,
                Char, std::true_type>())
          , object(nullptr)
        {
        }

        basic_any(basic_any const& x)
          : table(detail::any::get_table<detail::any::empty>::get<IArch, OArch,
                Char, std::true_type>())
          , object(nullptr)
        {
            assign(x);
        }

        // Move constructor
        basic_any(basic_any&& x) noexcept
          : table(x.table)
          , object(x.object)
        {
            x.table = detail::any::get_table<detail::any::empty>::get<IArch,
                OArch, Char, std::true_type>();
            x.object = nullptr;
        }

        // Perfect forwarding of T
        template <typename T>
            requires(!std::is_same_v<basic_any, std::decay_t<T>>)
        basic_any(T&& x,
            std::enable_if_t<std::is_copy_constructible_v<std::decay_t<T>>>* =
                nullptr)
          : table(detail::any::get_table<std::decay_t<T>>::template get<IArch,
                OArch, Char, std::true_type>())
          , object(nullptr)
        {
            using value_type = std::decay_t<T>;
            new_object<T>(object,
                typename detail::any::get_table<value_type>::is_small(),
                HPX_FORWARD(T, x));
        }

        template <typename T, typename... Ts>
            requires(std::is_constructible_v<std::decay_t<T>, Ts...> &&
                        std::is_copy_constructible_v<std::decay_t<T>>)
        explicit basic_any(std::in_place_type_t<T>, Ts&&... ts)
          : table(detail::any::get_table<std::decay_t<T>>::template get<IArch,
                OArch, Char, std::true_type>())
          , object(nullptr)
        {
            using value_type = std::decay_t<T>;
            new_object<T>(object,
                typename detail::any::get_table<value_type>::is_small(),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename T, typename U, typename... Ts>
            requires(std::is_constructible_v<std::decay_t<T>, Ts...> &&
                        std::is_copy_constructible_v<std::decay_t<T>>)
        explicit basic_any(
            std::in_place_type_t<T>, std::initializer_list<U> il, Ts&&... ts)
          : table(detail::any::get_table<std::decay_t<T>>::template get<IArch,
                OArch, Char, std::true_type>())
          , object(nullptr)
        {
            using value_type = std::decay_t<T>;
            new_object<T>(object,
                typename detail::any::get_table<value_type>::is_small(), il,
                HPX_FORWARD(Ts, ts)...);
        }

        ~basic_any()
        {
            table->static_delete(&object);
        }

    private:
        // assignment
        basic_any& assign(basic_any const& x)
        {
            if (&x != this)
            {
                // are we copying between the same type?
                if (table == x.table)
                {
                    // if so, we can avoid reallocation
                    table->copy(&x.object, &object);
                }
                else
                {
                    reset();
                    x.table->clone(&x.object, &object);
                    table = x.table;
                }
            }
            return *this;
        }

        template <typename T, typename... Ts>
        static void new_object(void*& object, std::true_type, Ts&&... ts)
        {
            using value_type = std::decay_t<T>;
            // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
            new (&object) value_type(HPX_FORWARD(Ts, ts)...);
        }

        template <typename T, typename... Ts>
        static void new_object(void*& object, std::false_type, Ts&&... ts)
        {
            using value_type = std::decay_t<T>;
            object = new value_type(HPX_FORWARD(Ts, ts)...);
        }

    public:
        // copy assignment operator
        // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
        basic_any& operator=(basic_any const& x)
        {
            basic_any(x).swap(*this);
            return *this;
        }

        // move assignment
        // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
        basic_any& operator=(basic_any&& rhs) noexcept
        {
            rhs.swap(*this);
            basic_any().swap(rhs);
            return *this;
        }

        // Perfect forwarding of T
        template <typename T>
            requires(!std::is_same_v<basic_any, std::decay_t<T>> &&
                std::is_copy_constructible_v<std::decay_t<T>>)
        basic_any& operator=(T&& rhs)
        {
            basic_any(HPX_FORWARD(T, rhs)).swap(*this);
            return *this;
        }

        // utility functions
        basic_any& swap(basic_any& x) noexcept
        {
            std::swap(table, x.table);
            std::swap(object, x.object);
            return *this;
        }

        [[nodiscard]] std::type_info const& type() const
        {
            return table->get_type();
        }

        template <typename T>
        T const& cast() const
        {
            if (type() != typeid(T))
                throw hpx::bad_any_cast(type(), typeid(T));

            return detail::any::get_table<T>::is_small::value ?
                *reinterpret_cast<T const*>(&object) :
                *static_cast<T const*>(object);
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return type() != typeid(detail::any::empty);
        }

        void reset()
        {
            if (has_value())
            {
                table->static_delete(&object);
                table = detail::any::get_table<detail::any::empty>::get<IArch,
                    OArch, Char, std::true_type>();
                object = nullptr;
            }
        }

        // equality operator
        bool equal_to(basic_any const& rhs) const noexcept
        {
            if (this == &rhs)    // same object
            {
                return true;
            }

            if (type() == rhs.type())    // same type
            {
                return table->equal_to(&object, &rhs.object);    // equal value?
            }

            return false;
        }

    private:
        friend class hpx::serialization::access;

        void load(IArch& ar, unsigned const version)
        {
            bool is_empty;
            // clang-format off
            ar & is_empty;
            // clang-format on

            if (is_empty)
            {
                reset();
            }
            else
            {
                detail::any::fxn_ptr_table<IArch, OArch, Char, std::true_type>*
                    p = nullptr;
                ar >> hpx::serialization::detail::raw_ptr(p);
                table = p->get_ptr();    // -V522
                delete p;
                table->load_object(&object, ar, version);
            }
        }

        void save(OArch& ar, unsigned const version) const
        {
            bool is_empty = !has_value();
            // clang-format off
            ar & is_empty;
            // clang-format on

            if (!is_empty)
            {
                ar << hpx::serialization::detail::raw_ptr(table);
                table->save_object(&object, ar, version);
            }
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

    private:    // types
        friend struct detail::any::any_cast_support;
        friend struct detail::any::stream_support;

        // fields
        detail::any::fxn_ptr_table<IArch, OArch, Char, std::true_type>* table;
        void* object;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Char, typename... Ts>
    basic_any<serialization::input_archive, serialization::output_archive, Char>
    make_any(Ts&&... ts)
    {
        return basic_any<serialization::input_archive,
            serialization::output_archive, Char, std::true_type>(
            std::in_place_type<T>, HPX_FORWARD(Ts, ts)...);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Char, typename U,
        typename... Ts>
    basic_any<serialization::input_archive, serialization::output_archive, Char>
    make_any(std::initializer_list<U> il, Ts&&... ts)
    {
        return basic_any<serialization::input_archive,
            serialization::output_archive, Char, std::true_type>(
            std::in_place_type<T>, il, HPX_FORWARD(Ts, ts)...);
    }

    ////////////////////////////////////////////////////////////////////////////
    // backwards compatibility
    HPX_CXX_CORE_EXPORT using wany = basic_any<serialization::input_archive,
        serialization::output_archive, wchar_t, std::true_type>;

    ////////////////////////////////////////////////////////////////////////////
    // support for hashing any
    HPX_CXX_CORE_EXPORT struct hash_any
    {
        template <typename Char>
        HPX_CORE_EXPORT std::size_t operator()(
            basic_any<serialization::input_archive,
                serialization::output_archive, Char, std::true_type> const&
                elem) const;
    };

    // explicitly instantiate the operator()()
    extern template HPX_CORE_EXPORT std::size_t hash_any::operator()(
        basic_any<serialization::input_archive, serialization::output_archive,
            char, std::true_type> const& elem) const;

    extern template HPX_CORE_EXPORT std::size_t hash_any::operator()(
        basic_any<serialization::input_archive, serialization::output_archive,
            wchar_t, std::true_type> const& elem) const;
}    // namespace hpx::util
// namespace hpx::util

namespace hpx {

    /// Constructs an any object containing an object of type \a T, passing the
    /// provided arguments to T's constructor. Equivalent to:
    /// \code
    ///     return std::any(std::in_place_type<T>, std::forward<Args>(args)...);
    /// \endcode
    HPX_CXX_CORE_EXPORT template <typename T, typename Char>
    util::basic_any<serialization::input_archive, serialization::output_archive,
        Char>
    make_any(T&& t)
    {
        return util::basic_any<serialization::input_archive,
            serialization::output_archive, Char, std::true_type>(
            HPX_FORWARD(T, t));
    }

    HPX_CXX_CORE_EXPORT using any =
        util::basic_any<serialization::input_archive,
            serialization::output_archive, char, std::true_type>;
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_MSVC) && HPX_MSVC >= 1400
#pragma warning(pop)
#endif
