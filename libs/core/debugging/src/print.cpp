//  Copyright (c) 2019-2020 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/debugging/environ.hpp>
#include <hpx/debugging/print.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__FreeBSD__)
HPX_CORE_EXPORT char** freebsd_environ = nullptr;
#endif
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif

// Used to wrap function call parameters to prevent evaluation
// when debugging is disabled
#define HPX_DP_LAZY(Expr, printer) printer.eval([&] { return Expr; })

// ------------------------------------------------------------
/// \cond NODETAIL
namespace hpx::debug {

    // ------------------------------------------------------------------
    // format as zero padded int
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        void print_dec(std::ostream& os, Int const& v, int N)
        {
            os << std::right << std::setfill('0') << std::setw(N)
               << std::noshowbase << std::dec << v;
        }

        template void print_dec(std::ostream&, std::int16_t const&, int);
        template void print_dec(std::ostream&, std::uint16_t const&, int);
        template void print_dec(std::ostream&, std::int32_t const&, int);
        template void print_dec(std::ostream&, std::uint32_t const&, int);
        template void print_dec(std::ostream&, std::int64_t const&, int);
        template void print_dec(std::ostream&, std::uint64_t const&, int);
#if defined(__APPLE__)
        template void print_dec(std::ostream&, long const&, int);
        template void print_dec(std::ostream&, unsigned long const&, int);
#endif

        template void print_dec(std::ostream&, std::atomic<int> const&, int);
        template void print_dec(
            std::ostream&, std::atomic<unsigned int> const&, int);
    }    // namespace detail

    // ------------------------------------------------------------------
    // format as pointer
    // ------------------------------------------------------------------
    ptr::ptr(void const* v) noexcept
      : data_(v)
    {
    }

    ptr::ptr(std::uintptr_t v) noexcept
      : data_(reinterpret_cast<void const*>(v))
    {
    }

    std::ostream& operator<<(std::ostream& os, ptr const& d)
    {
        os << d.data_;
        return os;
    }

    // ------------------------------------------------------------------
    // format as zero padded hex
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        void print_hex(std::ostream& os, Int v, int N)
        {
            os << std::right << "0x" << std::setfill('0') << std::setw(N)
               << std::noshowbase << std::hex << v;
        }

        template HPX_CORE_EXPORT void print_hex(
            std::ostream&, std::thread::id, int);
        template HPX_CORE_EXPORT void print_hex(
            std::ostream&, unsigned long, int);
        template HPX_CORE_EXPORT void print_hex(std::ostream&, int, int);

        void print_ptr(std::ostream& os, void const* v, int N)
        {
            os << std::right << std::setw(N) << std::noshowbase << std::hex
               << v;
        }

    }    // namespace detail

    // ------------------------------------------------------------------
    // format as binary bits
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        HPX_CORE_EXPORT void print_bin(std::ostream& os, Int v, int N)
        {
            char const* beg = reinterpret_cast<char const*>(&v);
            char const* end = beg + sizeof(v);

            N = (N + CHAR_BIT - 1) / CHAR_BIT;
            while (beg != end && N-- > 0)
            {
                os << std::bitset<CHAR_BIT>(*beg++);
            }
        }

        template HPX_CORE_EXPORT void print_bin(
            std::ostream&, std::uint64_t, int);

#if defined(__APPLE__)
        // Explicit instantiation necessary to solve undefined symbol for MacOS
        template HPX_CORE_EXPORT void print_bin(
            std::ostream&, unsigned long, int);
