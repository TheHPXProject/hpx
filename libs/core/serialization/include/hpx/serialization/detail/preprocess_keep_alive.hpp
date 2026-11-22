//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::serialization::detail {

    // A buffer a serialize() function hands to the archive as a zero-copy
    // chunk although the serialized object doesn't own that memory, e.g. a
    // host copy of device memory. 'owner' keeps the memory alive.
    HPX_CXX_CORE_EXPORT struct keep_alive_buffer
    {
        std::shared_ptr<void> owner;
        void const* data = nullptr;
        std::size_t size = 0;
    };

    // A buffer and the objects serialized through tracked pointers (e.g. a
    // shared_ptr) the object it was created for is part of, outermost first.
    HPX_CXX_CORE_EXPORT struct keep_alive_entry
    {
        keep_alive_buffer buffer;
        std::vector<void const*> enclosing;
    };

    // Buffers in the order the objects they were created for are serialized.
    HPX_CXX_CORE_EXPORT using keep_alive_list = std::vector<keep_alive_entry>;

    // This class carries keep-alive buffers from the preprocessing pass of a
    // parcel to its real serialization pass. The preprocessing pass creates
    // them (and may register futures for their contents), the parcel holds
    // them until it has been sent, and the real pass takes them again. The
    // owner of the preprocessing archive creates it before serializing, to
    // say that it does this (and waits for the futures before the real pass);
    // serialization creates no buffers for archives without it.
    //
    // The buffers are matched to the objects by the order in which these are
    // serialized: the n-th object needing a buffer gets the n-th buffer. A
    // parcel serializes the same objects in the same order in every
    // preprocessing round and in the real pass, while an object a serialize()
    // function creates on the fly has a new address in every pass. The real
    // pass checks the size of each buffer, and throws if a parcel was
    // serialized differently than it was preprocessed.
    //
    // A future that isn't ready during a preprocessing round is serialized
    // without the objects it holds, which the next round serializes. The
    // buffers of these objects are inserted where the future is (see
    // enter_future()), the following ones keep their positions.
    //
    // The parcels of a message are serialized into one archive: an object
    // that an earlier parcel of the message has written already through a
    // tracked pointer is written as a back-reference, and the buffers of the
    // objects it contains aren't needed (see skip_back_reference). Each
    // buffer knows these objects from preprocessing (enter() and leave()).
    // Within a parcel, the first occurrence of such an object can move to an
    // earlier position in a later preprocessing round (into a future that
    // wasn't ready before): the buffers created at its former position are
    // dropped there (see drop_back_reference).
    HPX_CXX_CORE_EXPORT class preprocess_keep_alive
    {
    public:
        preprocess_keep_alive() = default;

        // Preprocessing: the buffer an earlier preprocessing round has
        // created for the next object needing one, or nullptr if this round
        // creates it (add()).
        [[nodiscard]] keep_alive_buffer const* next(std::size_t const size)
        {
            if (inserting_ == 0 && next_ != buffers_.size())
            {
                return &checked(next_++, size);
            }
            return nullptr;
        }

        // Preprocessing: the buffer created for the object next() returned
        // nullptr for.
        keep_alive_buffer const& add(keep_alive_buffer buffer)
        {
            HPX_ASSERT(inserting_ != 0 || next_ == buffers_.size());
            auto const it = buffers_.insert(
                buffers_.begin() + static_cast<std::ptrdiff_t>(next_++),
                keep_alive_entry{HPX_MOVE(buffer), enclosing_});
            return it->buffer;
        }

        // Preprocessing: the object at 'object', pointed to by a tracked
        // pointer, is serialized until leave() is called.
        void enter(void const* object)
        {
            enclosing_.push_back(object);
        }

        void leave() noexcept
        {
            HPX_ASSERT(!enclosing_.empty());
            enclosing_.pop_back();
        }

        // Preprocessing: the future with the shared state 'state' isn't
        // ready, the objects it holds are serialized in the next round.
        void future_pending(void const* state)
        {
            pending_.push_back(state);
        }

        // Preprocessing: the future with the shared state 'state' is ready
        // and is serialized with the objects it holds. If it wasn't ready in
        // the previous round, the buffers of these objects are new: they are
        // inserted at this position until leave_future(), instead of being
        // matched to the buffers of that round. Returns whether
        // leave_future() is needed.
        //
        // A future serialized more than once may have been pending only at
        // some of its occurrences in the previous round (it became ready in
        // between): each occurrence that was pending inserts its buffers, the
        // others are matched to the buffers created for them.
        [[nodiscard]] bool enter_future(void const* state)
        {
            auto const it =
                std::find(revealed_.begin(), revealed_.end(), state);
            if (it == revealed_.end())
            {
                return false;
            }
            revealed_.erase(it);
            ++inserting_;
            return true;
        }

        void leave_future() noexcept
        {
            HPX_ASSERT(inserting_ != 0);
            --inserting_;
        }

        // The real pass: the buffer of the next object needing one.
        [[nodiscard]] keep_alive_buffer const& get_next(std::size_t const size)
        {
            std::size_t const pos = next_++;
            if (pos >= buffers_.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "preprocess_keep_alive::get_next",
                    "no keep-alive buffer for object {} of this parcel: it "
                    "was serialized differently than during preprocessing",
                    pos);
            }
            return checked(pos, size);
        }

        // Preprocessing: the object at 'object' is written as a
        // back-reference. If the next buffers were created for the objects it
        // contains (an earlier round has written it here for the first time),
        // its first occurrence has new buffers (see enter_future()): these
        // aren't used anymore.
        void drop_back_reference(void const* object)
        {
            // an object that is still being serialized is referred to by one
            // it contains (a cycle): this is its first occurrence, the
            // buffers that follow belong to it
            if (std::find(enclosing_.begin(), enclosing_.end(), object) !=
                enclosing_.end())
            {
                return;
            }

            auto const first =
                buffers_.begin() + static_cast<std::ptrdiff_t>(next_);
            auto last = first;
            while (last != buffers_.end() &&
                std::find(last->enclosing.begin(), last->enclosing.end(),
                    object) != last->enclosing.end())
            {
                ++last;
            }
            buffers_.erase(first, last);
        }

        // The real pass: the object at 'object' is written as a
        // back-reference to an earlier parcel of the same message (see
        // parcel_start). The buffers of the objects it contains, which this
        // parcel's preprocessing created next, are not sent.
        void skip_back_reference(void const* object) noexcept
        {
            while (next_ != buffers_.size())
            {
                auto const& enclosing = buffers_[next_].enclosing;
                if (std::find(enclosing.begin(), enclosing.end(), object) ==
                    enclosing.end())
                {
                    break;
                }
                ++next_;
            }
        }

        // A new pass over the same objects (preprocessing round) starts at
        // the first one again; the futures that weren't ready in the last
        // round may be now.
        void reset_position() noexcept
        {
            next_ = 0;
            enclosing_.clear();
            revealed_ = std::exchange(pending_, {});
            inserting_ = 0;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return buffers_.empty();
        }

        // hand the buffers to the parcel after preprocessing; none are left
        // here (the archive may preprocess the next parcel)
        [[nodiscard]] keep_alive_list move_buffers() noexcept
        {
            reset_position();
            revealed_.clear();
            return std::exchange(buffers_, keep_alive_list{});
        }

        // The real pass: make the buffers of the parcel that is serialized
        // next, starting at 'parcel_start' in the archive (bytes_written()),
        // available to its serialization (they replace those of an earlier
        // parcel); the parcel keeps its own references.
        void set_buffers(keep_alive_list const& buffers,
            std::uint64_t const parcel_start = 0)
        {
            buffers_ = buffers;
            reset_position();
            revealed_.clear();
            parcel_start_ = parcel_start;
        }

        // The real pass: where the current parcel starts in the archive. An
        // object the output pointer tracker recorded before that belongs to
        // an earlier parcel of the message.
        [[nodiscard]] std::uint64_t parcel_start() const noexcept
        {
            return parcel_start_;
        }

    private:
        keep_alive_buffer const& checked(
            std::size_t const pos, std::size_t const size) const
        {
            keep_alive_buffer const& buffer = buffers_[pos].buffer;
            if (buffer.size != size)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "preprocess_keep_alive::checked",
                    "the keep-alive buffer for object {} of this parcel has "
                    "{} bytes instead of {}: the parcel was serialized "
                    "differently than during preprocessing",
                    pos, buffer.size, size);
            }
            return buffer;
        }

        keep_alive_list buffers_;
        std::size_t next_ = 0;
        std::vector<void const*> enclosing_;
        std::uint64_t parcel_start_ = 0;

        // the shared states of the futures that aren't ready in this round,
        // and of those that weren't in the previous one
        std::vector<void const*> pending_;
        std::vector<void const*> revealed_;
        std::size_t inserting_ = 0;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries. Resetting the archive keeps the buffers: preprocessing a parcel
// may take several rounds, each round starts at the first object again and
// must find the buffers of the previous one.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::preprocess_keep_alive>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static void reset(
        serialization::detail::preprocess_keep_alive* data) noexcept
    {
        data->reset_position();
    }
};
