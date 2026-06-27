//  Copyright (c) 2014 Anuj R. Sharma
//  Copyright (c) 2014-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/partitioned_vector/partitioned_vector_component_decl.hpp

#pragma once

/// \brief The partitioned_vector_partition as the hpx component is defined
///        here.
///
/// The partitioned_vector_partition is the wrapper to the stl vector class
/// except all APIs are defined as component action. All the APIs in stubs
/// classes are asynchronous API which return the futures.

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_fwd.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::detail {

    HPX_HAS_XXX_TRAIT_DEF(allocator_type);

    template <typename T, typename Data, typename Enable = void>
    struct extract_allocator_type
    {
        using type = std::allocator<T>;
    };

    template <typename T, typename Data>
    struct extract_allocator_type<T, Data,
        std::enable_if_t<has_allocator_type_v<Data>>>
    {
        using type = typename Data::allocator_type;
    };

    template <typename T, typename Data>
    using extract_allocator_type_t =
        typename extract_allocator_type<T, Data>::type;
}    // namespace hpx::detail

namespace hpx::server {

    /// \brief This is the basic wrapper class for stl vector.
    ///
    /// This contains the implementation of the partitioned_vector partition's
    /// component functionality.
    template <typename T, typename Data>
    class partitioned_vector
      : public components::locking_hook<
            components::component_base<partitioned_vector<T, Data>>>
    {
    public:
        using data_type = std::decay_t<Data>;

        using allocator_type = hpx::detail::extract_allocator_type_t<T, Data>;
        using size_type = typename data_type::size_type;
        using iterator_type = typename data_type::iterator;
        using const_iterator_type = typename data_type::const_iterator;

        using base_type = components::locking_hook<
            components::component_base<partitioned_vector<T, Data>>>;

        data_type partitioned_vector_partition_;

        ///////////////////////////////////////////////////////////////////////
        // Constructors
        ///////////////////////////////////////////////////////////////////////

        /// Default Constructor which create partitioned_vector_partition with
        /// size 0.
        partitioned_vector();

        explicit partitioned_vector(
            std::size_t partnum, std::vector<size_type> const& partition_sizes);

        /// Constructor which create and initialize partitioned_vector_partition
        /// with all elements as \a val.
        ///
        /// param partition_size The size of vector
        /// param val Default value for the elements in partitioned_vector_partition
        ///
        partitioned_vector(std::size_t partnum,
            std::vector<size_type> const& partition_sizes, T const& val);

        partitioned_vector(std::size_t partnum,
            std::vector<size_type> const& partition_sizes, T const& val,
            allocator_type const& alloc);

        // support components::copy
        partitioned_vector(partitioned_vector const& rhs) = default;
        partitioned_vector(partitioned_vector&& rhs) = default;

        partitioned_vector& operator=(partitioned_vector const& rhs) = default;
        partitioned_vector& operator=(partitioned_vector&& rhs) = default;

        ///////////////////////////////////////////////////////////////////////
        data_type& get_data();
        data_type const& get_data() const;

        /// Duplicate the copy method for action naming
        data_type get_copied_data() const;

        ///////////////////////////////////////////////////////////////////////
        void set_data(data_type&& other);

        ///////////////////////////////////////////////////////////////////////
        iterator_type begin();
        const_iterator_type begin() const;
        const_iterator_type cbegin() const;

        iterator_type end();
        const_iterator_type end() const;
        const_iterator_type cend() const;

        ///////////////////////////////////////////////////////////////////////
        // Capacity Related APIs in the server class
        ///////////////////////////////////////////////////////////////////////

        /// Returns the number of elements
        size_type size() const;

        /// Returns the maximum possible number of elements
        size_type max_size() const;

        /// Returns the number of elements that the container has currently
        /// allocated space for.
        size_type capacity() const;

        /// Checks if the container has no elements, i.e. whether
        /// begin() == end().
        bool empty() const;

        /// Changes the number of elements stored .
        ///
        /// \param n    new size of the partitioned_vector_partition
        /// \param val  value to be copied if \a n is greater than the
        ///              current size
        ///
        void resize(size_type n, T const& val);

        /// Request the change in partitioned_vector_partition capacity so that it
        /// can hold \a n elements.
        ///
        /// This function request partitioned_vector_partition capacity should
        /// be at least enough to contain n elements. If n is greater than current
        /// partitioned_vector_partition capacity, the function causes the
        /// partitioned_vector_partition to
        /// reallocate its storage increasing its capacity to n (or greater).
        /// In other cases the partitioned_vector_partition capacity is not
        /// affected. It does not change the partitioned_vector_partition size.
        ///
        /// \param n minimum capacity of partitioned_vector_partition
        ///
        ///
        void reserve(size_type n);

        ///////////////////////////////////////////////////////////////////////
        // Element access APIs
        ///////////////////////////////////////////////////////////////////////

        /// Return the element at the position \a pos in the
        /// partitioned_vector_partition container.
        ///
        /// \param pos Position of the element in the partitioned_vector_partition
        ///
        /// \return Return the value of the element at position represented
        ///         by \a pos.
        ///
        T get_value(size_type pos) const;

        /// Return the element at the position \a pos in the
        /// partitioned_vector_partition container.
        ///
        /// \param pos Positions of the elements in the
        ///            partitioned_vector_partition
        ///
        /// \return Return the values of the elements at position represented
        ///         by \a pos.
        ///
        std::vector<T> get_values(std::vector<size_type> const& pos) const;

        /// Access the value of first element in the partitioned_vector_partition.
        ///
        /// Calling the function on empty container cause undefined behavior.
        ///
        /// \return Return the value of the first element in the
        ///         partitioned_vector_partition
        ///
        T front() const;

        /// Access the value of last element in the partitioned_vector_partition.
        ///
        /// Calling the function on empty container cause undefined behavior.
        ///
        /// \return Return the value of the last element in the
        ///         partitioned_vector_partition
        ///
        T back() const;

        ///////////////////////////////////////////////////////////////////////
        // Modifiers APIs in server class
        ///////////////////////////////////////////////////////////////////////

        /// Assigns new contents to the partitioned_vector_partition, replacing
        /// its current contents and modifying its size accordingly.
        ///
        /// \param n     new size of partitioned_vector_partition
        /// \param val   Value to fill the container with
        ///
        void assign(size_type n, T const& val);

        /// Add new element at the end of partitioned_vector_partition. The added
        /// element contain the \a val as value.
        ///
        /// \param val Value to be copied to new element
        ///
        void push_back(T const& val);

        /// Remove the last element from partitioned_vector_partition effectively
        /// reducing the size by one. The removed element is destroyed.
        ///
        void pop_back();

        //  This API is required as we do not returning the reference to the
        //  element in Any API.

        /// Copy the value of \a val in the element at position \a pos in the
        /// partitioned_vector_partition container.
        ///
        /// \param pos   Position of the element in the
        ///              partitioned_vector_partition
        ///
        /// \param val   The value to be copied
        ///
        void set_value(size_type pos, T const& val);

        /// Copy the value of \a val for the elements at positions \a pos in
        /// the partitioned_vector_partition container.
        ///
        /// \param pos   Positions of the elements in the
        ///              partitioned_vector_partition
        ///
        /// \param val   The value to be copied
        ///
        void set_values(std::vector<size_type> const& pos, std::vector<T> val);

        /// Remove all elements from the vector leaving the
        /// partitioned_vector_partition with size 0.
        ///
        void clear();

        /// Macros to define HPX component actions for all exported functions.
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, size)

        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, max_size)

        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, resize)

        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, capacity)
        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, empty)
        // HPX_DEFINE_COMPONENT_ACTION(partitioned_vector_partition, reserve)

        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_value)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_values)

        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, front)
        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, back)
        // HPX_DEFINE_COMPONENT_ACTION(partitioned_vector_partition, assign)
        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, push_back)
        // HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector_partition, pop_back)

        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_value)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_values)

        // HPX_DEFINE_COMPONENT_ACTION(partitioned_vector_partition, clear)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_copied_data)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_data)

        /// Invoke given function on given element
        ///
        /// \return This returns whatever the given function invocation returns
        template <typename F, typename... Ts>
        util::invoke_result_t<F, T, Ts...> apply(
            std::size_t pos, F f, Ts... ts);

        template <typename F, typename... Ts>
        struct apply_action
          : hpx::actions::make_action_t<
                decltype(&partitioned_vector::apply<F, Ts...>),
                &partitioned_vector::apply<F, Ts...>, apply_action<F, Ts...>>
        {
        };
    };
}    // namespace hpx::server

