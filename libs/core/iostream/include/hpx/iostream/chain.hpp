//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/resolve.hpp>
#include <hpx/iostream/device/null.hpp>
#include <hpx/iostream/optimal_buffer_size.hpp>
#include <hpx/iostream/pipeline.hpp>
#include <hpx/iostream/positioning.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>
#include <exception>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace hpx::iostream {

    //--------------Definition of chain and wchain--------------------------------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Chain>
        class chain_client;

        // Concept name: Chain.
        // Description: Represents a chain of stream buffers which provides access
        //     to the first buffer in the chain and sends notifications when the
        //     streambufs are added to or removed from chain.
        // Refines: Closable device with mode equal to typename Chain::mode.
        // Models: chain, converting_chain.
        // Example:
        //
        //    class chain {
        //    public:
        //        typedef xxx chain_type;
        //        typedef xxx client_type;
        //        typedef xxx mode;
        //        bool is_complete() const;                  // Ready for i/o.
        //        template<typename T>
        //        void push( const T& t,                     // Adds a stream buffer to
        //                   streamsize,                     // chain, based on t, with
        //                   streamsize );                   // given buffer and putback
        //                                                   // buffer sizes. Pass -1 to
        //                                                   // request default size.
        //    protected:
        //        void register_client(client_type* client); // Associate client.
        //        void notify();                             // Notify client.
        //    };
        //

        //
        // Description: Represents a chain of filters with an optional device at the
        //      end.
        // Template parameters:
        //      Self - A class deriving from the current instantiation of this template.
        //          This is an example of the Curiously Recurring Template Pattern.
        //      Ch - The character type.
        //      Tr - The character traits type.
        //      Alloc - The allocator type.
        //      Mode - A mode tag.
        //
        HPX_CXX_CORE_EXPORT template <typename Self, typename Ch, typename Tr,
            typename Alloc, typename Mode>
        class chain_base
        {
        public:
            using char_type = Ch;
            using traits_type = Tr;
            using int_type = traits_type::int_type;
            using off_type = traits_type::off_type;
            using pos_type = traits_type::pos_type;
            using allocator_type = Alloc;
            using mode = Mode;

            struct category
              : Mode
              , device_tag
            {
            };

            using client_type = chain_client<Self>;
            friend class chain_client<Self>;

        private:
            using streambuf_type = linked_streambuf<Ch, Tr>;
            using list_type = std::list<streambuf_type*>;

        private:
            chain_base()
              : pimpl_(std::make_shared<chain_impl>())
            {
            }

            chain_base(chain_base const& rhs) = default;
            chain_base(chain_base&& rhs) noexcept = default;

            chain_base& operator=(chain_base const&) = default;
            chain_base& operator=(chain_base&&) noexcept = default;

            ~chain_base() = default;

            friend Self;

        public:
            // dual_use is a pseudo-mode to facilitate filter writing, not a
            // genuine mode.
            static_assert(!std::is_convertible_v<mode, dual_use>);

            //----------Buffer sizing-------------------------------------------------//

            // Sets the size of the buffer created for the devices to be added
            // to this chain. Does not affect the size of the buffer for devices
            // already added.
            void set_device_buffer_size(std::streamsize const n)
            {
                pimpl_->device_buffer_size_ = n;
            }

            // Sets the size of the buffer created for the filters to be added
            // to this chain. Does not affect the size of the buffer for filters
            // already added.
            void set_filter_buffer_size(std::streamsize const n)
            {
                pimpl_->filter_buffer_size_ = n;
            }

            // Sets the size of the putback buffer for filters and devices to be added
            // to this chain. Does not affect the size of the buffer for filters or
            // devices already added.
            void set_pback_size(std::streamsize const n)
            {
                pimpl_->pback_size_ = n;
            }

            //----------Device interface----------------------------------------------//
            std::streamsize read(char_type* s, std::streamsize n);
            std::streamsize write(char_type const* s, std::streamsize n);
            std::streampos seek(stream_offset off, std::ios_base::seekdir way);

            //----------Direct component access---------------------------------------//
            [[nodiscard]] std::type_info const& component_type(
                int const n) const
            {
                if (static_cast<size_type>(n) >= size())
                    throw std::out_of_range("bad chain offset");
                return (*std::next(list().begin(), n))->component_type();
            }

            // Deprecated.
            template <int N>
            [[nodiscard]] std::type_info const& component_type() const
            {
                return component_type(N);
            }

            template <typename T>
            T* component(int n) const
            {
                return component(n, hpx::type_identity<T>());
            }

            // Deprecated.
            template <int N, typename T>
            T* component() const
            {
                return component<T>(N);
            }

        private:
            template <typename T>
            T* component(int n, hpx::type_identity<T>) const
            {
                if (static_cast<size_type>(n) >= size())
                    throw std::out_of_range("bad chain offset");

                streambuf_type* link = *std::next(list().begin(), n);
                if (link->component_type().name() == typeid(T).name())
                    return static_cast<T*>(link->component_impl());

                return nullptr;
            }

        public:
            //----------Container-like interface--------------------------------------//
            using size_type = list_type::size_type;

            streambuf_type& front()
            {
                return *list().front();
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_streambuf<CharType, TraitsType>& sb,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(sb),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_istream<CharType, TraitsType>& is,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                static_assert((!std::is_convertible_v<mode, output>),
                    "!std::is_convertible_v<mode, output>");
                this->push_impl(detail::resolve<mode, char_type>(is),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_ostream<CharType, TraitsType>& os,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                static_assert((!std::is_convertible_v<mode, input>),
                    "!std::is_convertible_v<mode, input>");
                this->push_impl(detail::resolve<mode, char_type>(os),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_iostream<CharType, TraitsType>& io,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(io),
                    buffer_size, pback_size);
            }

            template <typename Iter>
            void push(util::iterator_range<Iter> const& rng,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                return this->push_impl(
                    detail::range_adapter<mode, util::iterator_range<Iter>>(
                        rng),
                    buffer_size, pback_size);
            }

            template <typename Pipeline, typename Concept>
            void push(pipeline<Pipeline, Concept> const& p)
            {
                p.push(*this);
            }

            template <typename T>
                requires(!hpx::iostream::is_std_io_v<T>)
            void push(T const& t, std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(t),
                    buffer_size, pback_size);
            }

            void pop();

            [[nodiscard]] bool empty() const
            {
                return list().empty();
            }

            [[nodiscard]] size_type size() const
            {
                return list().size();
            }

            void reset();

            //----------Additional i/o functions--------------------------------------//

            // Returns true if this chain is non-empty and its final link is a
            // source or sink, i.e., if it is ready to perform i/o.
            [[nodiscard]] bool is_complete() const;
            [[nodiscard]] bool auto_close() const;

            void set_auto_close(bool close);

            bool sync()
            {
                return front().sync() != -1;
            }

            bool strict_sync();

        private:
            template <typename T>
            void push_impl(T const& t, std::streamsize buffer_size = -1,
                std::streamsize pback_size = -1)
            {
                using category = category_of<T>::type;
                using component_type = util::lazy_conditional_t<is_std_io_v<T>,
                    util::unwrap_reference<T>, type_identity<T>>;
                using streambuf_t = stream_buffer<component_type,
                    std::char_traits<char_type>, Alloc, Mode>;
                using iterator = list_type::iterator;

                static_assert(std::is_convertible_v<category, Mode>);

                if (is_complete())
                    throw std::logic_error("chain complete");

                streambuf_type* prev = !empty() ? list().back() : nullptr;
                buffer_size =
                    buffer_size != -1 ? buffer_size : optimal_buffer_size(t);
                pback_size =
                    pback_size != -1 ? pback_size : pimpl_->pback_size_;

                auto buf =
                    std::make_unique<streambuf_t>(t, buffer_size, pback_size);

                list().push_back(buf.get());
                buf.release();

                if (is_device_v<component_type>)
                {
                    pimpl_->flags_ |= flags::complete | flags::open;
                    for (iterator first = list().begin(), last = list().end();
                        first != last; ++first)
                    {
                        (*first)->set_needs_close();
                    }
                }
                if (prev)
                    prev->set_next(list().back());

                notify();
            }

            list_type& list()
            {
                return pimpl_->links_;
            }
            list_type const& list() const
            {
                return pimpl_->links_;
            }

            void register_client(client_type* client)
            {
                pimpl_->client_ = client;
            }

            void notify()
            {
                if (pimpl_->client_)
                    pimpl_->client_->notify();
            }

            //----------Nested classes------------------------------------------------//
            static void close(streambuf_type* b, std::ios_base::openmode m)
            {
                if constexpr (std::is_convertible_v<Mode, output>)
                {
                    if (m == std::ios_base::out)
                    {
                        b->sync();
                    }
                }
                b->close(m);
            }

            static void set_next(streambuf_type* b, streambuf_type* next)
            {
                b->set_next(next);
            }

            static void set_auto_close(streambuf_type* b, bool close)
            {
                b->set_auto_close(close);
            }

            struct closer
            {
                using argument_type = streambuf_type*;
                using result_type = void;

                explicit constexpr closer(std::ios_base::openmode m) noexcept
                  : mode_(m)
                {
                }

                void operator()(streambuf_type* b)
                {
                    close(b, mode_);
                }

                std::ios_base::openmode mode_;
            };
            friend struct closer;

            enum class flags : std::uint8_t
            {
                complete = 1,
                open = 2,
                auto_close = 4
            };

            friend constexpr int operator&(int const lhs, flags rhs) noexcept
            {
                return lhs & static_cast<int>(rhs);
            }
            friend constexpr int operator|(flags lhs, flags rhs) noexcept
            {
                return static_cast<int>(lhs) | static_cast<int>(rhs);
            }
            friend constexpr int operator|(int const lhs, flags rhs) noexcept
            {
                return lhs | static_cast<int>(rhs);
            }
            friend constexpr int operator~(flags rhs) noexcept
            {
                return ~static_cast<int>(rhs);
            }

            struct chain_impl
            {
                chain_impl() = default;

                ~chain_impl()
                {
                    try
                    {
                        close();
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                    }

                    try
                    {
                        reset();
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                    }
                }

                chain_impl(chain_impl const&) = delete;
                chain_impl(chain_impl&&) = default;
                chain_impl& operator=(chain_impl const&) = delete;
                chain_impl& operator=(chain_impl&&) = default;

                void close()
                {
                    if ((flags_ & flags::open) != 0)
                    {
                        flags_ &= ~flags::open;

                        stream_buffer<basic_null_device<Ch, Mode>> null;
                        if ((flags_ & flags::complete) == 0)
                        {
                            null.open(basic_null_device<Ch, Mode>());
                            set_next(links_.back(), &null);
                        }
                        links_.front()->sync();

                        try
                        {
                            detail::execute_foreach(links_.rbegin(),
                                links_.rend(), closer(std::ios_base::in));
                        }
                        catch (...)
                        {
                            try
                            {
                                detail::execute_foreach(links_.begin(),
                                    links_.end(), closer(std::ios_base::out));
                            }
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                            catch (...)
                            {
                            }
                            throw;
                        }
                        detail::execute_foreach(links_.begin(), links_.end(),
                            closer(std::ios_base::out));
                    }
                }

                void reset()
                {
                    for (auto first = links_.begin(), last = links_.end();
                        first != last; ++first)
                    {
                        if ((flags_ & flags::complete) == 0 ||
                            (flags_ & flags::auto_close) == 0)
                        {
                            set_auto_close(*first, false);
                        }

                        streambuf_type* buf = nullptr;
                        std::swap(buf, *first);
                        delete buf;
                    }

                    links_.clear();
                    flags_ &= ~flags::complete;
                    flags_ &= ~flags::open;
                }

                list_type links_;
                client_type* client_ = nullptr;
                std::streamsize device_buffer_size_ =
                    default_device_buffer_size;
                std::streamsize filter_buffer_size_ =
                    default_filter_buffer_size;
                std::streamsize pback_size_ = default_pback_buffer_size;
                int flags_ = static_cast<int>(flags::auto_close);
            };
            friend struct chain_impl;

        private:
            std::shared_ptr<chain_impl> pimpl_;
        };
    }    // End namespace detail.

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = char,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>>
    struct chain
      : detail::chain_base<chain<Mode, Ch, Tr, Alloc>, Ch, Tr, Alloc, Mode>
    {
        struct category
          : Mode
          , device_tag
        {
        };

        using mode = Mode;
        using char_type = Ch;
        using traits_type = Tr;
        using int_type = traits_type::int_type;
        using off_type = traits_type::off_type;
    };

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = wchar_t,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>>
    using wchain = chain<Mode, Ch, Tr, Alloc>;
