//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/access.hpp>
#include <hpx/serialization/basic_archive.hpp>
#include <hpx/serialization/detail/non_default_constructible.hpp>
#include <hpx/serialization/detail/polymorphic_id_factory.hpp>
#include <hpx/serialization/detail/polymorphic_intrusive_factory.hpp>
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp>
#include <hpx/serialization/detail/preprocess_keep_alive.hpp>
#include <hpx/serialization/macros.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/string.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace hpx::serialization::detail {

    struct ptr_helper;

    // we need top use shared_ptr as util::any requires for the held type
    // to be copy-constructible
    using ptr_helper_ptr = std::unique_ptr<ptr_helper>;

    // A reference to an object whose address the output pointer tracker
    // records. The tracker keeps the object alive for as long as it records
    // the address: an object that died during serialization (e.g. a
    // temporary a serialize() function created) could otherwise be replaced
    // by another one at the same address, which would then be written as a
    // back-reference to the first.
    //
    // It holds a copy of the smart pointer in place (a std::shared_ptr,
    // boost::shared_ptr or intrusive pointer fits), so keeping the object
    // alive costs a reference count increment, but no allocation.
    HPX_CXX_CORE_EXPORT class kept_pointer
    {
        // moves the pointer from 'from' to 'to' (if 'to' isn't nullptr) and
        // destroys it in 'from'
        using manage_type = void (*)(void* to, void* from) noexcept;

    public:
        kept_pointer() = default;

        template <typename Pointer>
        explicit kept_pointer(Pointer const& ptr) noexcept(
            std::is_nothrow_copy_constructible_v<Pointer>)
        {
            static_assert(sizeof(Pointer) <= sizeof(storage_) &&
                    alignof(Pointer) <= alignof(void*),
                "the smart pointer is too large to be kept in place");
            static_assert(std::is_nothrow_move_constructible_v<Pointer>,
                "the smart pointer must be moved without exceptions");

            ::new (static_cast<void*>(storage_)) Pointer(ptr);
            manage_ = &manage<Pointer>;
        }

        kept_pointer(kept_pointer const&) = delete;
        kept_pointer& operator=(kept_pointer const&) = delete;

        kept_pointer(kept_pointer&& rhs) noexcept
          : manage_(std::exchange(rhs.manage_, nullptr))
        {
            if (manage_ != nullptr)
            {
                manage_(storage_, rhs.storage_);
            }
        }

        kept_pointer& operator=(kept_pointer&& rhs) noexcept
        {
            if (this != &rhs)
            {
                reset();
                manage_ = std::exchange(rhs.manage_, nullptr);
                if (manage_ != nullptr)
                {
                    manage_(storage_, rhs.storage_);
                }
            }
            return *this;
        }

        ~kept_pointer()
        {
            reset();
        }

    private:
        template <typename Pointer>
        static void manage(void* to, void* from) noexcept
        {
            auto* p = std::launder(static_cast<Pointer*>(from));
            if (to != nullptr)
            {
                ::new (to) Pointer(HPX_MOVE(*p));
            }
            std::destroy_at(p);
        }

        void reset() noexcept
        {
            if (manage_ != nullptr)
            {
                manage_(nullptr, storage_);
                manage_ = nullptr;
            }
        }

        alignas(void*) unsigned char storage_[2 * sizeof(void*)];
        manage_type manage_ = nullptr;
    };

    // the position of a tracked object in the archive, and its reference
    HPX_CXX_CORE_EXPORT struct tracked_output_pointer
    {
        std::uint64_t pos;
        kept_pointer keep_alive;
    };

    using input_pointer_tracker = std::map<std::uint64_t, ptr_helper_ptr>;
    using output_pointer_tracker =
        std::map<void const*, tracked_output_pointer>;
}    // namespace hpx::serialization::detail

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    template <>
    struct extra_data_helper<serialization::detail::input_pointer_tracker>
    {
        HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
        static constexpr void reset(
            serialization::detail::input_pointer_tracker*) noexcept
        {
        }
    };

    template <>
    struct extra_data_helper<serialization::detail::output_pointer_tracker>
    {
        HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
        HPX_CORE_EXPORT static void reset(
            serialization::detail::output_pointer_tracker* data);
    };
}    // namespace hpx::util

namespace hpx::serialization {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void register_pointer(
        input_archive& ar, std::uint64_t pos, detail::ptr_helper_ptr helper);

    HPX_CXX_CORE_EXPORT
    [[nodiscard]] HPX_CORE_EXPORT detail::ptr_helper& tracked_pointer(
        input_archive& ar, std::uint64_t pos);

