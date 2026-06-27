//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2012 Thomas Heller
//  Copyright (c)      2012 Bryce Adelstein-Lelbach
//
//  Parts of this code were taken from the Boost.Regex library
//  Copyright (c) 2004 John Maddock
//
//  Parts of this code were taking from this article:
//  http://timday.bitbucket.org/lru.html
//  Copyright (c) 2010-2011 Tim Day
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    /// This class implements an LRU cache to hold connections. It includes
    /// entries checked out from the cache in its cache size.
    HPX_CXX_EXPORT template <typename Connection, typename Key>
    class connection_cache
    {
    public:
        using mutex_type = hpx::spinlock;

        using connection_type = std::shared_ptr<Connection>;
        using value_type = std::deque<connection_type>;
        using key_type = Key;
        using key_tracker_type = std::list<key_type>;
        using cache_value_type = hpx::tuple<
            value_type,     // cached (available) connections
            std::size_t,    // number of existing connections
            std::size_t,    // max number of cached connections
            typename key_tracker_type::iterator>;    // reference into LRU list

        using cache_type = std::map<key_type, cache_value_type>;
        using size_type = typename cache_type::size_type;

        connection_cache(
            size_type max_connections, size_type max_connections_per_locality)
          : max_connections_(max_connections < 2 ? 2 : max_connections)
          , max_connections_per_locality_(max_connections_per_locality < 2 ?
                    2 :
                    max_connections_per_locality)
          , connections_(0)
          , shutting_down_(false)
          , insertions_(0)
          , evictions_(0)
          , hits_(0)
          , misses_(0)
          , reclaims_(0)
          , reservation_failures_(0)
          , last_exhaustion_log_ns_(0)
        {
            if (max_connections_per_locality_ > max_connections_)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "connection_cache::connection_cache",
                    "the maximum number of connections per locality cannot "
                    "exceed the overall maximum number of connections");
            }
        }

        void shutdown()
        {
            shutting_down_ = true;
        }

    private:
        static value_type& cached_connections(cache_value_type& entry)
        {
            return hpx::get<0>(entry);
        }
        static value_type const& cached_connections(
            cache_value_type const& entry)
        {
            return hpx::get<0>(entry);
        }

        static std::size_t& num_existing_connections(cache_value_type& entry)
        {
            return hpx::get<1>(entry);
        }
        static std::size_t const& num_existing_connections(
            cache_value_type const& entry)
        {
            return hpx::get<1>(entry);
        }

        static std::size_t& max_num_connections(cache_value_type& entry)
        {
            return hpx::get<2>(entry);
        }
        static std::size_t const& max_num_connections(
            cache_value_type const& entry)
        {
            return hpx::get<2>(entry);
        }

        static typename key_tracker_type::iterator& lru_reference(
            cache_value_type& entry)
        {
            return hpx::get<3>(entry);
        }
        static typename key_tracker_type::iterator const& lru_reference(
            cache_value_type const& entry)
        {
            return hpx::get<3>(entry);
        }

        ///////////////////////////////////////////////////////////////////////
        // Increase the per-locality and overall connection counts.
        void increment_connection_count(cache_value_type& e)
        {
            std::size_t& num_connections = num_existing_connections(e);
            ++num_connections;
            ++connections_;

            // If appropriate, update the maximum number of allowed cached
            // connections.
            std::size_t& max_connections = max_num_connections(e);
            if (num_connections > max_connections * 2)
            {
                max_connections = static_cast<std::size_t>(
                    static_cast<double>(max_connections) * 1.5);
            }
        }

        // Decrease the per-locality and overall connection counts.
        void decrement_connection_count(cache_value_type& e)
        {
            std::size_t& num_connections = num_existing_connections(e);
            --num_connections;
            --connections_;

            // If appropriate, update the maximum number of allowed
            // cached connections.
            std::size_t& max_connections = max_num_connections(e);
            if (num_connections < max_connections / 2)
            {
                max_connections = static_cast<std::size_t>(
                    static_cast<double>(max_connections) / 1.5);
            }
        }

    public:
        /// Try to get a connection to \a l from the cache.
        ///
        /// \returns A usable connection to \a l if a connection could be
        ///          found, otherwise a default constructed connection.
        ///
        /// \note    The connection must be returned to the cache by calling
        ///          \a reclaim().
        connection_type get(key_type const& l)
        {
            std::lock_guard<mutex_type> lock(mtx_);

            // Check if this key already exists in the cache.
            typename cache_type::iterator const it = cache_.find(l);

            // Check if this key already exists in the cache.
            if (it != cache_.end())
            {
                // Key exists in cache.

                // Update LRU meta data.
                key_tracker_.splice(key_tracker_.end(), key_tracker_,
                    lru_reference(it->second));

                // If connections to the locality are available in the cache,
                // remove the oldest one and return it.
                if (!cached_connections(it->second).empty())
                {
                    value_type& connections = cached_connections(it->second);
                    connection_type result = connections.front();
                    connections.pop_front();

                    ++hits_;
                    check_invariants();
                    return result;
                }
            }

            // If we get here then the item is not in the cache.
            ++misses_;
            check_invariants();
            return connection_type();
        }

        /// Try to get a connection to \a l from the cache, or reserve space for
        /// a new connection to \a l. This function may evict entries from the
        /// cache.
        ///
        /// \returns If a connection was found in the cache, its value is
        ///          assigned to \a conn and this function returns true. If a
        ///          connection was not found but space was reserved, \a conn is
        ///          set such that conn.get() == 0, and this function returns
        ///          true. If a connection could not be found and space could
        ///          not be returned, \a conn is unmodified and this function
        ///          returns false.
        ///          If force_insert is true, a new connection entry will be
        ///          created even if that means the cache limits will be
        ///          exceeded.
        ///
        /// \note    The connection must be returned to the cache by calling
        ///          \a reclaim().
        bool get_or_reserve(
            key_type const& l, connection_type& conn, bool force_insert = false)
        {
            std::lock_guard<mutex_type> lock(mtx_);

            typename cache_type::iterator const it = cache_.find(l);

            // Check if this key already exists in the cache.
            if (it != cache_.end())
            {
                // Key exists in cache.

                // Update LRU meta data.
                key_tracker_.splice(key_tracker_.end(), key_tracker_,
                    lru_reference(it->second));

                // If connections to the locality are available in the cache,
                // remove the oldest one and return it.
                if (!cached_connections(it->second).empty())
                {
                    value_type& connections = cached_connections(it->second);
                    conn = connections.front();
                    connections.pop_front();

#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
                    conn->set_state(Connection::state_reinitialized);
#endif
                    ++hits_;
                    check_invariants();
                    return true;
                }

                // Otherwise, if we have less connections for this locality
                // than the maximum, try to reserve space in the cache for a new
                // connection.
                if (num_existing_connections(it->second) <
                        max_num_connections(it->second) ||
                    force_insert)
                {
                    // See if we have enough space or can make space available.

                    // Note that if we don't have any space and there are no
                    // outstanding connections for this locality, we grow the
                    // cache size beyond its limit (hoping that it will be
                    // reduced in size next time some connection is handed back
                    // to the cache).

                    if (!free_space() &&
                        num_existing_connections(it->second) != 0 &&
                        !force_insert)
                    {
                        // If we can't find or make space, give up.
                        ++misses_;
                        ++reservation_failures_;
                        maybe_log_exhaustion();
                        check_invariants();
                        return false;
                    }

                    // Make sure the input connection shared_ptr doesn't hold
                    // anything.
                    conn.reset();

                    // Increase the per-locality and overall connection counts.
                    increment_connection_count(it->second);

                    // Statistics
                    ++insertions_;
                    check_invariants();
                    return true;
                }

                // We've reached the maximum number of connections for this
                // locality, and none of them are checked into the cache, so
                // we have to give up.
                ++misses_;
                ++reservation_failures_;
                maybe_log_exhaustion();
                check_invariants();
                return false;
            }

            // Key (locality) isn't in cache.

            // See if we have enough space or can make space available.

            // Note that we ignore the outcome of free_space() here as we have
            // to guarantee to have space for the new connection as there are
            // no connections outstanding for this locality. If free_space
            // fails we grow the cache size beyond its limit (hoping that it
            // will be reduced in size next time some connection is handed back
            // to the cache).
            free_space();

            // Update LRU meta data.
            typename key_tracker_type::iterator kt =
                key_tracker_.insert(key_tracker_.end(), l);

            cache_.emplace(l,
                hpx::make_tuple(
                    value_type(), 1, max_connections_per_locality_, kt));

            // Make sure the input connection shared_ptr doesn't hold anything.
            conn.reset();

            // Increase the overall connection counts.
            ++connections_;

            ++insertions_;
            check_invariants();
            return true;
        }

        /// Returns a connection for \a l to the cache.
        ///
        /// \note The cache must already be aware of the connection, through
        ///       a prior call to \a get() or \a get_or_reserve().
        void reclaim(key_type const& l, connection_type const& conn)
        {
            std::lock_guard<mutex_type> lock(mtx_);

            // Search for an entry for this key.
            typename cache_type::iterator const ct = cache_.find(l);

            if (ct != cache_.end())
            {
                // Verify that there are actually outstanding (checked-out)
                // connections for this locality. If all existing connections
                // are already cached, this reclaim is a caller error.
                if (cached_connections(ct->second).size() >=
                    num_existing_connections(ct->second))
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "connection_cache::reclaim",
                        "reclaiming a connection that was not "
                        "properly checked out");
                }

                // Update LRU meta data.
                key_tracker_.splice(key_tracker_.end(), key_tracker_,
                    lru_reference(ct->second));

                // Return the connection back to the cache only if the number
                // of connections does not need to be shrunk.
                if (num_existing_connections(ct->second) <=
                    max_num_connections(ct->second))
                {
                    // Add the connection to the entry.
                    cached_connections(ct->second).push_back(conn);

                    ++reclaims_;

#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
                    conn->set_state(Connection::state_reclaimed);
#endif
                }
                else
                {
                    // Adjust the number of existing connections for this key.
                    decrement_connection_count(ct->second);

                    // do the accounting
                    ++evictions_;

                    // the connection itself will go out of scope on return
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
                    conn->set_state(Connection::state_deleting);
#endif
                }

                check_invariants();
            }
        }

        /// Returns true if the overall connection count is equal to or larger
        /// than the maximum number of overall connections, and false otherwise.
        bool full() const
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return (connections_ >= max_connections_);
        }

        /// Returns true if the connection count for \a l is equal to or larger
        /// than the maximum connection count per locality, and false otherwise.
        bool full(key_type const& l) const
        {
            std::lock_guard<mutex_type> lock(mtx_);

            if (!cache_.count(l))
                return false || (connections_ >= max_connections_);

            auto ct = cache_.find(l);
            if (ct == cache_.end())
                return false;

            return (num_existing_connections(ct->second) >=
                       max_num_connections(ct->second)) ||
                (connections_ >= max_connections_);
        }

        /// Destroys all connections in the cache, and resets all counts.
        ///
        /// \note Calling this function while connections are still checked out
        ///       of the cache is a bad idea, and will violate this class'
        ///       invariants.
        void clear()
        {
            std::lock_guard<mutex_type> lock(mtx_);

            // Verify that no connections are currently checked out. Clearing
            // the cache while connections are outstanding is a caller error
            // that would silently corrupt connection counts.
            for (auto const& entry : cache_)
            {
                if (cached_connections(entry.second).size() <
                    num_existing_connections(entry.second))
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "connection_cache::clear",
                        "clearing cache while connections are still "
                        "checked out");
                }
            }

            key_tracker_.clear();
            cache_.clear();
            connections_ = 0;

            insertions_ = 0;
            evictions_ = 0;
            hits_ = 0;
            misses_ = 0;
            reclaims_ = 0;
            reservation_failures_ = 0;

            check_invariants();
        }

        /// Destroys all connections for the given locality in the cache, reset
        /// all associated counts.
        ///
        /// \note Calling this function while connections are still checked out
        ///       of the cache is a bad idea, and will violate this classes
        ///       invariants.
        void clear(key_type const& l)
        {
            std::lock_guard<mutex_type> lock(mtx_);

            // Check if this key already exists in the cache.
            typename cache_type::iterator it = cache_.find(l);
            if (it != cache_.end())
            {
                // Verify that no connections are currently checked out for
                // this locality. Clearing while connections are outstanding
                // is a caller error that would corrupt connection counts.
                if (cached_connections(it->second).size() <
                    num_existing_connections(it->second))
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "connection_cache::clear",
                        "clearing locality cache entry while connections "
                        "are still checked out");
                }

                // Remove from LRU meta data.
                key_tracker_.erase(lru_reference(it->second));

                // correct counter to avoid assertions later on
                std::size_t num_existing = num_existing_connections(it->second);
                connections_ -= num_existing;
                evictions_ += num_existing;

                // Erase entry if key exists in the cache.
                cache_.erase(it);
            }

            check_invariants();
        }

        /// Destroys all connections for the given locality in the cache, reset
        /// all associated counts.
        void clear(
            key_type const& l, [[maybe_unused]] connection_type const& conn)
        {
            std::lock_guard<mutex_type> lock(mtx_);

            // Check if this key already exists in the cache.
            typename cache_type::iterator const it = cache_.find(l);
            if (it != cache_.end())
            {
                // Verify that there are actually outstanding (checked-out)
                // connections for this locality. If all existing connections
                // are already cached, this clear is a caller error.
                if (cached_connections(it->second).size() >=
                    num_existing_connections(it->second))
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "connection_cache::clear",
                        "clearing a connection that was not "
                        "properly checked out");
                }

                // Adjust the number of existing connections for this key.
                decrement_connection_count(it->second);

                // do the accounting
                ++evictions_;

                // the connection itself will go out of scope on return
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
                conn->set_state(Connection::state_deleting);
