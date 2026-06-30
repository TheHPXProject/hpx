//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/config/defines.hpp>
#include <hpx/assert.hpp>
#include <hpx/serialization/access.hpp>
#include <hpx/serialization/basic_archive.hpp>
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp>
#include <hpx/serialization/detail/raw_ptr.hpp>
#include <hpx/serialization/output_container.hpp>
#include <hpx/serialization/traits/is_serialization_supported.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::serialization {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Container>
        std::unique_ptr<erased_output_container> create_output_container(
            Container& buffer, std::vector<serialization_chunk>* chunks,
            binary_filter* filter,
            std::size_t zero_copy_serialization_threshold, std::false_type)
        {
            std::unique_ptr<erased_output_container> res;
            if (filter == nullptr)
            {
                if (chunks == nullptr)
                {
                    res.reset(
                        new output_container<Container, basic_chunker>(buffer));
                }
                else
                {
                    res.reset(new output_container<Container, vector_chunker>(
                        buffer, chunks, zero_copy_serialization_threshold));
                }
            }
            else
            {
                if (chunks == nullptr)
                {
                    res.reset(
                        new filtered_output_container<Container, basic_chunker>(
                            buffer));
                }
                else
                {
                    res.reset(new filtered_output_container<Container,
                        vector_chunker>(
                        buffer, chunks, zero_copy_serialization_threshold));
                }
            }
            return res;
        }

        HPX_CXX_CORE_EXPORT template <typename Container>
        std::unique_ptr<erased_output_container> create_output_container(
            Container& buffer, std::vector<serialization_chunk>* chunks,
            binary_filter* filter,
            std::size_t zero_copy_serialization_threshold, std::true_type)
        {
            std::unique_ptr<erased_output_container> res;
            if (filter == nullptr)
            {
                res.reset(new output_container<Container, counting_chunker>(
                    buffer, chunks, zero_copy_serialization_threshold));
            }
            else
            {
                res.reset(
                    new filtered_output_container<Container, counting_chunker>(
                        buffer, chunks, zero_copy_serialization_threshold));
            }
            return res;
        }
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct output_archive : basic_archive<output_archive>
    {
    private:
        static constexpr std::uint32_t make_flags(std::uint32_t const flags,
            std::vector<serialization_chunk> const* chunks) noexcept
        {
            return flags | archive_flags::archive_is_saving |
                archive_flags::enable_type_checking |
                (chunks == nullptr ?
                        (archive_flags::disable_data_chunking |
                            archive_flags::disable_receive_data_chunking) :
                        archive_flags::no_archive_flags);
        }

    public:
        using base_type = basic_archive<output_archive>;

        template <typename Container>
        explicit output_archive(Container& buffer,
            std::uint32_t const flags = 0U,
            std::vector<serialization_chunk>* chunks = nullptr,
            binary_filter* filter = nullptr,
            std::size_t zero_copy_serialization_threshold = 0)
          : base_type(make_flags(flags, chunks))
          , buffer_(detail::create_output_container(buffer, chunks, filter,
                zero_copy_serialization_threshold,
                typename traits::serialization_access_data<
                    Container>::preprocessing_only()))
        {
            // cache the preprocessing flag in the base class to avoid asking
            // the buffer repeatedly
            if (buffer_->is_preprocessing())
            {
                flags_ = flags_ | archive_flags::archive_is_preprocessing;
            }

            // endianness needs to be saved separately as it is needed to
            // properly interpret the flags
            bool const endianness = endian_big();
            save_binary(
                detail::fundamental_types::none, &endianness, sizeof(bool));

            // type checking needs to be saved separately as it is needed to
            // properly interpret the flags
            bool const type_checking = enable_type_checking();
            save_binary(
                detail::fundamental_types::none, &type_checking, sizeof(bool));

            // send flags to the other end to make sure both ends have the same
            // assumptions about the archive format
            save(flags_);

            // send the zero-copy limit
            bool const has_zero_copy_serialization_threshold =
                zero_copy_serialization_threshold != 0;
            save(has_zero_copy_serialization_threshold);

            if (has_zero_copy_serialization_threshold)
            {
                save(static_cast<std::uint64_t>(
                    zero_copy_serialization_threshold));
            }

            bool const has_filter = filter != nullptr;
            save(has_filter);

            if (has_filter && enable_compression())
            {
                *this << detail::raw_ptr(filter);
                buffer_->set_filter(filter);
            }

            overhead_ = base_type::current_pos();
        }

        template <typename Container>
        output_archive(Container& buffer, archive_flags flags,
            std::vector<serialization_chunk>* chunks = nullptr,
            binary_filter* filter = nullptr,
            std::size_t zero_copy_serialization_threshold = 0)
          : output_archive(buffer, static_cast<std::uint32_t>(flags), chunks,
                filter, zero_copy_serialization_threshold)
        {
        }

        [[nodiscard]] constexpr std::size_t bytes_written() const noexcept
        {
            return size_;
        }

        [[nodiscard]] std::size_t get_num_chunks() const noexcept
        {
            return buffer_->get_num_chunks();
        }

        // this function is needed to avoid a MSVC linker error
        [[nodiscard]] constexpr std::size_t current_pos() const noexcept
        {
            return base_type::current_pos();
        }

        void reset()
        {
            buffer_->reset();
            base_type::reset();
        }

        void flush() const
        {
            buffer_->flush();
        }

        template <typename T>
        HPX_FORCEINLINE void invoke(T const& t)
        {
            save(t);
        }

        template <typename T>
        HPX_FORCEINLINE void invoke_impl(T const& t)
        {
            save(t);
        }

        // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
        template <typename T>
        void save(T const& t)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALLOW_RAW_POINTER_SERIALIZATION)
            static_assert(!std::is_pointer_v<T>,
                "HPX does not support serialization of raw pointers. "
                "Please use smart pointers instead.");
