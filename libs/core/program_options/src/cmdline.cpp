//  Copyright Vladimir Prus 2002-2004.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/program_options/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/program_options/detail/cmdline.hpp>
#include <hpx/program_options/errors.hpp>
#include <hpx/program_options/options_description.hpp>
#include <hpx/program_options/positional_options.hpp>
#include <hpx/program_options/value_semantic.hpp>

#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace hpx::program_options {

    using namespace hpx::program_options::command_line_style;

    std::string invalid_syntax::get_template(kind_t kind)
    {
        // Initially, store the message in 'const char*' variable,
        // to avoid conversion to string in all cases.
        char const* msg;
        switch (kind)
        {
        case empty_adjacent_parameter:
            msg = "the argument for option '%canonical_option%' should follow "
                  "immediately after the equal sign";
            break;
        case missing_parameter:
            msg = "the required argument for option '%canonical_option%' is "
                  "missing";
            break;
        case unrecognized_line:
            msg = "the options configuration file contains an invalid line "
                  "'%invalid_line%'";
            break;
        // none of the following are currently used:
        case long_not_allowed:
            msg = "the un-abbreviated option '%canonical_option%' is not valid";
            break;
        case long_adjacent_not_allowed:
            msg =
                "the un-abbreviated option '%canonical_option%' does not take "
                "any arguments";
            break;
        case short_adjacent_not_allowed:
            msg = "the abbreviated option '%canonical_option%' does not take "
                  "any arguments";
            break;
        case extra_parameter:
            msg = "option '%canonical_option%' does not take any arguments";
            break;
        default:
            msg = "unknown command line syntax error for '%s'";
            break;
        }
        return msg;
    }
}    // namespace hpx::program_options

namespace hpx::program_options::detail {

    cmdline::cmdline(std::vector<std::string> const& args)
    {
        init(args);
    }

    cmdline::cmdline(int argc, char const* const* argv)
    {
        init(std::vector<std::string>(argv + 1,
            argv + static_cast<std::size_t>(argc) +
                static_cast<std::size_t>(!argc)));
    }

    void cmdline::init(std::vector<std::string> const& args)
    {
        this->m_args = args;
        m_style = command_line_style::default_style;
        m_desc = nullptr;
        m_positional = nullptr;
        m_allow_unregistered = false;
    }

    void cmdline::style(int style) noexcept
    {
        if (style == 0)
            style = default_style;

        check_style(style);
        this->m_style = static_cast<style_t>(style);
    }

    void cmdline::allow_unregistered() noexcept
    {
        this->m_allow_unregistered = true;
    }

    void cmdline::check_style(int style)
    {
        bool const allow_some_long =
            (style & allow_long) || (style & allow_long_disguise);

        char const* error = nullptr;
        if (allow_some_long && !(style & long_allow_adjacent) &&
            !(style & long_allow_next))
        {
            error =
                "hpx::program_options misconfiguration: choose one or other of "
                "'command_line_style::long_allow_next' (whitespace separated "
                "arguments) or 'command_line_style::long_allow_adjacent' ('=' "
                "separated arguments) for long options.";
        }

        if (!error && (style & allow_short) &&
            !(style & short_allow_adjacent) && !(style & short_allow_next))
        {
            error =
                "hpx::program_options misconfiguration: choose one or other of "
                "'command_line_style::short_allow_next' (whitespace separated "
                "arguments) or 'command_line_style::short_allow_adjacent' ('=' "
                "separated arguments) for short options.";
        }

        if (!error && (style & allow_short) &&
            !(style & allow_dash_for_short) && !(style & allow_slash_for_short))
        {
            error =
                "hpx::program_options misconfiguration: choose one or other of "
                "'command_line_style::allow_slash_for_short' (slashes) or "
                "'command_line_style::allow_dash_for_short' (dashes) for short "
                "options.";
        }

        if (error)
            throw invalid_command_line_style(error);

        // Need to check that if guessing and long disguise are enabled -f will
        // mean the same as -foo
    }

    bool cmdline::is_style_active(style_t style) const noexcept
    {
        return ((m_style & style) ? true : false);
    }

