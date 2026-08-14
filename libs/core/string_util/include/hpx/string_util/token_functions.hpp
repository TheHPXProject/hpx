//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Copyright John R. Bandela 2001.

// See http://www.boost.org/libs/tokenizer/ for documentation.

// Revision History:
// 01 Oct 2004   Joaquin M Lopez Munoz
//      Workaround for a problem with string::assign in msvc-stlport
// 06 Apr 2004   John Bandela
//      Fixed a bug involving using char_delimiter with a true input iterator
// 28 Nov 2003   Robert Zeh and John Bandela
//      Converted into "fast" functions that avoid using += when
//      the supplied iterator isn't an input_iterator; based on
//      some work done at Archelon and a version that was checked into
//      the boost CVS for a short period of time.
// 20 Feb 2002   John Maddock
//      Removed using namespace std declarations and added
//      workaround for BOOST_NO_STDC_NAMESPACE (the library
//      can be safely mixed with regex).
// 06 Feb 2002   Jeremy Siek
//      Added char_separator.
// 02 Feb 2002   Jeremy Siek
//      Removed tabs and a little cleanup.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>

#include <algorithm>    // for find_if
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace hpx::string_util {

    //=========================================================================
    // The escaped_list_separator class. Which is a model of TokenizerFunction
    // An escaped list is a super-set of what is commonly known as a comma
    // separated value (csv) list.It is separated into fields by a comma or
    // other character. If the delimiting character is inside quotes, then it is
    // counted as a regular character.To allow for embedded quotes in a field,
    // there can be escape sequences using the \ much like C. The role of the
    // comma, the quotation mark, and the escape character (backslash \), can be
    // assigned to other characters.
    HPX_CXX_CORE_EXPORT template <typename Char,
        typename Traits = typename std::basic_string<Char>::traits_type,
        typename Allocator = typename std::basic_string<Char>::allocator_type>
    class escaped_list_separator
    {
    private:
        using string_type = std::basic_string<Char, Traits, Allocator>;

        struct char_eq
        {
            Char e_;

            explicit char_eq(Char e) noexcept
              : e_(e)
            {
            }

            bool operator()(Char c) noexcept
            {
                return Traits::eq(e_, c);
            }
        };

        string_type escape_;
        string_type c_;
        string_type quote_;
        bool last_ = false;

        bool is_escape(Char e)
        {
            char_eq f(e);
            return std::find_if(escape_.begin(), escape_.end(), f) !=
                escape_.end();
        }

        bool is_c(Char e)
        {
            char_eq f(e);
            return std::find_if(c_.begin(), c_.end(), f) != c_.end();
        }

        bool is_quote(Char e)
        {
            char_eq f(e);
            return std::find_if(quote_.begin(), quote_.end(), f) !=
                quote_.end();
        }

        template <typename Iterator, typename Token>
        void do_escape(Iterator& next, Iterator end, Token& tok)
        {
            if (++next == end)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "escaped_list_separator::do_escape",
                    "cannot end with escape");
            }

            if (Traits::eq(*next, 'n'))
            {
                tok += '\n';
                return;
            }
            if (is_quote(*next) || is_c(*next) || is_escape(*next))
            {
                tok += *next;
                return;
            }

            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "escaped_list_separator::do_escape", "unknown escape sequence");
        }

    public:
        explicit escaped_list_separator(
            Char e = '\\', Char c = ',', Char q = '\"')
          : escape_(1, e)
          , c_(1, c)
          , quote_(1, q)
        {
        }

        escaped_list_separator(
            string_type e, string_type c, string_type q) noexcept
          : escape_(HPX_MOVE(e))
          , c_(HPX_MOVE(c))
          , quote_(HPX_MOVE(q))
        {
        }

        void reset() noexcept
        {
            last_ = false;
        }

        template <typename InputIterator, typename Token>
        bool operator()(InputIterator& next, InputIterator end, Token& tok)
        {
            bool in_quote = false;
            tok = Token();

            if (next == end)
            {
                if (last_)
                {
                    last_ = false;
                    return true;
                }
                else
                {
                    return false;
                }
            }

            last_ = false;
            for (/**/; next != end; ++next)
            {
                if (is_escape(*next))
                {
                    do_escape(next, end, tok);
                }
                else if (is_c(*next))
                {
                    if (!in_quote)
                    {
                        // If we are not in quote, then we are done
                        ++next;

                        // The last character was a c, that means there is 1
                        // more blank field
                        last_ = true;
                        return true;
                    }
                    else
                    {
                        tok += *next;
                    }
                }
                else if (is_quote(*next))
                {
                    in_quote = !in_quote;
                }
                else
                {
                    tok += *next;
                }
            }
            return true;
        }
    };

    //=========================================================================
    // The classes here are used by offset_separator and char_separator to
    // implement faster assigning of tokens using assign instead of +=

    namespace detail {

        //=====================================================================
        // Tokenizer was broken for wide character separators, at least on
        // Windows, since CRT functions isspace etc only expect values in [0,
        // 0xFF]. Debug build asserts if higher values are passed in. The traits
        // extension class should take care of this. Assuming that the
        // conditional will always get optimized out in the function
        // implementations, argument types are not a problem since both forms of
        // character classifiers expect an int.
        HPX_CXX_CORE_EXPORT template <typename Traits, int N>
        struct traits_extension_details : public Traits
        {
            using char_type = typename Traits::char_type;

            static bool isspace(char_type c) noexcept
            {
                return std::iswspace(c) != 0;
            }

            static bool ispunct(char_type c) noexcept
            {
                return std::iswpunct(c) != 0;
            }
        };

        template <typename Traits>
        struct traits_extension_details<Traits, 1> : public Traits
        {
            using char_type = typename Traits::char_type;

            static bool isspace(char_type c) noexcept
            {
                return std::isspace(c) != 0;
            }

            static bool ispunct(char_type c) noexcept
            {
                return std::ispunct(c) != 0;
            }
        };

        // In case there is no cwctype header, we implement the checks manually.
        // We make use of the fact that the tested categories should fit in
        // ASCII.
        HPX_CXX_CORE_EXPORT template <typename Traits>
        struct traits_extension : public Traits
        {
            using char_type = typename Traits::char_type;

            static bool isspace(char_type c) noexcept
            {
                return traits_extension_details<Traits,
                    sizeof(char_type)>::isspace(c);
            }

            static bool ispunct(char_type c) noexcept
            {
                return traits_extension_details<Traits,
                    sizeof(char_type)>::ispunct(c);
            }
        };

        // The assign_or_plus_equal struct contains functions that implement
        // assign, +=, and clearing based on the iterator type. The generic case
        // does nothing for plus_equal and clearing, while passing through the
        // call for assign.
        //
        // When an input iterator is being used, the situation is reversed. The
        // assign method does nothing, plus_equal invokes operator +=, and the
        // clearing method sets the supplied token to the default token
        // constructor's result.
        HPX_CXX_CORE_EXPORT template <typename IteratorTag>
        struct assign_or_plus_equal
        {
            template <typename Iterator, typename Token>
            static constexpr void assign(Iterator b, Iterator e, Token& t)
            {
                t.assign(b, e);
            }

            template <typename Token, typename Value>
            static constexpr void plus_equal(Token&, Value&&) noexcept
            {
            }

            // If we are doing an assign, there is no need for the the clear.
            template <typename Token>
            static constexpr void clear(Token&) noexcept
            {
            }
        };

        template <>
        struct assign_or_plus_equal<std::input_iterator_tag>
        {
            template <class Iterator, class Token>
            static constexpr void assign(Iterator, Iterator, Token&) noexcept
            {
            }

            template <class Token, class Value>
            static constexpr void plus_equal(Token& t, Value&& v)
            {
                t += HPX_FORWARD(Value, v);
            }

            template <class Token>
            static constexpr void clear(Token& t)
            {
                t = Token();
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct class_iterator_category
        {
            using type = typename Iterator::iterator_category;
        };

        // This portably gets the iterator_tag without partial template
        // specialization
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct get_iterator_category
        {
            using iterator_category =
                std::conditional_t<std::is_pointer_v<Iterator>,
                    std::random_access_iterator_tag,
                    typename class_iterator_category<Iterator>::type>;
        };
    }    // namespace detail

    //===========================================================================
    // The offset_separator class, which is a model of TokenizerFunction. Offset
    // breaks a string into tokens based on a range of offsets
    HPX_CXX_CORE_EXPORT class offset_separator
    {
    private:
        std::vector<int> offsets_;
        unsigned int current_offset_ = 0;
        bool wrap_offsets_ = true;
        bool return_partial_last_ = true;

    public:
        template <typename Iter>
        offset_separator(Iter begin, Iter end, bool wrap_offsets = true,
            bool return_partial_last = true)
          : offsets_(begin, end)
          , wrap_offsets_(wrap_offsets)
          , return_partial_last_(return_partial_last)
        {
        }

        offset_separator(std::initializer_list<int> init,
            bool wrap_offsets = true, bool return_partial_last = true)
          : offsets_(HPX_MOVE(init))
          , wrap_offsets_(wrap_offsets)
          , return_partial_last_(return_partial_last)
        {
        }

        offset_separator()
          : offsets_(1, 1)
        {
        }

        void reset()
        {
            current_offset_ = 0;
        }

        template <typename InputIterator, typename Token>
        bool operator()(InputIterator& next, InputIterator end, Token& tok)
        {
            using assigner = detail::assign_or_plus_equal<typename detail::
                    get_iterator_category<InputIterator>::iterator_category>;

            HPX_ASSERT(!offsets_.empty());

            assigner::clear(tok);
            InputIterator start(next);

            if (next == end)
            {
                return false;
            }

            if (static_cast<std::size_t>(current_offset_) == offsets_.size())
            {
                if (wrap_offsets_)
                {
                    current_offset_ = 0;
                }
                else
                {
                    return false;
                }
            }

            int const c = offsets_[static_cast<std::size_t>(current_offset_)];
            int i = 0;
            for (; i < c; ++i)
            {
                if (next == end)
                {
                    break;
                }
                assigner::plus_equal(tok, *next++);
            }
            assigner::assign(start, next, tok);

            if (!return_partial_last_)
            {
                if (i < (c - 1))
                {
                    return false;
                }
            }

            ++current_offset_;
            return true;
        }
    };

    //=========================================================================
    // The char_separator class breaks a sequence of characters into tokens
    // based on the character delimiters (very much like bad old strtok). A
    // delimiter character can either be kept or dropped. A kept delimiter shows
    // up as an output token, whereas a dropped delimiter does not.

    // This class replaces the char_delimiters_separator class. The constructor
    // for the char_delimiters_separator class was too confusing and needed to
    // be deprecated. However, because of the default arguments to the
    // constructor, adding the new constructor would cause ambiguity, so instead
    // I deprecated the whole class. The implementation of the class was also
    // simplified considerably.
    HPX_CXX_CORE_EXPORT enum class empty_token_policy : std::uint8_t {
        drop,
        keep
    };

    HPX_CXX_CORE_EXPORT template <typename Char,
        typename Traits = typename std::basic_string<Char>::traits_type,
        typename Allocator = typename std::basic_string<Char>::allocator_type>
    class char_separator
    {
        using traits_type = detail::traits_extension<Traits>;
        using string_type = std::basic_string<Char, Traits, Allocator>;

    public:
        explicit char_separator(Char const* dropped_delims,
            Char const* kept_delims = nullptr,
            empty_token_policy empty_tokens = empty_token_policy::drop)
          : m_dropped_delims(dropped_delims)
          , m_use_ispunct(false)
          , m_use_isspace(false)
          , m_empty_tokens(empty_tokens)
        {
            if (kept_delims)
                m_kept_delims = kept_delims;
        }

        // use ispunct() for kept delimiters and isspace for dropped.
        char_separator() = default;

        static constexpr void reset() noexcept {}

        template <typename InputIterator, typename Token>
        bool operator()(InputIterator& next, InputIterator end, Token& tok)
        {
            using assigner = detail::assign_or_plus_equal<typename detail::
                    get_iterator_category<InputIterator>::iterator_category>;

            assigner::clear(tok);

            // skip past all dropped_delims
            if (m_empty_tokens == empty_token_policy::drop)
            {
                for (/**/; next != end && is_dropped(*next); ++next)
                {
                }
            }

            InputIterator start(next);

            if (m_empty_tokens == empty_token_policy::drop)
            {
                if (next == end)
                    return false;

                // if we are on a kept_delims move past it and stop
                if (is_kept(*next))
                {
                    assigner::plus_equal(tok, *next);
                    ++next;
                }
                else
                {
                    // append all the non delim characters
                    for (/**/;
                        next != end && !is_dropped(*next) && !is_kept(*next);
                        ++next)
                    {
                        assigner::plus_equal(tok, *next);
                    }
                }
            }
            else
            {
                // m_empty_tokens == empty_token_policy::keep

                // Handle empty token at the end
                if (next == end)
                {
                    if (!m_output_done)
                    {
                        m_output_done = true;
                        assigner::assign(start, next, tok);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                if (is_kept(*next))
                {
                    if (!m_output_done)
                    {
                        m_output_done = true;
                    }
                    else
                    {
                        assigner::plus_equal(tok, *next);
                        ++next;
                        m_output_done = false;
                    }
                }
                else if (!m_output_done && is_dropped(*next))
                {
                    m_output_done = true;
                }
                else
                {
                    if (is_dropped(*next))
                    {
                        start = ++next;
                    }

                    for (/**/;
                        next != end && !is_dropped(*next) && !is_kept(*next);
                        ++next)
                    {
                        assigner::plus_equal(tok, *next);
                    }

                    m_output_done = true;
                }
            }

            assigner::assign(start, next, tok);
            return true;
        }

    private:
        string_type m_kept_delims;
        string_type m_dropped_delims;
        bool m_use_ispunct = true;
        bool m_use_isspace = true;
        empty_token_policy m_empty_tokens = empty_token_policy::drop;
        bool m_output_done = false;

        bool is_kept(Char E) const
        {
            if (m_kept_delims.length())
            {
                return m_kept_delims.find(E) != string_type::npos;
            }
            else if (m_use_ispunct)
            {
                return traits_type::ispunct(E) != 0;
            }
            return false;
        }

        bool is_dropped(Char E) const
        {
            if (m_dropped_delims.length())
            {
                return m_dropped_delims.find(E) != string_type::npos;
            }
            else if (m_use_isspace)
            {
                return traits_type::isspace(E) != 0;
            }
            return false;
        }
    };

    //===========================================================================
    // The char_delimiters_separator class, which is a model of
    // TokenizerFunction. char_delimiters_separator breaks a string into tokens
    // based on character delimiters. There are 2 types of delimiters.
    // Returnable delimiters can be returned as tokens. These are often
    // punctuation. Nonreturnable delimiters cannot be returned as tokens. These
    // are often whitespace

    HPX_CXX_CORE_EXPORT template <typename Char,
        typename Traits = typename std::basic_string<Char>::traits_type,
        typename Allocator = typename std::basic_string<Char>::allocator_type>
    class char_delimiters_separator
    {
    private:
        using traits_type = detail::traits_extension<Traits>;
        using string_type = std::basic_string<Char, Traits, Allocator>;

        string_type returnable_;
        string_type nonreturnable_;
        bool return_delims_;
        bool no_ispunct_;
        bool no_isspace_;

        bool is_ret(Char E) const noexcept
        {
            if (returnable_.length())
            {
                return returnable_.find(E) != string_type::npos;
            }
            else
            {
                if (no_ispunct_)
                {
                    return false;
                }
                else
                {
                    int const r = traits_type::ispunct(E);
                    return r != 0;
                }
            }
        }

        bool is_nonret(Char E) const noexcept
        {
            if (nonreturnable_.length())
            {
                return nonreturnable_.find(E) != string_type::npos;
            }
            else
            {
                if (no_isspace_)
                {
                    return false;
                }
                else
                {
                    int const r = traits_type::isspace(E);
                    return r != 0;
                }
            }
        }

    public:
        explicit char_delimiters_separator(bool return_delims = false,
            Char const* returnable = nullptr,
            Char const* nonreturnable = nullptr)
          : returnable_(returnable ? returnable : string_type().c_str())
          , nonreturnable_(
                nonreturnable ? nonreturnable : string_type().c_str())
          , return_delims_(return_delims)
          , no_ispunct_(returnable != nullptr)
          , no_isspace_(nonreturnable != nullptr)
        {
        }

        static constexpr void reset() noexcept {}

    public:
        template <typename InputIterator, typename Token>
        bool operator()(InputIterator& next, InputIterator end, Token& tok)
        {
            tok = Token();

            // skip past all nonreturnable delims
            // skip past the returnable only if we are not returning delims
            for (/**/; next != end &&
                (is_nonret(*next) || (is_ret(*next) && !return_delims_));
                ++next)
            {
            }

            if (next == end)
            {
                return false;
            }

            // if we are to return delims and we are one a returnable one move
            // past it and stop
            if (is_ret(*next) && return_delims_)
            {
                tok += *next;
                ++next;
            }
            else
            {
                // append all the non delim characters
                for (/**/; next != end && !is_nonret(*next) && !is_ret(*next);
                    ++next)
                {
                    tok += *next;
                }
            }

            return true;
        }
    };
}    // namespace hpx::string_util