#endif

    //--------------Definition of chain_client------------------------------------//
    namespace detail {

        //
        // Template name: chain_client
        // Description: Class whose instances provide access to an underlying chain
        //      using an interface similar to the chains.
        // Subclasses: the various stream and stream buffer templates.
        //
        HPX_CXX_CORE_EXPORT template <typename Chain>
        class chain_client
        {
        public:
            using chain_type = Chain;
            using char_type = chain_type::char_type;
            using traits_type = chain_type::traits_type;
            using size_type = chain_type::size_type;
            using mode = chain_type::mode;

            explicit chain_client(chain_type* chn = nullptr) noexcept
              : chain_(chn)
            {
            }

            explicit chain_client(chain_client const* client) noexcept
              : chain_(client->chain_)
            {
            }

            virtual ~chain_client() = default;

            chain_client(chain_client const&) = delete;
            chain_client(chain_client&&) = delete;
            chain_client& operator=(chain_client const&) = delete;
            chain_client& operator=(chain_client&&) = delete;

            std::type_info const& component_type(int n) const
            {
                return chain_->component_type(n);
            }

            // Deprecated.
            template <int N>
            std::type_info const& component_type() const
            {
                return chain_->template component_type<N>();
            }

            template <typename T>
            T* component(int n) const
            {
                return chain_->template component<T>(n);
            }

            // Deprecated.
            template <int N, typename T>
            T* component() const
            {
                return chain_->template component<N, T>();
            }

            [[nodiscard]] bool is_complete() const
            {
                return chain_->is_complete();
            }
            [[nodiscard]] bool auto_close() const
            {
                return chain_->auto_close();
            }

            void set_auto_close(bool close)
            {
                chain_->set_auto_close(close);
            }
            bool strict_sync()
            {
                return chain_->strict_sync();
            }

            void set_device_buffer_size(std::streamsize n)
            {
                chain_->set_device_buffer_size(n);
            }
            void set_filter_buffer_size(std::streamsize n)
            {
                chain_->set_filter_buffer_size(n);
            }
            void set_pback_size(std::streamsize n)
            {
                chain_->set_pback_size(n);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_streambuf<CharType, TraitsType>& sb,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(sb),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_istream<CharType, TraitsType>& is,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                static_assert(!std::is_convertible_v<mode, output>,
                    "!std::is_convertible_v<mode, output>");
                this->push_impl(detail::resolve<mode, char_type>(is),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_ostream<CharType, TraitsType>& os,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                static_assert(!std::is_convertible_v<mode, input>,
                    "!std::is_convertible_v<mode, input>");
                this->push_impl(detail::resolve<mode, char_type>(os),
                    buffer_size, pback_size);
            }

            template <typename CharType, typename TraitsType>
            void push(std::basic_iostream<CharType, TraitsType>& io,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(io),
                    buffer_size, pback_size);
            }

            template <typename Iter>
            void push(util::iterator_range<Iter> const& rng,
                std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                return this->push_impl(
                    detail::range_adapter<mode, util::iterator_range<Iter>>(
                        rng),
                    buffer_size, pback_size);
            }

            template <typename Pipeline, typename Concept>
            void push(pipeline<Pipeline, Concept> const& p)
            {
                p.push(*this);
            }

            template <typename T>
                requires(!is_std_io_v<T>)
            void push(T const& t, std::streamsize buffer_size = -1,
                std::streamsize pback_size = -1)
            {
                this->push_impl(detail::resolve<mode, char_type>(t),
                    buffer_size, pback_size);
            }

            void pop()
            {
                chain_->pop();
            }

            [[nodiscard]] bool empty() const
            {
                return chain_->empty();
            }
            [[nodiscard]] size_type size() const
            {
                return chain_->size();
            }
            void reset()
            {
                chain_->reset();
            }

            // Returns a copy of the underlying chain.
            chain_type filters()
            {
                return *chain_;
            }
            chain_type filters() const
            {
                return *chain_;
            }

        protected:
            template <typename T>
            void push_impl(T const& t, std::streamsize const buffer_size = -1,
                std::streamsize const pback_size = -1)
            {
                chain_->push(t, buffer_size, pback_size);
            }

            chain_type& ref()
            {
                return *chain_;
            }

            void set_chain(chain_type* c)
            {
                chain_ = c;
                chain_->register_client(this);
            }

            template <typename S, typename C, typename T, typename A,
                typename M>
            friend class chain_base;

            virtual void notify() {}

        private:
            chain_type* chain_;
        };

        //--------------Implementation of chain_base----------------------------------//
        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        std::streamsize chain_base<Self, Ch, Tr, Alloc, Mode>::read(
            char_type* s, std::streamsize n)
        {
            return iostream::read(*list().front(), s, n);
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        std::streamsize chain_base<Self, Ch, Tr, Alloc, Mode>::write(
            char_type const* s, std::streamsize n)
        {
            return iostream::write(*list().front(), s, n);
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        std::streampos chain_base<Self, Ch, Tr, Alloc, Mode>::seek(
            stream_offset off, std::ios_base::seekdir way)
        {
            return iostream::seek(*list().front(), off, way);
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        void chain_base<Self, Ch, Tr, Alloc, Mode>::reset()
        {
            pimpl_->close();
            pimpl_->reset();
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        bool chain_base<Self, Ch, Tr, Alloc, Mode>::is_complete() const
        {
            return (pimpl_->flags_ & flags::complete) != 0;
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        bool chain_base<Self, Ch, Tr, Alloc, Mode>::auto_close() const
        {
            return (pimpl_->flags_ & flags::auto_close) != 0;
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        void chain_base<Self, Ch, Tr, Alloc, Mode>::set_auto_close(bool close)
        {
            pimpl_->flags_ = (pimpl_->flags_ & ~flags::auto_close) |
                (close ? static_cast<int>(flags::auto_close) : 0);
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        bool chain_base<Self, Ch, Tr, Alloc, Mode>::strict_sync()
        {
            bool result = true;
            for (auto first = list().begin(), last = list().end();
                first != last; ++first)
            {
                bool const s = (*first)->strict_sync();
                result = result && s;
            }
            return result;
        }

        template <typename Self, typename Ch, typename Tr, typename Alloc,
            typename Mode>
        void chain_base<Self, Ch, Tr, Alloc, Mode>::pop()
        {
            HPX_ASSERT(!empty());
            if (auto_close())
                pimpl_->close();

            streambuf_type* buf = nullptr;
            std::swap(buf, list().back());
            buf->set_auto_close(false);
            buf->set_next(nullptr);
            delete buf;

            list().pop_back();
            pimpl_->flags_ = pimpl_->flags_ & ~flags::complete;
            if (auto_close() || list().empty())
                pimpl_->flags_ &= ~flags::open;
        }
    }    // namespace detail
}    // namespace hpx::iostream