    void cmdline::set_options_description(
        options_description const& desc) noexcept
    {
        m_desc = &desc;
    }

    void cmdline::set_positional_options(
        positional_options_description const& positional) noexcept
    {
        m_positional = &positional;
    }

    int cmdline::get_canonical_option_prefix() const noexcept
    {
        if (m_style & allow_long)
            return allow_long;

        if (m_style & allow_long_disguise)
            return allow_long_disguise;

        if ((m_style & allow_short) && (m_style & allow_dash_for_short))
            return allow_dash_for_short;

        if ((m_style & allow_short) && (m_style & allow_slash_for_short))
            return allow_slash_for_short;

        return 0;
    }

    std::vector<option> cmdline::run()
    {
        // The parsing is done by having a set of 'style parsers' and trying
        // then in order. Each parser is passed a vector of unparsed tokens and
        // can consume some of them (by removing elements on front) and return a
        // vector of options.
        //
        // We try each style parser in turn, until some input is consumed. The
        // returned vector of option may contain the result of just syntactic
        // parsing of token, say --foo will be parsed as option with name 'foo',
        // and the style parser is not required to care if that option is
        // defined, and how many tokens the value may take. So, after vector is
        // returned, we validate them.
        HPX_ASSERT(m_desc);

        std::vector<style_parser> style_parsers;
        style_parsers.reserve(7);

        if (m_style_parser)
            style_parsers.push_back(m_style_parser);

        if (m_additional_parser)
        {
            style_parsers.emplace_back([this](auto&& arg) {
                return handle_additional_parser(
                    std::forward<decltype(arg)>(arg));
            });
        }

        if (m_style & allow_long)
        {
            style_parsers.emplace_back([this](auto&& arg) {
                return parse_long_option(std::forward<decltype(arg)>(arg));
            });
        }

        if ((m_style & allow_long_disguise))
        {
            style_parsers.emplace_back([this](auto&& arg) {
                return parse_disguised_long_option(
                    std::forward<decltype(arg)>(arg));
            });
        }

        if ((m_style & allow_short) && (m_style & allow_dash_for_short))
        {
            style_parsers.emplace_back([this](auto&& arg) {
                return parse_short_option(std::forward<decltype(arg)>(arg));
            });
        }

        if ((m_style & allow_short) && (m_style & allow_slash_for_short))
        {
            style_parsers.emplace_back([this](auto&& arg) {
                return parse_dos_option(std::forward<decltype(arg)>(arg));
            });
        }

        style_parsers.emplace_back([](auto&& arg) {
            return parse_terminator(std::forward<decltype(arg)>(arg));
        });

        std::vector<option> result;
        std::vector<std::string>& args = m_args;    //-V826
        while (!args.empty())
        {
            bool ok = false;
            for (std::size_t i = 0; i < style_parsers.size(); ++i)
            {
                std::size_t const current_size = args.size();
                std::vector<option> next = style_parsers[i](args);

                // Check that option names are valid, and that all values are in
                // place.
                if (!next.empty())
                {
                    std::vector<std::string> e;
                    for (std::size_t k = 0; k < next.size() - 1; ++k)
                    {
                        finish_option(next[k], e, style_parsers);
                    }
                    // For the last option, pass the unparsed tokens so that
                    // they can be added to next.back()'s values if appropriate.
                    finish_option(next.back(), args, style_parsers);
                    for (auto const& j : next)
                        result.push_back(j);
                }

                if (args.size() != current_size)
                {
                    ok = true;
                    break;
                }
            }

            if (!ok)
            {
                option opt;
                opt.value.push_back(args[0]);
                opt.original_tokens.push_back(args[0]);
                result.push_back(opt);
                args.erase(args.begin());
            }
        }

        /* If an key option is followed by a positional option,
           can can consume more tokens (e.g. it's multitoken option),
           give those tokens to it.  */
        std::vector<option> result2;
        for (std::size_t i = 0; i < result.size(); ++i)
        {
            result2.push_back(result[i]);
            option& opt = result2.back();

            if (opt.string_key.empty())
                continue;

            option_description const* xd;
            try
            {
                xd = m_desc->find_nothrow(opt.string_key,
                    is_style_active(allow_guessing),
                    is_style_active(long_case_insensitive),
                    is_style_active(short_case_insensitive));
            }
            catch (error_with_option_name& e)
            {
                // add context and rethrow
                e.add_context(opt.string_key, opt.original_tokens[0],
                    get_canonical_option_prefix());
                throw;
            }

            if (!xd)
                continue;

            std::size_t const min_tokens =
                static_cast<std::size_t>(xd->semantic()->min_tokens());
            std::size_t const max_tokens =
                static_cast<std::size_t>(xd->semantic()->max_tokens());
            if (min_tokens < max_tokens && opt.value.size() < max_tokens)
            {
                // This option may grab some more tokens. We only allow to grab
                // tokens that are not already recognized as key options.

                std::size_t can_take_more = max_tokens - opt.value.size();
                std::size_t j = i + 1;
                for (; can_take_more && j < result.size(); --can_take_more, ++j)
                {
                    option& opt2 = result[j];
                    if (!opt2.string_key.empty())
                        break;

                    if (opt2.position_key == INT_MAX)
                    {
                        // We use INT_MAX to mark positional options that
                        // were found after the '--' terminator and therefore
                        // should stay positional forever.
                        break;
                    }

                    HPX_ASSERT(opt2.value.size() == 1);

                    opt.value.push_back(opt2.value[0]);

                    HPX_ASSERT(opt2.original_tokens.size() == 1);

                    opt.original_tokens.push_back(opt2.original_tokens[0]);
                }
                i = j - 1;
            }
        }
        result.swap(result2);

        // Assign position keys to positional options.
        int position_key = 0;
        for (auto& i : result)
        {
            if (i.string_key.empty())
                i.position_key = position_key++;
        }

        if (m_positional)
        {
            unsigned position = 0;
            for (option& opt : result)
            {
                if (opt.position_key != -1)
                {
                    if (position >= m_positional->max_total_count())
                    {
                        throw too_many_positional_options_error();
                    }
                    opt.string_key = m_positional->name_for_position(position);
                    ++position;
                }
            }
        }

        // set case sensitive flag
        for (auto& i : result)
        {
            if (i.string_key.size() > 2 ||
                (i.string_key.size() > 1 && i.string_key[0] != '-'))
            {
                // it is a long option
                i.case_insensitive = is_style_active(long_case_insensitive);
            }
            else
            {
                // it is a short option
                i.case_insensitive = is_style_active(short_case_insensitive);
            }
        }

        return result;
    }