    // Returns the position the object at 'pos' was written at, or -1 if it
    // wasn't written yet: then it records the position and keeps the object
    // alive through 'keep_alive' until the archive is reset or destroyed.
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::uint64_t
    track_pointer(
        output_archive& ar, void const* pos, detail::kept_pointer keep_alive);

    // The same without keeping the object alive, for binaries built against
    // the headers of earlier versions.
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::uint64_t
    track_pointer(output_archive& ar, void const* pos);

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Pointer>
        struct erase_ptr_helper final : ptr_helper
        {
            using referred_type = Pointer::element_type;

            erase_ptr_helper(Pointer&& t, Pointer& ptr) noexcept
              : t_(HPX_MOVE(t))
            {
                ptr = t_;
            }

            std::type_info const& type() const noexcept override
            {
                return typeid(Pointer);
            }

            Pointer t_;
        };

        template <typename Pointer>
        class pointer_input_dispatcher
        {
            using referred_type = Pointer::element_type;

            struct intrusive_polymorphic
            {
                static Pointer call(input_archive& ar)
                {
                    std::string name;
                    ar >> name;

                    Pointer t(polymorphic_intrusive_factory::instance()
                            .create<referred_type>(name));
                    ar >> *t;
                    return t;
                }
            };

            struct polymorphic_with_id
            {
                static Pointer call(input_archive& ar)
                {
#if !defined(HPX_DEBUG)
                    std::uint32_t id;
                    ar >> id;

                    Pointer t(
                        polymorphic_id_factory::create<referred_type>(id));
                    ar >> *t;
                    return t;
#else
                    std::uint32_t id;
                    std::string name;
                    ar >> name;
                    ar >> id;

                    Pointer t(polymorphic_id_factory::create<referred_type>(
                        id, &name));
                    ar >> *t;
                    return t;
#endif
                }
            };

            struct nonintrusive_polymorphic
            {
                static Pointer call(input_archive& ar)
                {
                    return Pointer(polymorphic_nonintrusive_factory::instance()
                            .load<referred_type>(ar));
                }
            };

            struct usual
            {
                static Pointer call(input_archive& ar)
                {
                    Pointer t(
                        constructor_selector_ptr<referred_type>::create(ar));
                    return t;
                }
            };

        public:
            using type = util::lazy_conditional_t<
                hpx::traits::is_serialized_with_id_v<referred_type>,
                hpx::type_identity<polymorphic_with_id>,
                std::conditional<
                    hpx::traits::is_intrusive_polymorphic_v<referred_type>,
                    intrusive_polymorphic,
                    std::conditional_t<
                        hpx::traits::is_nonintrusive_polymorphic_v<
                            referred_type>,
                        nonintrusive_polymorphic, usual>>>;
        };

        template <typename Pointer>
        class pointer_output_dispatcher
        {
            using referred_type = Pointer::element_type;

            struct intrusive_polymorphic
            {
                static void call(output_archive& ar, Pointer const& ptr)
                {
                    std::string const name = access::get_name(ptr.get());
                    ar << name;
                    ar << *ptr;
                }
            };

            struct polymorphic_with_id
            {
                static void call(output_archive& ar, Pointer const& ptr)
                {
#if !defined(HPX_DEBUG)
                    std::uint32_t const id = polymorphic_id_factory::get_id(
                        access::get_name(ptr.get()));
                    ar << id;
                    ar << *ptr;
#else
                    std::string const name = access::get_name(ptr.get());
                    std::uint32_t const id =
                        polymorphic_id_factory::get_id(name);
                    ar << name;
                    ar << id;
                    ar << *ptr;
#endif
                }
            };

            struct usual
            {
                static void call(output_archive& ar, Pointer const& ptr)
                {
                    using element_type = Pointer::element_type;
                    if constexpr (std::is_constructible_v<element_type>)
                    {
                        ar << *ptr;
                    }
                    else
                    {
                        save_construct_data(ar, ptr.get(), 0);
                        ar << *ptr;
                    }
                }
            };

        public:
            using type = std::conditional_t<
                hpx::traits::is_serialized_with_id_v<referred_type>,
                polymorphic_with_id,
                std::conditional_t<
                    hpx::traits::is_intrusive_polymorphic_v<referred_type>,
                    intrusive_polymorphic, usual>>;
        };

