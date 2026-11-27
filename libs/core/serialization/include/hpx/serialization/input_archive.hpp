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
#include <hpx/modules/errors.hpp>
#include <hpx/serialization/access.hpp>
#include <hpx/serialization/basic_archive.hpp>
#include <hpx/serialization/detail/device_chunk_transport.hpp>
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp>
#include <hpx/serialization/detail/post_receive_actions.hpp>
#include <hpx/serialization/detail/raw_ptr.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/input_container.hpp>
#include <hpx/serialization/traits/is_serialization_supported.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT struct input_archive : basic_archive<input_archive>
    {
        using base_type = basic_archive<input_archive>;

        template <typename Container>
        explicit input_archive(Container& buffer,
            std::size_t inbound_data_size = 0,
            std::vector<serialization_chunk>* chunks = nullptr)
          : base_type(0U)
          , buffer_(new input_container<Container>(
                buffer, chunks, inbound_data_size))
        {
            // endianness needs to be saved separately as it is needed to
            // properly interpret the flags
            bool endianness = false;
            load_binary(
                detail::fundamental_types::none, &endianness, sizeof(bool));
            if (endianness)
            {
                flags_ = static_cast<std::uint32_t>(
                    hpx::serialization::archive_flags::endian_big);
            }

#if !defined(HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS)
            if ((endianness && (endian::native == endian::little)) ||
                (!endianness && (endian::native == endian::big)))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_request,
                    "hpx::serialization::input_archive::input_archive",
                    "Converting endianness is not supported by the "
                    "serialization library, please reconfigure HPX with "
                    "-DHPX_SERIALIZATION_WITH_SUPPORTS_ENDIANESS=On");
            }
#endif

            // type checking needs to be saved separately as it is needed to
            // properly interpret the flags
            bool type_checking = false;
            load_binary(
                detail::fundamental_types::none, &type_checking, sizeof(bool));
            if (type_checking)
            {
                flags_ = flags_ |
                    hpx::serialization::archive_flags::enable_type_checking;
            }

            // Load flags sent by the other end to make sure both ends have
            // the same assumptions about the archive format. It is safe to
            // overwrite the flags_ now.
            std::uint32_t flags = 0;
            load(flags);
            flags_ = flags;

            // load the zero-copy limit used by the other end
            std::uint64_t zero_copy_serialization_threshold = 0;
            bool has_zero_copy_serialization_threshold = false;
            load(has_zero_copy_serialization_threshold);

            if (has_zero_copy_serialization_threshold)
            {
                load(zero_copy_serialization_threshold);
            }

            buffer_->set_zero_copy_serialization_threshold(
                detail::clamp_to_size(zero_copy_serialization_threshold));

            bool has_filter = false;
            load(has_filter);

            if (has_filter && enable_compression())
            {
                serialization::binary_filter* filter = nullptr;
                *this >> detail::raw_ptr(filter);
                buffer_->set_filter(filter);
                filtered_ = true;
            }
        }

        template <typename T>
        HPX_FORCEINLINE void invoke(T& t)
        {
            load(t);
        }

        template <typename T>
        HPX_FORCEINLINE void invoke_impl(T& t)
        {
            load(t);
        }

        // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
        template <typename T>
        void load(T& t)
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
                    detail::polymorphic_nonintrusive_factory::instance().load(
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
                    load_binary(detail::array_of_fundamental_type_v<std::byte>,
                        &t, sizeof(t));
                }
                else if constexpr (hpx::traits::is_serialization_supported<
                                       T>::has_refl_serialize ||
                    hpx::traits::has_struct_serialization_v<T>)
                {
                    // struct serialization or reflection-based serialization
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
                    T val;
                    load_binary(detail::fundamental_type_v<T>, &val, sizeof(T));
                    reverse_bytes(sizeof(T), reinterpret_cast<char*>(&val));
                    t = val;
                }
                else
                {
                    load_binary(detail::fundamental_type_v<T>, &t, sizeof(T));
                }
#else
                load_binary(detail::fundamental_type_v<T>, &t, sizeof(T));