    void cmdline::finish_option(option& opt,
        std::vector<std::string>& other_tokens,
        std::vector<style_parser> const& style_parsers) const
    {
        if (opt.string_key.empty())
            return;

        // Be defensive: will have no original token if option created by
        // handle_additional_parser()
        std::string original_token_for_exceptions = opt.string_key;
        if (!opt.original_tokens.empty())
            original_token_for_exceptions = opt.original_tokens[0];

        try
        {
            // First check that the option is valid, and get its description.
            option_description const* xd = m_desc->find_nothrow(opt.string_key,
                is_style_active(allow_guessing),
                is_style_active(long_case_insensitive),
                is_style_active(short_case_insensitive));

            if (!xd)
            {
                if (m_allow_unregistered)
                {
                    opt.unregistered = true;
                    return;
                }
                throw unknown_option();
            }
            option_description const& d = *xd;

            // Canonize the name
            opt.string_key = d.key(opt.string_key);

            // We check that the min/max number of tokens for the option
            // agrees with the number of tokens we have. The 'adjacent_value'
            // (the value in --foo=1) counts as a separate token, and if present
            // must be consumed. The following tokens on the command line may be
            // left unconsumed.
            std::size_t min_tokens =
                static_cast<std::size_t>(d.semantic()->min_tokens());
            std::size_t const max_tokens =
                static_cast<std::size_t>(d.semantic()->max_tokens());

            std::size_t const present_tokens =
                opt.value.size() + other_tokens.size();

            if (present_tokens >= min_tokens)
            {
                if (!opt.value.empty() && max_tokens == 0)
                {
                    throw invalid_command_line_syntax(
                        invalid_command_line_syntax::extra_parameter);
                }

                // Grab min_tokens values from other_tokens, but only if those tokens
                // are not recognized as options themselves.
                if (opt.value.size() <= min_tokens)
                {
                    min_tokens -= opt.value.size();    //-V101
                }
                else
                {
                    min_tokens = 0;
                }

                // Everything is OK, move the values to the result.
                while (!other_tokens.empty() && min_tokens--)
                {
                    // check if extra parameter looks like a known option we use
                    // style parsers to check if it is syntactically an option,
                    // additionally we check if an option_description exists
                    std::vector<option> followed_option;
                    std::vector next_token(1, other_tokens[0]);
                    for (std::size_t i = 0;
                        followed_option.empty() && i < style_parsers.size();
                        ++i)
                    {
                        followed_option = style_parsers[i](next_token);
                    }
                    if (!followed_option.empty())
                    {
                        original_token_for_exceptions = other_tokens[0];

                        option_description const* followed_desc =
                            m_desc->find_nothrow(other_tokens[0],
                                is_style_active(allow_guessing),
                                is_style_active(long_case_insensitive),
                                is_style_active(short_case_insensitive));

                        // Additional/style parsers may transform an input token
                        // into a known option key without preserving an original
                        // token. Check that parsed key as a fallback. Do not do
                        // this for regular long options, whose existing
                        // value-consumption semantics must remain unchanged.
                        if (followed_desc == nullptr)
                        {
                            for (option const& followed : followed_option)
                            {
                                if (followed.original_tokens.empty() &&
                                    !followed.string_key.empty())
                                {
                                    followed_desc = m_desc->find_nothrow(
                                        followed.string_key,
                                        is_style_active(allow_guessing),
                                        is_style_active(long_case_insensitive),
                                        is_style_active(
                                            short_case_insensitive));

                                    if (followed_desc != nullptr)
                                        break;
                                }
                            }
                        }

                        if (followed_desc != nullptr)
                        {
                            throw invalid_command_line_syntax(
                                invalid_command_line_syntax::missing_parameter);
                        }
                    }
                    opt.value.push_back(other_tokens[0]);
                    opt.original_tokens.push_back(other_tokens[0]);
                    other_tokens.erase(other_tokens.begin());
                }
            }
            else
            {
                throw invalid_command_line_syntax(
                    invalid_command_line_syntax::missing_parameter);
            }
        }
        // use only original token for unknown_option / ambiguous_option since
        // by definition
        //    they are unrecognized / unparsable
        catch (error_with_option_name& e)
        {
            // add context and rethrow
            e.add_context(opt.string_key, original_token_for_exceptions,
                get_canonical_option_prefix());
            throw;
        }
    }

