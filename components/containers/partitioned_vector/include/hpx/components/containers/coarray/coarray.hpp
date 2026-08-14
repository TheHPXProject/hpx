//  Copyright (c) 2017 Antoine Tran Tan
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/coarray/coarray.hpp

#pragma once

#include <hpx/modules/collectives.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_view.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
/// \cond NOINTERNAL

#define HPX_REGISTER_COARRAY_DECLARATION(...)                                  \
    HPX_REGISTER_PARTITIONED_VECTOR_DECLARATION(__VA_ARGS__)
#define HPX_REGISTER_COARRAY(...) HPX_REGISTER_PARTITIONED_VECTOR(__VA_ARGS__)

namespace hpx {
    namespace detail {
        struct auto_subscript
        {
            constexpr auto_subscript() {}

            // Defined to pass the coarray_sizes constructor
            operator std::size_t()
            {
                return std::size_t(-1);
            }
        };

        template <typename C>
        struct cast_if_autosubscript
        {
            using type = C;
        };

        template <>
        struct cast_if_autosubscript<detail::auto_subscript>
        {
            using type = int;
        };

        template <std::size_t N>
        struct coarray_sizes
        {
            using iterator = typename std::array<std::size_t, N>::iterator;

            using const_iterator =
                typename std::array<std::size_t, N>::const_iterator;

            template <typename... I>
            coarray_sizes(I... i)
              : sizes_({{i...}})
            {
                using last_element =
                    typename util::at_index<sizeof...(I) - 1, I...>::type;
                using condition =
                    typename std::is_same<last_element, detail::auto_subscript>;

                static_assert(condition::value,
                    "hpx::coarray() needs the last size to be equal to hpx::_");

                static_assert(N == sizeof...(I),
                    "hpx::coarray() needs the number of sizes to be "
                    "equal to its dimension");

                static_assert(
                    util::all_of<typename std::is_integral<typename detail::
                            cast_if_autosubscript<I>::type>::type...>::value,
                    "One or more elements in sizes given to hpx::coarray() "
                    "is not integral");
            }

            iterator begin()
            {
                return sizes_.begin();
            }

            iterator end()
            {
                return sizes_.end();
            }

            const_iterator begin() const
            {
                return sizes_.cbegin();
            }

            const_iterator end() const
            {
                return sizes_.cend();
            }

        private:
            std::array<std::size_t, N> sizes_;
        };
    }    // namespace detail

    // Used for "automatic" coarray subscript and "automatic" coarray size
    namespace container { namespace placeholders {
        constexpr hpx::detail::auto_subscript _;
    }}    // namespace container::placeholders

    template <typename T, std::size_t N, typename Data = std::vector<T>>
    struct coarray : public partitioned_vector_view<T, N, Data>
    {
    private:
        using base_type = partitioned_vector_view<T, N, Data>;
        using indices = typename hpx::util::make_index_pack<N>::type;

        std::vector<hpx::id_type> get_unrolled_localities(
            std::vector<hpx::id_type> const& in, std::size_t num_segments,
            std::size_t unroll)
        {
            using iterator = typename std::vector<hpx::id_type>::iterator;

            using const_iterator =
                typename std::vector<hpx::id_type>::const_iterator;

            std::vector<hpx::id_type> out(num_segments);

            iterator o_end = out.end();
            const_iterator i_begin = in.cbegin();
            const_iterator i_end = in.cend();
            const_iterator i = i_begin;

            for (iterator o = out.begin(); o < o_end;
                o += static_cast<std::ptrdiff_t>(unroll))
            {
                std::fill(o,
                    (std::min) (o + static_cast<std::ptrdiff_t>(unroll), o_end),
                    *i);
                i = (++i != i_end) ? i : i_begin;
            }

            return out;
        }

        template <typename Iterator, std::size_t... I>
        base_type update_view(hpx::detail::coarray_sizes<N> const& cosizes,
            std::size_t num_images, hpx::util::index_pack<I...>,
            hpx::lcos::spmd_block const& block, Iterator&& begin,
            Iterator&& last) const
        {
            return base_type(block, HPX_FORWARD(Iterator, begin),
                HPX_FORWARD(Iterator, last),
                {(cosizes.begin()[I] != std::size_t(-1) ? cosizes.begin()[I] :
                                                          num_images)...});
        }

    public:
        explicit coarray(hpx::lcos::spmd_block const& block,
            std::string const& name, hpx::detail::coarray_sizes<N> cosizes,
            std::size_t segment_size)
          : base_type(block)
          , vector_()
          , this_image_(block.this_image())
        {
            // Used to access base members
            base_type& view(*this);

            std::size_t num_images = block.get_num_images();

            if (block.this_image() == 0)
            {
                std::size_t num_segments = N > 0 ? 1 : 0;

                for (std::size_t const& i : cosizes)
                {
                    num_segments *= (i != std::size_t(-1) ? i : num_images);
                }

                std::vector<hpx::id_type> localities =
                    hpx::find_all_localities();

                vector_ = hpx::partitioned_vector<T, Data>(
                    segment_size * num_segments, T(0),
                    hpx::container_layout(num_segments,
                        get_unrolled_localities(localities, num_segments,
                            num_segments / localities.size())));

                vector_.register_as(hpx::launch::sync, name + "_hpx_coarray");
            }
            else
            {
                vector_.connect_to(hpx::launch::sync, name + "_hpx_coarray");
            }

            view = update_view(cosizes, num_images, indices(), block,
                vector_.begin(), vector_.end());
        }

        template <typename... I,
            typename = typename std::enable_if<!std::is_same<
                typename util::at_index<sizeof...(I) - 1, I...>::type,
                detail::auto_subscript>::value>::type>
        hpx::detail::view_element<T, Data> operator()(I... index) const
        {
            return base_type::operator()(
                (std::is_same<I, detail::auto_subscript>::value ? this_image_ :
                                                                  index)...);
        }

        template <typename... I,
            typename = typename std::enable_if<std::is_same<
                typename util::at_index<sizeof...(I) - 1, I...>::type,
                detail::auto_subscript>::value>::type>
        Data& operator()(I... index)
        {
            return base_type::operator()(
                (std::is_same<I, detail::auto_subscript>::value ? this_image_ :
                                                                  index)...)
                .data();
        }

    private:
        hpx::partitioned_vector<T, Data> vector_;
        std::size_t this_image_;
    };
}    // namespace hpx
