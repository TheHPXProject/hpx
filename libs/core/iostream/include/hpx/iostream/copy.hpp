//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Contains: The function template copy, which reads data from a Source
// and writes it to a Sink until the end of the sequence is reached, returning
// the number of characters transferred.

// The implementation is complicated by the need to handle smart adapters
// and direct devices.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/chain.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/detail/adapter/non_blocking_adapter.hpp>
#include <hpx/iostream/detail/buffer.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/functional.hpp>
#include <hpx/iostream/detail/resolve.hpp>
#include <hpx/iostream/detail/wrap_unwrap.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/pipeline.hpp>

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <type_traits>
#include <utility>

namespace hpx::iostream {

    namespace detail {

        // The following implementation of copy_impl() optimizes copying in the
        // case that one or both of the two devices models Direct (see
        // http://www.boost.org/libs/iostreams/doc/index.html?path=4.1.1.4)

        HPX_CXX_CORE_EXPORT template <typename Source, typename Sink>
        std::streamsize do_copy(Source&& src, Sink&& snk,
            [[maybe_unused]] std::streamsize buffer_size)
        {
            using source_t = std::decay_t<Source>;
            using sink_t = std::decay_t<Sink>;

            std::streamsize total = 0;

            if constexpr (is_direct_v<source_t> && is_direct_v<sink_t>)
            {
                // Copy from a direct source to a direct sink
                auto p1 = iostream::input_sequence(HPX_FORWARD(Source, src));
                auto p2 = iostream::output_sequence(HPX_FORWARD(Sink, snk));
                total = static_cast<std::streamsize>(
                    (std::min) (p1.size(), p2.size()));
                std::copy(p1.data(), p1.data() + total, p2.data());
            }
            else if constexpr (is_direct_v<source_t> && !is_direct_v<sink_t>)
            {
                // Copy from a direct source to an indirect sink
                auto p = iostream::input_sequence(HPX_FORWARD(Source, src));
                for (std::streamsize const size =
                         static_cast<std::streamsize>(p.size());
                    total < size;
                    /**/)
                {
                    std::streamsize const amt =
                        iostream::write(snk, p.data() + total, size - total);
                    total += amt;
                }
            }
            else if constexpr (!is_direct_v<source_t> && is_direct_v<sink_t>)
            {
                // Copy from an indirect source to a direct sink
                using char_type = char_type_of_t<source_t>;
                detail::basic_buffer<char_type> buf(buffer_size);

                auto p = snk.output_sequence();
                std::ptrdiff_t const capacity = p.size();
                while (true)
                {
                    std::streamsize const amt = iostream::read(src, buf.data(),
                        buffer_size < capacity - total ? buffer_size :
                                                         capacity - total);
                    if (amt == -1)
                        break;

                    std::copy(buf.data(), buf.data() + amt, p.data() + total);
                    total += amt;
                }
            }
            else
            {
                // !is_direct_v<source_t> && !is_direct_v<sink_t>

                // Copy from an indirect source to an indirect sink
                using char_type = char_type_of_t<source_t>;
                detail::basic_buffer<char_type> buf(buffer_size);

                non_blocking_adapter<sink_t> nb(snk);
                while (true)
                {
                    std::streamsize const amt =
                        iostream::read(src, buf.data(), buffer_size);
                    if (amt == -1)
                        break;

                    iostream::write(nb, buf.data(), amt);
                    total += amt;
                }
            }

            return total;
        }

        // Primary overload of copy_impl. Delegates to one of the above four
        // overloads of compl_impl(), depending on which of the two given
        // devices, if any, models Direct (see
        // http://www.boost.org/libs/iostreams/doc/index.html?path=4.1.1.4)
        HPX_CXX_CORE_EXPORT template <typename Source, typename Sink>
        std::streamsize copy_impl(
            Source src, Sink snk, std::streamsize buffer_size)
        {
            static_assert(
                std::is_same_v<char_type_of_t<Source>, char_type_of_t<Sink>>);

            return detail::execute_all(
                [&src, &snk, buffer_size]() mutable {
                    return do_copy(src, snk, buffer_size);
                },
                detail::call_close_all(src), detail::call_close_all(snk));
        }
    }    // End namespace detail.

    //------------------Definition of copy----------------------------------------//
    HPX_CXX_CORE_EXPORT template <typename Source, typename Sink>
    std::streamsize copy(Source&& src, Sink&& snk,
        std::streamsize buffer_size = default_device_buffer_size)
    {
        using source_t = std::decay_t<Source>;
        using sink_t = std::decay_t<Sink>;
        using char_type = char_type_of_t<source_t>;

        if constexpr (!is_std_io_v<source_t> && !is_std_io_v<sink_t>)
        {
            // Neither the source nor the sink is a standard stream or stream
            // buffer
            return detail::copy_impl(
                detail::resolve<input, char_type>(HPX_FORWARD(Source, src)),
                detail::resolve<output, char_type>(HPX_FORWARD(Sink, snk)),
                buffer_size);
        }
        else if constexpr (is_std_io_v<source_t> && !is_std_io_v<sink_t>)
        {
            // The source, but not the sink, is a standard stream or stream
            // buffer
            return detail::copy_impl(detail::wrap(HPX_FORWARD(Source, src)),
                detail::resolve<output, char_type>(HPX_FORWARD(Sink, snk)),
                buffer_size);
        }
        else if constexpr (!is_std_io_v<source_t> && is_std_io_v<sink_t>)
        {
            // The sink, but not the source, is a standard stream or stream
            // buffer
            return detail::copy_impl(
                detail::resolve<input, char_type>(HPX_FORWARD(Source, src)),
                detail::wrap(HPX_FORWARD(Sink, snk)), buffer_size);
        }
        else
        {
            // is_std_io_v<source_t> && is_std_io_v<sink_t>

            // Neither the source nor the sink is a standard stream or stream
            // buffer
            return detail::copy_impl(detail::wrap(HPX_FORWARD(Source, src)),
                detail::wrap(HPX_FORWARD(Sink, snk)), buffer_size);
        }
    }
}    // namespace hpx::iostream