#endif
            }
            else
            {
                static_assert(std::is_enum_v<T>);
                using underlying_type = std::underlying_type_t<T>;

                underlying_type val;
                load_binary(detail::fundamental_type_v<underlying_type>, &val,
                    sizeof(underlying_type));
                t = static_cast<T>(val);
            }
        }

        void load(float& f)
        {
            load_binary(detail::fundamental_type_v<float>, &f, sizeof(float));
        }

        void load(double& d)
        {
            load_binary(detail::fundamental_type_v<double>, &d, sizeof(double));
        }

        void load(long double& d)
        {
            load_binary(detail::fundamental_type_v<long double>, &d,
                sizeof(long double));
        }

        void load(char& c)
        {
            load_binary(detail::fundamental_type_v<char>, &c, sizeof(char));
        }

        void load(signed char& c)
        {
            load_binary(detail::fundamental_type_v<signed char>, &c,
                sizeof(signed char));
        }

        void load(unsigned char& c)
        {
            load_binary(detail::fundamental_type_v<unsigned char>, &c,
                sizeof(unsigned char));
        }

        void load(bool& b)
        {
            load_binary(detail::fundamental_type_v<bool>, &b, sizeof(bool));
            HPX_ASSERT(0 == static_cast<int>(b) || 1 == static_cast<int>(b));
        }

#if defined(HPX_SERIALIZATION_HAVE_ALLOW_RAW_POINTER_SERIALIZATION)
        template <typename T>
        void load(T*& p)
        {
            load_binary(detail::fundamental_type_v<std::size_t>, &p,
                sizeof(std::size_t));
        }
