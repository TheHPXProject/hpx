//  Copyright (c) 2013-2014 Thomas Heller
//  Copyright (c) 2013-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file broadcast_direct.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

///////////////////////////////////////////////////////////////////////////////
// from collectives/broadcast_direct.hpp
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION(...)                    \
    HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_(__VA_ARGS__)               \
/**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_(...)                   \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_,  \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_1(Action)               \
    HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_2(Action, Action)           \
/**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_2(Action, Name)         \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_broadcast_post_action<Action>::type,         \
        HPX_PP_CAT(broadcast_post_, Name))                                     \
    HPX_REGISTER_APPLY_COLOCATED_DECLARATION(                                  \
        ::hpx::lcos::detail::make_broadcast_post_action<Action>::type,         \
        HPX_PP_CAT(post_colocated_broadcast_, Name))                           \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_ACTION(...)                                \
    HPX_REGISTER_BROADCAST_POST_ACTION_(__VA_ARGS__)                           \
/**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_(...)                               \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_POST_ACTION_,              \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_POST_ACTION_1(Action)                           \
    HPX_REGISTER_BROADCAST_POST_ACTION_2(Action, Action)                       \
/**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_2(Action, Name)                     \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_broadcast_post_action<Action>::type,         \
        HPX_PP_CAT(broadcast_post_, Name))                                     \
    HPX_REGISTER_APPLY_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_post_action<Action>::type,         \
        HPX_PP_CAT(post_colocated_broadcast_, Name))                           \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION(...)         \
    HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_(__VA_ARGS__)    \
/**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_(...)        \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_, \
            HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                           \
    /**/

#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_1(Action)    \
    HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_2(               \
        Action, Action)                                                        \
/**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_2(           \
    Action, Name)                                                              \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_broadcast_post_action<                       \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(broadcast_post_with_index_, Name))                          \
    HPX_REGISTER_APPLY_COLOCATED_DECLARATION(                                  \
        ::hpx::lcos::detail::make_broadcast_post_action<                       \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(post_colocated_broadcast_with_index_, Name))                \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION(...)                     \
    HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_(__VA_ARGS__)                \
/**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_(...)                    \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_,   \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_1(Action)                \
    HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_2(Action, Action)            \
/**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_2(Action, Name)          \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_broadcast_post_action<                       \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(broadcast_post_with_index_, Name))                          \
    HPX_REGISTER_APPLY_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_post_action<                       \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(post_colocated_broadcast_with_index_, Name))                \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION(...)                         \
    HPX_REGISTER_BROADCAST_ACTION_DECLARATION_(__VA_ARGS__)                    \
/**/
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_(...)                        \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_ACTION_DECLARATION_,       \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_1(Action)                    \
    HPX_REGISTER_BROADCAST_ACTION_DECLARATION_2(Action, Action)                \
/**/
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_2(Action, Name)              \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(broadcast_, Name))                                          \
    HPX_REGISTER_ASYNC_COLOCATED_DECLARATION(                                  \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(async_colocated_broadcast_, Name))                          \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_ACTION(...)                                     \
    HPX_REGISTER_BROADCAST_ACTION_(__VA_ARGS__)                                \
/**/
#define HPX_REGISTER_BROADCAST_ACTION_(...)                                    \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_ACTION_,                   \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_ACTION_1(Action)                                \
    HPX_REGISTER_BROADCAST_ACTION_2(Action, Action)                            \
/**/
#define HPX_REGISTER_BROADCAST_ACTION_2(Action, Name)                          \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(broadcast_, Name))                                          \
    HPX_REGISTER_ASYNC_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(async_colocated_broadcast_, Name))                          \
/**/
#define HPX_REGISTER_BROADCAST_ACTION_ID(Action, Name, Id)                     \
    HPX_REGISTER_ACTION_ID(                                                    \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(broadcast_, Name), Id)                                      \
    HPX_REGISTER_ASYNC_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_action<Action>::type,              \
        HPX_PP_CAT(async_colocated_broadcast_, Name))                          \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION(...)              \
    HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_(__VA_ARGS__)         \
/**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_(...)             \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_,      \
            HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                           \
    /**/

#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_1(Action)         \
    HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_2(Action, Action)     \