    std::vector<option> cmdline::parse_long_option(
        std::vector<std::string>& args) const
    {
        std::vector<option> result;
        std::string const& tok = args[0];
        if (tok.size() >= 3 && tok[0] == '-' && tok[1] == '-')
        {
            std::string name, adjacent;

            std::string::size_type const p = tok.find('=');
            if (p != std::string::npos)
            {
                name = tok.substr(2, p - 2);
                adjacent = tok.substr(p + 1);
                if (adjacent.empty())
                {
                    throw invalid_command_line_syntax(
                        invalid_command_line_syntax::empty_adjacent_parameter,
                        name, name, get_canonical_option_prefix());
                }
            }
            else
            {
                name = tok.substr(2);
            }
            option opt;
            opt.string_key = HPX_MOVE(name);
            if (!adjacent.empty())
                opt.value.push_back(adjacent);
            opt.original_tokens.push_back(tok);
            result.push_back(opt);
            args.erase(args.begin());
        }
        return result;
    }

    std::vector<option> cmdline::parse_short_option(
        std::vector<std::string>& args) const
    {
        std::string const& tok = args[0];
        if (tok.size() >= 2 && tok[0] == '-' && tok[1] != '-')
        {
            std::vector<option> result;

            std::string name = tok.substr(0, 2);
            std::string adjacent = tok.substr(2);

            // Short options can be 'grouped', so that "-d -a" becomes "-da".
            // Loop, processing one option at a time. We exit the loop when
            // either we've processed all the token, or when the remainder of
            // token is considered to be value, not further grouped option.
            for (;;)
            {
                option_description const* d;
                try
                {
                    d = m_desc->find_nothrow(name, false, false,
                        is_style_active(short_case_insensitive));
                }
                catch (error_with_option_name& e)
                {
                    // add context and rethrow
                    e.add_context(name, name, get_canonical_option_prefix());
                    throw;
                }

                // FIXME: check for 'allow_sticky'.
                if (d && (m_style & allow_sticky) &&
                    d->semantic()->max_tokens() == 0 && !adjacent.empty())
                {
                    // 'adjacent' is in fact further option.
                    option opt;
                    opt.string_key = name;
                    result.push_back(opt);

                    if (adjacent.empty())    //-V547
                    {
                        args.erase(args.begin());
                        break;
                    }

                    name = std::string("-") + adjacent[0];
                    adjacent.erase(adjacent.begin());
                }
                else
                {
                    option opt;
                    opt.string_key = name;
                    opt.original_tokens.push_back(tok);
                    if (!adjacent.empty())
                        opt.value.push_back(adjacent);
                    result.push_back(opt);
                    args.erase(args.begin());
                    break;
                }
            }
            return result;
        }
        return {};
    }

