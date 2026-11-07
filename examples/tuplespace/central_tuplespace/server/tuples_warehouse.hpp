//  Copyright (c) 2013 Shuangyang Yang
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/chrono.hpp>
#include <hpx/hpx.hpp>
#include <hpx/include/components.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/mutex.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tuple.hpp"

// #define TS_DEBUG

///////////////////////////////////////////////////////////////////////////////
namespace examples { namespace server {

    class tuples_warehouse
    {
    public:
        typedef hpx::util::storage::tuple tuple_type;
        typedef hpx::util::storage::tuple::elem_type elem_type;
        typedef hpx::util::storage::tuple::hash_elem_functor hash_elem_functor;
        typedef hpx::util::storage::tuple::compare_elem_functor
            compare_elem_functor;

        typedef hpx::mutex mutex_type;
        typedef std::uint64_t index_type;
        typedef std::set<index_type> matched_indices_type;

        tuples_warehouse()
          : index_(1)
        {
        }

        bool empty() const
        {
            return tuple_fields_[0].empty();
        }

        int insert(tuple_type const& tp)
        {
            if (tp.empty())    // empty tuple
                return -1;

            // whether size(tp) > size(tuple_fields_)
            while (tp.size() > tuple_fields_.size())
            {
                tuple_field_container tmp;
                tuple_fields_.push_back(tmp);
            }

            tuple_type::const_iterator it;
            unsigned int pos;
            for (it = tp.begin(), pos = 0; it != tp.end(); ++it, ++pos)
            {
                tuple_fields_[pos].insert(index_, *it);    // insert field
            }

            ++index_;    // step up the index
            return 0;
        }

        tuple_type match(tuple_type const& tp) const
        {
            tuple_type result;

            if (tp.empty())
            {
                return read_random_tuple();
                // return random results for an empty tuple
            }

            matched_indices_type matched_indices;

            matched_indices = find_matched_indices(tp);

            if (!matched_indices.empty())
            {
                // return the tuple at the first index
                result = read_tuple_at(*(matched_indices.begin()));
            }

            return result;
        }

        tuple_type match_and_erase(tuple_type const& tp)
        {
            tuple_type result;

            if (tp.empty())
            {
                return take_random_tuple();
                // return random results for an empty tuple
            }

            matched_indices_type matched_indices;

            matched_indices = find_matched_indices(tp);

            if (!matched_indices.empty())
            {
                // return the tuple at the first index
                result = take_tuple_at(*(matched_indices.begin()));
            }

            return result;
        }

    private:    // private member functions
        matched_indices_type find_matched_indices(tuple_type const& tp) const
        {
            tuple_type::const_iterator it;
            unsigned int pos;

            matched_indices_type matched_indices, empty_set;

            for (it = tp.begin(), pos = 0; it != tp.end(); ++it, ++pos)
            {
                if (!(*it).has_value())    // empty any object
                {
                    continue;    // will match any record
                }

                typedef std::pair<
                    tuple_field_container ::field_index_map_const_iterator_type,
                    tuple_field_container::field_index_map_const_iterator_type>
                    equal_range_type;
                typedef std::pair<elem_type, index_type> const pair_type;

                equal_range_type found_range =
                    tuple_fields_[pos].field_index_map_.equal_range(*it);

                if (found_range.first ==
                    tuple_fields_[pos].field_index_map_.end())
                // no match
                {
                    return empty_set;    // empty
                }

                // update index set
                if (matched_indices.empty())    // not found yet
                {
                    std::for_each(found_range.first, found_range.second,
                        [&matched_indices](pair_type& p) {
                            matched_indices.insert(p.second);
                        });
                }
                else
                {
                    matched_indices_type new_matched_indices;

                    std::for_each(found_range.first, found_range.second,
                        [&new_matched_indices, &matched_indices](pair_type& p) {
                            if (matched_indices.find(p.second) !=
                                matched_indices.end())    // found
                            {
                                new_matched_indices.insert(p.second);
                            }
                        });

                    if (new_matched_indices.empty())    // no common index
                    {
                        return empty_set;
                    }
                    else
                    {
                        matched_indices = new_matched_indices;
                    }
                }
            }

            return matched_indices;
        }

        tuple_type read_random_tuple() const
        {
            tuple_type result;

            if (tuple_fields_.empty())
                return result;

            return read_tuple_at(tuple_fields_[0].random_index());
        }

        tuple_type read_tuple_at(index_type const& id) const
        {
            tuple_type result;

            if (tuple_fields_.empty())
                return result;

            for (unsigned int pos = 0; pos < tuple_fields_.size(); ++pos)
            {
                tuple_field_container::index_field_map_const_iterator_type it;
                it = tuple_fields_[pos].index_field_map_.find(id);

                if (it ==
                    tuple_fields_[pos].index_field_map_.end())    // not found
                {
                    break;
                }
                else
                {
                    result.push_back(
                        (it->second)->first);    // append the any object
                }
            }

            return result;
        }

        tuple_type take_random_tuple()
        {
            tuple_type result;

            if (tuple_fields_.empty())
                return result;

            return take_tuple_at(tuple_fields_[0].random_index());
        }

        tuple_type take_tuple_at(index_type const& id)
        {
            tuple_type result;

            if (tuple_fields_.empty())
                return result;

            for (unsigned int pos = 0; pos < tuple_fields_.size(); ++pos)
            {
                tuple_field_container& tf = tuple_fields_[pos];
                tuple_field_container::index_field_map_iterator_type it =
                    tf.index_field_map_.find(id);

                if (it == tf.index_field_map_.end())    // not found
                {
                    break;
                }
                else
                {
                    result.push_back(
                        (it->second)->first);    // append the any object

                    // erase the record
                    tf.field_index_map_.erase(it->second);
                    tf.index_field_map_.erase(it);
                }
            }

            return result;
        }

    private:    // member fields
        struct tuple_field_container
        {
            typedef examples::server ::tuples_warehouse::hash_elem_functor
                hash_elem_functor;
            typedef examples::server::tuples_warehouse::elem_type elem_type;
            typedef examples::server::tuples_warehouse::index_type index_type;

            typedef std::unordered_multimap<elem_type, index_type,
                hash_elem_functor, compare_elem_functor>
                field_index_map_type;
            typedef field_index_map_type::iterator
                field_index_map_iterator_type;
            typedef field_index_map_type::const_iterator
                field_index_map_const_iterator_type;

            typedef std::map<index_type, field_index_map_iterator_type>
                index_field_map_type;
            typedef index_field_map_type::iterator
                index_field_map_iterator_type;
            typedef index_field_map_type::const_iterator
                index_field_map_const_iterator_type;

            field_index_map_type field_index_map_;
            index_field_map_type index_field_map_;

            bool empty() const
            {
                return field_index_map_.empty();
            }

            int insert(index_type const& id, elem_type const& elem)
            {
                field_index_map_iterator_type it;

                it = field_index_map_.insert(std::make_pair(elem, id));
                index_field_map_.insert(std::make_pair(id, it));

                return 0;
            }

            index_type random_index() const
            {
                if (empty())
                    return 0;

                return field_index_map_.begin()->second;
            }
        };

        typedef std::vector<tuple_field_container> tuple_fields_type;

        index_type index_;    // starts from 1
        tuple_fields_type tuple_fields_;
    };
}}    // namespace examples::server

#undef TS_DEBUG
#endif
