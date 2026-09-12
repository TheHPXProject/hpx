//  Copyright (c) 2019-2020 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// ------------------------------------------------------------
// This file provides a simple to use printf style debugging
// tool that can be used on a per-file basis to enable output.
// It is not intended to be exposed to users, but rather as
// an aid for hpx development.
// ------------------------------------------------------------
// Usage: Instantiate a debug print object at the top of a file
// using a template param of true/false to enable/disable output.
// When the template parameter is false, the optimizer will
// not produce code and so the impact is nil.
//
// static hpx::debug::enable_print<true> spq_deb("SUBJECT");
//
// Later in code you may print information using
//
//             spq_deb.debug(str<16>("cleanup_terminated"), "v1"
//                  , "D" , dec<2>(domain_num)
//                  , "Q" , dec<3>(q_index)
//                  , "thread_num", dec<3>(local_num));
//
// various print formatters (dec/hex/str) are supplied to make
// the output regular and aligned for easy parsing/scanning.
//
// In tight loops, huge amounts of debug information might be
// produced, so a simple timer based output is provided
// To instantiate a timed output
//      static auto getnext = spq_deb.make_timer(1
//              , str<16>("get_next_thread"));
// then inside a tight loop
//      spq_deb.timed(getnext, dec<>(thread_num));
// The output will only be produced every N seconds
// ------------------------------------------------------------

// ------------------------------------------------------------
/// \cond NODETAIL
namespace hpx::debug {

    // ------------------------------------------------------------------
    // format as zero padded int
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        HPX_CORE_EXPORT void print_dec(std::ostream& os, Int const& v, int n);

        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::int16_t const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::uint16_t const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::int32_t const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::uint32_t const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::int64_t const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::uint64_t const&, int);