#endif
            }

            check_invariants();
        }

        // access statistics
        std::int64_t get_cache_insertions(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(insertions_, reset);
        }

        std::int64_t get_cache_evictions(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(evictions_, reset);
        }

        std::int64_t get_cache_hits(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(hits_, reset);
        }

        std::int64_t get_cache_misses(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(misses_, reset);
        }

        std::int64_t get_cache_reclaims(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(reclaims_, reset);
        }

        // Returns the cumulative count of times get_or_reserve returned false
        // because the connection pool was fully checked out (pool saturation).
        // Unlike cache-misses, this counter is zero during normal operation and
        // only increments when parcels must be deferred due to exhaustion.
        std::int64_t get_reservation_failures(bool reset)
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return util::get_and_reset_value(reservation_failures_, reset);
        }

        // Returns the current number of connections tracked by the cache
        // (both available and checked-out). Use together with
        // get_max_connections() to compute the pool utilisation ratio.
        std::int64_t get_num_connections() const
        {
            std::lock_guard<mutex_type> lock(mtx_);
            return static_cast<std::int64_t>(connections_);
        }

        // Returns the configured maximum number of connections. This value is
        // fixed at construction time (hpx.max_connections ini key).
        std::int64_t get_max_connections() const
        {
            return static_cast<std::int64_t>(max_connections_);
        }

    private:
        // Emit a throttled LPT_(warning) when the pool is exhausted. Called
        // while mtx_ is held; must not re-acquire it.  Logs at most once per
        // 5 seconds to avoid flooding the log on sustained saturation.
        void maybe_log_exhaustion()
        {
            using namespace std::chrono;
            auto const now_ns =
                static_cast<std::int64_t>(duration_cast<nanoseconds>(
                    steady_clock::now().time_since_epoch())
                        .count());
            constexpr std::int64_t throttle_ns = 5'000'000'000LL;    // 5 s
            if (now_ns - last_exhaustion_log_ns_ >= throttle_ns)
            {
                last_exhaustion_log_ns_ = now_ns;
                LPT_(warning).format(
                    "parcelport connection cache exhausted: {}/{} connections "
                    "in use, {} deferred reservation(s) since last reset",
                    connections_, max_connections_, reservation_failures_);
            }
        }

        /// Verify class invariants
        void check_invariants() const
        {
#if defined(HPX_DEBUG)
            using const_iterator = typename cache_type::const_iterator;

            size_type in_cache_count = 0, total_count = 0;
            const_iterator end = cache_.end();
            for (const_iterator ct = cache_.begin(); ct != end; ++ct)
            {
                cache_value_type const& val = ct->second;

                std::size_t num_connections = cached_connections(val).size();
                std::size_t num_existing = num_existing_connections(val);

                // The separate item counter has to properly count all the
                // existing elements, not only those in the cache entry.
                HPX_ASSERT(num_connections <= num_existing);

                // Count all connections (both those in the cache and those
                // checked out of the cache).
                in_cache_count += num_connections;
                total_count += num_existing;
            }

            // Overall connection count should be larger than or equal to the
            // number of entries in the cache.
            HPX_ASSERT(in_cache_count <= connections_);

            // Overall connection count should be equal to the sum of connection
            // counts for all localities.
            HPX_ASSERT(total_count == connections_);

            // The list of key trackers should have the same size as the cache.
            HPX_ASSERT(key_tracker_.size() == cache_.size());
#endif
        }

        /// Evict the least recently used removable entry from the cache if the
        /// cache is full.
        ///
        /// \returns Returns true if an entry was evicted or if the cache is not
        ///          full, and false if nothing could be evicted.
        bool free_space()
        {
            // If the cache isn't full, just return true.
            if (connections_ < max_connections_)
                return true;

            // Find the least recently used key.
            auto kt = key_tracker_.begin();
            while (connections_ >= max_connections_)
            {
                // Find the least recently used keys data.
                auto ct = cache_.find(*kt);
                if (ct == cache_.end())
                    break;

                // If the entry is empty, ignore it and try the next least
                // recently used entry.
                if (cached_connections(ct->second).empty())
                {
                    // Remove the key if its connection count is zero.
                    if (0 == num_existing_connections(ct->second))
                    {
                        cache_.erase(ct);
                        key_tracker_.erase(kt);
                        kt = key_tracker_.begin();
                    }
                    else
                    {
                        // REVIEW: Should we reorder key_tracker_ to speed up
                        // the eviction?
                        ++kt;
                    }

                    // If we've gone through key_tracker_ and haven't found
                    // anything evict-able, then all the entries must be
                    // currently checked out.
                    if (key_tracker_.end() == kt)
                        return false;

                    continue;
                }

                // Remove the oldest connection.
                cached_connections(ct->second).pop_front();

                // Adjust the overall and per-locality connection count.
                decrement_connection_count(ct->second);

                // Statistics
                ++evictions_;
            }

            return true;
        }

        mutable mutex_type mtx_;
        size_type const max_connections_;
        size_type const max_connections_per_locality_;
        key_tracker_type key_tracker_;
        cache_type cache_;
        size_type connections_;
        bool shutting_down_;

        // statistics support
        std::int64_t insertions_;
        std::int64_t evictions_;
        std::int64_t hits_;
        std::int64_t misses_;
        std::int64_t reclaims_;

        // Count of get_or_reserve() calls that returned false because every
        // connection slot was checked out (pool exhaustion, not normal misses).
        std::int64_t reservation_failures_;

        // Timestamp of the last LPT_(warning) emission, in nanoseconds since
        // epoch (std::chrono::steady_clock). Used for log throttling.
        std::int64_t last_exhaustion_log_ns_;
    };
}    // namespace hpx::util

#endif
