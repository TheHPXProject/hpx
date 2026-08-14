//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_HAVE_STATIC_LINKING)
#include <hpx/modules/distribution_policies.hpp>

#include <hpx/components/containers/partitioned_vector/export_definitions.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component.hpp>

#include <vector>

#include <hpx/config/warnings_prefix.hpp>

HPX_REGISTER_PARTITIONED_VECTOR(int)
using long_long = long long;
HPX_REGISTER_PARTITIONED_VECTOR(long_long)

// an out-of-line definition of a member of a class template cannot have default
// arguments
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 5037)
#endif

#include <hpx/config/warnings_prefix.hpp>

template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::server::partitioned_vector<int, std::vector<int>>;
template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::partitioned_vector_partition<int, std::vector<int>>;
template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::partitioned_vector<int, std::vector<int>>;
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<int, std::vector<int>>::partitioned_vector(
    size_type, hpx::container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<int, std::vector<int>>::partitioned_vector(
    size_type, int const&, hpx::container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<int, std::vector<int>>::partitioned_vector(
    size_type, hpx::explicit_container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<int, std::vector<int>>::partitioned_vector(size_type,
    int const&, hpx::explicit_container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<int, std::vector<int>>::partitioned_vector(
    std::vector<int>::const_iterator, std::vector<int>::const_iterator,
    hpx::explicit_container_distribution_policy const&, void*);

template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::server::partitioned_vector<long long, std::vector<long long>>;
template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::partitioned_vector_partition<long long, std::vector<long long>>;
template class HPX_PARTITIONED_VECTOR_EXPORT
    hpx::partitioned_vector<long long, std::vector<long long>>;
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<long long, std::vector<long long>>::partitioned_vector(
    size_type, hpx::container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT hpx::partitioned_vector<long long,
    std::vector<long long>>::partitioned_vector(size_type, long long const&,
    hpx::container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<long long, std::vector<long long>>::partitioned_vector(
    size_type, hpx::explicit_container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT hpx::partitioned_vector<long long,
    std::vector<long long>>::partitioned_vector(size_type, long long const&,
    hpx::explicit_container_distribution_policy const&, void*);
template HPX_PARTITIONED_VECTOR_EXPORT
hpx::partitioned_vector<long long, std::vector<long long>>::partitioned_vector(
    std::vector<long long>::const_iterator,
    std::vector<long long>::const_iterator,
    hpx::explicit_container_distribution_policy const&, void*);

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

#endif
