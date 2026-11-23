//  Copyright (c) 2024 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// ---------------------------------------------------------------------------
// This file has been taken from
// https://github.com/maddyscientist/reproducible_floating_sums commit
// b5a065741d4ea459437ca004b508de9dcb6a3e52. The boost copyright has been added
// to this file in accordance with the dual license terms for the Reproducible
// Floating-Point Summations and conformance with the HPX policy
// https://github.com/maddyscientist/reproducible_floating_sums/blob/feature/cuda/LICENSE.md
// ---------------------------------------------------------------------------
//
/// Copyright 2022 Richard Barnes, Peter Ahrens, James Demmel
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
//Reproducible Floating Point Accumulations via Binned Floating Point
//Adapted to C++ by Richard Barnes from ReproBLAS v2.1.0.
//ReproBLAS by Peter Ahrens, Hong Diep Nguyen, and James Demmel.
//
//The code accomplishes several objectives:
//
//1. Reproducible summation, independent of summation order, assuming only a
//   subset of the IEEE 754 Floating Point Standard
//
//2. Has accuracy at least as good as conventional summation, and tunable
//
//3. Handles overflow, underflow, and other exceptions reproducibly.
//
//4. Makes only one read-only pass over the summands.
//
//5. Requires only one parallel reduction.
//
//6. Uses minimal memory (6 doubles per accumulator with fold=3).
//
//7. Relatively easy to use

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

#include <hpx/config.hpp>

namespace hpx::parallel::detail::rfa {

    HPX_CXX_CORE_EXPORT template <typename F>
    struct type4
    {
        F x;
        F y;
        F z;
        F w;
    };

    HPX_CXX_CORE_EXPORT template <typename F>
    struct type2
    {
        F x;
        F y;
    };

    HPX_CXX_CORE_EXPORT using float4 = type4<float>;
    HPX_CXX_CORE_EXPORT using double4 = type4<double>;
    HPX_CXX_CORE_EXPORT using float2 = type2<float>;
    HPX_CXX_CORE_EXPORT using double2 = type2<double>;

    HPX_CXX_CORE_EXPORT inline auto abs_max(float4 a)
    {
        auto x = std::abs(a.x);
        auto y = std::abs(a.y);
        auto z = std::abs(a.z);
        auto w = std::abs(a.w);
        std::vector<float> const v = {x, y, z, w};
        return *std::max_element(v.begin(), v.end());
    }

    HPX_CXX_CORE_EXPORT inline auto abs_max(double4 a)
    {
        auto x = std::abs(a.x);
        auto y = std::abs(a.y);
        auto z = std::abs(a.z);
        auto w = std::abs(a.w);
        std::vector<double> const v = {x, y, z, w};
        return *std::max_element(v.begin(), v.end());
    }

    HPX_CXX_CORE_EXPORT inline auto abs_max(float2 a)
    {
        auto x = std::abs(a.x);
        auto y = std::abs(a.y);
        std::vector<float> const v = {x, y};
        return *std::max_element(v.begin(), v.end());
    }

    HPX_CXX_CORE_EXPORT inline auto abs_max(double2 a)
    {
        auto x = std::abs(a.x);
        auto y = std::abs(a.y);
        std::vector<double> const v = {x, y};
        return *std::max_element(v.begin(), v.end());
    }

// disable zero checks
#define DISABLE_ZERO

// disable nan / infinity checks
#define DISABLE_NANINF

// jump table for indexing into data
#define MAX_JUMP 5
    static_assert(MAX_JUMP <= 5, "MAX_JUMP greater than max");

    HPX_CXX_CORE_EXPORT template <typename Real>
    inline constexpr Real ldexp_impl(Real arg, int exp) noexcept
    {
        return std::ldexp(arg, exp);
        // while (arg == 0)
        // {
        //     return arg;
        // }
        // while (exp > 0)
        // {
        //     arg *= static_cast<Real>(2);
        //     --exp;
        // }
        // while (exp < 0)
        // {
        //     arg /= static_cast<Real>(2);
        //     ++exp;
        // }

        // return arg;
    }

    HPX_CXX_CORE_EXPORT template <class ftype>
    struct RFA_bins
    {
        static constexpr auto BIN_WIDTH =
            std::is_same_v<ftype, double> ? 40 : 13;
        static constexpr auto MIN_EXP =
            std::numeric_limits<ftype>::min_exponent;
        static constexpr auto MAX_EXP =
            std::numeric_limits<ftype>::max_exponent;
        static constexpr auto MANT_DIG = std::numeric_limits<ftype>::digits;
        ///Binned floating-point maximum index
        static constexpr auto MAXINDEX =
            ((MAX_EXP - MIN_EXP + MANT_DIG - 1) / BIN_WIDTH) - 1;
        //The maximum floating-point fold supported by the library
        static constexpr auto MAXFOLD = MAXINDEX + 1;

