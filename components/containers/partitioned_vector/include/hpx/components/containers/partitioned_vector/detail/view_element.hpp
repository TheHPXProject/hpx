//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/partitioned_vector/partitioned_vector_view.hpp

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_decl.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_segmented_iterator.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
/// \cond NOINTERNAL

namespace hpx { namespace detail {
    template <typename T, typename Data>
    struct const_view_element;

    template <typename T, typename Data>
    struct view_element : public hpx::partitioned_vector_partition<T, Data>
    {
        using pvector_iterator = hpx::segmented::vector_iterator<T, Data>;
        using segment_iterator = typename pvector_iterator::segment_iterator;
        using local_segment_iterator =
            typename pvector_iterator::local_segment_iterator;
        using traits =
            typename hpx::traits::segmented_iterator_traits<pvector_iterator>;

    public:
        explicit view_element(hpx::lcos::spmd_block const& block,
            segment_iterator begin, segment_iterator end, segment_iterator it)

          : hpx::partitioned_vector_partition<T, Data>(it->get_id())
          , it_(it)
        {
            std::uint32_t here = hpx::get_locality_id();

            is_data_here_ =
                (here == hpx::naming::get_locality_id_from_id(it_->get_id()));

            is_owned_by_current_thread_ = is_data_here_ &&
                ((std::distance(
                      local_segment_iterator(begin.base(), end.base(), here),
                      local_segment_iterator(it_.base(), end.base(), here)) %
                     block.get_images_per_locality()) ==
                    (block.this_image() % block.get_images_per_locality()));
        }

        view_element(view_element const&) = default;

        // Not copy-assygnable
        view_element& operator=(view_element const&) = delete;

        // But movable
        view_element(view_element&& other) = default;

        // Explicit conversion allows to perform Get operations
        explicit operator Data() const
        {
            return const_data();
        }

        // operator overloading (Useful for manual view definition)
        segment_iterator&& operator&()
        {
            return HPX_MOVE(it_);
        }

    private:
        bool is_data_here() const
        {
            return is_data_here_;
        }

        Data const_data() const
        {
            if (is_data_here())
            {
                return this->get_ptr()->get_data();
            }
            else
                return this->get_copied_data(hpx::launch::sync);
        }

    public:
        Data& data()
        {
            return this->get_ptr()->get_data();
        }

        Data const& data() const
        {
            return this->get_ptr()->get_data();
        }

        bool is_owned_by_current_thread() const
        {
            return is_owned_by_current_thread_;
        }

        // Note: Put operation. Race condition may occur, be sure that
        // operator=() is called by only one thread at a time.
        void operator=(Data&& other)
        {
            if (is_data_here())
            {
                Data& ref = data();

                HPX_ASSERT_MSG(ref.size() == other.size(),
                    "r-value vector has invalid size");

                ref = other;
            }
            else
            {
                this->set_data(hpx::launch::sync, HPX_MOVE(other));
            }
        }

        // Note: Put operation. Race condition may occur, be sure that
        // operator=() is called by only one thread at a time.
        void operator=(Data const& other)
        {
            if (is_data_here())
            {
                Data& ref = data();

                HPX_ASSERT_MSG(ref.size() == other.size(),
                    "r-value vector has invalid size");

                ref = other;
            }
            else
            {
                this->set_data(hpx::launch::sync, Data(other));
            }
        }

        // Note: Put operation. Free of race conditions.
        void operator=(view_element<T, Data>&& other)
        {
            if (other.is_owned_by_current_thread())
            {
                if (is_data_here())
                {
                    Data& ref = data();

                    HPX_ASSERT_MSG(ref.size() == other.size(),
                        "Indexed r-value element has "
                        "invalid size");

                    ref = other.data();
                }

                else
                    this->set_data(hpx::launch::sync, other.const_data());
            }
        }

        // Note: Put operation. Free of race conditions.
        void operator=(const_view_element<T, Data>&& other)
        {
            if (other.is_owned_by_current_thread())
            {
                if (is_data_here())
                {
                    Data& ref = data();

                    HPX_ASSERT_MSG(ref.size() == other.size(),
                        "Indexed r-value element has "
                        "invalid size");

                    ref = other.data();
                }

                else
                    this->set_data(hpx::launch::sync, other.const_data());
            }
        }

        T operator[](std::size_t i) const
        {
            if (is_data_here())
            {
                return data()[i];
            }

            else
                return this->get_value(hpx::launch::sync, i);
        }

    private:
        bool is_data_here_;
        bool is_owned_by_current_thread_;
        segment_iterator it_;
    };

    template <typename T, typename Data>
    struct const_view_element
      : public hpx::partitioned_vector_partition<T, Data>
    {
        using const_pvector_iterator =
            hpx::segmented::const_vector_iterator<T, Data>;
        using const_segment_iterator =
            typename const_pvector_iterator::segment_iterator;
        using const_local_segment_iterator =
            typename const_pvector_iterator::local_segment_iterator;
        using traits = typename hpx::traits::segmented_iterator_traits<
            const_pvector_iterator>;

    public:
        explicit const_view_element(hpx::lcos::spmd_block const& block,
            const_segment_iterator begin, const_segment_iterator end,
            const_segment_iterator it)

          : hpx::partitioned_vector_partition<T, Data>(it->get_id())
          , it_(it)
        {
            std::uint32_t here = hpx::get_locality_id();

            is_data_here_ =
                (here == hpx::naming::get_locality_id_from_id(it_->get_id()));

            is_owned_by_current_thread_ = is_data_here_ &&
                ((std::distance(const_local_segment_iterator(
                                    begin.base(), end.base(), here),
                      const_local_segment_iterator(
                          it_.base(), end.base(), here)) %
                     block.get_images_per_locality()) ==
                    (block.this_image() % block.get_images_per_locality()));
        }

        const_view_element(const_view_element const&) = default;

        // Not copy-assignable
        const_view_element& operator=(const_view_element const&) = delete;

        // But movable
        const_view_element(const_view_element&& other) = default;

        // Explicit conversion allows to perform Get operations
        explicit operator Data() const
        {
            return const_data();
        }

    private:
        bool is_data_here() const
        {
            return is_data_here_;
        }

    public:
        Data const_data() const
        {
            if (is_data_here())
            {
                return this->get_ptr()->get_data();
            }
            else
                return this->get_copied_data(hpx::launch::sync);
        }

        Data const& data() const
        {
            return this->get_ptr()->get_data();
        }

        bool is_owned_by_current_thread() const
        {
            return is_owned_by_current_thread_;
        }

        T operator[](std::size_t i) const
        {
            if (is_data_here())
            {
                return data()[i];
            }

            else
                return this->get_value(hpx::launch::sync, i);
        }

    private:
        bool is_data_here_;
        bool is_owned_by_current_thread_;
        const_segment_iterator it_;
    };
}}    // namespace hpx::detail