#endif
    }    // namespace detail

    // ------------------------------------------------------------------
    // format as padded string
    // ------------------------------------------------------------------
    namespace detail {

        void print_str(std::ostream& os, char const* v, int N)
        {
            os << std::left << std::setfill(' ') << std::setw(N) << v;
        }
    }    // namespace detail

    // ------------------------------------------------------------------
    // format as ip address
    // ------------------------------------------------------------------
    ipaddr::ipaddr(void const* a) noexcept
      : data_(static_cast<std::uint8_t const*>(a))
      , ipdata_(0)
    {
    }

    ipaddr::ipaddr(std::uint32_t a) noexcept
      : data_(reinterpret_cast<uint8_t const*>(&ipdata_))    //-V206
      , ipdata_(a)
    {
    }

    std::ostream& operator<<(std::ostream& os, ipaddr const& p)
    {
        os << std::dec << static_cast<int>(p.data_[0]) << "."
           << static_cast<int>(p.data_[1]) << "."
           << static_cast<int>(p.data_[2]) << "."
           << static_cast<int>(p.data_[3]);
        return os;
    }

    // ------------------------------------------------------------------
    // helper class for printing time since start
    // ------------------------------------------------------------------
    namespace detail {

        std::ostream& operator<<(std::ostream& os, current_time_print_helper)
        {
            static std::chrono::steady_clock::time_point log_t_start =
                std::chrono::steady_clock::now();

            auto const now = std::chrono::steady_clock::now();    //-V656
            auto const nowt =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    now - log_t_start)
                    .count();

            os << debug::dec<10>(nowt) << " ";
            return os;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        std::function<void(std::ostream&)> print_info;

        void register_print_info(void (*printer)(std::ostream&))
        {
            print_info = printer;
        }

        void generate_prefix(std::ostream& os)
        {
            os << detail::current_time_print_helper();
            if (print_info)
            {
                print_info(os);
            }
            os << detail::hostname_print_helper();
        }

        void display_to_cout(std::string const& str)
        {
            std::cout << str;
        }
    }    // namespace detail

    // ------------------------------------------------------------------
    // helper function for printing short memory dump and crc32
    // useful for debugging corruptions in buffers during
    // rma or other transfers
    // ------------------------------------------------------------------
    mem_crc32::mem_crc32(
        void const* a, std::size_t len, char const* txt) noexcept
      : addr_(static_cast<std::uint64_t const*>(a))    //-V206
      , len_(len)
      , txt_(txt)
    {
    }

    std::ostream& operator<<(std::ostream& os, mem_crc32 const& p)
    {
        auto uintBuf = static_cast<std::uint64_t const*>(p.addr_);
        os << "Memory:";
        os << " address " << hpx::debug::ptr(p.addr_) << " length "
           << hpx::debug::hex<6>(p.len_)
           << " CRC32:" << hpx::debug::hex<8>(crc32(p.addr_, p.len_)) << "\n";

        auto const max_value =
            (std::min) (static_cast<std::size_t>(
                            std::ceil(static_cast<double>(p.len_) / 8.0)),
                static_cast<std::size_t>(128));
        for (std::size_t i = 0; i < max_value; i++)
        {
            os << hpx::debug::hex<16>(*uintBuf++) << " ";
        }
        os << " : " << p.txt_;
        return os;
    }

    namespace detail {

        // ------------------------------------------------------------------
        // helper class for printing time since start
        // ------------------------------------------------------------------
        [[nodiscard]] char const* hostname_print_helper::get_hostname() const
        {
            static bool initialized = false;
            static char hostname_[32] = {'\0'};
            if (!initialized)
            {
                initialized = true;
#if !defined(__FreeBSD__)
                gethostname(hostname_, static_cast<std::size_t>(12));
#endif
                int const rank = guess_rank();
                if (rank >= 0)
                {
                    std::size_t const len = std::strlen(hostname_);
                    std::snprintf(
                        hostname_ + len, sizeof(hostname_) - len, "(%d)", rank);
                }
            }
            return hostname_;
        }

        [[nodiscard]] int hostname_print_helper::guess_rank() const
        {
#if defined(__FreeBSD__)
            char** env = freebsd_environ;
#else
            char** env = environ;
#endif
            std::vector<std::string> const env_strings{//-V826
                "_RANK=", "_NODEID="};
            for (char** current = env; *current; ++current)
            {
                auto e = std::string(*current);
                for (auto const& s : env_strings)
                {
                    auto const pos = e.find(s);
                    if (pos != std::string::npos)
                    {
                        return std::stoi(e.substr(pos + s.size(), 5));
                    }
                }
            }
            return -1;
        }

        std::ostream& operator<<(std::ostream& os, hostname_print_helper h)
        {
            os << debug::str<13>(h.get_hostname()) << " ";
            return os;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        HPX_CORE_EXPORT void print_array(
            std::string const& name, T const* data, std::size_t size)
        {
            std::cout << str<20>(name.c_str()) << ": {" << debug::dec<4>(size)
                      << "} : ";
            std::copy(
                data, data + size, std::ostream_iterator<T>(std::cout, ", "));
            std::cout << "\n";
        }

        template HPX_CORE_EXPORT void print_array(
            std::string const&, std::uint64_t const*, std::size_t);
    }    // namespace detail
}    // namespace hpx::debug
/// \endcond
