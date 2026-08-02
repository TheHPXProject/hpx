//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST(...)                                                          \
    HPX_TEST_(__VA_ARGS__)                                                     \
    /**/

#define HPX_TEST_(...)                                                         \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))         \
    /**/
#define HPX_TEST_1(expr)                                                       \
    HPX_TEST_IMPL(::hpx::util::detail::global_fixture(), expr)
#define HPX_TEST_2(strm, expr)                                                 \
    HPX_TEST_IMPL(::hpx::util::detail::fixture{strm}, expr)

#define HPX_TEST_IMPL(fixture, expr)                                           \
    fixture.check_(__FILE__, __LINE__, __func__,                               \
        ::hpx::util::counter_type::test, expr,                                 \
        "test '" HPX_PP_STRINGIZE(expr) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_MSG(...)                                                      \
    HPX_TEST_MSG_(__VA_ARGS__)                                                 \
    /**/

#define HPX_TEST_MSG_(...)                                                     \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))     \
    /**/
#define HPX_TEST_MSG_2(expr, msg)                                              \
    HPX_TEST_MSG_IMPL(::hpx::util::detail::global_fixture(), expr, msg)
#define HPX_TEST_MSG_3(strm, expr, msg)                                        \
    HPX_TEST_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr, msg)

#define HPX_TEST_MSG_IMPL(fixture, expr, msg)                                  \
    fixture.check_(__FILE__, __LINE__, __func__,                               \
        ::hpx::util::counter_type::test, expr, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_EQ(...)                                                       \
    HPX_TEST_EQ_(__VA_ARGS__)                                                  \
    /**/

#define HPX_TEST_EQ_(...)                                                      \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_EQ_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))      \
    /**/
#define HPX_TEST_EQ_2(expr1, expr2)                                            \
    HPX_TEST_EQ_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_TEST_EQ_3(strm, expr1, expr2)                                      \
    HPX_TEST_EQ_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_TEST_EQ_IMPL(fixture, expr1, expr2)                                \
    fixture.check_equal(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::test, expr1, expr2,                         \
        "test '" HPX_PP_STRINGIZE(expr1) " == " HPX_PP_STRINGIZE(expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_NEQ(...)                                                      \
    HPX_TEST_NEQ_(__VA_ARGS__)                                                 \
    /**/

#define HPX_TEST_NEQ_(...)                                                     \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_NEQ_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))     \
    /**/
#define HPX_TEST_NEQ_2(expr1, expr2)                                           \
    HPX_TEST_NEQ_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_TEST_NEQ_3(strm, expr1, expr2)                                     \
    HPX_TEST_NEQ_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_TEST_NEQ_IMPL(fixture, expr1, expr2)                               \
    fixture.check_not_equal(__FILE__, __LINE__, __func__,                      \
        ::hpx::util::counter_type::test, expr1, expr2,                         \
        "test '" HPX_PP_STRINGIZE(expr1) " != " HPX_PP_STRINGIZE(expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_LT(...)                                                       \
    HPX_TEST_LT_(__VA_ARGS__)                                                  \
    /**/

#define HPX_TEST_LT_(...)                                                      \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_LT_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))      \
    /**/
#define HPX_TEST_LT_2(expr1, expr2)                                            \
    HPX_TEST_LT_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_TEST_LT_3(strm, expr1, expr2)                                      \
    HPX_TEST_LT_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_TEST_LT_IMPL(fixture, expr1, expr2)                                \
    fixture.check_less(__FILE__, __LINE__, __func__,                           \
        ::hpx::util::counter_type::test, expr1, expr2,                         \
        "test '" HPX_PP_STRINGIZE(expr1) " < " HPX_PP_STRINGIZE(expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_LTE(...)                                                      \
    HPX_TEST_LTE_(__VA_ARGS__)                                                 \
    /**/

#define HPX_TEST_LTE_(...)                                                     \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_LTE_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))     \
    /**/
#define HPX_TEST_LTE_2(expr1, expr2)                                           \
    HPX_TEST_LTE_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_TEST_LTE_3(strm, expr1, expr2)                                     \
    HPX_TEST_LTE_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_TEST_LTE_IMPL(fixture, expr1, expr2)                               \
    fixture.check_less_equal(__FILE__, __LINE__, __func__,                     \
        ::hpx::util::counter_type::test, expr1, expr2,                         \
        "test '" HPX_PP_STRINGIZE(expr1) " <= " HPX_PP_STRINGIZE(expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_RANGE(...)                                                    \
    HPX_TEST_RANGE_(__VA_ARGS__)                                               \
    /**/

#define HPX_TEST_RANGE_(...)                                                   \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_RANGE_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/
#define HPX_TEST_RANGE_3(expr1, expr2, expr3)                                  \
    HPX_TEST_RANGE_IMPL(                                                       \
        ::hpx::util::detail::global_fixture(), expr1, expr2, expr3)
#define HPX_TEST_RANGE_4(strm, expr1, expr2, expr3)                            \
    HPX_TEST_RANGE_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2, expr3)

#define HPX_TEST_RANGE_IMPL(fixture, expr1, expr2, expr3)                      \
    fixture.check_range(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::test, expr1, expr2, expr3,                  \
        "test '" HPX_PP_STRINGIZE(expr2) " <= " HPX_PP_STRINGIZE(              \
            expr1) " <= " HPX_PP_STRINGIZE(expr3) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_EQ_MSG(...)                                                   \
    HPX_TEST_EQ_MSG_(__VA_ARGS__)                                              \
    /**/

#define HPX_TEST_EQ_MSG_(...)                                                  \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_EQ_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))  \
    /**/
#define HPX_TEST_EQ_MSG_3(expr1, expr2, msg)                                   \
    HPX_TEST_EQ_MSG_IMPL(                                                      \
        ::hpx::util::detail::global_fixture(), expr1, expr2, msg)
#define HPX_TEST_EQ_MSG_4(strm, expr1, expr2, msg)                             \
    HPX_TEST_EQ_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2, msg)

#define HPX_TEST_EQ_MSG_IMPL(fixture, expr1, expr2, msg)                       \
    fixture.check_equal(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::test, expr1, expr2, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_NEQ_MSG(...)                                                  \
    HPX_TEST_NEQ_MSG_(__VA_ARGS__)                                             \
    /**/

#define HPX_TEST_NEQ_MSG_(...)                                                 \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_NEQ_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)) \
    /**/
#define HPX_TEST_NEQ_MSG_3(expr1, expr2, msg)                                  \
    HPX_TEST_NEQ_MSG_IMPL(                                                     \
        ::hpx::util::detail::global_fixture(), expr1, expr2, msg)
#define HPX_TEST_NEQ_MSG_4(strm, expr1, expr2, msg)                            \
    HPX_TEST_NEQ_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2, msg)

#define HPX_TEST_NEQ_MSG_IMPL(fixture, expr1, expr2, msg)                      \
    fixture.check_not_equal(__FILE__, __LINE__, __func__,                      \
        ::hpx::util::counter_type::test, expr1, expr2, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_LT_MSG(...)                                                   \
    HPX_TEST_LT_MSG_(__VA_ARGS__)                                              \
    /**/

#define HPX_TEST_LT_MSG_(...)                                                  \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_LT_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))  \
    /**/
#define HPX_TEST_LT_MSG_3(expr1, expr2, msg)                                   \
    HPX_TEST_LT_MSG_IMPL(                                                      \
        ::hpx::util::detail::global_fixture(), expr1, expr2, msg)
#define HPX_TEST_LT_MSG_4(strm, expr1, expr2, msg)                             \
    HPX_TEST_LT_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2, msg)

#define HPX_TEST_LT_MSG_IMPL(fixture, expr1, expr2, msg)                       \
    fixture.check_less(__FILE__, __LINE__, __func__,                           \
        ::hpx::util::counter_type::test, expr1, expr2, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_LTE_MSG(...)                                                  \
    HPX_TEST_LTE_MSG_(__VA_ARGS__)                                             \
    /**/

#define HPX_TEST_LTE_MSG_(...)                                                 \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_LTE_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)) \
    /**/
#define HPX_TEST_LTE_MSG_3(expr1, expr2, msg)                                  \
    HPX_TEST_LTE_MSG_IMPL(                                                     \
        ::hpx::util::detail::global_fixture(), expr1, expr2, msg)
#define HPX_TEST_LTE_MSG_4(strm, expr1, expr2, msg)                            \
    HPX_TEST_LTE_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2, msg)

#define HPX_TEST_LTE_MSG_IMPL(fixture, expr1, expr2, msg)                      \
    fixture.check_less_equal(__FILE__, __LINE__, __func__,                     \
        ::hpx::util::counter_type::test, expr1, expr2, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_RANGE_MSG(...)                                                \
    HPX_TEST_RANGE_MSG_(__VA_ARGS__)                                           \
    /**/

#define HPX_TEST_RANGE_MSG_(...)                                               \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_TEST_RANGE_MSG_, HPX_PP_NARGS(__VA_ARGS__))(  \
        __VA_ARGS__))                                                          \
    /**/
#define HPX_TEST_RANGE_MSG_4(expr1, expr2, expr3, msg)                         \
    HPX_TEST_RANGE_MSG_IMPL(                                                   \
        ::hpx::util::detail::global_fixture(), expr1, expr2, expr3, msg)
#define HPX_TEST_RANGE_MSG_5(strm, expr1, expr2, expr3, msg)                   \
    HPX_TEST_RANGE_MSG_IMPL(                                                   \
        ::hpx::util::detail::fixture{strm}, expr1, expr2, expr3, msg)

#define HPX_TEST_RANGE_MSG_IMPL(fixture, expr1, expr2, expr3, msg)             \
    fixture.check_range(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::test, expr1, expr2, expr3, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY(...)                                                        \
    HPX_SANITY_(__VA_ARGS__)                                                   \
    /**/

#define HPX_SANITY_(...)                                                       \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))       \
    /**/
#define HPX_SANITY_1(expr)                                                     \
    HPX_TEST_IMPL(::hpx::util::detail::global_fixture(), expr)
#define HPX_SANITY_2(strm, expr)                                               \
    HPX_SANITY_IMPL(::hpx::util::detail::fixture{strm}, expr)

#define HPX_SANITY_IMPL(fixture, expr)                                         \
    fixture.check_(__FILE__, __LINE__, __func__,                               \
        ::hpx::util::counter_type::sanity, expr,                               \
        "sanity check '" HPX_PP_STRINGIZE(expr) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_MSG(...)                                                    \
    HPX_SANITY_MSG_(__VA_ARGS__)                                               \
    /**/

#define HPX_SANITY_MSG_(...)                                                   \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_MSG_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/
#define HPX_SANITY_MSG_2(expr, msg)                                            \
    HPX_SANITY_MSG_IMPL(::hpx::util::detail::global_fixture(), expr, msg)
#define HPX_SANITY_MSG_3(strm, expr, msg)                                      \
    HPX_SANITY_MSG_IMPL(::hpx::util::detail::fixture{strm}, expr, msg)

#define HPX_SANITY_MSG_IMPL(fixture, expr, msg)                                \
    fixture.check_(__FILE__, __LINE__, __func__,                               \
        ::hpx::util::counter_type::sanity, expr, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_EQ(...)                                                     \
    HPX_SANITY_EQ_(__VA_ARGS__)                                                \
    /**/

#define HPX_SANITY_EQ_(...)                                                    \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_EQ_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))    \
    /**/
#define HPX_SANITY_EQ_2(expr1, expr2)                                          \
    HPX_SANITY_EQ_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_SANITY_EQ_3(strm, expr1, expr2)                                    \
    HPX_SANITY_EQ_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_SANITY_EQ_IMPL(fixture, expr1, expr2)                              \
    fixture.check_equal(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::sanity, expr1, expr2,                       \
        "sanity check '" HPX_PP_STRINGIZE(expr1) " == " HPX_PP_STRINGIZE(      \
            expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_NEQ(...)                                                    \
    HPX_SANITY_NEQ_(__VA_ARGS__)                                               \
    /**/

#define HPX_SANITY_NEQ_(...)                                                   \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_NEQ_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/
#define HPX_SANITY_NEQ_2(expr1, expr2)                                         \
    HPX_SANITY_NEQ_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_SANITY_NEQ_3(strm, expr1, expr2)                                   \
    HPX_SANITY_NEQ_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_SANITY_NEQ_IMPL(fixture, expr1, expr2)                             \
    fixture.check_not_equal(__FILE__, __LINE__, __func__,                      \
        ::hpx::util::counter_type::sanity, expr1, expr2,                       \
        "sanity check '" HPX_PP_STRINGIZE(expr1) " != " HPX_PP_STRINGIZE(      \
            expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_LT(...)                                                     \
    HPX_SANITY_LT_(__VA_ARGS__)                                                \
    /**/

#define HPX_SANITY_LT_(...)                                                    \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_LT_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))    \
    /**/
#define HPX_SANITY_LT_2(expr1, expr2)                                          \
    HPX_SANITY_LT_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_SANITY_LT_3(strm, expr1, expr2)                                    \
    HPX_SANITY_LT_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_SANITY_LT_IMPL(fixture, expr1, expr2)                              \
    fixture.check_less(__FILE__, __LINE__, __func__,                           \
        ::hpx::util::counter_type::sanity, expr1, expr2,                       \
        "sanity check '" HPX_PP_STRINGIZE(expr1) " < " HPX_PP_STRINGIZE(       \
            expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_LTE(...)                                                    \
    HPX_SANITY_LTE_(__VA_ARGS__)                                               \
    /**/

#define HPX_SANITY_LTE_(...)                                                   \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_LTE_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/
#define HPX_SANITY_LTE_2(expr1, expr2)                                         \
    HPX_SANITY_LTE_IMPL(::hpx::util::detail::global_fixture(), expr1, expr2)
#define HPX_SANITY_LTE_3(strm, expr1, expr2)                                   \
    HPX_SANITY_LTE_IMPL(::hpx::util::detail::fixture{strm}, expr1, expr2)

#define HPX_SANITY_LTE_IMPL(fixture, expr1, expr2)                             \
    fixture.check_less_equal(__FILE__, __LINE__, __func__,                     \
        ::hpx::util::counter_type::sanity, expr1, expr2,                       \
        "sanity check '" HPX_PP_STRINGIZE(expr1) " <= " HPX_PP_STRINGIZE(      \
            expr2) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_RANGE(...)                                                  \
    HPX_SANITY_RANGE_(__VA_ARGS__)                                             \
    /**/

#define HPX_SANITY_RANGE_(...)                                                 \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SANITY_RANGE_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)) \
    /**/
#define HPX_SANITY_RANGE_3(expr1, expr2, expr3)                                \
    HPX_SANITY_RANGE_IMPL(                                                     \
        ::hpx::util::detail::global_fixture(), expr1, expr2, expr3)
#define HPX_SANITY_RANGE_4(strm, expr1, expr2, expr3)                          \
    HPX_SANITY_RANGE_IMPL(                                                     \
        ::hpx::util::detail::fixture{strm}, expr1, expr2, expr3)

#define HPX_SANITY_RANGE_IMPL(fixture, expr1, expr2, expr3)                    \
    fixture.check_range(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::sanity, expr1, expr2, expr3,                \
        "sanity check '" HPX_PP_STRINGIZE(expr2) " <= " HPX_PP_STRINGIZE(      \
            expr1) " <= " HPX_PP_STRINGIZE(expr3) "'")

////////////////////////////////////////////////////////////////////////////////
#define HPX_SANITY_EQ_MSG(...)                                                 \
    HPX_SANITY_EQ_MSG_(__VA_ARGS__)                                            \
    /**/

#define HPX_SANITY_EQ_MSG_(...)                                                \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SANITY_EQ_MSG_, HPX_PP_NARGS(__VA_ARGS__))(   \
        __VA_ARGS__))                                                          \
    /**/
#define HPX_SANITY_EQ_MSG_3(expr1, expr2, msg)                                 \
    HPX_SANITY_EQ_MSG_IMPL(                                                    \
        ::hpx::util::detail::global_fixture(), expr1, expr2, msg)
#define HPX_SANITY_EQ_MSG_4(strm, expr1, expr2, msg)                           \
    HPX_SANITY_EQ_MSG_IMPL(                                                    \
        ::hpx::util::detail::fixture{strm}, expr1, expr2, msg)

#define HPX_SANITY_EQ_MSG_IMPL(fixture, expr1, expr2, msg)                     \
    fixture.check_equal(__FILE__, __LINE__, __func__,                          \
        ::hpx::util::counter_type::sanity, expr1, expr2, msg)

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_THROW(...)                                                    \
    HPX_TEST_THROW_(__VA_ARGS__)                                               \
    /**/

#define HPX_TEST_THROW_(...)                                                   \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_TEST_THROW_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/
#define HPX_TEST_THROW_2(expression, exception)                                \
    HPX_TEST_THROW_IMPL(                                                       \
        ::hpx::util::detail::global_fixture(), expression, exception)
#define HPX_TEST_THROW_3(strm, expression, exception)                          \
    HPX_TEST_THROW_IMPL(                                                       \
        ::hpx::util::detail::fixture{strm}, expression, exception)

#define HPX_TEST_THROW_IMPL(fixture, expression, exception)                    \
    do                                                                         \
    {                                                                          \
        bool caught_exception = false;                                         \
        try                                                                    \
        {                                                                      \
            expression;                                                        \
            HPX_TEST_MSG_IMPL(                                                 \
                fixture, false, "expected exception not thrown");              \
        }                                                                      \
        catch (exception&)                                                     \
        {                                                                      \
            caught_exception = true;                                           \
        }                                                                      \
        catch (...)                                                            \
        {                                                                      \
            HPX_TEST_MSG_IMPL(fixture, false, "unexpected exception caught");  \
        }                                                                      \
        HPX_TEST_IMPL(fixture, caught_exception);                              \
    } while (false) /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_TEST_NO_THROW(...)                                                 \
    HPX_TEST_NO_THROW_(__VA_ARGS__)                                            \
    /**/

#define HPX_TEST_NO_THROW_(...)                                                \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_TEST_NO_THROW_, HPX_PP_NARGS(__VA_ARGS__))(   \
        __VA_ARGS__))                                                          \
    /**/
#define HPX_TEST_NO_THROW_1(expression)                                        \
    HPX_TEST_NO_THROW_IMPL(::hpx::util::detail::global_fixture(), expression)

#define HPX_TEST_NO_THROW_IMPL(fixture, expression)                            \
    do                                                                         \
    {                                                                          \
        bool caught_exception = false;                                         \
        try                                                                    \
        {                                                                      \
            expression;                                                        \
        }                                                                      \
        catch (...)                                                            \
        {                                                                      \
            caught_exception = true;                                           \
        }                                                                      \
        HPX_TEST_MSG_IMPL(                                                     \
            fixture, !caught_exception, "unexpected exception caught");        \
    } while (false) /**/
