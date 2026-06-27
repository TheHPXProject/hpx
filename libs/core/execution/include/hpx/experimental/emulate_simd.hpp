//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EMULATION)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <ostream>
#include <type_traits>
#include <utility>

namespace hpx::datapar::experimental {

    namespace detail {

        inline constexpr int max_vector_pack_size = 32;

        template <typename Numeric>
        struct simd_impl
        {
            using T = Numeric;
            static constexpr std::size_t size =
                max_vector_pack_size / sizeof(Numeric);
        };
    }    // namespace detail

    namespace simd_abi {

        struct scalar
        {
        };
        struct simd_emulation_abi
        {
        };

        template <typename T>
        inline constexpr int max_fixed_size = detail::max_vector_pack_size;

        template <typename T>
        using compatible = simd_emulation_abi;

        template <typename T>
        using native = simd_emulation_abi;

        template <typename T, size_t N>
        using fixed_size = simd_emulation_abi;
    }    // namespace simd_abi

    HPX_CXX_CORE_EXPORT struct element_aligned_tag
    {
    };
    HPX_CXX_CORE_EXPORT struct vector_aligned_tag
    {
    };
    HPX_CXX_CORE_EXPORT template <size_t>
    struct overaligned_tag
    {
    };

    HPX_CXX_CORE_EXPORT inline constexpr element_aligned_tag element_aligned{};
    HPX_CXX_CORE_EXPORT inline constexpr vector_aligned_tag vector_aligned{};

    HPX_CXX_CORE_EXPORT template <size_t N>
    inline constexpr overaligned_tag<N> overaligned{};

    // ----------------------------------------------------------------------
    // traits [simd.traits]
    // ----------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_abi_tag : std::is_same<T, simd_abi::simd_emulation_abi>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_abi_tag_v = is_abi_tag<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_simd;
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_simd_v = is_simd<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_simd_mask;
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_simd_mask_v = is_simd_mask<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_simd_flag_type : std::false_type
    {
    };
    template <>
    struct is_simd_flag_type<element_aligned_tag> : std::true_type
    {
    };
    template <>
    struct is_simd_flag_type<vector_aligned_tag> : std::true_type
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_simd_flag_type_v = is_simd_flag_type<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Abi = simd_abi::compatible<T>>
    struct simd_size
    {
        static constexpr size_t value = detail::simd_impl<T>::size;
    };
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Abi = simd_abi::compatible<T>>
    inline constexpr size_t simd_size_v = simd_size<T, Abi>::value;

    HPX_CXX_CORE_EXPORT template <typename T, class U = T::value_type>
    struct memory_alignment
    {
        static constexpr size_t value = detail::max_vector_pack_size;
    };
    HPX_CXX_CORE_EXPORT template <typename T, class U = T::value_type>
    inline constexpr size_t memory_alignment_v = memory_alignment<T, U>::value;

    // ----------------------------------------------------------------------
    // class template simd [simd.class]
    // ----------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Abi = simd_abi::compatible<T>>
    class simd;
    HPX_CXX_CORE_EXPORT template <typename T>
    using native_simd = simd<T, simd_abi::native<T>>;
    HPX_CXX_CORE_EXPORT template <typename T, size_t N>
    using fixed_size_simd = simd<T, simd_abi::fixed_size<T, N>>;

    // ----------------------------------------------------------------------
    // class template simd_mask [simd.mask.class]
    // ----------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Abi = simd_abi::compatible<T>>
    class simd_mask;
    HPX_CXX_CORE_EXPORT template <typename T>
    using native_simd_mask = simd_mask<T, simd_abi::native<T>>;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_simd : std::false_type
    {
    };

    template <typename T, typename Abi>
    struct is_simd<simd<T, Abi>> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_simd_mask : std::false_type
    {
    };

    template <typename T, typename Abi>
    struct is_simd_mask<simd_mask<T, Abi>> : std::true_type
    {
    };

