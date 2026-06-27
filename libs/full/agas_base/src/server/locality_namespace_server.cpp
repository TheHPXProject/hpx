//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2025 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/agas_base/server/locality_namespace.hpp>
#include <hpx/agas_base/server/primary_namespace.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace hpx::agas {

    naming::gid_type bootstrap_locality_namespace_gid()
    {
        return naming::gid_type(agas::primary_ns_msb, agas::locality_ns_lsb);
    }

    hpx::id_type bootstrap_locality_namespace_id()
    {
        return hpx::id_type(agas::locality_ns_msb, agas::locality_ns_lsb,
            hpx::id_type::management_type::unmanaged);
    }
}    // namespace hpx::agas

namespace hpx::agas::server {

    void locality_namespace::register_server_instance(
        char const* servicename, error_code& ec)
    {
        // now register this AGAS instance with AGAS :-P
        instance_name_ = agas::service_name;
        instance_name_ += servicename;
        instance_name_ += agas::server::locality_namespace_service_name;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(
            launch::sync, instance_name_, get_unmanaged_id().get_gid(), ec);
    }

    void locality_namespace::unregister_server_instance(error_code& ec)
    {
        agas::unregister_name(launch::sync, instance_name_, ec);
        this->base_type::finalize();
    }

    void locality_namespace::finalize()
    {
        if (!instance_name_.empty())
        {
            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, instance_name_, ec);
        }
    }

    std::uint32_t locality_namespace::allocate(
        parcelset::endpoints_type const& endpoints, std::uint64_t count,
        std::uint32_t num_threads, naming::gid_type suggested_prefix)
    {    // {{{ allocate implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.allocate_.time_, counter_data_.allocate_.enabled_);
        counter_data_.increment_allocate_count();

        using hpx::get;

        std::unique_lock<mutex_type> l(mutex_);

#if defined(HPX_DEBUG)
        for (partition_table_type::value_type const& partition : partitions_)
        {
            HPX_ASSERT(get<0>(partition.second) != endpoints);
        }
#endif
        // Check for address space exhaustion.
        if (HPX_UNLIKELY(0xFFFFFFFE < partitions_.size()))    //-V104
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::internal_server_error,
                "locality_namespace::allocate",
                "primary namespace has been exhausted");
        }

        // Compute the locality's prefix.
        std::uint32_t prefix = naming::invalid_locality_id;

        // check if the suggested prefix can be used instead of the next
        // free one
        std::uint32_t suggested_locality_id =
            naming::get_locality_id_from_gid(suggested_prefix);

        partition_table_type::iterator it = partitions_.end();
        if (suggested_locality_id != naming::invalid_locality_id)
        {
            it = partitions_.find(suggested_locality_id);

            if (it == partitions_.end())
            {
                prefix = suggested_locality_id;
            }
            else
            {
                do
                {
                    prefix = prefix_counter_++;
                    it = partitions_.find(prefix);
                } while (it != partitions_.end());
            }
        }
        else
        {
            do
            {
                prefix = prefix_counter_++;
                it = partitions_.find(prefix);
            } while (it != partitions_.end());
        }

        // We need to create an entry in the partition table for this
        // locality.
        if (HPX_UNLIKELY(!util::insert_checked(
                partitions_.insert(std::make_pair(
                    prefix, partition_type(endpoints, num_threads))),
                it)))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::lock_error,
                "locality_namespace::allocate",
                "partition table insertion failed due to a locking "
                "error or memory corruption, endpoint({1}), "
                "prefix({2})",
                endpoints, prefix);
        }

        // Now that we've inserted the locality into the partition table
        // successfully, we need to put the locality's GID into the GVA
        // table so that parcels can be sent to the memory of a locality.
        if (primary_)
        {
            naming::gid_type id(naming::get_gid_from_locality_id(prefix));
            gva const g(id,
                to_int(hpx::components::component_enum_type::runtime_support),
                count);

            if (!primary_->bind_gid(g, id, id))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_request,
                    "locality_namespace::allocate",
                    "unable to bind prefix({1}) to a gid", prefix);
            }
            return prefix;
        }

        LAGAS_(info).format(
            "locality_namespace::allocate, ep({1}), count({2}), prefix({3})",
            endpoints, count, prefix);

        return prefix;
    }    // }}}

    parcelset::endpoints_type locality_namespace::resolve_locality(
        naming::gid_type const& locality)
    {    // {{{ resolve_locality implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.resolve_locality_.time_,
            counter_data_.resolve_locality_.enabled_);
        counter_data_.increment_resolve_locality_count();

        using hpx::get;
        std::uint32_t prefix = naming::get_locality_id_from_gid(locality);

        std::lock_guard<mutex_type> l(mutex_);
        partition_table_type::iterator it = partitions_.find(prefix);

        if (it != partitions_.end())
        {
            return get<0>(it->second);
        }

        return parcelset::endpoints_type();
    }    // }}}

    void locality_namespace::free(naming::gid_type const& locality)
    {    // {{{ free implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.free_.time_, counter_data_.free_.enabled_);
        counter_data_.increment_free_count();

        using hpx::get;

        // parameters
        std::uint32_t prefix = naming::get_locality_id_from_gid(locality);

        std::unique_lock<mutex_type> l(mutex_);

        partition_table_type::iterator pit = partitions_.find(prefix),
                                       pend = partitions_.end();

        if (pit != pend)
        {
            /*
        // Wipe the locality from the tables.
        naming::gid_type locality =
            naming::get_gid_from_locality_id(get<0>(pit->second));

        // first remove entry from reverse partition table
        prefixes_.erase(get<0>(pit->second));
        */

            // now remove it from the main partition table
            partitions_.erase(pit);

            if (primary_)
            {
                l.unlock();

                // remove primary namespace
                {
                    naming::gid_type service(
                        agas::primary_ns_msb, agas::primary_ns_lsb);
                    primary_->unbind_gid(
                        1, naming::replace_locality_id(service, prefix));
                }

                // remove symbol namespace
                {
                    naming::gid_type service(
                        agas::symbol_ns_msb, agas::symbol_ns_lsb);
                    primary_->unbind_gid(
                        1, naming::replace_locality_id(service, prefix));
                }

                // remove locality itself
                {
                    primary_->unbind_gid(0, locality);
                }
            }

            /*LAGAS_(info).format("locality_namespace::free, ep({1})", ep);*/
        }

        /*LAGAS_(info).format(
            "locality_namespace::free, ep({1}), response(no_success)", ep);*/
    }    // }}}

    std::vector<std::uint32_t> locality_namespace::localities()
    {    // {{{ localities implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.localities_.time_,
            counter_data_.localities_.enabled_);
        counter_data_.increment_localities_count();

        std::lock_guard<mutex_type> l(mutex_);

        std::vector<std::uint32_t> p;

        partition_table_type::const_iterator it = partitions_.begin(),
                                             end = partitions_.end();

        for (/**/; it != end; ++it)
            p.push_back(it->first);

        LAGAS_(info).format(
            "locality_namespace::localities, localities({1})", p.size());

        return p;
    }    // }}}

    std::uint32_t locality_namespace::get_num_localities()
    {    // {{{ get_num_localities implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.num_localities_.time_,
            counter_data_.num_localities_.enabled_);
        counter_data_.increment_num_localities_count();
        std::lock_guard<mutex_type> l(mutex_);

        std::uint32_t num_localities =
            static_cast<std::uint32_t>(partitions_.size());

        LAGAS_(info).format(
            "locality_namespace::get_num_localities, localities({1})",
            num_localities);

        return num_localities;
    }    // }}}

    std::vector<std::uint32_t> locality_namespace::get_num_threads()
    {    // {{{ get_num_threads implementation
        std::lock_guard<mutex_type> l(mutex_);

        std::vector<std::uint32_t> num_threads;

        partition_table_type::iterator end = partitions_.end();
        for (partition_table_type::iterator it = partitions_.begin(); it != end;
            ++it)
        {
            using hpx::get;
            num_threads.push_back(get<1>(it->second));
        }

        LAGAS_(info).format(
            "locality_namespace::get_num_threads, localities({1})",
            num_threads.size());

        return num_threads;
    }    // }}}

    std::uint32_t locality_namespace::get_num_overall_threads()
    {
        std::lock_guard<mutex_type> l(mutex_);

        std::uint32_t num_threads = 0;

        partition_table_type::iterator end = partitions_.end();
        for (partition_table_type::iterator it = partitions_.begin(); it != end;
            ++it)
        {
            using hpx::get;
            num_threads += get<1>(it->second);
        }

        LAGAS_(info).format(
            "locality_namespace::get_num_overall_threads, localities({1})",
            num_threads);

        return num_threads;
    }

    // access current counter values
    std::int64_t locality_namespace::counter_data::get_allocate_count(
        bool reset)
    {
        return util::get_and_reset_value(allocate_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_resolve_locality_count(
        bool reset)
    {
        return util::get_and_reset_value(resolve_locality_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_free_count(bool reset)
    {
        return util::get_and_reset_value(free_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_localities_count(
        bool reset)
    {
        return util::get_and_reset_value(localities_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_num_localities_count(
        bool reset)
    {
        return util::get_and_reset_value(num_localities_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_num_threads_count(
        bool reset)
    {
        return util::get_and_reset_value(num_threads_.count_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_overall_count(bool reset)
    {
        return util::get_and_reset_value(allocate_.count_, reset) +
            util::get_and_reset_value(resolve_locality_.count_, reset) +
            util::get_and_reset_value(free_.count_, reset) +
            util::get_and_reset_value(localities_.count_, reset) +
            util::get_and_reset_value(num_localities_.count_, reset) +
            util::get_and_reset_value(num_threads_.count_, reset);
    }

    void locality_namespace::counter_data::enable_all()
    {
        allocate_.enabled_ = true;
        resolve_locality_.enabled_ = true;
        free_.enabled_ = true;
        localities_.enabled_ = true;
        num_localities_.enabled_ = true;
        num_threads_.enabled_ = true;
    }

    // access execution time counters
    std::int64_t locality_namespace::counter_data::get_allocate_time(bool reset)
    {
        return util::get_and_reset_value(allocate_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_resolve_locality_time(
        bool reset)
    {
        return util::get_and_reset_value(resolve_locality_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_free_time(bool reset)
    {
        return util::get_and_reset_value(free_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_localities_time(
        bool reset)
    {
        return util::get_and_reset_value(localities_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_num_localities_time(
        bool reset)
    {
        return util::get_and_reset_value(num_localities_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_num_threads_time(
        bool reset)
    {
        return util::get_and_reset_value(num_threads_.time_, reset);
    }

    std::int64_t locality_namespace::counter_data::get_overall_time(bool reset)
    {
        return util::get_and_reset_value(allocate_.time_, reset) +
            util::get_and_reset_value(resolve_locality_.time_, reset) +
            util::get_and_reset_value(free_.time_, reset) +
            util::get_and_reset_value(localities_.time_, reset) +
            util::get_and_reset_value(num_localities_.time_, reset) +
            util::get_and_reset_value(num_threads_.time_, reset);
    }

    // increment counter values
    void locality_namespace::counter_data::increment_allocate_count()
    {
        if (allocate_.enabled_)
        {
            ++allocate_.count_;
        }
    }

    void locality_namespace::counter_data::increment_resolve_locality_count()
    {
        if (resolve_locality_.enabled_)
        {
            ++resolve_locality_.count_;
        }
    }

    void locality_namespace::counter_data::increment_free_count()
    {
        if (free_.enabled_)
        {
            ++free_.count_;
        }
    }

    void locality_namespace::counter_data::increment_localities_count()
    {
        if (localities_.enabled_)
        {
            ++localities_.count_;
        }
    }

    void locality_namespace::counter_data::increment_num_localities_count()
    {
        if (num_localities_.enabled_)
        {
            ++num_localities_.count_;
        }
    }

    void locality_namespace::counter_data::increment_num_threads_count()
    {
        if (num_threads_.enabled_)
        {
            ++num_threads_.count_;
        }
    }
}    // namespace hpx::agas::server