        ///The binned floating-point reference bins
        std::array<ftype, MAXINDEX + MAXFOLD> bins = {};

        constexpr ftype& operator[](int d)
        {
            return bins[d];
        }

        void initialize_bins()
        {
            if constexpr (std::is_same_v<ftype, float>)
            {
                bins[0] = static_cast<ftype>(std::ldexp(0.75, MAX_EXP));
            }
            else
            {
                bins[0] = 2.0 * std::ldexp(0.75, MAX_EXP - 1);
            }

            for (int index = 1; index <= MAXINDEX; index++)
            {
                bins[index] = static_cast<ftype>(std::ldexp(0.75,
                    MAX_EXP + MANT_DIG - BIN_WIDTH + 1 - index * BIN_WIDTH));
            }
            for (int index = MAXINDEX + 1; index < MAXINDEX + MAXFOLD; index++)
            {
                bins[index] = bins[index - 1];
            }
        }
    };

    HPX_CXX_CORE_EXPORT inline char
        hpx_rfa_bin_host_buffer[sizeof(RFA_bins<double>)];

    ///Class to hold a reproducible summation of the numbers passed to it
    ///
    ///@param ftype Floating-point data type; either `float` or `double
    ///@param FOLD  The fold; use 3 as a default unless you understand it.
    HPX_CXX_CORE_EXPORT template <class ftype_, int FOLD_ = 3,
        typename std::enable_if_t<std::is_floating_point_v<ftype_>>* = nullptr>
    class alignas(2 * sizeof(ftype_)) reproducible_floating_accumulator
    {
    public:
        using ftype = ftype_;
        static constexpr int FOLD = FOLD_;

    private:
        std::array<ftype, static_cast<std::size_t>(2) * FOLD> data = {{0}};

        ///Floating-point precision bin width
        static constexpr auto BIN_WIDTH =
            std::is_same_v<ftype, double> ? 40 : 13;
        static constexpr auto MIN_EXP =
            std::numeric_limits<ftype>::min_exponent;
        static constexpr auto MAX_EXP =
            std::numeric_limits<ftype>::max_exponent;
        static constexpr auto MANT_DIG = std::numeric_limits<ftype>::digits;
        ///Binned floating-point maximum index
        static constexpr auto MAXINDEX =
            ((MAX_EXP - MIN_EXP + MANT_DIG - 1) / BIN_WIDTH) - 1;
        //The maximum floating-point fold supported by the library
        static constexpr auto MAXFOLD = MAXINDEX + 1;
        ///Binned floating-point compression factor
        ///This factor is used to scale down inputs before deposition into the bin of
        ///highest index
        static constexpr auto COMPRESSION =
            1.0 / (1 << (MANT_DIG - BIN_WIDTH + 1));
        ///Binned double precision expansion factor
        ///This factor is used to scale up inputs after deposition into the bin of
        ///highest index
        static constexpr auto EXPANSION =
            1.0 * (1 << (MANT_DIG - BIN_WIDTH + 1));
        static constexpr auto EXP_BIAS = MAX_EXP - 2;
        static constexpr auto EPSILON = std::numeric_limits<ftype>::epsilon();
        ///Binned floating-point deposit endurance
        ///The number of deposits that can be performed before a renorm is necessary.
        ///Applies also to binned complex double precision.
        static constexpr auto ENDURANCE = 1 << (MANT_DIG - BIN_WIDTH - 2);
        ///Return a binned floating-point reference bin
        inline ftype const* binned_bins(int const x) const
        {
            return &reinterpret_cast<RFA_bins<ftype>&>(
                hpx_rfa_bin_host_buffer)[x];
        }

        ///Get the bit representation of a float
        static inline uint32_t& get_bits(float& x)
        {
            return *reinterpret_cast<uint32_t*>(&x);
        }
        ///Get the bit representation of a double
        static inline uint64_t& get_bits(double& x)
        {
            return *reinterpret_cast<uint64_t*>(&x);
        }
        ///Get the bit representation of a const float
        static inline uint32_t get_bits(float const& x)
        {
            return *reinterpret_cast<uint32_t const*>(&x);
        }
        ///Get the bit representation of a const double
        static inline uint64_t get_bits(double const& x)
        {
            return *reinterpret_cast<uint64_t const*>(&x);
        }

        ///Return primary vector value const ref
        inline ftype const& primary(int i) const
        {
            if constexpr (FOLD <= MAX_JUMP)
            {
                switch (i)
                {
                case 0:
                    if constexpr (FOLD >= 1)
                        return data[0];
                case 1:
                    if constexpr (FOLD >= 2)
                        return data[1];
                case 2:
                    if constexpr (FOLD >= 3)
                        return data[2];
                case 3:
                    if constexpr (FOLD >= 4)
                        return data[3];
                case 4:
                    if constexpr (FOLD >= 5)
                        return data[4];
                default:
                    return data[FOLD - 1];
                }
            }
            else
            {
                return data[i];
            }
        }

