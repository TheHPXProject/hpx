//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/cache/policies/always.hpp>
#include <hpx/cache/statistics/no_statistics.hpp>

#include <algorithm>
#include <deque>
#include <functional>
#include <map>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::cache {

    ///////////////////////////////////////////////////////////////////////////
    /// \class local_cache local_cache.hpp hpx/cache/local_cache.hpp
    ///
    /// \brief The \a local_cache implements the basic functionality needed for
    ///        a local (non-distributed) cache.
    ///
    /// \tparam Key           The type of the keys to use to identify the
    ///                       entries stored in the cache
    /// \tparam Entry         The type of the items to be held in the cache,
    ///                       must model the CacheEntry concept
    /// \tparam UpdatePolicy  A (optional) type specifying a (binary) function
    ///                       object used to sort the cache entries based on
    ///                       their 'age'. The 'oldest' entries (according to
    ///                       this sorting criteria) will be discarded first if
    ///                       the maximum capacity of the cache is reached.
    ///                       The default is std::less<Entry>. The function
    ///                       object will be invoked using 2 entry instances of
    ///                       the type \a Entry. This type must model the
    ///                       UpdatePolicy model.
    /// \tparam InsertPolicy  A (optional) type specifying a (unary) function
    ///                       object used to allow global decisions whether a
    ///                       particular entry should be added to the cache or
    ///                       not. The default is \a policies#always,
    ///                       imposing no global insert related criteria on the
    ///                       cache. The function object will be invoked using
    ///                       the entry instance to be inserted into the cache.
    ///                       This type must model the InsertPolicy model.
    /// \tparam CacheStorage  A (optional) container type used to store the
    ///                       cache items. The container must be an associative
    ///                       and STL compatible container.The default is a
    ///                       std::map<Key, Entry>.
    /// \tparam Statistics    A (optional) type allowing to collect some basic
    ///                       statistics about the operation of the cache
    ///                       instance. The type must conform to the
    ///                       CacheStatistics concept. The default value is
    ///                       the type \a statistics#no_statistics which does
    ///                       not collect any numbers, but provides empty stubs
    ///                       allowing the code to compile.
    HPX_CXX_CORE_EXPORT template <typename Key, typename Entry,
        typename UpdatePolicy = std::less<Entry>,
        typename InsertPolicy = policies::always<Entry>,
        typename CacheStorage = std::map<Key, Entry>,
        typename Statistics = statistics::no_statistics>
    class local_cache
    {
        ///////////////////////////////////////////////////////////////////////
        // The UpdatePolicy Concept expects to get passed references to
        // instances of the Entry type. But our internal data structures hold
        // a pointer to the stored entry only. We use the \a adapt function
        // object to wrap any user supplied UpdatePolicy, dereferencing the
        // pointers.
        template <typename Func, typename Iterator>
        struct adapt
        {
            explicit adapt(Func const& f)
              : f_(f)
            {
            }
            explicit adapt(Func&& f) noexcept
              : f_(HPX_MOVE(f))
            {
            }

            bool operator()(Iterator const& lhs, Iterator const& rhs) const
            {
                return f_((*lhs).second, (*rhs).second);
            }

            Func f_;    // user supplied UpdatePolicy
        };

    public:
        using key_type = Key;
        using entry_type = Entry;
        using update_policy_type = UpdatePolicy;
        using insert_policy_type = InsertPolicy;
        using storage_type = CacheStorage;
        using statistics_type = Statistics;

        using value_type = typename entry_type::value_type;
        using size_type = typename storage_type::size_type;
        using storage_value_type = typename storage_type::value_type;

    private:
        using iterator = typename storage_type::iterator;
        using const_iterator = typename storage_type::const_iterator;

        using heap_type = std::deque<iterator>;
        using heap_iterator = typename heap_type::iterator;

        using adapted_update_policy_type = adapt<UpdatePolicy, iterator>;

        using update_on_exit = typename statistics_type::update_on_exit;

    public:
        ///////////////////////////////////////////////////////////////////////
        /// \brief Construct an instance of a local_cache.
        ///
        /// \param max_size   [in] The maximal size this cache is allowed to
        ///                   reach any time. The default is zero (no size
        ///                   limitation). The unit of this value is usually
        ///                   determined by the unit of the values returned by
        ///                   the entry's \a get_size function.
        /// \param up         [in] An instance of the \a UpdatePolicy to use
        ///                   for this cache. The default is to use a default
        ///                   constructed instance of the type as defined by
        ///                   the \a UpdatePolicy template parameter.
        /// \param ip         [in] An instance of the \a InsertPolicy to use for
        ///                   this cache. The default is to use a default
        ///                   constructed instance of the type as defined by
        ///                   the \a InsertPolicy template parameter.
        ///
        explicit local_cache(size_type max_size = 0,
            update_policy_type const& up = update_policy_type(),
            insert_policy_type const& ip = insert_policy_type())
          : max_size_(max_size)
          , current_size_(0)
          , update_policy_(up)
          , insert_policy_(ip)
        {
        }

        local_cache(local_cache const& other) = default;
        local_cache(local_cache&& other) = default;
        local_cache& operator=(local_cache const& other) = default;
        local_cache& operator=(local_cache&& other) = default;

        ~local_cache() = default;

        ///////////////////////////////////////////////////////////////////////
        /// \brief Return current size of the cache.
        ///
        /// \returns The current size of this cache instance.
        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return current_size_;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Access the maximum size the cache is allowed to grow to.
        ///
        /// \note       The unit of this value is usually determined by the
        ///             unit of the return values of the entry's function
        ///             \a entry#get_size.
        ///
        /// \returns    The maximum size this cache instance is currently
        ///             allowed to reach. If this number is zero the cache has
        ///             no limitation with regard to a maximum size.
        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            return max_size_;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Change the maximum size this cache can grow to
        ///
        /// \param max_size    [in] The new maximum size this cache will be
        ///             allowed to grow to.
        ///
        /// \returns    This function returns \a true if successful. It returns
        ///             \a false if the new \a max_size is smaller than the
        ///             current limit and the cache could not be shrunk to
        ///             the new maximum size.
        bool reserve(size_type max_size)
        {
            // we need to shrink the cache if the new max size if smaller than
            // the old one
            bool retval = true;
            if (max_size && max_size < max_size_ &&
                !free_space(static_cast<long>(max_size_ - max_size)))
            {
                retval = false;    // not able to shrink cache
            }

            max_size_ = max_size;    // change capacity in any case
            return retval;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Check whether the cache currently holds an entry identified
        ///        by the given key
        ///
        /// \param k      [in] The key for the entry which should be looked up
        ///               in the cache.
        ///
        /// \note         This function does not call the entry's function
        ///               \a entry#touch. It just checks if the cache contains
        ///               an entry corresponding to the given key.
        ///
        /// \returns      This function returns \a true if the cache holds the
        ///               referenced entry, otherwise it returns \a false.
        [[nodiscard]] bool holds_key(key_type const& k) const
        {
            return store_.find(k) != store_.end();
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Get a specific entry identified by the given key.
        ///
        /// \param[in] k The key for the entry which should be retrieved
        ///               from the cache.
        /// \param[out] realkey Return the full real key found in the cache
        /// \param[out] val If the entry indexed by the key is found in the
        ///               cache this value on successful return will be a copy
        ///               of the corresponding entry.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the value corresponding to the provided
        ///               key is found in the cache.
        ///
        /// \returns      This function returns \a true if the cache holds the
        ///               referenced entry, otherwise it returns \a false.
        bool get_entry(key_type const& k, key_type& realkey, entry_type& val)
        {
            update_on_exit update(statistics_, statistics::method::get_entry);

            // locate the requested entry
            iterator it = store_.find(k);
            if (it == store_.end())
            {
                statistics_.got_miss();    // update statistics
                return false;              // doesn't exist in this cache
            }

            // touch the found entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            // return the value
            realkey = (*it).first;
            val = (*it).second;

            return true;
        }

        /// \brief Get a specific entry identified by the given key.
        ///
        /// \param k      [in] The key for the entry which should be retrieved
        ///               from the cache.
        /// \param val    [out] If the entry indexed by the key is found in the
        ///               cache this value on successful return will be a copy
        ///               of the corresponding entry.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the value corresponding to the provided
        ///               key is found in the cache.
        ///
        /// \returns      This function returns \a true if the cache holds the
        ///               referenced entry, otherwise it returns \a false.
        bool get_entry(key_type const& k, entry_type& val)
        {
            update_on_exit update(statistics_, statistics::method::get_entry);

            // locate the requested entry
            iterator it = store_.find(k);
            if (it == store_.end())
            {
                statistics_.got_miss();    // update statistics
                return false;              // doesn't exist in this cache
            }

            // touch the found entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            // return the value
            val = (*it).second;

            return true;
        }

        /// \brief Get a specific entry identified by the given key.
        ///
        /// \param k      [in] The key for the entry which should be retrieved
        ///               from the cache
        /// \param val    [out] If the entry indexed by the key is found in the
        ///               cache this value on successful return will be a copy
        ///               of the corresponding value.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the value corresponding to the provided
        ///               is found in the cache.
        ///
        /// \returns      This function returns \a true if the cache holds the
        ///               referenced entry, otherwise it returns \a false.
        bool get_entry(key_type const& k, value_type& val)
        {
            update_on_exit update(statistics_, statistics::method::get_entry);

            // locate the requested entry
            iterator it = store_.find(k);
            if (it == store_.end())
            {
                statistics_.got_miss();    // update statistics
                return false;              // doesn't exist in this cache
            }

            // touch the found entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            // return the value
            val = (*it).second.get();

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Insert a new element into this cache
        ///
        /// \param k      [in] The key for the entry which should be added to
        ///               the cache.
        /// \param val    [in] The value which should be added to the cache.
        ///
        /// \note         This function invokes both, the insert policy as
        ///               provided to the constructor and the function
        ///               \a entry#insert of the newly constructed entry
        ///               instance. If either of these functions returns false
        ///               the key/value pair doesn't get inserted into the
        ///               cache and the \a insert function will return
        ///               \a false. Other reasons for this function to fail
        ///               (return \a false) are a) the key/value pair is
        ///               already held in the cache or b) inserting the new
        ///               value into the cache maxed out its capacity and
        ///               it was not possible to free any of the existing
        ///               entries.
        ///
        /// \returns      This function returns \a true if the entry has been
        ///               successfully added to the cache, otherwise it returns
        ///               \a false.
        bool insert(key_type const& k, value_type const& val)
        {
            entry_type e(val);
            return insert(k, e);
        }

        bool insert(key_type const& k, value_type&& val)
        {
            return insert(k, entry_type(HPX_MOVE(val)));
        }

        /// \brief Insert a new entry into this cache
        ///
        /// \param k      [in] The key for the entry which should be added to
        ///               the cache.
        /// \param e      [in] The entry which should be added to the cache.
        ///
        /// \note         This function invokes both, the insert policy as
        ///               provided to the constructor and the function
        ///               \a entry#insert of the provided entry instance.
        ///               If either of these functions returns false
        ///               the key/value pair doesn't get inserted into the
        ///               cache and the \a insert function will return
        ///               \a false. Other reasons for this function to fail
        ///               (return \a false) are a) the key/value pair is
        ///               already held in the cache or b) inserting the new
        ///               value into the cache maxed out its capacity and
        ///               it was not possible to free any of the existing
        ///               entries.
        ///
        /// \returns      This function returns \a true if the entry has been
        ///               successfully added to the cache, otherwise it returns
        ///               \a false.
        template <typename Entry_,
            std::enable_if_t<
                std::is_convertible_v<std::decay_t<Entry_>, entry_type>, int> =
                0>
        bool insert(key_type const& k, Entry_&& e)
        {
            update_on_exit update(
                statistics_, statistics::method::insert_entry);

            // ask entry if it really wants to be inserted
            if (!insert_policy_(e) || !e.insert())
                return false;

            // make sure cache doesn't get too large
            size_type entry_size = e.get_size();
            if (0 != max_size_ && current_size_ + entry_size > max_size_ &&
                !free_space(
                    static_cast<long>(current_size_ - max_size_ + entry_size)))
            {
                return false;
            }

            // insert new entry to cache
            std::pair<iterator, bool> p =
                store_.insert(storage_value_type(k, HPX_FORWARD(Entry_, e)));
            if (!p.second)
                return false;

            current_size_ += entry_size;

            // update the entry heap
            entry_heap_.push_back(p.first);
            std::push_heap(
                entry_heap_.begin(), entry_heap_.end(), update_policy_);

            // update statistics
            statistics_.got_insertion();

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Update an existing element in this cache
        ///
        /// \param k      [in] The key for the value which should be updated in
        ///               the cache.
        /// \param val    [in] The value which should be used as a replacement
        ///               for the existing value in the cache. Any existing
        ///               cache entry is not changed except for its value.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the indexed value is found in the cache.
        /// \note         The difference to the other overload of the \a insert
        ///               function is that this overload replaces the cached
        ///               value only, while the other overload replaces the
        ///               whole cache entry, updating the cache entry
        ///               properties.
        ///
        /// \returns      This function returns \a true if the entry has been
        ///               successfully updated, otherwise it returns \a false.
        ///               If the entry currently is not held by the cache it is
        ///               added and the return value reflects the outcome of
        ///               the corresponding insert operation.
        template <typename Value,
            std::enable_if_t<
                std::is_convertible_v<std::decay_t<Value>, value_type>, int> =
                0>
        bool update(key_type const& k, Value&& val)
        {
            update_on_exit update(
                statistics_, statistics::method::update_entry);

            iterator it = store_.find(k);
            if (it == store_.end())
            {
                // doesn't exist in this cache
                statistics_.got_miss();    // update statistics
                return insert(
                    k, HPX_FORWARD(Value, val));    // insert into cache
            }

            // update cache entry
            (*it).second.get() = HPX_FORWARD(Value, val);

            // touch the entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Update an existing element in this cache
        ///
        /// \param k      [in] The key for the value which should be updated in
        ///               the cache.
        /// \param val    [in] The value which should be used as a replacement
        ///               for the existing value in the cache. Any existing
        ///               cache entry is not changed except for its value.
        /// \param f      [in] A callable taking two arguments, \a k and the
        ///               key found in the cache (in that order). If \a f
        ///               returns true, then the update will continue. If \a f
        ///               returns false, then the update will not succeed.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the indexed value is found in the cache.
        /// \note         The difference to the other overload of the \a insert
        ///               function is that this overload replaces the cached
        ///               value only, while the other overload replaces the
        ///               whole cache entry, updating the cache entry
        ///               properties.
        ///
        /// \returns      This function returns \a true if the entry has been
        ///               successfully updated, otherwise it returns \a false.
        ///               If the entry currently is not held by the cache it is
        ///               added and the return value reflects the outcome of
        ///               the corresponding insert operation.
        template <typename F, typename Value,
            typename = std::enable_if_t<
                std::is_convertible_v<std::decay_t<Value>, value_type>>>
        bool update_if(key_type const& k, Value&& val, F&& f)
        {
            update_on_exit update(
                statistics_, statistics::method::update_entry);

            iterator it = store_.find(k);
            if (it == store_.end())
            {
                // doesn't exist in this cache
                statistics_.got_miss();    // update statistics
                return insert(
                    k, HPX_FORWARD(Value, val));    // insert into cache
            }

            if (!f(k, (*it).first))
                return false;

            // update cache entry
            (*it).second.get() = HPX_FORWARD(Value, val);

            // touch the entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            return true;
        }

        /// \brief Update an existing entry in this cache
        ///
        /// \param k      [in] The key for the entry which should be updated in
        ///               the cache.
        /// \param e      [in] The entry which should be used as a replacement
        ///               for the existing entry in the cache. Any existing
        ///               entry is first removed and then this entry is added.
        ///
        /// \note         The function will call the entry's \a entry#touch
        ///               function if the indexed value is found in the cache.
        /// \note         The difference to the other overload of the \a insert
        ///               function is that this overload replaces the whole
        ///               cache entry, while the other overload replaces the
        ///               cached value only, leaving the cache entry properties
        ///               untouched.
        ///
        /// \returns      This function returns \a true if the entry has been
        ///               successfully updated, otherwise it returns \a false.
        ///               If the entry currently is not held by the cache it is
        ///               added and the return value reflects the outcome of
        ///               the corresponding insert operation.
        template <typename Entry_,
            std::enable_if_t<
                std::is_convertible_v<std::decay_t<Entry_>, entry_type>, int> =
                0>
        bool update(key_type const& k, Entry_&& e)
        {
            update_on_exit update(
                statistics_, statistics::method::update_entry);

            iterator it = store_.find(k);
            if (it == store_.end())
            {
                // doesn't exist in this cache
                statistics_.got_miss();    // update statistics
                return insert(k, e);       // insert into cache
            }

            // make sure the old entry agrees to be removed
            if (!(*it).second.remove())
                return false;    // entry doesn't want to be removed

            // make sure the new entry agrees to be inserted
            if (!insert_policy_(e) || !e.insert())
                return false;    // entry doesn't want to be inserted

            // update cache entry
            (*it).second = HPX_FORWARD(Entry_, e);

            // touch the entry
            if ((*it).second.touch())
            {
                // reorder heap based on the changed entry attributes
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            // update statistics
            statistics_.got_hit();

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Remove stored entries from the cache for which the supplied
        ///        function object returns true.
        ///
        /// \param ep     [in] This parameter has to be a (unary) function
        ///               object. It is invoked for each of the entries
        ///               currently held in the cache. An entry is considered
        ///               for removal from the cache whenever the value
        ///               returned from this invocation is \a true. Even then
        ///               the entry might not be removed from the cache as its
        ///               \a entry#remove function might return false.
        ///
        /// \returns      This function returns the overall size of the removed
        ///               entries (which is the sum of the values returned by
        ///               the \a entry#get_size functions of the removed
        ///               entries).
        template <typename Func = policies::always<storage_value_type>>
        size_type erase(Func&& ep = Func())
        {
            update_on_exit update(statistics_, statistics::method::erase_entry);

            size_type erased = 0;
            for (heap_iterator it = entry_heap_.begin();
                it != entry_heap_.end();
                /**/)
            {
                iterator sit = *it;

                // check if this item needs to be erased
                // do not remove this entry from the cache if either the
                // function object or the entries' remove function return false
                typename storage_type::value_type& val = *sit;
                if (ep(val) && val.second.remove())
                {
                    // update the current size and the overall size of the
                    // removed items
                    size_type entry_size = (*(*it)).second.get_size();
                    current_size_ -= entry_size;
                    erased += entry_size;

                    // we remove the element manually, forcing the heap to be
                    // rebuilt at the end
                    it = entry_heap_.erase(it);

                    // remove the cache entry
                    store_.erase(sit);

                    // update statistics
                    statistics_.got_eviction();
                }
                else
                {
                    // do not remove this item from cache
                    ++it;
                }
            }

            // reorder heap based on the changed entry list
            std::make_heap(
                entry_heap_.begin(), entry_heap_.end(), update_policy_);

            return erased;
        }

        /// \brief Remove all stored entries from the cache
        ///
        /// \note         All entries are considered for removal, but in the
        ///               end an entry might not be removed from the cache as
        ///               its \a entry#remove function might return false.
        ///               This function is very useful for instance in
        ///               conjunction with an entry's \a entry#remove function
        ///               enforcing additional criteria like entry expiration,
        ///               etc.
        ///
        /// \returns      This function returns the overall size of the removed
        ///               entries (which is the sum of the values returned by
        ///               the \a entry#get_size functions of the removed
        ///               entries).
        size_type erase()
        {
            return erase(policies::always<storage_value_type>());
        }

        /// \brief Clear the cache
        ///
        /// Unconditionally removes all stored entries from the cache.
        void clear()
        {
            store_.clear();
            entry_heap_.clear();
            statistics_.clear();
            current_size_ = 0;
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Allow to access the embedded statistics instance
        ///
        /// \returns      This function returns a reference to the statistics
        ///               instance embedded inside this cache
        [[nodiscard]] constexpr statistics_type const& get_statistics()
            const noexcept
        {
            return statistics_;
        }

        [[nodiscard]] statistics_type& get_statistics() noexcept
        {
            return statistics_;
        }

    protected:
        ///////////////////////////////////////////////////////////////////////
        // Free some space in the cache
        bool free_space(long num_free)
        {
            if (entry_heap_.empty())
                return false;

            bool is_heap = true;
            for (heap_iterator it = entry_heap_.begin();
                num_free > 0 && it != entry_heap_.end();
                /**/)
            {
                iterator sit = *it;
                if (!(*sit).second.remove())
                {
                    ++it;    // do not remove this entry from the cache
                }
                else
                {
                    size_type entry_size = (*(*it)).second.get_size();

                    if (it == entry_heap_.begin())
                    {
                        // if we're at the top of the list, just pop the item
                        // this doesn't disturb the heap property of the heap
                        ++it;
                        std::pop_heap(entry_heap_.begin(), entry_heap_.end(),
                            update_policy_);
                        entry_heap_.pop_back();
                    }
                    else
                    {
                        // otherwise we remove the element manually, forcing
                        // the heap to be rebuilt at the end
                        it = entry_heap_.erase(it);
                        is_heap = false;
                    }

                    // remove the cache entry
                    store_.erase(sit);
                    num_free -= static_cast<long>(entry_size);
                    current_size_ -= entry_size;

                    // update statistics
                    statistics_.got_eviction();
                }
            }

            // reorder heap based on the changed entry list
            if (!is_heap)
            {
                std::make_heap(
                    entry_heap_.begin(), entry_heap_.end(), update_policy_);
            }

            return num_free <= 0;
        }

    private:
        size_type max_size_;        // cache capacity
        size_type current_size_;    // current cache size
        storage_type store_;        // the cache itself

        // we store a list of pointers to the held keys in a std::heap which
        // is being sorted based on the criteria defined by the UpdatePolicy
        heap_type entry_heap_;

        adapted_update_policy_type update_policy_;
        insert_policy_type insert_policy_;

        statistics_type statistics_;    // embedded statistics instance
    };
}    // namespace hpx::util::cache