///////////////////////////////////////////////////////////////////////////////
#if 0
#define HPX_REGISTER_PARTITIONED_VECTOR_DECLARATION(...)
#else
#define HPX_REGISTER_PARTITIONED_VECTOR_DECLARATION(...)                       \
    HPX_REGISTER_VECTOR_DECLARATION_(__VA_ARGS__)                              \
    /**/
#define HPX_REGISTER_VECTOR_DECLARATION_(...)                                  \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_VECTOR_DECLARATION_,                 \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_VECTOR_DECLARATION_IMPL(type, name)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        type::get_value_action, HPX_PP_CAT(__vector_get_value_action_, name))  \
    HPX_REGISTER_ACTION_DECLARATION(type::get_values_action,                   \
        HPX_PP_CAT(__vector_get_values_action_, name))                         \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        type::set_value_action, HPX_PP_CAT(__vector_set_value_action_, name))  \
    HPX_REGISTER_ACTION_DECLARATION(type::set_values_action,                   \
        HPX_PP_CAT(__vector_set_values_action_, name))                         \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        type::size_action, HPX_PP_CAT(__vector_size_action_, name))            \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        type::resize_action, HPX_PP_CAT(__vector_resize_action_, name))        \
    HPX_REGISTER_ACTION_DECLARATION(type::get_copied_data_action,              \
        HPX_PP_CAT(__vector_get_copied_data_action_, name))                    \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        type::set_data_action, HPX_PP_CAT(__vector_set_data_action_, name))    \
    /**/