/**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_2(Action, Name)   \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(broadcast_with_index_, Name))                               \
    HPX_REGISTER_ASYNC_COLOCATED_DECLARATION(                                  \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(async_colocated_broadcast_with_index_, Name))               \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION(...)                          \
    HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_(__VA_ARGS__)                     \
/**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_(...)                         \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_,        \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_1(Action)                     \
    HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_2(Action, Action)                 \
/**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_2(Action, Name)               \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(broadcast_with_index_, Name))                               \
    HPX_REGISTER_ASYNC_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(async_colocated_broadcast_with_index_, Name))               \
/**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_ID(Action, Name, Id)          \
    HPX_REGISTER_ACTION_ID(                                                    \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(broadcast_with_index_, Name), Id)                           \
    HPX_REGISTER_ASYNC_COLOCATED(                                              \
        ::hpx::lcos::detail::make_broadcast_action<                            \
            ::hpx::lcos::detail::broadcast_with_index<Action>>::type,          \
        HPX_PP_CAT(async_colocated_broadcast_with_index_, Name))               \
    /**/
#else

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION(...)  /**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_(...) /**/

#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_1(Action)       /**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_DECLARATION_2(Action, Name) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_ACTION(...)                        /**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_(...)                       /**/

#define HPX_REGISTER_BROADCAST_POST_ACTION_1(Action)                    /**/
#define HPX_REGISTER_BROADCAST_POST_ACTION_2(Action, Name)              /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION(...)  /**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_(...) /**/

#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_1(Action) /**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_DECLARATION_2(           \
    Action, Name)                                           /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION(...)  /**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_(...) /**/

#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_1(Action)       /**/
#define HPX_REGISTER_BROADCAST_POST_WITH_INDEX_ACTION_2(Action, Name) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION(...)                /**/
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_(...)               /**/

#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_1(Action)       /**/
#define HPX_REGISTER_BROADCAST_ACTION_DECLARATION_2(Action, Name) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_ACTION(...)                        /**/
#define HPX_REGISTER_BROADCAST_ACTION_(...)                       /**/

#define HPX_REGISTER_BROADCAST_ACTION_1(Action)                    /**/
#define HPX_REGISTER_BROADCAST_ACTION_2(Action, Name)              /**/
#define HPX_REGISTER_BROADCAST_ACTION_ID(Action, Name, Id)         /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION(...)  /**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_(...) /**/

#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_1(Action) /**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_DECLARATION_2(                \
    Action, Name)                                      /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION(...)  /**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_(...) /**/

#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_1(Action)            /**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_2(Action, Name)      /**/
#define HPX_REGISTER_BROADCAST_WITH_INDEX_ACTION_ID(Action, Name, Id) /**/

#endif    //COMPUTE_DEVICE_CODE

////////////////////////////////////////////////////////////////////////////////
// from collectives/fold.hpp
#define HPX_REGISTER_FOLD_ACTION_DECLARATION(...)                              \
    HPX_REGISTER_FOLD_ACTION_DECLARATION_(__VA_ARGS__)                         \
/**/
#define HPX_REGISTER_FOLD_ACTION_DECLARATION_(...)                             \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_FOLD_ACTION_DECLARATION_,            \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_FOLD_ACTION_DECLARATION_2(Action, FoldOp)                 \
    HPX_REGISTER_ACTION_DECLARATION(::hpx::lcos::detail::make_fold_action<     \
                                        Action>::fold_invoker<FoldOp>::type,   \
        HPX_PP_CAT(HPX_PP_CAT(fold_, Action), FoldOp))                         \
/**/
#define HPX_REGISTER_FOLD_ACTION_DECLARATION_3(Action, FoldOp, Name)           \
    HPX_REGISTER_ACTION_DECLARATION(::hpx::lcos::detail::make_fold_action<     \
                                        Action>::fold_invoker<FoldOp>::type,   \
        HPX_PP_CAT(fold_, Name))                                               \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_FOLD_ACTION(...)                                          \
    HPX_REGISTER_FOLD_ACTION_(__VA_ARGS__)                                     \
/**/
#define HPX_REGISTER_FOLD_ACTION_(...)                                         \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_REGISTER_FOLD_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))    \
    /**/