        ///Return carry vector value const ref
        inline ftype const& carry(int i) const
        {
            if constexpr (FOLD <= MAX_JUMP)
            {
                switch (i)
                {
                case 0:
                    if constexpr (FOLD >= 1)
                        return data[FOLD + 0];
                case 1:
                    if constexpr (FOLD >= 2)
                        return data[FOLD + 1];
                case 2:
                    if constexpr (FOLD >= 3)
                        return data[FOLD + 2];
                case 3:
                    if constexpr (FOLD >= 4)
                        return data[FOLD + 3];
                case 4:
                    if constexpr (FOLD >= 5)
                        return data[FOLD + 4];
                default:
                    return data[2 * FOLD - 1];
                }
            }
            else
            {
                return data[FOLD + i];
            }
        }

        ///Return primary vector value ref
        inline ftype& primary(int i)
        {
            auto const& c = *this;
            return const_cast<ftype&>(c.primary(i));
        }

        ///Return carry vector value ref
        inline ftype& carry(int i)
        {
            auto const& c = *this;
            return const_cast<ftype&>(c.carry(i));
        }

#ifdef DISABLE_ZERO
        static inline constexpr bool ISZERO(ftype const)
        {
            return false;
        }
#else
        static inline constexpr bool ISZERO(ftype const x)
        {
            return x == 0.0;
        }
#endif

#ifdef DISABLE_NANINF
        static inline constexpr int ISNANINF(ftype const)
        {
            return false;
        }
#else
        static inline constexpr int ISNANINF(ftype const x)
        {
            auto const bits = get_bits(x);
            return (bits & ((2ull * MAX_EXP - 1) << (MANT_DIG - 1))) ==
                ((2ull * MAX_EXP - 1) << (MANT_DIG - 1));
        }
#endif

        static inline constexpr int EXP(ftype const x)
        {
            auto const bits = get_bits(x);
            return (bits >> (MANT_DIG - 1)) & (2 * MAX_EXP - 1);
        }

        ///Get index of float-point precision
        ///The index of a non-binned type is the smallest index a binned type would
        ///need to have to sum it reproducibly. Higher indices correspond to smaller
        ///bins.
        static inline constexpr int binned_dindex(ftype const x)
        {
            int exp = EXP(x);
            if (exp == 0)
            {
                if (x == static_cast<ftype>(0.0))
                {
                    return MAXINDEX;
                }
                else
                {
                    std::frexp(x, &exp);
                    return (std::max) ((MAX_EXP - exp) / BIN_WIDTH, MAXINDEX);
                }
            }
            return ((MAX_EXP + EXP_BIAS) - exp) / BIN_WIDTH;
        }

        ///Get index of manually specified binned double precision
        ///The index of a binned type is the bin that it corresponds to. Higher
        ///indices correspond to smaller bins.
        inline int binned_index() const
        {
            return ((MAX_EXP + MANT_DIG - BIN_WIDTH + 1 + EXP_BIAS) -
                       EXP(primary(0))) /
                BIN_WIDTH;
        }

        ///Check if index of manually specified binned floating-point is 0
        ///A quick check to determine if the index is 0
        inline bool binned_index0() const
        {
            return EXP(primary(0)) == MAX_EXP + EXP_BIAS;
        }

        ///Update manually specified binned fp with a scalar (X -> Y)
        ///
        ///This method updates the binned fp to an index suitable for adding numbers
        ///with absolute value less than @p max_abs_val
        ///
        ///@param incpriY stride within Y's primary vector (use every incpriY'th element)
        ///@param inccarY stride within Y's carry vector (use every inccarY'th element)
        void binned_dmdupdate(
            ftype const max_abs_val, int const incpriY, int const inccarY)
        {
            if (ISNANINF(primary(0)))
                return;

            int X_index = binned_dindex(max_abs_val);
            if (ISZERO(primary(0)))
            {
                ftype const* const bins = binned_bins(X_index);
                for (int i = 0; i < FOLD; i++)
                {
                    primary(i * incpriY) = bins[i];
                    carry(i * inccarY) = 0.0;
                }
            }
            else
            {
                int shift = binned_index() - X_index;
                if (shift > 0)
                {
#if !defined(HPX_CLANG_VERSION)
                    HPX_UNROLL
#endif
                    for (int i = FOLD - 1; i >= 1; i--)
                    {
                        if (i < shift)
                            break;
                        primary(i * incpriY) = primary((i - shift) * incpriY);
                        carry(i * inccarY) = carry((i - shift) * inccarY);
                    }
                    ftype const* const bins = binned_bins(X_index);
#if !defined(HPX_CLANG_VERSION)
                    HPX_UNROLL
#endif
                    for (int j = 0; j < FOLD; j++)
                    {
                        if (j >= shift)
                            break;
                        primary(j * incpriY) = bins[j];
                        carry(j * inccarY) = 0.0;
                    }
                }
            }
        }

