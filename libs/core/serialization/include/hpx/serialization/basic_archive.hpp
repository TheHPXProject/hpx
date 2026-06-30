//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/config/endian.hpp>
#include <hpx/serialization/config/defines.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::serialization {

    namespace detail {

        struct ptr_helper
        {
            virtual ~ptr_helper() = default;
            virtual std::type_info const& type() const noexcept = 0;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT enum class archive_flags : std::uint32_t {
        no_archive_flags = 0x00000000,
        enable_compression = 0x00002000,
        endian_big = 0x00004000,
        endian_little = 0x00008000,
        disable_array_optimization = 0x00010000,
        disable_data_chunking = 0x00020000,
        disable_receive_data_chunking = 0x00040000,
        archive_is_saving = 0x00080000,
        archive_is_preprocessing = 0x00100000,
        enable_type_checking = 0x00200000,
        all_archive_flags = 0x002fe000    // all of the above
    };

    HPX_CXX_CORE_EXPORT constexpr archive_flags operator|(
        archive_flags lhs, archive_flags rhs) noexcept
    {
        return static_cast<archive_flags>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t operator|(
        std::uint32_t const lhs, archive_flags rhs) noexcept
    {
        return lhs | static_cast<std::uint32_t>(rhs);
    }
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t operator&(
        std::uint32_t const lhs, archive_flags rhs) noexcept
    {
        return lhs & static_cast<std::uint32_t>(rhs);
    }
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t operator~(
        archive_flags rhs) noexcept
    {
        return ~static_cast<std::uint32_t>(rhs);
    }

#if defined(HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS)
    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE void reverse_bytes(
        std::size_t size, char* address)
    {
        std::reverse(address, address + size);
    }
#endif

    namespace detail {

        enum class fundamental_types : std::uint8_t
        {
            none = static_cast<std::uint8_t>(~0x0),
            unknown = 0,
            bool_ = 1,
            wchar_t_ = 2,
            int8 = 3,
            uint8 = 4,
            int16 = 5,
            uint16 = 6,
            int32 = 7,
            uint32 = 8,
            int64 = 9,
            uint64 = 10,
            float_ = 11,
            double_ = 12,
            long_double = 13,
            std_byte = 14,
#if __cpp_char8_t
            char8 = 15,
#endif
#if __cpp_unicode_characters
            char16 = 16,
            char32 = 17,
#endif
            array = 0x80
        };

        constexpr fundamental_types operator|(
            fundamental_types const lhs, fundamental_types const rhs) noexcept
        {
            return static_cast<fundamental_types>(
                static_cast<std::uint8_t>(lhs) |
                static_cast<std::uint8_t>(rhs));
        }

        template <typename T>
        inline constexpr auto fundamental_type_v = fundamental_types::unknown;

        template <typename T>
        inline constexpr fundamental_types fundamental_type_v<T const> =
            fundamental_type_v<T>;

        template <>
        inline constexpr auto fundamental_type_v<bool> =
            fundamental_types::bool_;

        template <>
        inline constexpr auto fundamental_type_v<char> =
            std::is_signed_v<char> ? fundamental_types::int8 :
                                     fundamental_types::uint8;

        template <>
        inline constexpr auto fundamental_type_v<signed char> =
            fundamental_types::int8;

        template <>
        inline constexpr auto fundamental_type_v<unsigned char> =
            fundamental_types::uint8;

        template <>
        inline constexpr auto fundamental_type_v<wchar_t> =
            fundamental_types::wchar_t_;

        template <>
        inline constexpr auto fundamental_type_v<std::int16_t> =
            fundamental_types::int16;

        template <>
        inline constexpr auto fundamental_type_v<std::uint16_t> =
            fundamental_types::uint16;

        template <>
        inline constexpr auto fundamental_type_v<std::int32_t> =
            fundamental_types::int32;

        template <>
        inline constexpr auto fundamental_type_v<std::uint32_t> =
            fundamental_types::uint32;

        template <>
        inline constexpr auto fundamental_type_v<std::int64_t> =
            fundamental_types::int64;

        template <>
        inline constexpr auto fundamental_type_v<std::uint64_t> =
            fundamental_types::uint64;

        template <>
        inline constexpr auto fundamental_type_v<float> =
            fundamental_types::float_;

        template <>
        inline constexpr auto fundamental_type_v<double> =
            fundamental_types::double_;

        template <>
        inline constexpr auto fundamental_type_v<long double> =
            fundamental_types::long_double;

        template <>
        inline constexpr auto fundamental_type_v<std::byte> =
            fundamental_types::std_byte;

#if __cpp_char8_t
        template <>
        inline constexpr auto fundamental_type_v<char8_t> =
            fundamental_types::char8;
#endif
#if __cpp_unicode_characters
        template <>
        inline constexpr auto fundamental_type_v<char16_t> =
            fundamental_types::char16;
        template <>
        inline constexpr auto fundamental_type_v<char32_t> =
            fundamental_types::char32;
#endif

        template <typename T>
        inline constexpr fundamental_types array_of_fundamental_type_v =
            fundamental_type_v<T> | fundamental_types::array;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Archive>
    struct basic_archive
    {
        static constexpr std::uint64_t npos = static_cast<std::uint64_t>(-1);

    protected:
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        explicit constexpr basic_archive(std::uint32_t const flags) noexcept
          : flags_(flags)
        {
        }

    public:
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        basic_archive(basic_archive const&) = delete;
        basic_archive& operator=(basic_archive const&) = delete;

        virtual ~basic_archive() = default;

        template <typename T>
        void invoke(T& t)
        {
            static_cast<Archive*>(this)->invoke_impl(t);
        }

        [[nodiscard]] constexpr bool archive_is_saving() const noexcept
        {
            return static_cast<bool>(flags_ & archive_flags::archive_is_saving);
        }

        [[nodiscard]] constexpr bool enable_compression() const noexcept
        {
            return static_cast<bool>(
                flags_ & archive_flags::enable_compression);
        }

        [[nodiscard]] constexpr bool endian_big() const noexcept
        {
            return static_cast<bool>(flags_ & archive_flags::endian_big);
        }

        [[nodiscard]] constexpr bool endian_little() const noexcept
        {
            return static_cast<bool>(flags_ & archive_flags::endian_little);
        }

#if defined(HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS)
        [[nodiscard]] constexpr bool endianess_differs() const noexcept
        {
            return endian::native == endian::big ? endian_little() :
                                                   endian_big();
        }
#else
        [[nodiscard]] static constexpr bool endianess_differs() noexcept
        {
            return false;
        }
#endif

        [[nodiscard]] constexpr bool disable_array_optimization() const noexcept
        {
            return static_cast<bool>(
                flags_ & archive_flags::disable_array_optimization);
        }

        [[nodiscard]] constexpr bool disable_data_chunking() const noexcept
        {
            return disable_array_optimization() ||
                static_cast<bool>(
                    flags_ & archive_flags::disable_data_chunking);
        }

        [[nodiscard]] constexpr bool disable_receive_data_chunking()
            const noexcept
        {
            return disable_data_chunking() ||
                static_cast<bool>(
                    flags_ & archive_flags::disable_receive_data_chunking);
        }

        [[nodiscard]] constexpr std::uint32_t flags() const noexcept
        {
            return flags_;
        }

        // Archives can be used to do 'fake' serialization, in which case no
        // data is being stored/restored and no side effects should be performed
        // during serialization/de-serialization.
        [[nodiscard]] constexpr bool is_preprocessing() const noexcept
        {
            return static_cast<bool>(
                flags_ & archive_flags::archive_is_preprocessing);
        }

        [[nodiscard]] constexpr std::size_t current_pos() const noexcept
        {
            return size_;
        }

        [[nodiscard]] constexpr bool enable_type_checking() const noexcept
        {
            return static_cast<bool>(
                flags_ & archive_flags::enable_type_checking);
        }

        void save_binary(void const* address, std::size_t count)
        {
            static_cast<Archive*>(this)->save_binary(address, count);
        }

        void load_binary(void* address, std::size_t count)
        {
            static_cast<Archive*>(this)->load_binary(address, count);
        }

        void reset()
        {
            size_ = overhead_;
            extra_data_.reset();
        }

        // access extra data stored
        template <typename T>
        T& get_extra_data()
        {
            return extra_data_.get<T>();
        }

        // try accessing extra data stored, might return nullptr
        template <typename T>
        [[nodiscard]] T* try_get_extra_data() const noexcept
        {
            return extra_data_.try_get<T>();
        }

    protected:
        std::uint32_t flags_ = 0;
        std::size_t size_ = 0;
        std::size_t overhead_ = 0;
        util::extra_data extra_data_;
    };

    HPX_CXX_CORE_EXPORT template <typename Archive>
    void save_binary(Archive& ar, detail::fundamental_types t,
        void const* address, std::size_t count)
    {
        ar.save_binary(t, address, count);
    }

    HPX_CXX_CORE_EXPORT template <typename Archive>
    void load_binary(Archive& ar, detail::fundamental_types expected,
        void* address, std::size_t count)
    {
        ar.load_binary(expected, address, count);
    }

    HPX_CXX_CORE_EXPORT template <typename Archive>
    std::size_t current_pos(Archive const& ar) noexcept
    {
        return ar.current_pos();
    }
}    // namespace hpx::serialization

#include <hpx/config/warnings_suffix.hpp>