#define HPX_REGISTER_FOLD_ACTION_2(Action, FoldOp)                             \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_fold_action<                 \
                            Action>::fold_invoker<FoldOp>::type,               \
        HPX_PP_CAT(HPX_PP_CAT(fold_, Action), FoldOp))                         \
/**/
#define HPX_REGISTER_FOLD_ACTION_3(Action, FoldOp, Name)                       \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_fold_action<                 \
                            Action>::fold_invoker<FoldOp>::type,               \
        HPX_PP_CAT(fold_, Name))                                               \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION(...)                   \
    HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION_(__VA_ARGS__)              \
/**/
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION_(...)                  \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION_, \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION_2(Action, FoldOp)      \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_fold_action<::hpx::lcos::detail::            \
                fold_with_index<Action>>::fold_invoker<FoldOp>::type,          \
        HPX_PP_CAT(HPX_PP_CAT(fold_, Action), FoldOp))                         \
/**/
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_DECLARATION_3(                     \
    Action, FoldOp, Name)                                                      \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_fold_action<::hpx::lcos::detail::            \
                fold_with_index<Action>>::fold_invoker<FoldOp>::type,          \
        HPX_PP_CAT(fold_, Name))                                               \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION(...)                               \
    HPX_REGISTER_FOLD_WITH_INDEX_ACTION_(__VA_ARGS__)                          \
/**/
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_(...)                              \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_FOLD_WITH_INDEX_ACTION_,             \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_2(Action, FoldOp)                  \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_fold_action<::hpx::lcos::detail::            \
                fold_with_index<Action>>::fold_invoker<FoldOp>::type,          \
        HPX_PP_CAT(HPX_PP_CAT(fold_, Action), FoldOp))                         \
/**/
#define HPX_REGISTER_FOLD_WITH_INDEX_ACTION_3(Action, FoldOp, Name)            \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::detail::make_fold_action<::hpx::lcos::detail::            \
                fold_with_index<Action>>::fold_invoker<FoldOp>::type,          \
        HPX_PP_CAT(fold_, Name))                                               \
    /**/

////////////////////////////////////////////////////////////////////////////////
// from collectives/reduce.hpp
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION(...)                            \
    HPX_REGISTER_REDUCE_ACTION_DECLARATION_(__VA_ARGS__)                       \
/**/
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_REDUCE_ACTION_DECLARATION_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_2(Action, ReduceOp)             \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            Action>::reduce_invoker_helper<ReduceOp>::type,                    \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_3(Action, ReduceOp, Name)       \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            Action>::reduce_invoker_helper<ReduceOp>::type,                    \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_ACTION(...)                                        \
    HPX_REGISTER_REDUCE_ACTION_(__VA_ARGS__)                                   \
/**/
#define HPX_REGISTER_REDUCE_ACTION_(...)                                       \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_REGISTER_REDUCE_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))  \
    /**/

#define HPX_REGISTER_REDUCE_ACTION_2(Action, ReduceOp)                         \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            Action>::reduce_invoker_helper<ReduceOp>::type,    \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_ACTION_3(Action, ReduceOp, Name)                   \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            Action>::reduce_invoker_helper<ReduceOp>::type,    \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION(...)                 \
    HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_(__VA_ARGS__)            \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_(...)                \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_,         \
            HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                           \
    /**/

#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_2(Action, ReduceOp)  \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            ::hpx::lcos::detail::reduce_with_index<Action>>::                  \
            reduce_invoker_helper<ReduceOp>::type,                             \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_3(                   \
    Action, ReduceOp, Name)                                                    \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            ::hpx::lcos::detail::reduce_with_index<Action>>::                  \
            reduce_invoker_helper<ReduceOp>::type,                             \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION(...)                             \
    HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_(__VA_ARGS__)                        \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_(...)                            \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_,           \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_2(Action, ReduceOp)              \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            ::hpx::lcos::detail::reduce_with_index<Action>>::  \
                            reduce_invoker_helper<ReduceOp>::type,             \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_3(Action, ReduceOp, Name)        \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            ::hpx::lcos::detail::reduce_with_index<Action>>::  \
                            reduce_invoker_helper<ReduceOp>::type,             \
        HPX_PP_CAT(reduce_, Name))                                             \
    /**/
