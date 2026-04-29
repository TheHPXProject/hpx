//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstring>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::detail {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct empty_vtable_type
    {
        static_assert(
            sizeof(T) == 0, "No empty vtable type defined for given type T");
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using empty_vtable_t = typename empty_vtable_type<T>::type;

#if !defined(HPX_MSVC) && !defined(__CUDACC__) &&                              \
    !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr empty_vtable_t<T> empty_vtable{};

    HPX_CXX_CORE_EXPORT template <typename T>
    T const* get_empty_vtable()
    {
        static_assert(std::is_base_of_v<T, empty_vtable_t<T>>,
            "Given empty vtable type should be a base of T");

        return &empty_vtable<T>;
    }
#else
    HPX_CXX_CORE_EXPORT template <typename T>
    T const* get_empty_vtable()
    {
        static_assert(std::is_base_of_v<T, empty_vtable_t<T>>,
            "Given empty vtable type should be a base of T");

        static empty_vtable_t<T> empty;
        return &empty;
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename Base,
        std::size_t EmbeddedStorageSize,
        std::size_t AlignmentSize = sizeof(void*)>
    class movable_sbo_storage
    {
    protected:
        using base_type = Base;
        static constexpr std::size_t embedded_storage_size =
            EmbeddedStorageSize;
        static constexpr std::size_t alignment_size = AlignmentSize;

        // The union has two members:
        // - embedded_storage: Embedded storage size array used for types that
        //   are at most embedded_storage_size bytes large, and require at most
        //   alignment_size alignment.
        // - heap_storage: A pointer to base_type that is used when objects
        //   don't fit in the embedded storage.
        union storage
        {
            hpx::aligned_storage_t<embedded_storage_size, alignment_size>
                embedded_storage;
            base_type* heap_storage = nullptr;

            storage()
              : embedded_storage{{}}
            {
            }
        } data;
        base_type* object =
            const_cast<base_type*>(get_empty_vtable<base_type>());

        // Returns true when it's safe to use the embedded storage, i.e.
        // when the size and alignment of Impl are small enough.
        template <typename Impl>
        static constexpr bool can_use_embedded_storage() noexcept
        {
            constexpr bool fits_storage =
                sizeof(std::decay_t<Impl>) <= embedded_storage_size;
            constexpr bool sufficiently_aligned =
                alignof(std::decay_t<Impl>) <= alignment_size;
            return fits_storage && sufficiently_aligned;
        }

        [[nodiscard]] bool using_embedded_storage() const noexcept
        {
            return object ==
                reinterpret_cast<base_type const*>(&data.embedded_storage);
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return get().empty();
        }

        void reset_vtable()
        {
            object = const_cast<base_type*>(get_empty_vtable<base_type>());
        }

        void release()
        {
            HPX_ASSERT(!empty());

            if (using_embedded_storage())
            {
                get().~base_type();
            }
            else
            {
                delete data.heap_storage;
                data.heap_storage = nullptr;
            }

            reset_vtable();
        }

        void move_assign(movable_sbo_storage&& other) &

        {
            HPX_ASSERT(&other != this);
            HPX_ASSERT(empty());

            if (!other.empty())
            {
                if (other.using_embedded_storage())
                {
                    auto p =
                        reinterpret_cast<base_type*>(&data.embedded_storage);
                    other.get().move_into(p);
                    object = p;
                }
                else
                {
                    data.heap_storage = other.data.heap_storage;
                    other.data.heap_storage = nullptr;
                    object = data.heap_storage;
                }

                other.reset_vtable();
            }
        }

    public:
        movable_sbo_storage() = default;

        ~movable_sbo_storage()
        {
            if (!empty())
            {
                release();
            }
        }

        movable_sbo_storage(movable_sbo_storage&& other) noexcept
        {
            move_assign(HPX_MOVE(other));
        }

        movable_sbo_storage& operator=(movable_sbo_storage&& other) noexcept
        {
            if (&other != this)
            {
                if (!empty())
                {
                    release();
                }

                move_assign(HPX_MOVE(other));
            }
            return *this;
        }

        movable_sbo_storage(movable_sbo_storage const&) = delete;
        movable_sbo_storage& operator=(movable_sbo_storage const&) = delete;

        [[nodiscard]] base_type const& get() const noexcept
        {
            return *object;
        }

        [[nodiscard]] base_type& get() noexcept
        {
            return *object;
        }

        template <typename Impl, typename... Ts>
        void store(Ts&&... ts)
        {
            if (!empty())
            {
                release();
            }

            if constexpr (can_use_embedded_storage<Impl>())
            {
                Impl* p = reinterpret_cast<Impl*>(&data.embedded_storage);
                hpx::construct_at(p, HPX_FORWARD(Ts, ts)...);
                object = p;
            }
            else
            {
                data.heap_storage = new Impl(HPX_FORWARD(Ts, ts)...);
                object = data.heap_storage;
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Base,
        std::size_t EmbeddedStorageSize,
        std::size_t AlignmentSize = sizeof(void*)>
    class copyable_sbo_storage
      : public movable_sbo_storage<Base, EmbeddedStorageSize, AlignmentSize>
    {
        using storage_base_type =
            movable_sbo_storage<Base, EmbeddedStorageSize, AlignmentSize>;

        using typename storage_base_type::base_type;

        using storage_base_type::data;
        using storage_base_type::empty;
        using storage_base_type::object;
        using storage_base_type::release;
        using storage_base_type::using_embedded_storage;

        void copy_assign(copyable_sbo_storage const& other) &
        {
            HPX_ASSERT(&other != this);
            HPX_ASSERT(empty());

            if (!other.empty())
            {
                if (other.using_embedded_storage())
                {
                    base_type* p =
                        reinterpret_cast<base_type*>(&data.embedded_storage);
                    other.get().clone_into(p);
                    object = p;
                }
                else
                {
                    data.heap_storage = other.get().clone();
                    object = data.heap_storage;
                }
            }
        }

    public:
        using storage_base_type::get;
        using storage_base_type::store;

        copyable_sbo_storage() = default;
        ~copyable_sbo_storage() = default;

        copyable_sbo_storage(copyable_sbo_storage&&) = default;
        copyable_sbo_storage& operator=(copyable_sbo_storage&&) = default;

        copyable_sbo_storage(copyable_sbo_storage const& other)
          : storage_base_type()
        {
            copy_assign(other);
        }

        copyable_sbo_storage& operator=(copyable_sbo_storage const& other)
        {
            if (&other != this)
            {
                if (!empty())
                {
                    release();
                }
                copy_assign(other);
            }
            return *this;
        }
    };
}    // namespace hpx::detail

namespace hpx::execution::experimental::detail {

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT any_operation_state_base
    {
        virtual ~any_operation_state_base() = default;

        [[nodiscard]] virtual bool empty() const noexcept
        {
            return false;
        }
        virtual void start() & noexcept = 0;
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT empty_any_operation_state final
      : any_operation_state_base
    {
        [[nodiscard]] bool empty() const noexcept override;
        void start() & noexcept override;
    };
}    // namespace hpx::execution::experimental::detail

template <>
struct hpx::detail::empty_vtable_type<
    hpx::execution::experimental::detail::any_operation_state_base>
{
    using type =
        hpx::execution::experimental::detail::empty_any_operation_state;
};    // namespace hpx::detail

namespace hpx::execution::experimental::detail {

    HPX_CXX_CORE_EXPORT template <typename Sender, typename Receiver>
    struct any_operation_state_impl final : any_operation_state_base
    {
        std::decay_t<connect_result_t<Sender, Receiver>> operation_state;

        template <typename Sender_, typename Receiver_>
        any_operation_state_impl(Sender_&& sender, Receiver_&& receiver)
          : operation_state(hpx::execution::experimental::connect(
                HPX_FORWARD(Sender_, sender), HPX_FORWARD(Receiver_, receiver)))
        {
        }

        void start() & noexcept override
        {
            hpx::execution::experimental::start(operation_state);
        }
    };

    HPX_CXX_CORE_EXPORT class immovable
    {
    public:
        immovable() = default;
        immovable(immovable&&) = delete;
    };

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT any_operation_state : immovable
    {
        using base_type = detail::any_operation_state_base;
        template <typename Sender, typename Receiver>
        using impl_type = detail::any_operation_state_impl<Sender, Receiver>;
        using storage_type =
            hpx::detail::movable_sbo_storage<base_type, 8 * sizeof(void*)>;

        storage_type storage{};

    public:
        template <typename Sender, typename Receiver>
        any_operation_state(immovable, Sender&& sender, Receiver&& receiver)
        {
            storage.template store<impl_type<Sender, Receiver>>(
                HPX_FORWARD(Sender, sender), HPX_FORWARD(Receiver, receiver));
        }

        ~any_operation_state() = default;

        any_operation_state(any_operation_state&&) = delete;
        any_operation_state(any_operation_state const&) = delete;
        any_operation_state& operator=(any_operation_state&&) = delete;
        any_operation_state& operator=(any_operation_state const&) = delete;

        HPX_CORE_EXPORT friend void tag_invoke(
            hpx::execution::experimental::start_t,
            any_operation_state& os) noexcept;
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct any_receiver_base
    {
        using is_receiver = void;

        virtual ~any_receiver_base() = default;
        virtual void move_into(void* p) = 0;
        virtual void set_value(Ts... ts) && = 0;
        virtual void set_error(std::exception_ptr ep) && noexcept = 0;
        virtual void set_stopped() && noexcept = 0;
        [[nodiscard]] virtual bool empty() const noexcept
        {
            return false;
        }
    };

    HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void throw_bad_any_call(
        char const* class_name, char const* function_name);

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct empty_any_receiver final : any_receiver_base<Ts...>
    {
        void move_into(void*) override
        {
            HPX_UNREACHABLE;
        }

        [[nodiscard]] bool empty() const noexcept override
        {
            return true;
        }

        void set_value(Ts...) && override
        {
            throw_bad_any_call("any_receiver", "set_value");
        }

        [[noreturn]] void set_error(std::exception_ptr) && noexcept override
        {
            throw_bad_any_call("any_receiver", "set_error");
        }

        [[noreturn]] void set_stopped() && noexcept override
        {
            throw_bad_any_call("any_receiver", "set_stopped");
        }
    };
}    // namespace hpx::execution::experimental::detail

HPX_CXX_CORE_EXPORT template <typename... Ts>
struct hpx::detail::empty_vtable_type<
    hpx::execution::experimental::detail::any_receiver_base<Ts...>>
{
    using type =
        hpx::execution::experimental::detail::empty_any_receiver<Ts...>;
};    // namespace hpx::detail

namespace hpx::execution::experimental::detail {

    template <typename Receiver, typename... Ts>
    struct any_receiver_impl final : any_receiver_base<Ts...>
    {
        std::decay_t<Receiver> receiver;

        template <typename Receiver_,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Receiver_>, any_receiver_impl>>>
        explicit any_receiver_impl(Receiver_&& receiver)
          : receiver(HPX_FORWARD(Receiver_, receiver))
        {
        }

        void move_into(void* p) override
        {
            hpx::construct_at(
                static_cast<any_receiver_impl*>(p), HPX_MOVE(receiver));
        }

        void set_value(Ts... ts) && override
        {
            hpx::execution::experimental::set_value(
                HPX_MOVE(receiver), HPX_MOVE(ts)...);
        }

        void set_error(std::exception_ptr ep) && noexcept override
        {
            hpx::execution::experimental::set_error(
                HPX_MOVE(receiver), HPX_MOVE(ep));
        }

        void set_stopped() && noexcept override
        {
            hpx::execution::experimental::set_stopped(HPX_MOVE(receiver));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    class any_receiver
    {
        using base_type = detail::any_receiver_base<Ts...>;
        template <typename Receiver>
        using impl_type = detail::any_receiver_impl<Receiver, Ts...>;
        using storage_type =
            hpx::detail::movable_sbo_storage<base_type, 4 * sizeof(void*)>;

        storage_type storage{};

    public:
        using is_receiver = void;
        using receiver_concept = hpx::execution::experimental::receiver_t;

        template <typename Receiver,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Receiver>, any_receiver>>>
        explicit any_receiver(Receiver&& receiver)
        {
            storage.template store<impl_type<Receiver>>(
                HPX_FORWARD(Receiver, receiver));
        }

        template <typename Receiver,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Receiver>, any_receiver>>>
        any_receiver& operator=(Receiver&& receiver)
        {
            storage.template store<impl_type<Receiver>>(
                HPX_FORWARD(Receiver, receiver));
            return *this;
        }

        ~any_receiver() = default;

        any_receiver(any_receiver&&) = default;
        any_receiver(any_receiver const&) = delete;
        any_receiver& operator=(any_receiver&&) = default;
        any_receiver& operator=(any_receiver const&) = delete;

        friend void tag_invoke(hpx::execution::experimental::set_value_t,
            any_receiver&& r, Ts&&... ts) noexcept
        {
            // We first move the storage to a temporary variable so that this
            // any_receiver is empty after this set_value. Doing
            // HPX_MOVE(storage.get()).set_value(...) would leave us with a
            // non-empty any_receiver holding a moved-from receiver.
            auto moved_storage = HPX_MOVE(r.storage);

            // the caller of set_value needs to forward errors to set_error
            try
            {
                HPX_MOVE(moved_storage.get()).set_value(HPX_FORWARD(Ts, ts)...);
            }
            catch (...)
            {
                HPX_MOVE(moved_storage.get())
                    .set_error(std::current_exception());
            }
        }

        friend void tag_invoke(hpx::execution::experimental::set_error_t,
            any_receiver&& /*r*/, std::exception_ptr /*ep*/) noexcept
        {
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct unique_any_sender_base
    {
        virtual ~unique_any_sender_base() = default;
        virtual void move_into(void* p) = 0;
        virtual any_operation_state connect(
            any_receiver<Ts...>&& receiver) && = 0;
        [[nodiscard]] virtual bool empty() const noexcept
        {
            return false;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct any_sender_base : public unique_any_sender_base<Ts...>
    {
        virtual any_sender_base* clone() const = 0;
        virtual void clone_into(void* p) const = 0;

        using unique_any_sender_base<Ts...>::connect;
        virtual any_operation_state connect(
            any_receiver<Ts...>&& receiver) & = 0;
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct empty_unique_any_sender final : unique_any_sender_base<Ts...>
    {
        void move_into(void*) override
        {
            HPX_UNREACHABLE;
        }

        [[nodiscard]] bool empty() const noexcept override
        {
            return true;
        }

        [[noreturn]] any_operation_state connect(any_receiver<Ts...>&&) &&
            override
        {
            throw_bad_any_call("unique_any_sender", "connect");
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    struct empty_any_sender final : any_sender_base<Ts...>
    {
        void move_into(void*) override
        {
            HPX_UNREACHABLE;
        }

        any_sender_base<Ts...>* clone() const override
        {
            HPX_UNREACHABLE;
        }

        void clone_into(void*) const override
        {
            HPX_UNREACHABLE;
        }

        [[nodiscard]] bool empty() const noexcept override
        {
            return true;
        }

        [[noreturn]] any_operation_state connect(any_receiver<Ts...>&&) &
            override
        {
            throw_bad_any_call("any_sender", "connect");
        }

        [[noreturn]] any_operation_state connect(any_receiver<Ts...>&&) &&
            override
        {
            throw_bad_any_call("any_sender", "connect");
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sender, typename... Ts>
    struct unique_any_sender_impl final : unique_any_sender_base<Ts...>
    {
        std::decay_t<Sender> sender;

        template <typename Sender_,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender_>, unique_any_sender_impl>>>
        explicit unique_any_sender_impl(Sender_&& sender)
          : sender(HPX_FORWARD(Sender_, sender))
        {
        }

        void move_into(void* p) override
        {
            hpx::construct_at(
                static_cast<unique_any_sender_impl*>(p), HPX_MOVE(sender));
        }

        any_operation_state connect(any_receiver<Ts...>&& receiver) && override
        {
            return {{}, HPX_MOVE(sender), HPX_MOVE(receiver)};
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sender, typename... Ts>
    struct any_sender_impl final : any_sender_base<Ts...>
    {
        std::decay_t<Sender> sender;

        template <typename Sender_,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender_>, any_sender_impl>>>
        explicit any_sender_impl(Sender_&& sender)
          : sender(HPX_FORWARD(Sender_, sender))
        {
        }

        void move_into(void* p) override
        {
            hpx::construct_at(
                static_cast<any_sender_impl*>(p), HPX_MOVE(sender));
        }

        any_sender_base<Ts...>* clone() const override
        {
            return new any_sender_impl(sender);
        }

        void clone_into(void* p) const override
        {
            hpx::construct_at(static_cast<any_sender_impl*>(p), sender);
        }

        any_operation_state connect(any_receiver<Ts...>&& receiver) & override
        {
            return any_operation_state{{}, sender, HPX_MOVE(receiver)};
        }

        any_operation_state connect(any_receiver<Ts...>&& receiver) && override
        {
            return any_operation_state{
                {}, HPX_MOVE(sender), HPX_MOVE(receiver)};
        }
    };
}    // namespace hpx::execution::experimental::detail

namespace hpx::execution::experimental {

#if defined(HPX_MSVC) || !defined(HPX_HAVE_CXX20_TRIVIAL_VIRTUAL_DESTRUCTOR)
    namespace detail {
        // This helper only exists to make it possible to use
        // any_(unique_)sender in global variables or in general static that may
        // be created before main. When used as a base for any_(unique_)_sender,
        // this ensures that the empty vtables for any_receiver and
        // any_operation_state are created as the first thing when creating an
        // any_(unique_)sender. The empty vtables for any_receiver and
        // any_operation_state may otherwise be created much later (when the
        // sender is connected and started), and thus destroyed before the
        // any_(unique_)sender is destroyed. This would be problematic since the
        // any_(unique_)sender can hold previously created any_receivers and
        // any_operation_states indirectly.
        HPX_CXX_CORE_EXPORT template <typename... Ts>
        struct any_sender_static_empty_vtable_helper
        {
            any_sender_static_empty_vtable_helper()
            {
                hpx::detail::get_empty_vtable<any_operation_state_base>();
                hpx::detail::get_empty_vtable<any_receiver_base<Ts...>>();
            }
        };
    }    // namespace detail
#endif

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    class unique_any_sender
#if defined(HPX_MSVC)
      : private detail::any_sender_static_empty_vtable_helper<Ts...>
#endif
    {
        using base_type = detail::unique_any_sender_base<Ts...>;

        template <typename Sender>
        using impl_type = detail::unique_any_sender_impl<Sender, Ts...>;

        using storage_type =
            hpx::detail::movable_sbo_storage<base_type, 4 * sizeof(void*)>;

        storage_type storage{};

    public:
        using sender_concept = hpx::execution::experimental::sender_t;

        unique_any_sender() = default;

        template <typename Sender,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender>, unique_any_sender>>>
        unique_any_sender(Sender&& sender)
        {
            storage.template store<impl_type<Sender>>(
                HPX_FORWARD(Sender, sender));
        }

        template <typename Sender,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender>, unique_any_sender>>>
        unique_any_sender& operator=(Sender&& sender)
        {
            storage.template store<impl_type<Sender>>(
                HPX_FORWARD(Sender, sender));
            return *this;
        }

        ~unique_any_sender() = default;

        unique_any_sender(unique_any_sender&&) = default;
        unique_any_sender(unique_any_sender const&) = delete;
        unique_any_sender& operator=(unique_any_sender&&) = default;
        unique_any_sender& operator=(unique_any_sender const&) = delete;

        // TODO: Remove this
        using completion_signatures =
            hpx::execution::experimental::completion_signatures<
                set_value_t(Ts...), set_error_t(std::exception_ptr)>;

        template <typename R>
        friend detail::any_operation_state tag_invoke(
            hpx::execution::experimental::connect_t, unique_any_sender&& s,
            R&& r)
        {
            // We first move the storage to a temporary variable so that this
            // any_sender is empty after this connect. Doing
            // HPX_MOVE(storage.get()).connect(...) would leave us with a
            // non-empty any_sender holding a moved-from sender.
            auto moved_storage = HPX_MOVE(s.storage);
            return HPX_MOVE(moved_storage.get())
                .connect(detail::any_receiver<Ts...>{HPX_FORWARD(R, r)});
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    class any_sender
#if defined(HPX_MSVC)
      : private detail::any_sender_static_empty_vtable_helper<Ts...>
#endif
    {
        using base_type = detail::any_sender_base<Ts...>;

        template <typename Sender>
        using impl_type = detail::any_sender_impl<Sender, Ts...>;

        using storage_type =
            hpx::detail::copyable_sbo_storage<base_type, 4 * sizeof(void*)>;

        storage_type storage{};

    public:
        using sender_concept = hpx::execution::experimental::sender_t;

        any_sender() = default;

        template <typename Sender,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender>, any_sender>>>
        any_sender(Sender&& sender)
        {
            static_assert(std::is_copy_constructible_v<std::decay_t<Sender>>,
                "any_sender requires the given sender to be copy "
                "constructible. Ensure the used sender type is copy "
                "constructible or use unique_any_sender if you do not require "
                "copyability.");
            storage.template store<impl_type<Sender>>(
                HPX_FORWARD(Sender, sender));
        }

        template <typename Sender,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sender>, any_sender>>>
        any_sender& operator=(Sender&& sender)
        {
            static_assert(std::is_copy_constructible_v<std::decay_t<Sender>>,
                "any_sender requires the given sender to be copy "
                "constructible. Ensure the used sender type is copy "
                "constructible or use unique_any_sender if you do not require "
                "copyability.");
            storage.template store<impl_type<Sender>>(
                HPX_FORWARD(Sender, sender));
            return *this;
        }

        ~any_sender() = default;

        any_sender(any_sender&&) = default;
        any_sender(any_sender const&) = default;
        any_sender& operator=(any_sender&&) = default;
        any_sender& operator=(any_sender const&) = default;

        // TODO: Remove this
        using completion_signatures =
            hpx::execution::experimental::completion_signatures<
                set_value_t(Ts...), set_error_t(std::exception_ptr)>;

        template <typename R>
        friend detail::any_operation_state tag_invoke(
            hpx::execution::experimental::connect_t, any_sender& s, R&& r)
        {
            return s.storage.get().connect(
                detail::any_receiver<Ts...>{HPX_FORWARD(R, r)});
        }

        template <typename R>
        friend detail::any_operation_state tag_invoke(
            hpx::execution::experimental::connect_t, any_sender&& s, R&& r)
        {
            // We first move the storage to a temporary variable so that this
            // any_sender is empty after this connect. Doing
            // HPX_MOVE(storage.get()).connect(...) would leave us with a
            // non-empty any_sender holding a moved-from sender.
            auto moved_storage = HPX_MOVE(s.storage);
            return HPX_MOVE(moved_storage.get())
                .connect(detail::any_receiver<Ts...>{HPX_FORWARD(R, r)});
        }
    };
}    // namespace hpx::execution::experimental

namespace hpx::detail {

    template <typename... Ts>
    struct empty_vtable_type<
        hpx::execution::experimental::detail::unique_any_sender_base<Ts...>>
    {
        using type =
            hpx::execution::experimental::detail::empty_unique_any_sender<
                Ts...>;
    };

    template <typename... Ts>
    struct empty_vtable_type<
        hpx::execution::experimental::detail::any_sender_base<Ts...>>
    {
        using type =
            hpx::execution::experimental::detail::empty_any_sender<Ts...>;
    };
}    // namespace hpx::detail

#include <hpx/config/warnings_suffix.hpp>