    // class template simd
    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    class simd
    {
    private:
        using Vector = std::array<T, detail::simd_impl<T>::size>;
        Vector vec;

    public:
        using value_type = T;
        using abi_type = Abi;
        using mask_type = simd_mask<T, Abi>;

        static constexpr std::size_t size()
        {
            return detail::simd_impl<T>::size;
        }

        // ----------------------------------------------------------------------
        //  constructors
        // ----------------------------------------------------------------------
        simd(simd const&) = default;
        simd(simd&&) noexcept = default;
        simd& operator=(simd const&) = default;
        simd& operator=(simd&&) noexcept = default;

        template <typename U, typename Flag>
        simd(U* ptr, Flag)
        {
            static_assert(std::is_same_v<std::remove_cvref_t<U>, T>,
                "pointer should be same type as value_type");
            static_assert(is_simd_flag_type_v<Flag>,
                "use element_aligned or vector_aligned tag");

            std::copy(ptr, ptr + size(), vec.begin());
        }

        simd(T val = {})
        {
            std::fill(vec.begin(), vec.end(), val);
        }

        simd(Vector v)
        {
            vec = v;
        }

        // ----------------------------------------------------------------------
        //  load and store
        // ----------------------------------------------------------------------
        template <typename U, typename Flag>
        void copy_from(U const* ptr, Flag)
        {
            static_assert(std::is_same_v<std::remove_cvref_t<U>, T>,
                "pointer should be same type as value_type");
            static_assert(is_simd_flag_type_v<Flag>,
                "use element_aligned or vector_aligned tag");
            std::copy(ptr, ptr + size(), vec.begin());
        }

        template <typename U, typename Flag>
        void copy_to(U* ptr, Flag) const
        {
            static_assert(std::is_same_v<std::remove_cvref_t<U>, T>,
                "pointer should be same type as value_type");
            static_assert(is_simd_flag_type_v<Flag>,
                "use element_aligned or vector_aligned tag");
            std::copy(vec.begin(), vec.end(), ptr);
        }

        // ----------------------------------------------------------------------
        //  get and set
        // ----------------------------------------------------------------------
        T get(int idx) const
        {
            if (idx < 0 || idx > static_cast<int>(size()))
                return -1;
            return vec[idx];
        }

        T operator[](int idx) const
        {
            return get(idx);
        }

        void set(int idx, T val)
        {
            if (idx < 0 || idx > static_cast<int>(size()))
                return;
            vec[idx] = val;
        }

        // ----------------------------------------------------------------------
        // ostream overload
        // ----------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os, simd const& x)
        {
            using type_ = std::decay_t<decltype(x)>;
            using value_type_ = type_::value_type;
            using printable_type =
                std::conditional_t<std::is_integral_v<value_type_>,
                    std::conditional_t<std::is_unsigned_v<value_type_>,
                        std::uint32_t, std::int32_t>,
                    value_type_>;

            os << "( ";
            for (int i = 0; i < static_cast<int>(x.size()); i++)
            {
                os << printable_type(x[i]) << ' ';
            }
            os << ")";
            return os;
        }

        // ----------------------------------------------------------------------
        //  Reduction
        // ----------------------------------------------------------------------
        auto addv() const
        {
            return std::accumulate(vec.begin(), vec.end(), T(0));
        }

        // ----------------------------------------------------------------------
        //  First and last elements
        // ----------------------------------------------------------------------
        auto first() const
        {
            return vec.front();
        }

        auto last() const
        {
            return vec.back();
        }

        // ----------------------------------------------------------------------
        //  unary operators [simd.unary]
        // ----------------------------------------------------------------------
        simd& operator++()
        {
            std::transform(vec.begin(), vec.end(), vec.begin(),
                [](T x) { return x + T(1); });
            return *this;
        }

        auto operator++(int)
        {
            auto vec_copy = *this;
            std::transform(vec.begin(), vec.end(), vec_copy.begin(),
                [](T x) { return x + T(1); });
            return vec_copy;
        }

        simd& operator--()
        {
            std::transform(vec.begin(), vec.end(), vec.begin(),
                [](T x) { return x - T(1); });
            return *this;
        }

        auto operator--(int)
        {
            auto vec_copy = *this;
            std::transform(vec.begin(), vec.end(), vec_copy.begin(),
                [](T x) { return x - T(1); });
            return vec_copy;
        }

        simd operator+() const
        {
            return *this;
        }

        simd operator-() const
        {
            auto vec_copy = *this;
            std::transform(vec.begin(), vec.end(), vec_copy.begin(),
                [](T x) { return -x; });
            return vec_copy;
        }

        // ----------------------------------------------------------------------
        // binary operators [simd.binary]
        // ----------------------------------------------------------------------
        friend simd operator+(simd const& x, simd const& y)
        {
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs + rhs; });
            return result;
        }

        friend simd operator-(simd const& x, simd const& y)
        {
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs - rhs; });
            return result;
        }

        friend simd operator*(simd const& x, simd const& y)
        {
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs * rhs; });
            return result;
        }

        friend simd operator/(simd const& x, simd const& y)
        {
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs / rhs; });
            return result;
        }

        friend simd operator&(simd const& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator& only works for integral types");
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs & rhs; });
            return result;
        }

        friend simd operator|(simd const& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator| only works for integral types");
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs | rhs; });
            return result;
        }

        friend simd operator^(simd const& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator^ only works for integral types");
            simd result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.vec.begin(), [](T lhs, T rhs) { return lhs | rhs; });
            return result;
        }

        // ----------------------------------------------------------------------
        // compound assignment [simd.cassign]
        // ----------------------------------------------------------------------
        friend simd& operator+=(simd& x, simd const& y)
        {
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs + rhs; });
            return x;
        }

        friend simd& operator-=(simd& x, simd const& y)
        {
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs - rhs; });
            return x;
        }

        friend simd& operator*=(simd& x, simd const& y)
        {
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs * rhs; });
            return x;
        }

        friend simd& operator/=(simd& x, simd const& y)
        {
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs / rhs; });
            return x;
        }

        friend simd operator&=(simd& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator&= only works for integral types");
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs & rhs; });
            return x;
        }

        friend simd operator|=(simd& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator|= only works for integral types");
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs | rhs; });
            return x;
        }

        friend simd operator^=(simd& x, simd const& y)
        {
            static_assert(std::is_integral_v<T>,
                "operator^= only works for integral types");
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                x.vec.begin(), [](T lhs, T rhs) { return lhs | rhs; });
            return x;
        }

        // ----------------------------------------------------------------------
        // compares [simd.comparison]
        // ----------------------------------------------------------------------
        friend mask_type operator==(simd const& x, simd const& y)
        {
            mask_type result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.mask.begin(), [](T lhs, T rhs) { return lhs == rhs; });
            return result;
        }

        friend mask_type operator!=(simd const& x, simd const& y)
        {
            return !(x == y);
        }

        friend mask_type operator>(simd const& x, simd const& y)
        {
            mask_type result;
            std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
                result.mask.begin(), [](T lhs, T rhs) { return lhs > rhs; });
            return result;
        }

        friend mask_type operator<(simd const& x, simd const& y)
        {
            return y > x;
        }

        friend mask_type operator>=(simd const& x, simd const& y)
        {
            return !(y > x);
        }

        friend mask_type operator<=(simd const& x, simd const& y)
        {
            return !(x > y);
        }

        // ----------------------------------------------------------------------
        // reduction algorithms
        // ----------------------------------------------------------------------
        T sum() const
        {
            return std::accumulate(vec.begin(), vec.end(), T(0));
        }

        T(min)() const
        {
            return *std::min_element(vec.begin(), vec.end());
        }

        T(max)() const
        {
            return *std::max_element(vec.begin(), vec.end());
        }

    private:
        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> choose(simd_mask<T_, Abi_> const& msk,
            simd<T_, Abi_> const& t, simd<T_, Abi_> const& f);

        template <typename T_, typename Abi_>
        friend void mask_assign(simd_mask<T_, Abi_> const& msk,
            simd<T_, Abi_>& v, simd<T_, Abi_> const& val);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_>(min)(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_>(max)(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename t_, typename abi_>
        friend simd<t_, abi_> copysign(
            simd<t_, abi_> const& valSrc, simd<t_, abi_> const& signSrc);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> sqrt(simd<T_, Abi_> const& x);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> abs(simd<T_, Abi_> const& x);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> fma(simd<T_, Abi_> const& a,
            simd<T_, Abi_> const& b, simd<T_, Abi_> const& z);

        template <typename T_, typename Abi_, typename Op>
        friend T_ reduce(simd<T_, Abi_> const& x, Op op);

        template <typename T_, typename Abi_, typename Op>
        friend simd<T_, Abi_> inclusive_scan(simd<T_, Abi_> const& x, Op op);

        template <typename T_, typename Abi_, typename Op>
        friend simd<T_, Abi_> exclusive_scan(
            simd<T_, Abi_> const& x, Op op, T_ init);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> index_series(T_ base, T_ step);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> interleave_even(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> interleave_odd(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> select_even(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> select_odd(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> lower_half(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> upper_half(
            simd<T_, Abi_> const& x, simd<T_, Abi_> const& y);

        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> reverse(simd<T_, Abi_> const& x);
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi>(min)(simd<T, Abi> const& x, simd<T, Abi> const& y)
    {
        simd<T, Abi> result;
        std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
            result.vec.begin(),
            [](T lhs, T rhs) { return (std::min) (lhs, rhs); });
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi>(max)(simd<T, Abi> const& x, simd<T, Abi> const& y)
    {
        simd<T, Abi> result;
        std::transform(x.vec.begin(), x.vec.end(), y.vec.begin(),
            result.vec.begin(),
            [](T lhs, T rhs) { return (std::max) (lhs, rhs); });
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    std::pair<simd<T, Abi>, simd<T, Abi>> minmax(
        simd<T, Abi> const& x, simd<T, Abi> const& y)
    {
        return {(min) (x, y), (max) (x, y)};
    }

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> copysign(
    //    simd<T_, Abi_> const& valSrc, simd<T_, Abi_> const& signSrc)
    //{
    //}

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi> sqrt(simd<T, Abi> const& x)
    {
        simd<T, Abi> result;
        std::transform(x.vec.begin(), x.vec.end(), result.vec.begin(),
            [](T x) { return std::sqrt(x); });
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi> abs(simd<T, Abi> const& x)
    {
        simd<T, Abi> result;
        std::transform(x.vec.begin(), x.vec.end(), result.vec.begin(),
            [](T x) { return std::abs(x); });
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi> fma(
        simd<T, Abi> const& a, simd<T, Abi> const& b, simd<T, Abi> const& z)
    {
        simd<T, Abi> result;
        for (int i = 0; i < static_cast<int>(simd<T, Abi>::size()); ++i)
        {
            result[i] = std::fma(a[i], b[i], z[i]);
        }
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi,
        typename Op = std::plus<>>
    T reduce(simd<T, Abi> const& x, Op op = {})
    {
        return std::accumulate(x.vec.begin(), x.vec.end(), T(0), op);
    }

    //template <typename T_, typename Abi_, typename Op = std::plus<>>
    //inline simd<T_, Abi_> inclusive_scan(simd<T_, Abi_> const& x, Op op = {})
    //{
    //}

    //template <typename T, typename Abi, typename Op = std::plus<>>
    //inline simd<T_, Abi_> exclusive_scan(
    //    simd<T_, Abi_> const& x, Op op = {}, T_ init = {})
    //{
    //}

    //template <typename T_, typename Abi_ = simd_abi::simd_emulation_abi>
    //inline simd<T_, Abi_> index_series(T_ base, T_ step)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> interleave_even(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> interleave_odd(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> select_even(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> select_odd(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> lower_half(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> upper_half(
    //    simd<T_, Abi_> const& x, simd<T_, Abi_> const& y)
    //{
    //}

    //template <typename T_, typename Abi_>
    //inline simd<T_, Abi_> reverse(simd<T_, Abi_> const& x)
    //{
    //}

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    class simd_mask
    {
    public:
        std::array<bool, simd<T, Abi>::size()> mask;

        using value_type = bool;
        using simd_type = simd<T, Abi>;
        using abi_type = Abi;

        static constexpr std::size_t size()
        {
            return simd<T, Abi>::size();
        }

        // ----------------------------------------------------------------------
        //  constructors
        // ----------------------------------------------------------------------
        simd_mask(simd_mask const&) = default;
        simd_mask(simd_mask&&) = default;
        simd_mask& operator=(simd_mask const&) = default;
        simd_mask& operator=(simd_mask&&) = default;

        simd_mask(bool val = false)
        {
            std::fill(mask.begin(), mask.end(), val);
        }

        // ----------------------------------------------------------------------
        //  get and set
        // ----------------------------------------------------------------------
        bool get(int idx) const
        {
            if (idx < 0 || idx > static_cast<int>(size()))
                return false;

            return mask[idx];
        }

        bool operator[](int idx) const
        {
            return get(idx);
        }

        void set(int idx, bool val)
        {
            if (idx < 0 || idx > static_cast<int>(size()))
                return;

            mask[idx] = val;
        }

        // ----------------------------------------------------------------------
        // ostream overload
        // ----------------------------------------------------------------------
        friend std::ostream& operator<<(std::ostream& os, simd_mask const& x)
        {
            os << "( ";
            for (int i = 0; i < static_cast<int>(x.size()); i++)
            {
                os << x[i] << ' ';
            }
            os << ")";
            return os;
        }

        // ----------------------------------------------------------------------
        //  unary operators
        // ----------------------------------------------------------------------
        simd_mask operator!() const noexcept
        {
            simd_mask result;
            std::transform(mask.begin(), mask.end(), result.mask.begin(),
                [](bool x) { return !x; });
            return result;
        }

        // ----------------------------------------------------------------------
        //  binary operators
        // ----------------------------------------------------------------------
        friend simd_mask operator&&(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            simd_mask result;
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                result.mask.begin(), [](bool x, bool y) { return x && y; });
            return result;
        }

        friend simd_mask operator||(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            simd_mask result;
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                result.mask.begin(), [](bool x, bool y) { return x || y; });
            return result;
        }

        friend simd_mask operator&(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            return (x && y);
        }

        friend simd_mask operator|(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            return (x || y);
        }

        friend simd_mask operator^(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            std::array<bool, simd<T, Abi>::size()> result;
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                result.begin(), [](bool x, bool y) { return x ^ y; });
            return result;
        }

        // ----------------------------------------------------------------------
        // simd_mask compound assignment [simd.mask.cassign]
        // ----------------------------------------------------------------------
        friend simd_mask& operator&=(simd_mask& x, simd_mask const& y) noexcept
        {
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                x.mask.begin(), [](bool x, bool y) { return x & y; });
            return x;
        }

        friend simd_mask& operator|=(simd_mask& x, simd_mask const& y) noexcept
        {
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                x.mask.begin(), [](bool x, bool y) { return x | y; });
            return x;
        }

        friend simd_mask& operator^=(simd_mask& x, simd_mask const& y) noexcept
        {
            std::transform(x.mask.begin(), x.mask.end(), y.mask.begin(),
                x.mask.begin(), [](bool x, bool y) { return x ^ y; });
            return x;
        }

        // ----------------------------------------------------------------------
        // simd_mask compares [simd.mask.comparison]
        // ----------------------------------------------------------------------
        friend simd_mask operator==(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            return std::equal(
                x.mask.begin(), x.mask.end(), y.mask.begin(), y.mask.end());
        }

        friend simd_mask operator!=(
            simd_mask const& x, simd_mask const& y) noexcept
        {
            return !(x == y);
        }

        // ----------------------------------------------------------------------
        //  algorithms
        // ----------------------------------------------------------------------
        auto popcount() const
        {
            return std::count(mask.begin(), mask.end(), true);
        }

        bool all_of() const
        {
            return popcount() == static_cast<int>(size());
        }

        bool any_of() const
        {
            return popcount() > 0;
        }

        bool none_of() const
        {
            return popcount() == 0;
        }

        bool some_of() const
        {
            auto c = popcount();
            return (c > 0) && (c < static_cast<int>(size()));
        }

        int find_first_set() const
        {
            for (int i = 0; i < static_cast<int>(size()); i++)
            {
                if (mask[i])
                    return i;
            }
            return -1;
        }

        int find_last_set() const
        {
            int ans = -1;
            for (int i = 0; i < static_cast<int>(size()); i++)
            {
                if (mask[i])
                    ans = i;
            }
            return ans;
        }

    private:
        template <typename T_, typename Abi_>
        friend simd<T_, Abi_> choose(simd_mask<T_, Abi_> const& msk,
            simd<T_, Abi_> const& t, simd<T_, Abi_> const& f);

        template <typename T_, typename Abi_>
        friend void mask_assign(simd_mask<T_, Abi_> const& msk,
            simd<T_, Abi_>& v, simd<T_, Abi_> const& val);
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    bool all_of(simd_mask<T, Abi> const& m)
    {
        return m.all_of();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    bool any_of(simd_mask<T, Abi> const& m)
    {
        return m.any_of();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    bool none_of(simd_mask<T, Abi> const& m)
    {
        return m.none_of();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    bool some_of(simd_mask<T, Abi> const& m)
    {
        return m.some_of();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    auto popcount(simd_mask<T, Abi> const& m)
    {
        return m.popcount();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    int find_first_set(simd_mask<T, Abi> const& m)
    {
        return m.find_first_set();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    int find_last_set(simd_mask<T, Abi> const& m)
    {
        return m.find_last_set();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    simd<T, Abi> choose(simd_mask<T, Abi> const& msk, simd<T, Abi> const& t,
        simd<T, Abi> const& f)
    {
        simd<T, Abi> result;
        for (int i = 0; i < static_cast<int>(msk.size()); i++)
        {
            result.set(i, msk.mask[i] ? t.get(i) : f.get(i));
        }
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    void mask_assign(
        simd_mask<T, Abi> const& msk, simd<T, Abi>& v, simd<T, Abi> const& val)
    {
        for (int i = 0; i < static_cast<int>(msk.size()); i++)
        {
            if (msk.mask[i])
                v.set(i, val.get(i));
        }
    }
}    // namespace hpx::datapar::experimental

#endif