#define HPX_REGISTER_VECTOR_DECLARATION_1(type)                                \
    HPX_REGISTER_VECTOR_DECLARATION_2(type, std::vector<type>)                 \
    /**/
#define HPX_REGISTER_VECTOR_DECLARATION_2(type, data)                          \
    HPX_REGISTER_VECTOR_DECLARATION_3(type, data, type)                        \
    /**/
#define HPX_REGISTER_VECTOR_DECLARATION_3(type, data, name)                    \
    typedef ::hpx::server::partitioned_vector<type, data> HPX_PP_CAT(          \
        __partitioned_vector_, HPX_PP_CAT(type, name));                        \
    HPX_REGISTER_VECTOR_DECLARATION_IMPL(                                      \
        HPX_PP_CAT(__partitioned_vector_, HPX_PP_CAT(type, name)), name)       \
    /**/
#endif

namespace hpx {

    template <typename T, typename Data>
    class partitioned_vector_partition
      : public components::client_base<partitioned_vector_partition<T, Data>,
            server::partitioned_vector<T, Data>>
    {
        using server_type = hpx::server::partitioned_vector<T, Data>;
        using base_type =
            hpx::components::client_base<partitioned_vector_partition<T, Data>,
                server::partitioned_vector<T, Data>>;

    public:
        partitioned_vector_partition() = default;

        explicit partitioned_vector_partition(
            id_type const& gid, bool make_unmanaged = false);

        explicit partitioned_vector_partition(
            hpx::shared_future<id_type> const& gid);

        // Return the pinned pointer to the underlying component
        std::shared_ptr<server::partitioned_vector<T, Data>> get_ptr() const;

        ///////////////////////////////////////////////////////////////////////
        //  Capacity related APIs in partitioned_vector_partition client class

        /// Asynchronously return the size of the partitioned_vector_partition
        /// component.
        ///
        /// \return This returns size as the hpx::future of type size_type
        ///
        future<std::size_t> size_async() const;

        /// Return the size of the partitioned_vector_partition component.
        ///
        /// \return This returns size as the hpx::future of type size_type
        ///
        std::size_t size() const;

        //         future<std::size_t> max_size_async() const
        //         {
        //             HPX_ASSERT(this->get_id());
        //             return this->base_type::max_size_async(this->get_id());
        //         }
        //         std::size_t max_size() const
        //         {
        //             return max_size_async().get();
        //         }

        /// Resize the partitioned_vector_partition component.
        /// If the \a val is not specified it use default constructor instead.
        ///
        /// \param n    New size of the partitioned_vector_partition
        /// \param val  Value to be copied if \a n is greater than the current
        ///             size
        ///
        void resize(std::size_t n, T const& val = T());

        /// Resize the partitioned_vector_partition component.
        /// If the \a val is not specified it use default constructor instead.
        ///
        /// \param n    New size of the partitioned_vector_partition
        /// \param val  Value to be copied if \a n is greater than the current
        ///             size
        ///
        /// \return This returns the hpx::future of type void which gets ready
        ///         once the operation is finished.
        ///
        future<void> resize_async(std::size_t n, T const& val = T());

        //         future<std::size_t> capacity_async() const
        //         {
        //             HPX_ASSERT(this->get_id());
        //             return this->base_type::capacity_async(this->get_id());
        //         }
        //         std::size_t capacity() const
        //         {
        //             return capacity_async().get();
        //         }

        //         future<bool> empty_async() const
        //         {
        //             HPX_ASSERT(this->get_id());
        //             return this->base_type::empty_async(this->get_id());
        //         }
        //         bool empty() const
        //         {
        //             return empty_async().get();
        //         }

        //         void reserve(std::size_t n)
        //         {
        //             HPX_ASSERT(this->get_id());
        //             this->base_type::reserve_async(this->get_id(), n).get();
        //         }

        //  Element Access APIs in Client class

        /// Returns the value at position \a pos in the partitioned_vector_partition
        /// component.
        ///
        /// \param pos  Position of the element in the partitioned_vector_partition
        ///
        /// \return Returns the value of the element at position represented
        ///         by \a pos
        ///
        T get_value(launch::sync_policy, std::size_t pos) const;

        /// Return the element at the position \a pos in the
        /// partitioned_vector_partition container.
        ///
        /// \param pos Position of the element in the partitioned_vector_partition
        ///
        /// \return This returns the value as the hpx::future
        ///
        future<T> get_value(std::size_t pos) const;

        /// Returns the value at position \a pos in the partitioned_vector_partition
        /// component.
        ///
        /// \param pos  Position of the element in the partitioned_vector_partition
        ///
        /// \return Returns the value of the element at position represented
        ///         by \a pos
        ///
        std::vector<T> get_values(
            launch::sync_policy, std::vector<std::size_t> const& pos) const;

        /// Return the element at the position \a pos in the
        /// partitioned_vector_partition container.
        ///
        /// \param pos Position of the element in the partitioned_vector_partition
        ///
        /// \return This returns the value as the hpx::future
        ///
        future<std::vector<T>> get_values(
            std::vector<std::size_t> const& pos) const;

        // future<T> front_async() const
        // {
        //     HPX_ASSERT(this->get_id());
        //     return this->base_type::front_async(this->get_id());
        // }
        // T front() const
        // {
        //     HPX_ASSERT(this->get_id());
        //     return front_async().get();
        // }

        // future<T> back_async() const
        // {
        //     HPX_ASSERT(this->get_id());
        //     return this->base_type::back_async(this->get_id());
        // }
        // T back() const
        // {
        //     return back_async().get();
        // }

        //  Modifiers APIs in client class
        // void assign(std::size_t n, T const& val)
        // {
        //     HPX_ASSERT(this->get_id());
        //     this->base_type::assign_async(this->get_id(), n, val).get();
        // }

        // template <typename T_>
        // void push_back(T_ && val)
        // {
        //     HPX_ASSERT(this->get_id());
        //     this->base_type::push_back_async(
        //         this->get_id(), HPX_FORWARD(T_, val)).get();
        // }

        // void pop_back()
        // {
        //     HPX_ASSERT(this->get_id());
        //     this->base_type::pop_back_async(this->get_id()).get();
        // }

        /// Copy the value of \a val in the element at position
        /// \a pos in the partitioned_vector_partition container.
        ///
        /// \param pos   Position of the element in the partitioned_vector_partition
        /// \param val   The value to be copied
        ///
        void set_value(launch::sync_policy, std::size_t pos, T&& val);
        void set_value(launch::sync_policy, std::size_t pos, T const& val);

        /// Copy the value of \a val in the element at position
        /// \a pos in the partitioned_vector_partition component.
        ///
        /// \param pos  Position of the element in the partitioned_vector_partition
        /// \param val  Value to be copied
        ///
        /// \return This returns the hpx::future of type void
        ///
        future<void> set_value(std::size_t pos, T&& val);
        future<void> set_value(std::size_t pos, T const& val);

        /// Copy the value of \a val in the element at position
        /// \a pos in the partitioned_vector_partition container.
        ///
        /// \param pos   Position of the element in the partitioned_vector_partition
        /// \param val   The value to be copied
        ///
        void set_values(launch::sync_policy,
            std::vector<std::size_t> const& pos, std::vector<T> const& val);

        /// Copy the value of \a val in the element at position
        /// \a pos in the partitioned_vector_partition component.
        ///
        /// \param pos  Position of the element in the partitioned_vector_partition
        /// \param val  Value to be copied
        ///
        /// \return This returns the hpx::future of type void
        ///
        future<void> set_values(
            std::vector<std::size_t> const& pos, std::vector<T> const& val);

        //         void clear()
        //         {
        //             HPX_ASSERT(this->get_id());
        //             this->base_type::clear_async(this->get_id()).get();
        //         }

        /// Returns a copy of the data owned by the partitioned_vector_partition
        /// component.
        ///
        /// \return This returns the data of the partitioned_vector_partition
        ///
        typename server_type::data_type get_copied_data(
            launch::sync_policy) const;

        /// Returns a copy of the data owned by the partitioned_vector_partition
        /// component.
        ///
        /// \return This returns the data as a hpx::future
        ///
        hpx::future<typename server_type::data_type> get_copied_data() const;

        /// Updates the data owned by the partition_vector component.
        ///
        void set_data(
            launch::sync_policy, typename server_type::data_type&& other) const;

        /// Updates the data owned by the partition_vector
        /// component.
        ///
        /// \return This returns the hpx::future of type void
        ///
        hpx::future<void> set_data(
            typename server_type::data_type&& other) const;

        /// Invoke given function on given element
        ///
        /// \return This returns whatever the given function invocation returns
        template <typename F, typename... Ts>
        hpx::future<util::invoke_result_t<F, T, Ts...>> apply(
            std::size_t pos, F&& f, Ts&&... ts);
    };
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