#if defined(__APPLE__)
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, long const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, unsigned long const&, int);
#endif

        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::atomic<int> const&, int);
        extern template HPX_CORE_EXPORT void print_dec(
            std::ostream&, std::atomic<unsigned int> const&, int);

        template <int N, typename T>
        struct dec
        {
            explicit constexpr dec(T const& v) noexcept
              : data_(v)
            {
            }

            T const& data_;

            friend std::ostream& operator<<(
                std::ostream& os, dec<N, T> const& d)
            {
                detail::print_dec(os, d.data_, N);
                return os;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <int N = 2, typename T>
    [[nodiscard]] constexpr detail::dec<N, T> dec(T const& v) noexcept
    {
        return detail::dec<N, T>(v);
    }

    // ------------------------------------------------------------------
    // format as pointer
    // ------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct ptr
    {
        HPX_CORE_EXPORT explicit ptr(void const* v) noexcept;
        HPX_CORE_EXPORT explicit ptr(std::uintptr_t v) noexcept;

        void const* data_;

        HPX_CORE_EXPORT friend std::ostream& operator<<(
            std::ostream& os, ptr const& d);
    };

    // ------------------------------------------------------------------
    // format as zero padded hex
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        HPX_CORE_EXPORT void print_hex(std::ostream& os, Int v, int n);

        template <int N = 4, typename T = int, typename Enable = void>
        struct hex;

        template <int N, typename T>
        struct hex<N, T, std::enable_if_t<!std::is_pointer_v<T>>>
        {
            explicit constexpr hex(T const& v) noexcept
              : data_(v)
            {
            }

            T const& data_;

            friend std::ostream& operator<<(
                std::ostream& os, hex<N, T> const& d)
            {
                detail::print_hex(os, d.data_, N);
                return os;
            }
        };

        HPX_CORE_EXPORT void print_ptr(std::ostream& os, void const* v, int n);

        template <int N, typename T>
        struct hex<N, T, std::enable_if_t<std::is_pointer_v<T>>>
        {
            explicit constexpr hex(T const& v) noexcept
              : data_(v)
            {
            }

            T const& data_;

            friend std::ostream& operator<<(
                std::ostream& os, hex<N, T> const& d)
            {
                detail::print_ptr(os, d.data_, N);
                return os;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <int N = 4, typename T>
    [[nodiscard]] constexpr detail::hex<N, T> hex(T const& v) noexcept
    {
        return detail::hex<N, T>(v);
    }

    // ------------------------------------------------------------------
    // format as binary bits
    // ------------------------------------------------------------------
    namespace detail {

        template <typename Int>
        HPX_CORE_EXPORT void print_bin(std::ostream& os, Int v, int n);

        template <int N = 8, typename T = int>
        struct bin
        {
            explicit constexpr bin(T const& v) noexcept
              : data_(v)
            {
            }

            T const& data_;

            friend std::ostream& operator<<(
                std::ostream& os, bin<N, T> const& d)
            {
                detail::print_bin(os, d.data_, N);
                return os;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <int N = 8, typename T>
    [[nodiscard]] constexpr detail::bin<N, T> bin(T const& v) noexcept
    {
        return detail::bin<N, T>(v);
    }

    // ------------------------------------------------------------------
    // format as padded string
    // ------------------------------------------------------------------
    namespace detail {

        HPX_CORE_EXPORT void print_str(std::ostream& os, char const* v, int n);
    }

    HPX_CXX_CORE_EXPORT template <int N = 20>
    struct str
    {
        explicit constexpr str(char const* v) noexcept
          : data_(v)
        {
        }

        char const* data_;

        friend std::ostream& operator<<(std::ostream& os, str<N> const& d)
        {
            detail::print_str(os, d.data_, N);
            return os;
        }
    };

    // ------------------------------------------------------------------
    // format as ip address
    // ------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct ipaddr
    {
        HPX_CORE_EXPORT explicit ipaddr(void const* a) noexcept;
        HPX_CORE_EXPORT explicit ipaddr(std::uint32_t a) noexcept;

        std::uint8_t const* data_;
        std::uint32_t const ipdata_;

        HPX_CORE_EXPORT friend std::ostream& operator<<(
            std::ostream& os, ipaddr const& a);
    };

    // ------------------------------------------------------------------
    // helper class for printing time since start
    // ------------------------------------------------------------------
    namespace detail {

        struct current_time_print_helper
        {
            HPX_CORE_EXPORT friend std::ostream& operator<<(
                std::ostream& os, current_time_print_helper);
        };
    }    // namespace detail

    // ------------------------------------------------------------------
    // helper function for printing CRC32
    // ------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr std::uint32_t crc32(
        void const*, std::size_t) noexcept
    {
        return 0;
    }

    // ------------------------------------------------------------------
    // helper function for printing short memory dump and crc32
    // useful for debugging corruptions in buffers during
    // rma or other transfers
    // ------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct mem_crc32
    {
        HPX_CORE_EXPORT mem_crc32(
            void const* a, std::size_t len, char const* txt) noexcept;

        void const* addr_;
        std::size_t const len_;
        char const* txt_;

        HPX_CORE_EXPORT friend std::ostream& operator<<(
            std::ostream& os, mem_crc32 const& a);
    };

    namespace detail {

        template <typename TupleType, std::size_t... I>
        void tuple_print(
            std::ostream& os, TupleType const& t, std::index_sequence<I...>)
        {
            (..., (os << (I == 0 ? "" : " ") << std::get<I>(t)));
        }

        template <typename... Args>
        void tuple_print(std::ostream& os, std::tuple<Args...> const& t)
        {
            tuple_print(os, t, std::make_index_sequence<sizeof...(Args)>());
        }
    }    // namespace detail

    namespace detail {

        // ------------------------------------------------------------------
        // helper class for printing time since start
        // ------------------------------------------------------------------
        struct hostname_print_helper
        {
            [[nodiscard]] HPX_CORE_EXPORT char const* get_hostname() const;
            [[nodiscard]] HPX_CORE_EXPORT int guess_rank() const noexcept;

            HPX_CORE_EXPORT friend std::ostream& operator<<(
                std::ostream& os, hostname_print_helper h);
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CORE_EXPORT void register_print_info(void (*)(std::ostream&));
        HPX_CORE_EXPORT void generate_prefix(std::ostream& os);

        ///////////////////////////////////////////////////////////////////////
        HPX_CORE_EXPORT void display_to_cout(std::string const& str);

        template <typename... Args>
        void display(char const* prefix, Args const&... args)
        {
            // using a temp stream object with a single copy to cout at the end
            // prevents multiple threads from injecting overlapping text
            std::stringstream tempstream;
            tempstream << prefix;
            generate_prefix(tempstream);
            ((tempstream << args << " "), ...);
            tempstream << std::endl;
            display_to_cout(tempstream.str());
        }

        template <typename... Args>
        void debug(Args const&... args)
        {
            display("<DEB> ", args...);
        }

        template <typename... Args>
        void warning(Args const&... args)
        {
            display("<WAR> ", args...);
        }

        template <typename... Args>
        void error(Args const&... args)
        {
            display("<ERR> ", args...);
        }

        template <typename... Args>
        void scope(Args const&... args)
        {
            display("<SCO> ", args...);
        }

        template <typename... Args>
        void trace(Args const&... args)
        {
            display("<TRC> ", args...);
        }

        template <typename... Args>
        void timed(Args const&... args)
        {
            display("<TIM> ", args...);
        }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename... Args>
    struct scoped_var
    {
        // capture tuple elements by reference - no temp vars in constructor please
        char const* prefix_;
        std::tuple<Args const&...> const message_;
        std::string buffered_msg;

        //
        explicit scoped_var(char const* p, Args const&... args)
          : prefix_(p)
          , message_(args...)
        {
            std::stringstream tempstream;
            detail::tuple_print(tempstream, message_);
            buffered_msg = tempstream.str();
            detail::display("<SCO> ", prefix_, debug::str<>(">> enter <<"),
                tempstream.str());
        }

        ~scoped_var()
        {
            detail::display(
                "<SCO> ", prefix_, debug::str<>("<< leave >>"), buffered_msg);
        }

        scoped_var(scoped_var const&) = delete;
        scoped_var(scoped_var&&) = delete;
        scoped_var& operator=(scoped_var const&) = delete;
        scoped_var& operator=(scoped_var&&) = delete;
    };

    HPX_CXX_CORE_EXPORT template <typename... Args>
    struct timed_var
    {
        mutable std::chrono::steady_clock::time_point time_start_;
        double const delay_;
        std::tuple<Args...> const message_;
        //
        explicit timed_var(double delay, Args const&... args)
          : time_start_(std::chrono::steady_clock::now())
          , delay_(delay)
          , message_(args...)
        {
        }

        bool elapsed(std::chrono::steady_clock::time_point const& now) const
        {
            double const elapsed_ =
                std::chrono::duration_cast<std::chrono::duration<double>>(
                    now - time_start_)
                    .count();

            if (elapsed_ > delay_)
            {
                time_start_ = now;
                return true;
            }
            return false;
        }

        friend std::ostream& operator<<(
            std::ostream& os, timed_var<Args...> const& ti)
        {
            detail::tuple_print(os, ti.message_);
            return os;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <bool Enable>
    struct enable_print;

    // when false, debug statements should produce no code
    template <>
    struct enable_print<false>
    {
        explicit constexpr enable_print(char const*) noexcept {}

        [[nodiscard]] static constexpr bool is_enabled() noexcept
        {
            return false;
        }

        template <typename... Args>
        static constexpr void debug(Args const&...) noexcept
        {
        }

        template <typename... Args>
        static constexpr void warning(Args const&...) noexcept
        {
        }

        template <typename... Args>
        static constexpr void trace(Args const&...) noexcept
        {
        }

        template <typename... Args>
        static constexpr void error(Args const&...) noexcept
        {
        }

        template <typename... Args>
        static constexpr void timed(Args const&...) noexcept
        {
        }

        template <typename T>
        static constexpr void array(
            std::string const&, std::vector<T> const&) noexcept
        {
        }

        template <typename T, std::size_t N>
        static constexpr void array(
            std::string const&, std::array<T, N> const&) noexcept
        {
        }

        template <typename T>
        static constexpr void array(
            std::string const&, T const*, std::size_t) noexcept
        {
        }

        template <typename... Args>
        static constexpr bool scope(Args const&...) noexcept
        {
            return true;
        }

        template <typename T, typename... Args>
        [[nodiscard]] static constexpr bool declare_variable(
            Args const&...) noexcept
        {
            return true;
        }

        template <typename T, typename V>
        static constexpr void set(T&, V const&) noexcept
        {
        }

        // @todo, return void so that timers have zero footprint when disabled
        template <typename... Args>
        [[nodiscard]] static constexpr int make_timer(
            double const, Args const&...) noexcept
        {
            return 0;
        }

        template <typename Expr>
        [[nodiscard]] static constexpr bool eval(Expr const&) noexcept
        {
            return true;
        }
    };

    namespace detail {

        template <typename T>
        HPX_CORE_EXPORT void print_array(
            std::string const& name, T const* data, std::size_t size);
    }

    // when true, debug statements produce valid output
    template <>
    struct enable_print<true>
    {
    private:
        char const* prefix_;

    public:
        constexpr enable_print() noexcept
          : prefix_("")
        {
        }

        explicit constexpr enable_print(char const* p) noexcept
          : prefix_(p)
        {
        }

        [[nodiscard]] static constexpr bool is_enabled() noexcept
        {
            return true;
        }

        template <typename... Args>
        constexpr void debug(Args const&... args) const
        {
            detail::debug(prefix_, args...);
        }

        template <typename... Args>
        constexpr void warning(Args const&... args) const
        {
            detail::warning(prefix_, args...);
        }

        template <typename... Args>
        constexpr void trace(Args const&... args) const
        {
            detail::trace(prefix_, args...);
        }

        template <typename... Args>
        constexpr void error(Args const&... args) const
        {
            detail::error(prefix_, args...);
        }

        template <typename... Args>
        [[nodiscard]] scoped_var<Args...> scope(Args const&... args)
        {
            return scoped_var<Args...>(prefix_, args...);
        }

        template <typename... T, typename... Args>
        void timed(timed_var<T...> const& init, Args const&... args) const
        {
            if (auto now = std::chrono::steady_clock::now(); init.elapsed(now))
            {
                detail::timed(prefix_, init, args...);
            }
        }

        template <typename T>
        static void array(std::string const& name, std::vector<T> const& v)
        {
            detail::print_array(name, v.data(), v.size());
        }

        template <typename T, std::size_t N>
        static void array(std::string const& name, std::array<T, N> const& v)
        {
            detail::print_array(name, v.data(), N);
        }

        template <typename T>
        static void array(
            std::string const& name, T const* data, std::size_t size)
        {
            detail::print_array(name, data, size);
        }

        template <typename T, typename... Args>
        [[nodiscard]] static T declare_variable(Args const&... args)
        {
            return T(args...);
        }

        template <typename T, typename V>
        static void set(T& var, V const& val)
        {
            var = val;
        }

        template <typename... Args>
        [[nodiscard]] static timed_var<Args...> make_timer(
            double const delay, Args const... args)
        {
            return timed_var<Args...>(delay, args...);
        }

        template <typename Expr>
        [[nodiscard]] static auto eval(Expr const& e)
        {
            return e();
        }
    };
}    // namespace hpx::debug
/// \endcond