    std::vector<option> cmdline::parse_dos_option(
        std::vector<std::string>& args) const
    {
        std::vector<option> result;
        std::string const& tok = args[0];
        if (tok.size() >= 2 && tok[0] == '/')
        {
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
            std::string name = "-" + tok.substr(1, 1);
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
            std::string const adjacent = tok.substr(2);

            option opt;
            opt.string_key = HPX_MOVE(name);
            if (!adjacent.empty())
                opt.value.push_back(adjacent);
            opt.original_tokens.push_back(tok);
            result.push_back(opt);
            args.erase(args.begin());
        }
        return result;
    }

    std::vector<option> cmdline::parse_disguised_long_option(
        std::vector<std::string>& args) const
    {
        std::string const& tok = args[0];
        if (tok.size() >= 2 &&
            ((tok[0] == '-' && tok[1] != '-') ||
                ((m_style & allow_slash_for_short) && tok[0] == '/')))
        {
            try
            {
                if (m_desc->find_nothrow(tok.substr(1, tok.find('=') - 1),
                        is_style_active(allow_guessing),
                        is_style_active(long_case_insensitive),
                        is_style_active(short_case_insensitive)))
                {
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
                    args[0].insert(0, "-");
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
                    if (args[0][1] == '/')
                        args[0][1] = '-';
                    return parse_long_option(args);
                }
            }
            catch (error_with_option_name& e)
            {
                // add context and rethrow
                e.add_context(tok, tok, get_canonical_option_prefix());
                throw;
            }
        }
        return {};
    }

    std::vector<option> cmdline::parse_terminator(
        std::vector<std::string>& args)
    {
        std::vector<option> result;
        std::string const& tok = args[0];
        if (tok == "--")
        {
            for (std::size_t i = 1; i < args.size(); ++i)
            {
                option opt;
                opt.value.push_back(args[i]);
                opt.original_tokens.push_back(args[i]);
                opt.position_key = INT_MAX;
                result.push_back(opt);
            }
            args.clear();
        }
        return result;
    }

    std::vector<option> cmdline::handle_additional_parser(
        std::vector<std::string>& args) const
    {
        std::vector<option> result;
        std::pair<std::string, std::string> const r =
            m_additional_parser(args[0]);
        if (!r.first.empty())
        {
            option next;
            next.string_key = r.first;
            if (!r.second.empty())
                next.value.push_back(r.second);
            result.push_back(next);
            args.erase(args.begin());
        }
        return result;
    }

    void cmdline::set_additional_parser(additional_parser p) noexcept
    {
        m_additional_parser = HPX_MOVE(p);
    }

    void cmdline::extra_style_parser(style_parser s) noexcept
    {
        m_style_parser = HPX_MOVE(s);
    }
}    // namespace hpx::program_options::detail