#endif
        // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

        // data below this size was copied into the archive, larger data may
        // come in a zero-copy chunk
        [[nodiscard]] std::size_t get_zero_copy_serialization_threshold()
            const noexcept
        {
            return buffer_->get_zero_copy_serialization_threshold();
        }

        [[nodiscard]] constexpr std::size_t bytes_read() const noexcept
        {
            return current_pos();
        }

        // Whether the data comes through a filter (e.g. decompression): the
        // filter writes all of it on the host, also the data of zero-copy
        // chunks.
        [[nodiscard]] constexpr bool is_filtered() const noexcept
        {
            return filtered_;
        }

        // this function is needed to avoid a MSVC linker error
        [[nodiscard]] constexpr std::size_t current_pos() const noexcept
        {
            return base_type::current_pos();
        }

    private:
        friend struct basic_archive<input_archive>;

    public:
        void load_binary(detail::fundamental_types expected, void* address,
            std::size_t const count)
        {
            if (HPX_UNLIKELY(count == 0))
                return;

            if (HPX_UNLIKELY(!enable_type_checking()))
            {
                expected = detail::fundamental_types::none;
            }

            size_ += buffer_->load_binary(expected, address, count);
        }

        /// Reads \a count bytes written by save_binary_chunk into the memory
        /// at \a address.
        ///
        /// With \a allow_zero_copy_receive (and an archive whose chunks the
        /// networking layer fills in after de-serialization, see
        /// serialization::detail::allow_zero_copy_receive), nothing is copied
        /// here: the chunk is pointed at \a address, and the networking layer
        /// places the data there only after this function (and the load()
        /// that calls it) has returned. The memory at \a address must stay
        /// valid until then and must not be read before. A load() that uses
        /// the data (or that can't receive it into its final place, e.g.
        /// device memory) receives it into a staging buffer instead and
        /// registers the step that completes the object with
        /// serialization::detail::post_receive_actions: a parcel runs these
        /// steps once its data has arrived, and its action only after the
        /// last of them has finished. Without \a allow_zero_copy_receive (and
        /// for data below the zero-copy threshold, which isn't sent as a
        /// chunk) the data is complete when this function returns.
        ///
        /// \param expected The type of the data, checked by archives with
        ///                 type checking.
        /// \param address  Where the data goes.
        /// \param count    The number of bytes.
        /// \param allow_zero_copy_receive Whether the data may arrive at
        ///                 \a address after this function has returned.
        /// \param memory   Whether \a address refers to device memory: the
        ///                 networking layer receives into it then (see
        ///                 serialization_chunk::is_device_memory).
        void load_binary_chunk(detail::fundamental_types expected,
            void* address, std::size_t const count,
            bool const allow_zero_copy_receive,
            chunk_memory const memory = chunk_memory::host)
        {
            if (HPX_UNLIKELY(count == 0))
                return;

            if (HPX_UNLIKELY(!enable_type_checking()))
            {
                expected = detail::fundamental_types::none;
            }

            if (HPX_UNLIKELY(enable_device_transport()) &&
                load_with_device_transport(
                    address, count, allow_zero_copy_receive))
            {
                return;
            }

            if (HPX_UNLIKELY(disable_data_chunking()))
            {
                // device memory must not be written on the host
                HPX_ASSERT(memory == chunk_memory::host);
                size_ += buffer_->load_binary(expected, address, count);
            }
            else
            {
                size_ += buffer_->load_binary_chunk(expected, address, count,
                    allow_zero_copy_receive && !disable_receive_data_chunking(),
                    memory);
            }
        }

        /// Whether the data of the next zero-copy chunk arrives through the
        /// archive's device transport (see
        /// detail::device_chunk_transport): then it is received into the
        /// memory handed to load_binary_chunk, device or host memory, after
        /// the de-serialization (as with zero-copy receive), also if the
        /// parcelport doesn't receive zero-copy chunks. An object asks this
        /// before choosing where its data goes, it then calls
        /// load_binary_chunk with allow_zero_copy_receive.
        [[nodiscard]] bool next_chunk_from_transport()
        {
            if (!enable_device_transport())
            {
                return false;
            }
            if (next_marker_ < 0)
            {
                std::uint8_t marker = 0;
                load(marker);
                next_marker_ = marker;
            }
            return next_marker_ == 1;
        }

    private:
        // The counterpart of output_archive::save_with_device_transport:
        // reads the marker of the chunk and, if the sender's transport moves
        // its data, receives it through this side's transport as a transfer
        // step of the parcel (see post_receive_actions::add_transfer).
        // Returns whether the transport moves the data.
        // The chunk may be received into device memory or into a host
        // staging buffer, the transfer runs before the other steps (e.g. the
        // copy from the staging buffer).
        bool load_with_device_transport(void* address, std::size_t const count,
            bool const allow_zero_copy_receive)
        {
            std::uint8_t use_transport = 0;
            if (next_marker_ >= 0)
            {
                // read by next_chunk_from_transport
                use_transport = static_cast<std::uint8_t>(next_marker_);
                next_marker_ = -1;
            }
            else
            {
                load(use_transport);
            }
            if (use_transport == 0)
            {
                return false;
            }

            detail::device_chunk_descriptor descriptor;
            load(descriptor.source);
            load(descriptor.tag);

            auto const* transport =
                try_get_extra_data<detail::device_transport>();
            if (transport == nullptr || !transport->transport)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "input_archive::load_binary_chunk",
                    "the sender moves device memory through a device "
                    "transport, but none is configured here (see "
                    "hpx.parcel.device_conduit)");
            }

            // The data arrives after the de-serialization (the parcel's action
            // runs after the transfer), as with zero-copy receive: the object
            // loading it must not use it before.
            if (!allow_zero_copy_receive)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "input_archive::load_binary_chunk",
                    "a chunk sent through a device transport can be "
                    "received only by an object that doesn't use the data "
                    "while it is de-serialized");
            }

            get_extra_data<detail::post_receive_actions>().add_transfer(
                [transport = transport->transport, descriptor, address, count](
                    detail::post_receive_actions::done_type done) {
                    transport->recv(descriptor, address, count,
                        [done = HPX_MOVE(done)](std::exception_ptr error) {
                            done(HPX_MOVE(error));
                        });
                });
            return true;
        }

        std::unique_ptr<erased_input_container> buffer_;
        bool filtered_ = false;

        // the marker of the next chunk if next_chunk_from_transport has read
        // it, else -1
        int next_marker_ = -1;
    };
}    // namespace hpx::serialization

#include <hpx/config/warnings_suffix.hpp>