#endif
            if constexpr (!std::is_integral_v<T> && !std::is_enum_v<T>)
            {
                if constexpr (traits::is_nonintrusive_polymorphic_v<T>)
                {
                    // non-bitwise polymorphic serialization
                    detail::polymorphic_nonintrusive_factory::instance().save(
                        *this, t);
                }
                else if constexpr (hpx::traits::is_serialization_supported<
                                       T>::has_serialize ||
                    access::has_serialize_v<T>)
                {
                    // non-bitwise normal serialization
                    access::serialize(*this, t, 0);
                }
                else if constexpr (hpx::traits::is_serialization_supported<
                                       T>::has_optimized)
                {
                    // bitwise serialization
                    static_assert(!std::is_abstract_v<T>,
                        "Can not bitwise serialize a class that is abstract");

#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
                    if (disable_array_optimization() || endianess_differs())
                    {
                        access::serialize(*this, t, 0);
                        return;
                    }
#else
                    HPX_ASSERT(
                        !(disable_array_optimization() || endianess_differs()));
#endif
                    save_binary(detail::array_of_fundamental_type_v<std::byte>,
                        &t, sizeof(t));
                }
                else if constexpr (hpx::traits::is_serialization_supported<
                                       T>::has_refl_serialize ||
                    hpx::traits::has_struct_serialization_v<T>)
                {
                    // struct serialization
                    access::serialize(*this, t, 0);
                }
                else
                {
                    static_assert(hpx::traits::is_serialization_supported_v<T>,
                        "hpx::traits::is_serialization_supported_v<T> must be "
                        "true");
                }
            }
            else if constexpr (std::is_integral_v<T>)
            {
                static_assert((std::is_unsigned_v<T> &&
                                  sizeof(T) <= sizeof(std::uint64_t)) ||
                        sizeof(T) <= sizeof(std::int64_t),
                    "integral type is larger than supported");

#if defined(HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS)
                if (HPX_UNLIKELY(endianess_differs()))
                {
                    T val = t;
                    reverse_bytes(sizeof(T), reinterpret_cast<char*>(&val));
                    save_binary(detail::fundamental_type_v<T>, &val, sizeof(T));
                }
                else
                {
                    save_binary(detail::fundamental_type_v<T>, &t, sizeof(T));
                }
#else
                save_binary(detail::fundamental_type_v<T>, &t, sizeof(T));
#endif
            }
            else
            {
                static_assert(std::is_enum_v<T>);
                using underlying_type = std::underlying_type_t<T>;

                auto const val = static_cast<underlying_type>(t);
                save_binary(detail::fundamental_type_v<underlying_type>, &val,
                    sizeof(underlying_type));
            }
        }

        void save(float const f)
        {
            save_binary(detail::fundamental_type_v<float>, &f, sizeof(float));
        }

        void save(double const d)
        {
            save_binary(detail::fundamental_type_v<double>, &d, sizeof(double));
        }

        void save(long double const d)
        {
            save_binary(detail::fundamental_type_v<long double>, &d,
                sizeof(long double));
        }

        void save(char const c)
        {
            save_binary(detail::fundamental_type_v<char>, &c, sizeof(char));
        }

        void save(signed char const c)
        {
            save_binary(detail::fundamental_type_v<signed char>, &c,
                sizeof(signed char));
        }

        void save(unsigned char const c)
        {
            save_binary(detail::fundamental_type_v<unsigned char>, &c,
                sizeof(unsigned char));
        }

        void save(bool const b)
        {
            HPX_ASSERT(0 == static_cast<int>(b) || 1 == static_cast<int>(b));
            save_binary(detail::fundamental_type_v<bool>, &b, sizeof(bool));
        }

#if defined(HPX_SERIALIZATION_HAVE_ALLOW_RAW_POINTER_SERIALIZATION)
        template <typename T>
        void save(T* p)
        {
            save_binary(detail::fundamental_type_v<std::size_t>, &p,
                sizeof(std::size_t));
        }
#endif
        // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

    private:
        friend struct basic_archive<output_archive>;

    public:
        void save_binary(detail::fundamental_types t, void const* address,
            std::size_t const count)
        {
            if (HPX_UNLIKELY(count == 0))
                return;

            if (HPX_UNLIKELY(!enable_type_checking()))
            {
                t = detail::fundamental_types::none;
            }

            size_ += buffer_->save_binary(t, address, count);
        }

        void save_binary_chunk(detail::fundamental_types t, void const* address,
            std::size_t const count)
        {
            if (HPX_UNLIKELY(count == 0))
                return;

            if (HPX_UNLIKELY(!enable_type_checking()))
            {
                t = detail::fundamental_types::none;
            }

            if (HPX_UNLIKELY(disable_data_chunking()))
            {
                size_ += buffer_->save_binary(t, address, count);
            }
            else
            {
                // the size might grow if optimizations are not used
                size_ += buffer_->save_binary_chunk(t, address, count);
            }
        }

    private:
        std::unique_ptr<erased_output_container> buffer_;
    };
}    // namespace hpx::serialization

#include <hpx/config/warnings_suffix.hpp>
