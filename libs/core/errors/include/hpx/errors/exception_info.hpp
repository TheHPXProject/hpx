//  Copyright (c) 2017 Agustin Berge
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/errors/error_code.hpp>

#include <exception>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

#if defined(HPX_WINDOWS)
#include <excpt.h>
#undef exception_info
#endif

#include <hpx/config/warnings_prefix.hpp>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Tag, typename Type>
    struct error_info
    {
        using tag = Tag;
        using type = Type;

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        explicit error_info(Type const& value) noexcept(
            std::is_nothrow_copy_constructible_v<Type>)
          : _value(value)
        {
        }

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        explicit error_info(Type&& value) noexcept
          : _value(HPX_MOVE(value))
        {
        }

        Type _value;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT class exception_info_node_base
        {
        public:
            virtual ~exception_info_node_base() = default;

            exception_info_node_base() = default;

            exception_info_node_base(exception_info_node_base const&) = default;
            exception_info_node_base(exception_info_node_base&&) = default;
            exception_info_node_base& operator=(
                exception_info_node_base const&) = default;
            exception_info_node_base& operator=(
                exception_info_node_base&&) = default;

            [[nodiscard]] virtual void const* lookup(
                std::type_info const& tag) const noexcept = 0;

            std::shared_ptr<exception_info_node_base> next;
        };

        HPX_CXX_CORE_EXPORT template <typename... Ts>
        class exception_info_node
          : public exception_info_node_base
          , Ts...
        {
        public:
            template <typename... ErrorInfo>
            explicit exception_info_node(ErrorInfo&&... tagged_values)
              : Ts(HPX_FORWARD(ErrorInfo, tagged_values))...
            {
            }

            [[nodiscard]] void const* lookup(
                std::type_info const& tag) const noexcept override
            {
                using entry_type =
                    std::pair<std::type_info const&, void const*>;
                entry_type const entries[] = {{typeid(typename Ts::tag),
                    std::addressof(static_cast<Ts const*>(this)->_value)}...};

                for (auto const& entry : entries)
                {
                    if (entry.first == tag)
                        return entry.second;
                }

                return next ? next->lookup(tag) : nullptr;
            }

            using exception_info_node_base::next;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class exception_info
    {
        using node_ptr = std::shared_ptr<detail::exception_info_node_base>;

    public:
        constexpr exception_info() noexcept = default;

        exception_info(exception_info const& other) noexcept = default;
        exception_info(exception_info&& other) noexcept = default;

        exception_info& operator=(
            exception_info const& other) noexcept = default;
        exception_info& operator=(exception_info&& other) noexcept = default;

        virtual ~exception_info() = default;

        template <typename... ErrorInfo>
        exception_info& set(ErrorInfo&&... tagged_values)
        {
            using node_type = detail::exception_info_node<ErrorInfo...>;

            node_ptr node = std::make_shared<node_type>(
                HPX_FORWARD(ErrorInfo, tagged_values)...);
            node->next = HPX_MOVE(_data);
            _data = HPX_MOVE(node);
            return *this;
        }

        template <typename Tag>
        [[nodiscard]] typename Tag::type const* get() const noexcept
        {
            auto const* data = _data.get();
            return static_cast<typename Tag::type const*>(
                data ? data->lookup(typeid(typename Tag::tag)) : nullptr);
        }

    private:
        node_ptr _data = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT struct exception_with_info_base : exception_info
        {
            exception_with_info_base(
                std::type_info const& type, exception_info xi) noexcept
              : exception_info(HPX_MOVE(xi))
              , type(type)
            {
            }

            std::type_info const& type;
        };

        HPX_CXX_CORE_EXPORT template <typename E>
        struct HPX_ALWAYS_EXPORT exception_with_info
          : E
          , exception_with_info_base
        {
            explicit exception_with_info(E const& e, exception_info xi)
              : E(e)
              , exception_with_info_base(typeid(E), HPX_MOVE(xi))
            {
            }

            explicit exception_with_info(E&& e, exception_info xi) noexcept
              : E(HPX_MOVE(e))
              , exception_with_info_base(typeid(E), HPX_MOVE(xi))
            {
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename E>
    [[noreturn]] void throw_with_info(
        E&& e, exception_info&& xi = exception_info())
    {
        using ED = std::decay_t<E>;
        static_assert(std::is_class_v<ED> && !std::is_final_v<ED>,
            "E shall be a valid base class");
        static_assert(!std::is_base_of_v<exception_info, ED>,
            "E shall not derive from exception_info");

        throw detail::exception_with_info<ED>(HPX_FORWARD(E, e), HPX_MOVE(xi));
    }

    HPX_CXX_CORE_EXPORT template <typename E>
    [[noreturn]] void throw_with_info(E&& e, exception_info const& xi)
    {
        throw_with_info(HPX_FORWARD(E, e), exception_info(xi));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] exception_info* get_exception_info(E& e) noexcept
    {
        return dynamic_cast<exception_info*>(std::addressof(e));
    }

    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] exception_info const* get_exception_info(E const& e) noexcept
    {
        return dynamic_cast<exception_info const*>(std::addressof(e));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename E, typename F>
    auto invoke_with_exception_info(E const& e, F&& f) noexcept(
        noexcept(HPX_FORWARD(F, f)(std::declval<exception_info const*>())))
        -> decltype(HPX_FORWARD(F, f)(std::declval<exception_info const*>()))
    {
        return HPX_FORWARD(F, f)(
            dynamic_cast<exception_info const*>(std::addressof(e)));
    }

    // clang-format off
    HPX_CXX_CORE_EXPORT template <typename F>
    auto invoke_with_exception_info(
        std::exception_ptr const& p, F&& f) noexcept(
        noexcept(HPX_FORWARD(F, f)(std::declval<exception_info const*>())))
        -> decltype(HPX_FORWARD(F, f)(std::declval<exception_info const*>()))
    // clang-format on
    {
        // NOLINTBEGIN(bugprone-empty-catch)
        try
        {
            if (p)
                std::rethrow_exception(p);
        }
        catch (exception_info const& xi)
        {
            return HPX_FORWARD(F, f)(&xi);
        }
        catch (...)
        {    //-V565
        }
        // NOLINTEND(bugprone-empty-catch)
        return HPX_FORWARD(F, f)(nullptr);
    }

    HPX_CXX_CORE_EXPORT template <typename F>
    auto invoke_with_exception_info(hpx::error_code const& ec, F&& f)
        -> decltype(HPX_FORWARD(F, f)(std::declval<exception_info const*>()))
    {
        return invoke_with_exception_info(
            detail::access_exception(ec), HPX_FORWARD(F, f));
    }
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
