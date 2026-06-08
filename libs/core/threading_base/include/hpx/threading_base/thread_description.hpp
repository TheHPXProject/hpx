//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
#include <hpx/modules/itt_notify.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    HPX_CORE_EXPORT char const* store_function_annotation(std::string name);
}    // namespace hpx::detail

namespace hpx::threads {

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct thread_description
    {
    public:
        enum class data_type : std::uint8_t
        {
            description = 0,
            address = 1
        };

    private:
        struct data
        {
            union
            {
                char const* desc_;    //-V117
                std::size_t addr_;    //-V117
            };
            data_type type_;

            constexpr data() noexcept
              : desc_(nullptr)
              , type_(data_type::description)
            {
            }
            explicit constexpr data(char const* str) noexcept
              : desc_(str)
              , type_(data_type::description)
            {
            }
        };

        data data_;
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
        util::itt::string_handle desc_itt_;
#endif

        HPX_CORE_EXPORT void init_from_alternative_name(char const* altname);

    public:
        constexpr thread_description() noexcept
          : data_("<unknown>")
        {
        }

        constexpr thread_description(char const* desc) noexcept
          : data_(desc ? desc : "<unknown>")
        {
        }

        explicit thread_description(std::string desc)
          : data_(hpx::detail::store_function_annotation(HPX_MOVE(desc)))
        {
        }

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
        thread_description(
            char const* desc, util::itt::string_handle sh) noexcept
          : data_(desc ? desc : "<unknown>")
          , desc_itt_(HPX_MOVE(sh))
        {
        }

        thread_description(std::string desc, util::itt::string_handle sh)
          : data_(hpx::detail::store_function_annotation(HPX_MOVE(desc)))
          , desc_itt_(HPX_MOVE(sh))
        {
        }
#endif

        // The priority of description is name, altname, address
        template <typename F,
            typename =
                std::enable_if_t<!std::is_same_v<F, thread_description> &&
                    !traits::is_action_v<F>>>
        explicit thread_description(
            F const& f, char const* altname = nullptr) noexcept
        {
            // If a name exists, use it, not the altname.
            if (char const* name = traits::get_function_annotation<F>::call(f);
                name != nullptr)    // -V547
            {
                altname = name;
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
                desc_itt_ = traits::get_function_annotation_itt<F>::call(f);
#endif
            }

#if defined(HPX_HAVE_THREAD_DESCRIPTION_FULL)
            if (altname != nullptr)
            {
                data_.desc_ = altname;
            }
            else
            {
                data_.type_ = data_type::address;
                data_.addr_ = traits::get_function_address<F>::call(f);
            }
#else
            init_from_alternative_name(altname);
#endif

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
            if (!desc_itt_)
            {
                desc_itt_ = util::itt::string_handle(get_description());
            }
#endif
        }

        template <typename Action,
            typename = std::enable_if_t<traits::is_action_v<Action>>>
        explicit thread_description(
            Action, char const* /* altname */ = nullptr) noexcept
          : data_(hpx::actions::detail::get_action_name<Action>())
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
          , desc_itt_(hpx::actions::detail::get_action_name_itt<Action>())
#endif
        {
        }

        [[nodiscard]] constexpr data_type kind() const noexcept
        {
            return data_.type_;
        }

        [[nodiscard]] constexpr char const* get_description() const noexcept
        {
            HPX_ASSERT(data_.type_ == data_type::description);
            return data_.desc_;
        }

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
        [[nodiscard]] util::itt::string_handle get_description_itt()
            const noexcept
        {
            HPX_ASSERT(data_.type_ == data_type::description);
            return desc_itt_ ? desc_itt_ :
                               util::itt::string_handle(get_description());
        }

        [[nodiscard]] util::itt::task get_task_itt(
            util::itt::domain const& domain) const noexcept
        {
            switch (kind())
            {
            case threads::thread_description::data_type::description:
                return {domain, get_description_itt()};

            case threads::thread_description::data_type::address:
                return {
                    domain, util::itt::string_handle("address"), get_address()};

            default:
                HPX_ASSERT(false);
                break;
            }

            return {domain, util::itt::string_handle("<error>")};
        }
#endif

        [[nodiscard]] constexpr std::size_t get_address() const noexcept
        {
            HPX_ASSERT(data_.type_ == data_type::address);
            return data_.addr_;
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return valid();
        }

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            if (data_.type_ == data_type::description)
                return nullptr != data_.desc_;

            HPX_ASSERT(data_.type_ == data_type::address);
            return 0 != data_.addr_;
        }
    };