        ///Add scalar @p X to suitably binned manually specified binned fp (Y += X)
        ///
        ///Performs the operation Y += X on an binned type Y where the index of Y is
        ///larger than the index of @p X
        ///
        ///@param incpriY stride within Y's primary vector (use every incpriY'th element)
        void binned_dmddeposit(ftype const X, int const incpriY)
        {
            ftype M;
            ftype x = X;

            if (ISNANINF(x) || ISNANINF(primary(0)))
            {
                primary(0) += x;
                return;
            }

            if (binned_index0())
            {
                M = primary(0);
                ftype qd = x * static_cast<ftype>(COMPRESSION);
                auto& ql = get_bits(qd);
                ql |= 1;
                qd += M;
                primary(0) = qd;
                M -= qd;
                auto temp_m = (double) (((double) EXPANSION) * 0.5);
                M *= static_cast<ftype>(temp_m);
                x += M;
                x += M;
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = 1; i < FOLD - 1; i++)
                {
                    M = primary(i * incpriY);
                    qd = x;
                    ql |= 1;
                    qd += M;
                    primary(i * incpriY) = qd;
                    M -= qd;
                    x += M;
                }
                qd = x;
                ql |= 1;
                primary((FOLD - 1) * incpriY) += qd;
            }
            else
            {
                ftype qd = x;
                auto& ql = get_bits(qd);
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = 0; i < FOLD - 1; i++)
                {
                    M = primary(i * incpriY);
                    qd = x;
                    ql |= 1;
                    qd += M;
                    primary(i * incpriY) = qd;
                    M -= qd;
                    x += M;
                }
                qd = x;
                ql |= 1;
                primary((FOLD - 1) * incpriY) += qd;
            }
        }

        ///Renormalize manually specified binned double precision
        ///
        ///Renormalization keeps the primary vector within the necessary bins by
        ///shifting over to the carry vector
        ///
        ///@param incpriX stride within X's primary vector (use every incpriX'th element)
        ///@param inccarX stride within X's carry vector (use every inccarX'th element)
        inline void binned_dmrenorm(int const incpriX, int const inccarX)
        {
            if (ISZERO(primary(0)) || ISNANINF(primary(0)))
                return;

            for (int i = 0; i < FOLD; i++)
            {
                auto tmp_renormd = primary(i * incpriX);
                auto& tmp_renorml = get_bits(tmp_renormd);

                carry(i * inccarX) +=
                    (int) ((tmp_renorml >> (MANT_DIG - 3)) & 3) - 2;

                tmp_renorml &= ~(1ull << (MANT_DIG - 3));
                tmp_renorml |= 1ull << (MANT_DIG - 2);
                primary(i * incpriX) = tmp_renormd;
            }
        }

        ///Add scalar to manually specified binned fp (Y += X)
        ///
        ///Performs the operation Y += X on an binned type Y
        ///
        ///@param incpriY stride within Y's primary vector (use every incpriY'th element)
        ///@param inccarY stride within Y's carry vector (use every inccarY'th element)
        void binned_dmdadd(ftype const X, int const incpriY, int const inccarY)
        {
            binned_dmdupdate(X, incpriY, inccarY);
            binned_dmddeposit(X, incpriY);
            binned_dmrenorm(incpriY, inccarY);
        }

        ///Convert manually specified binned fp to native double-precision (X -> Y)
        ///
        ///@param incpriX stride within X's primary vector (use every incpriX'th element)
        ///@param inccarX stride within X's carry vector (use every inccarX'th element)
        double binned_conv_double(int const incpriX, int const inccarX) const
        {
            int i = 0;

            if (ISNANINF(primary(0)))
                return (double) primary(0);
            if (ISZERO(primary(0)))
                return 0.0;

            double Y = 0.0;
            double scale_down;
            double scale_up;
            int scaled;
            auto const X_index = binned_index();
            auto const* const bins = binned_bins(X_index);
            if (X_index <= (3 * MANT_DIG) / BIN_WIDTH)
            {
                scale_down = std::ldexp(0.5, 1 - (2 * MANT_DIG - BIN_WIDTH));
                scale_up = std::ldexp(0.5, 1 + (2 * MANT_DIG - BIN_WIDTH));
                scaled = (std::max) ((std::min) (FOLD,
                                         (3 * MANT_DIG) / BIN_WIDTH - X_index),
                    0);
                if (X_index == 0)
                {
                    Y += ((double) carry(0)) *
                        ((((double) bins[0]) / 6.0) * scale_down * EXPANSION);
                    Y += ((double) carry(inccarX)) *
                        ((((double) bins[1]) / 6.0) * scale_down);
                    Y += ((double) primary(0) - (double) bins[0]) * scale_down *
                        EXPANSION;
                    i = 2;
                }
                else
                {
                    Y += ((double) carry(0)) *
                        (((double) bins[0] / 6.0) * scale_down);
                    i = 1;
                }
                for (; i < scaled; i++)
                {
                    Y += ((double) carry(i * inccarX)) *
                        (((double) bins[i] / 6.0) * scale_down);
                    Y += ((double) primary((i - 1) * incpriX) -
                             (double) (bins[i - 1])) *
                        scale_down;
                }
                if (i == FOLD)
                {
                    Y += ((double) primary((FOLD - 1) * incpriX) -
                             (double) (bins[FOLD - 1])) *
                        scale_down;
                    return Y * scale_up;
                }
                if (std::isinf(Y * scale_up))
                {
                    return Y * scale_up;
                }
                Y *= scale_up;
                for (; i < FOLD; i++)
                {
                    Y += ((double) carry(i * inccarX)) *
                        ((double) bins[i] / 6.0);
                    Y += (double) (primary((i - 1) * incpriX) - bins[i - 1]);
                }
                Y += ((double) primary((FOLD - 1) * incpriX) -
                    ((double) bins[FOLD - 1]));
            }
            else
            {
                Y += ((double) carry(0)) * ((double) bins[0] / 6.0);
                for (i = 1; i < FOLD; i++)
                {
                    Y += ((double) carry(i * inccarX)) *
                        ((double) bins[i] / 6.0);
                    Y += (double) (primary((i - 1) * incpriX) - bins[i - 1]);
                }
                Y += (double) (primary((FOLD - 1) * incpriX) - bins[FOLD - 1]);
            }
            return Y;
        }

        ///Convert manually specified binned fp to native single-precision (X -> Y)
        ///
        ///@param incpriX stride within X's primary vector (use every incpriX'th element)
        ///@param inccarX stride within X's carry vector (use every inccarX'th element)
        float binned_conv_single(int const incpriX, int const inccarX) const
        {
            int i = 0;
            double Y = 0.0;

            if (ISNANINF(primary(0)))
                return static_cast<float>(primary(0));
            if (ISZERO(primary(0)))
                return 0.0f;

            //Note that the following order of summation is in order of decreasing
            //exponent. The following code is specific to SBWIDTH=13, FLT_MANT_DIG=24, and
            //the number of carries equal to 1.
            auto const X_index = binned_index();
            auto const* const bins = binned_bins(X_index);
            if (X_index == 0)
            {
                Y += (double) carry(0) * (double) (((double) bins[0]) / 6.0) *
                    (double) EXPANSION;
                Y += (double) carry(inccarX) *
                    (double) (((double) bins[1]) / 6.0);
                Y += (double) (primary(0) - bins[0]) * (double) EXPANSION;
                i = 2;
            }
            else
            {
                Y += (double) carry(0) * (double) (((double) bins[0]) / 6.0);
                i = 1;
            }
            for (; i < FOLD; i++)
            {
                Y += (double) carry(i * inccarX) *
                    (double) (((double) bins[i]) / 6.0);
                Y += (double) (primary((i - 1) * incpriX) - bins[i - 1]);
            }
            Y += (double) (primary((FOLD - 1) * incpriX) - bins[FOLD - 1]);

            return (float) Y;
        }

        ///Add two manually specified binned fp (Y += X)
        ///Performs the operation Y += X
        ///
        ///@param other   Another binned fp of the same type
        ///@param incpriX stride within X's primary vector (use every incpriX'th element)
        ///@param inccarX stride within X's carry vector (use every inccarX'th element)
        ///@param incpriY stride within Y's primary vector (use every incpriY'th element)
        ///@param inccarY stride within Y's carry vector (use every inccarY'th element)
        void binned_dmdmadd(reproducible_floating_accumulator const& x,
            int const incpriX, int const inccarX, int const incpriY,
            int const inccarY)
        {
            if (ISZERO(x.primary(0)))
                return;

            if (ISZERO(primary(0)))
            {
                for (int i = 0; i < FOLD; i++)
                {
                    primary(i * incpriY) = x.primary(i * incpriX);
                    carry(i * inccarY) = x.carry(i * inccarX);
                }
                return;
            }

            if (ISNANINF(x.primary(0)) || ISNANINF(primary(0)))
            {
                primary(0) += x.primary(0);
                return;
            }

            auto const X_index = x.binned_index();
            auto const Y_index = this->binned_index();
            auto const shift = Y_index - X_index;
            if (shift > 0)
            {
                auto const* const bins = binned_bins(Y_index);
//shift Y upwards and add X to Y
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = FOLD - 1; i >= 1; i--)
                {
                    if (i < shift)
                        break;
                    primary(i * incpriY) = x.primary(i * incpriX) +
                        (primary((i - shift) * incpriY) - bins[i - shift]);
                    carry(i * inccarY) =
                        x.carry(i * inccarX) + carry((i - shift) * inccarY);
                }
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = 0; i < FOLD; i++)
                {
                    if (i == shift)
                        break;
                    primary(i * incpriY) = x.primary(i * incpriX);
                    carry(i * inccarY) = x.carry(i * inccarX);
                }
            }
            else if (shift < 0)
            {
                auto const* const bins = binned_bins(X_index);
//shift X upwards and add X to Y
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = 0; i < FOLD; i++)
                {
                    if (i < -shift)
                        continue;
                    primary(i * incpriY) +=
                        x.primary((i + shift) * incpriX) - bins[i + shift];
                    carry(i * inccarY) += x.carry((i + shift) * inccarX);
                }
            }
            else if (shift == 0)
            {
                auto const* const bins = binned_bins(X_index);
// add X to Y
#if !defined(HPX_CLANG_VERSION)
                HPX_UNROLL
#endif
                for (int i = 0; i < FOLD; i++)
                {
                    primary(i * incpriY) += x.primary(i * incpriX) - bins[i];
                    carry(i * inccarY) += x.carry(i * inccarX);
                }
            }

            binned_dmrenorm(incpriY, inccarY);
        }

        ///Add two manually specified binned fp (Y += X)
        ///Performs the operation Y += X
        void binned_dbdbadd(reproducible_floating_accumulator const& other)
        {
            binned_dmdmadd(other, 1, 1, 1, 1);
        }

    public:
        reproducible_floating_accumulator() = default;
        reproducible_floating_accumulator(
            reproducible_floating_accumulator const&) = default;
        ///Sets this binned fp equal to another binned fp
        reproducible_floating_accumulator& operator=(
            reproducible_floating_accumulator const&) = default;

        ///Set the binned fp to zero
        void zero()
        {
            data = {{0}};
        }

        ///Return the fold of the binned fp
        constexpr int fold() const
        {
            return FOLD;
        }

        ///Return the endurance of the binned fp
        constexpr size_t endurance() const
        {
            return ENDURANCE;
        }

        ///Returns the number of reference bins. Used for judging memory usage.
        constexpr size_t number_of_reference_bins()
        {
            return std::array<ftype, MAXINDEX + MAXFOLD>::size();
        }

        ///Accumulate an arithmetic @p x into the binned fp.
        ///NOTE: Casts @p x to the type of the binned fp
        template <typename U,
            typename std::enable_if_t<std::is_arithmetic_v<U>>* = nullptr>
        reproducible_floating_accumulator& operator+=(U const x)
        {
            binned_dmdadd(static_cast<ftype>(x), 1, 1);
            return *this;
        }

        ///Accumulate-subtract an arithmetic @p x into the binned fp.
        ///NOTE: Casts @p x to the type of the binned fp
        template <typename U,
            typename std::enable_if_t<std::is_arithmetic_v<U>>* = nullptr>
        reproducible_floating_accumulator& operator-=(U const x)
        {
            binned_dmdadd(-static_cast<ftype>(x), 1, 1);
            return *this;
        }

        ///Accumulate a binned fp @p x into the binned fp.
        reproducible_floating_accumulator& operator+=(
            reproducible_floating_accumulator const& other)
        {
            binned_dbdbadd(other);
            return *this;
        }

        ///Accumulate-subtract a binned fp @p x into the binned fp.
        ///NOTE: Makes a copy and performs arithmetic; slow.
        reproducible_floating_accumulator& operator-=(
            reproducible_floating_accumulator const& other)
        {
            auto const temp = -other;
            binned_dbdbadd(temp);
        }

        ///Determines if two binned fp are equal
        bool operator==(reproducible_floating_accumulator const& other) const
        {
            return data == other.data;
        }

        ///Determines if two binned fp are not equal
        bool operator!=(reproducible_floating_accumulator const& other) const
        {
            return !operator==(other);
        }

        ///Sets this binned fp equal to the arithmetic value @p x
        ///NOTE: Casts @p x to the type of the binned fp
        template <typename U,
            typename std::enable_if_t<std::is_arithmetic_v<U>>* = nullptr>
        reproducible_floating_accumulator& operator=(U const x)
        {
            zero();
            binned_dmdadd(static_cast<ftype>(x), 1, 1);
            return *this;
        }

        ///Returns the negative of this binned fp
        ///NOTE: Makes a copy and performs arithmetic; slow.
        reproducible_floating_accumulator operator-()
        {
            constexpr int incpriX = 1;
            constexpr int inccarX = 1;
            reproducible_floating_accumulator temp = *this;
            if (primary(0) != 0.0)
            {
                auto const* const bins = binned_bins(binned_index());
                for (int i = 0; i < FOLD; i++)
                {
                    temp.primary(i * incpriX) =
                        bins[i] - (primary(i * incpriX) - bins[i]);
                    temp.carry(i * inccarX) = -carry(i * inccarX);
                }
            }
            return temp;
        }

        ///Convert this binned fp into its native floating-point representation
        ftype conv() const
        {
            if (std::is_same_v<ftype, float>)
            {
                return static_cast<ftype>(binned_conv_single(1, 1));
            }
            else
            {
                return static_cast<ftype>(binned_conv_double(1, 1));
            }
        }

        ///@brief Get binned fp summation error bound
        ///
        ///This is a bound on the absolute error of a summation using binned types
        ///
        ///@param N           The number of single precision floating point summands
        ///@param max_abs_val The summand of maximum absolute value
        ///@param binned_sum  The value of the sum computed using binned types
        ///@return            The absolute error bound
        static constexpr ftype error_bound(
            uint64_t const N, ftype const max_abs_val, ftype const binned_sum)
        {
            double const X = std::abs(max_abs_val);
            double const S = std::abs(binned_sum);
            return static_cast<ftype>(
                (std::max) (X, std::ldexp(0.5, MIN_EXP - 1)) *
                    std::ldexp(0.5, (1 - FOLD) * BIN_WIDTH + 1) * N +
                ((7.0 * EPSILON) /
                    (1.0 - 6.0 * std::sqrt(static_cast<double>(EPSILON)) -
                        7.0 * EPSILON)) *
                    S);
        }

        ///Add @p x to the binned fp
        void add(ftype const x)
        {
            binned_dmdadd(x, 1, 1);
        }

        ///Add arithmetics in the range [first, last) to the binned fp
        ///
        ///@param first       Start of range
        ///@param last        End of range
        ///@param max_abs_val Maximum absolute value of any member of the range
        template <typename InputIt>
        void add(InputIt first, InputIt last, ftype const max_abs_val)
        {
            binned_dmdupdate(std::abs(max_abs_val), 1, 1);
            size_t count = 0;
            size_t N = last - first;
            for (; first != last; first++, count++)
            {
                binned_dmddeposit(static_cast<ftype>(*first), 1);
                // first conditional allows compiler to remove the call here when possible
                if (N > ENDURANCE && count == ENDURANCE)
                {
                    binned_dmrenorm(1, 1);
                    count = 0;
                }
            }
        }

        ///Add arithmetics in the range [first, last) to the binned fp
        ///
        ///NOTE: A maximum absolute value is calculated, so two passes are made over
        ///      the data
        ///
        ///@param first       Start of range
        ///@param last        End of range
        template <typename InputIt>
        void add(InputIt first, InputIt last)
        {
            auto const max_abs_val = *std::max_element(
                first, last, [](auto const& a, auto const& b) {
                    return std::abs(a) < std::abs(b);
                });
            add(first, last, static_cast<ftype>(max_abs_val));
        }

        ///Add @p N elements starting at @p input to the binned fp: [input, input+N)
        ///
        ///@param input       Start of the range
        ///@param N           Number of elements to add
        ///@param max_abs_val Maximum absolute value of any member of the range
        template <typename T,
            typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
        void add(T const* input, size_t const N, ftype const max_abs_val)
        {
            if (N == 0)
                return;
            add(input, input + N, max_abs_val);
        }

        ///Add @p N elements starting at @p input to the binned fp: [input, input+N)
        ///
        ///NOTE: A maximum absolute value is calculated, so two passes are made over
        ///      the data
        ///
        ///@param input       Start of the range
        ///@param N           Number of elements to add
        template <typename T,
            typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
        void add(T const* input, size_t const N)
        {
            if (N == 0)
                return;

            T max_abs_val = input[0];
            for (size_t i = 0; i < N; i++)
            {
                max_abs_val = (std::max) (max_abs_val, std::abs(input[i]));
            }
            add(input, N, max_abs_val);
        }

        ///Accumulate a float4 @p x into the binned fp.
        ///NOTE: Casts @p x to the type of the binned fp
        reproducible_floating_accumulator& operator+=(float4 const& x)
        {
            binned_dmdupdate(abs_max(x), 1, 1);
            binned_dmddeposit(static_cast<ftype>(x.x), 1);
            binned_dmddeposit(static_cast<ftype>(x.y), 1);
            binned_dmddeposit(static_cast<ftype>(x.z), 1);
            binned_dmddeposit(static_cast<ftype>(x.w), 1);
            return *this;
        }

        ///Accumulate a double2 @p x into the binned fp.
        ///NOTE: Casts @p x to the type of the binned fp
        reproducible_floating_accumulator& operator+=(float2 const& x)
        {
            binned_dmdupdate(abs_max(x), 1, 1);
            binned_dmddeposit(static_cast<ftype>(x.x), 1);
            binned_dmddeposit(static_cast<ftype>(x.y), 1);
            return *this;
        }

        ///Accumulate a double2 @p x into the binned fp.
        ///NOTE: Casts @p x to the type of the binned fp
        reproducible_floating_accumulator& operator+=(double2 const& x)
        {
            binned_dmdupdate(abs_max(x), 1, 1);
            binned_dmddeposit(static_cast<ftype>(x.x), 1);
            binned_dmddeposit(static_cast<ftype>(x.y), 1);
            return *this;
        }

        void add(float4 const* input, size_t const N, float max_abs_val)
        {
            if (N == 0)
                return;
            binned_dmdupdate(max_abs_val, 1, 1);

            size_t count = 0;
            for (size_t i = 0; i < N; i++)
            {
                binned_dmddeposit(static_cast<ftype>(input[i].x), 1);
                binned_dmddeposit(static_cast<ftype>(input[i].y), 1);
                binned_dmddeposit(static_cast<ftype>(input[i].z), 1);
                binned_dmddeposit(static_cast<ftype>(input[i].w), 1);

                if (N > ENDURANCE && count == ENDURANCE)
                {
                    binned_dmrenorm(1, 1);
                    count = 0;
                }
            }
        }

        void add(double2 const* input, size_t const N, double max_abs_val)
        {
            if (N == 0)
                return;
            binned_dmdupdate(max_abs_val, 1, 1);

            size_t count = 0;
            for (size_t i = 0; i < N; i++)
            {
                binned_dmddeposit(static_cast<ftype>(input[i].x), 1);
                binned_dmddeposit(static_cast<ftype>(input[i].y), 1);

                if (N > ENDURANCE && count == ENDURANCE)
                {
                    binned_dmrenorm(1, 1);
                    count = 0;
                }
            }
        }

        void add(float2 const* input, size_t const N, double max_abs_val)
        {
            if (N == 0)
                return;
            binned_dmdupdate(max_abs_val, 1, 1);

            size_t count = 0;
            for (size_t i = 0; i < N; i++)
            {
                binned_dmddeposit(static_cast<ftype>(input[i].x), 1);
                binned_dmddeposit(static_cast<ftype>(input[i].y), 1);

                if (N > ENDURANCE && count == ENDURANCE)
                {
                    binned_dmrenorm(1, 1);
                    count = 0;
                }
            }
        }

        void add(float4 const* input, size_t const N)
        {
            if (N == 0)
                return;

            auto max_abs_val = abs_max(input[0]);
            for (size_t i = 1; i < N; i++)
                max_abs_val =
                    fmaxf((float) max_abs_val, (float) abs_max(input[i]));

            add(input, N, max_abs_val);
        }

        void add(double2 const* input, size_t const N)
        {
            if (N == 0)
                return;

            auto max_abs_val = abs_max(input[0]);
            for (size_t i = 1; i < N; i++)
                max_abs_val =
                    fmax((double) max_abs_val, (double) abs_max(input[i]));

            add(input, N, max_abs_val);
        }

        void add(float2 const* input, size_t const N)
        {
            if (N == 0)
                return;

            auto max_abs_val = abs_max(input[0]);
            for (size_t i = 1; i < N; i++)
                max_abs_val =
                    fmaxf((float) max_abs_val, (float) abs_max(input[i]));

            add(input, N, max_abs_val);
        }

        //////////////////////////////////////
        //MANUAL OPERATIONS; USE WISELY
        //////////////////////////////////////

        ///Rebins for repeated accumulation of scalars with magnitude <= @p mav
        ///
        ///Once rebinned, `ENDURANCE` values <= @p mav can be added to the accumulator
        ///with `unsafe_add` after which `renorm()` must be called. See the source of
        ///`add()` for an example
        template <typename T,
            typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
        void set_max_abs_val(T const mav)
        {
            binned_dmdupdate(std::abs(mav), 1, 1);
        }

        ///Add @p x to the binned fp
        ///
        ///This is intended to be used after a call to `set_max_abs_val()`
        void unsafe_add(ftype const x)
        {
            binned_dmddeposit(x, 1);
        }

        ///Renormalizes the binned fp
        ///
        ///This is intended to be used after a call to `set_max_abs_val()` and one or
        ///more calls to `unsafe_add()`
        void renorm()
        {
            binned_dmrenorm(1, 1);
        }
    };
}    // namespace hpx::parallel::detail::rfa