        // Preprocessing: the keep-alive buffers created while the object a
        // tracked pointer points to is serialized know that they are part of
        // it (see preprocess_keep_alive).
        template <typename Archive>
        class keep_alive_enclosing
        {
        public:
            keep_alive_enclosing(Archive const& ar, void const* object)
              : keep_alive_(ar.is_preprocessing() ?
                        ar.template try_get_extra_data<
                            preprocess_keep_alive>() :
                        nullptr)
            {
                if (keep_alive_ != nullptr)
                {
                    keep_alive_->enter(object);
                }
            }

            keep_alive_enclosing(keep_alive_enclosing const&) = delete;
            keep_alive_enclosing(keep_alive_enclosing&&) = delete;
            keep_alive_enclosing& operator=(
                keep_alive_enclosing const&) = delete;
            keep_alive_enclosing& operator=(keep_alive_enclosing&&) = delete;

            ~keep_alive_enclosing()
            {
                if (keep_alive_ != nullptr)
                {
                    keep_alive_->leave();
                }
            }

        private:
            preprocess_keep_alive* keep_alive_;
        };

        // The real pass: the object at 'object', written at 'pos', is written
        // as a back-reference. If an earlier parcel of the same message has
        // written it, the keep-alive buffers of the objects it contains are
        // not sent.
        template <typename Archive>
        void skip_keep_alive_of_back_reference(
            Archive const& ar, std::uint64_t const pos, void const* object)
        {
            if (ar.is_preprocessing())
            {
                return;
            }
            auto* keep_alive =
                ar.template try_get_extra_data<preprocess_keep_alive>();
            if (keep_alive != nullptr && pos < keep_alive->parcel_start())
            {
                keep_alive->skip_back_reference(object);
            }
        }

        // forwarded serialize pointer functions
        template <typename Pointer>
        HPX_FORCEINLINE void serialize_pointer_tracked(
            output_archive& ar, Pointer const& ptr)
        {
            bool const valid = static_cast<bool>(ptr);
            ar << valid;
            if (valid)
            {
                std::uint64_t const cur_pos = current_pos(ar);
                std::uint64_t const pos =
                    track_pointer(ar, ptr.get(), detail::kept_pointer(ptr));
                ar << pos;
                if (pos == static_cast<std::uint64_t>(-1))
                {
                    ar << cur_pos;
                    keep_alive_enclosing<output_archive> const enclosing(
                        ar, ptr.get());
                    detail::pointer_output_dispatcher<Pointer>::type::call(
                        ar, ptr);
                }
                else
                {
                    skip_keep_alive_of_back_reference(ar, pos, ptr.get());
                }
            }
        }

        template <typename Pointer>
        HPX_FORCEINLINE void serialize_pointer_tracked(
            input_archive& ar, Pointer& ptr)
        {
            bool valid = false;
            ar >> valid;
            if (valid)
            {
                std::uint64_t pos = 0;
                ar >> pos;
                if (pos == static_cast<std::uint64_t>(-1))
                {
                    pos = 0;
                    ar >> pos;
                    Pointer temp =
                        detail::pointer_input_dispatcher<Pointer>::type::call(
                            ar);
                    register_pointer(ar, pos,
                        ptr_helper_ptr(    //-V824
                            new detail::erase_ptr_helper<Pointer>(
                                HPX_MOVE(temp), ptr)));
                }
                else
                {
                    auto& tracked = tracked_pointer(ar, pos);
                    if (tracked.type() != typeid(Pointer))
                    {
                        HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                            "hpx::serialization::serialize_pointer_tracked",
                            "Unexpected type found in received buffer (actual: "
                            "{}, expected: {}).",
                            tracked.type().name(), typeid(Pointer).name());
                    }

                    auto& helper =
                        static_cast<detail::erase_ptr_helper<Pointer>&>(
                            tracked);
                    ptr = helper.t_;
                }
            }
        }

        template <typename Pointer>
        HPX_FORCEINLINE void serialize_pointer_untracked(
            output_archive& ar, Pointer const& ptr)
        {
            bool const valid = static_cast<bool>(ptr);
            ar << valid;
            if (valid)
            {
                detail::pointer_output_dispatcher<Pointer>::type::call(ar, ptr);
            }
        }

        template <typename Pointer>
        HPX_FORCEINLINE void serialize_pointer_untracked(
            input_archive& ar, Pointer& ptr)
        {
            bool valid = false;
            ar >> valid;
            if (valid)
            {
                ptr = detail::pointer_input_dispatcher<Pointer>::type::call(ar);
            }
        }
    }    // namespace detail
}    // namespace hpx::serialization