#else
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct thread_description
    {
    public:
        enum class data_type : std::uint8_t
        {
            description = 0,
            address = 1
        };

    private:
        // expose for ABI compatibility reasons
        void init_from_alternative_name(char const* altname);

    public:
        thread_description() noexcept = default;

        constexpr thread_description(char const* /*desc*/) noexcept {}
        constexpr explicit thread_description(
            std::string const& /*desc*/) noexcept
        {
        }

        template <typename F,
            typename =
                std::enable_if_t<!std::is_same_v<F, thread_description> &&
                    !traits::is_action_v<F>>>
        explicit constexpr thread_description(
            F const& /*f*/, char const* /*altname*/ = nullptr) noexcept
        {
        }

        template <typename Action,
            typename = std::enable_if_t<traits::is_action_v<Action>>>
        explicit constexpr thread_description(
            Action, char const* /*altname*/ = nullptr) noexcept
        {
        }

        [[nodiscard]] static constexpr data_type kind() noexcept
        {
            return data_type::description;
        }

        [[nodiscard]] static constexpr char const* get_description() noexcept
        {
            return "<unknown>";
        }

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
        [[nodiscard]] util::itt::string_handle get_description_itt()
            const noexcept
        {
            HPX_ASSERT(data_.type_ == data_type::description);
            return util::itt::string_handle(get_description());
        }

        [[nodiscard]] util::itt::task get_task_itt(
            util::itt::domain const& domain) const noexcept
        {
            switch (kind())
            {
            case threads::thread_description::data_type::description:
                return {domain, get_description_itt()};

            case threads::thread_description::data_type::address:
                return {domain, "address", get_address()};

            default:
                HPX_ASSERT(false);
                break;
            }

            return {domain, "<error>"};
        }
#endif

        [[nodiscard]] static constexpr std::size_t get_address() noexcept
        {
            return 0;
        }

        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return valid();
        }

        [[nodiscard]] static constexpr bool valid() noexcept
        {
            return true;
        }
    };
#endif

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream&, thread_description const&);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string as_string(
        thread_description const& desc);
}    // namespace hpx::threads

namespace hpx::threads {
    ///////////////////////////////////////////////////////////////////////////
    /// The function get_thread_description is part of the thread related API
    /// allows to query the description of one of the threads known to the
    /// thread-manager.
    ///
    /// \param id         [in] The thread id of the thread being queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns the description of the
    ///                   thread referenced by the \a id parameter. If the
    ///                   thread is not known to the thread-manager the return
    ///                   value will be the string "<unknown>".
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_description
    get_thread_description(thread_id_type const& id, error_code& ec = throws);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_description
    set_thread_description(thread_id_type const& id,
        threads::thread_description const& desc = threads::thread_description(),
        error_code& ec = throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_description
    get_thread_lco_description(
        thread_id_type const& id, error_code& ec = throws);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_description
    set_thread_lco_description(thread_id_type const& id,
        threads::thread_description const& desc = threads::thread_description(),
        error_code& ec = throws);
}    // namespace hpx::threads

namespace hpx::util {

    using thread_description HPX_DEPRECATED_V(1, 9,
        "hpx::util::thread_description is deprecated, use "
        "hpx::threads::thread_description instead") =
        hpx::threads::thread_description;
}
