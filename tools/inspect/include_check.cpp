//  include_check implementation  --------------------------------------------//

//  Copyright Beman Dawes   2002.
//  Copyright Gennaro Prota 2006.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/format.hpp>

#include <algorithm>

#include "boost/regex.hpp"
#include "function_hyper.hpp"
#include "include_check.hpp"

namespace boost { namespace inspect {
    boost::regex include_regex("^\\s*#\\s*include\\s*<([^\n>]*)>(\\s|//"
                               "[^\\n]*|/\\*.*?\\*/)*$"    // # include <foobar>
                               "|"
                               "^\\s*#\\s*include\\s*\"([^\n\"]*)\"(\\s|//"
                               "[^\\n]*|/\\*.*?\\*/)*$"    // # include "foobar"
        ,
        boost::regex::normal);

    names_includes const names[] = {
        {"(\\bstd\\s*::\\s*make_shared\\b)", "std::make_shared", {"memory"}},
        {"(\\bstd\\s*::\\s*((map)|(set))\\b)", "std::\\2", {"\\2"}},
        {"(\\bstd\\s*::\\s*(multi((map)|(set)))\\b)", "std::\\2", {"\\3"}},
        {"(\\bstd\\s*::\\s*((shared|unique)_ptr)\\b)", "std::\\2", {"memory"}},
        {"(\\bstd\\s*::\\s*addressof\\b)", "std::addressof", {"memory"}},
        {"(\\bstd\\s*::\\s*(unordered_((map|set)))\\b)", "std::\\2",
            {"unordered_\\3"}},
        {"(\\bstd\\s*::\\s*(unordered_multi((map)|(set)))\\b)", "std::\\2",
            {"unordered_\\3"}},
        {"(\\bstd\\s*::\\s*list\\b)", "std::list", {"list"}},
        {"(\\bstd\\s*::\\s*string\\b)", "std::string", {"string"}},
        {"(\\bstd\\s*::\\s*vector\\b)", "std::vector", {"vector"}},
        {"(\\bstd\\s*::\\s*array\\b)", "std::array", {"array"}},
        {"(\\bstd\\s*::\\s*reference_wrapper\\b)", "std::reference_wrapper",
            {"functional"}},
        {"(\\bstd\\s*::\\s*(c?ref)\\b)", "std::\\2", {"functional"}},
        {"(\\bstd\\s*::\\s*(u?int[0-9]+_t)\\b)", "std::\\2", {"cstdint"}},
        {"(\\bstd\\s*::\\s*((i|o)?stringstream)\\b)", "std::\\2", {"sstream"}},
        {"(\\bstd\\s*::\\s*((i|o)?fstream)\\b)", "std::\\2", {"fstream"}},
        {"(\\bstd\\s*::\\s*(cin|cout|cerr|clog)\\b)", "std::\\2", {"iostream"}},
        {"(\\bstd\\s*::\\s*setw\\b)", "std::setw", {"iomanip"}},
        {"(\\bstd\\s*::\\s*setprecision\\b)", "std::setprecision", {"iomanip"}},
        // cstddef
        {"(\\bstd\\s*::\\s*size_t\\b)", "std::size_t", {"cstddef"}},
        {"(\\bstd\\s*::\\s*ptrdiff_t\\b)", "std::ptrdiff_t", {"cstddef"}},
        {"(\\bstd\\s*::\\s*nullptr_t\\b)", "std::nullptr_t", {"cstddef"}},
        // type_traits
        {"(\\bstd\\s*::\\s*true_type\\b)", "std::true_type", {"type_traits"}},
        {"(\\bstd\\s*::\\s*false_type\\b)", "std::false_type", {"type_traits"}},
        {"(\\bstd\\s*::\\s*integral_constant\\b)", "std::integral_constant",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*bool_constant\\b)", "std::bool_constant",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_void(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_null_pointer(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_integral(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_floating_point(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_array(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_enum(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_union(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_class(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_function(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_pointer(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_lvalue_reference(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_rvalue_reference(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_member_object_pointer(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_member_function_pointer(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_fundamental(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_arithmetic(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_scalar(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_object(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_compound(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_reference(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_member_pointer(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_const(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_volatile(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivial(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_copyable(_v)?)\\b)",
            "std\\2is_trivially_copyable", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_standard_layout(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_pod(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_literal_type(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_empty(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_polymorphic(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_abstract(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_signed(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_unsigned(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_default_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_default_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_default_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_copy_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_copy_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_copy_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_move_constructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_move_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_move_constructible(_v)?)\\b)",
            "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_assignable(_v)?)\\b)", "std::is_assignable\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_copy_assignable(_v)?)\\b)",
            "std::is_copy_assignable\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_copy_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_copy_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_move_assignable(_v)?)\\b)",
            "std::is_move_assignable\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_move_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_nothrow_move_assignable(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_destructible(_v)?)\\b)",
            "std::is_destructible\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_trivially_destructible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_same(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_base_of(_v)?)\\b)", "std::\\2", {"type_traits"}},
        {"(\\bstd\\s*::\\s*(is_convertible(_v)?)\\b)", "std::\\2",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*has_virtual_destructor\\b)",
            "std::has_virtual_destructor", {"type_traits"}},
        {"(\\bstd\\s*::\\s*alignment_of\\b)", "std::alignment_of",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*aligned_storage\\b)", "std::aligned_storage",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*aligned_union\\b)", "std::aligned_union",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*rank\\b)", "std::rank", {"type_traits"}},
        {"(\\bstd\\s*::\\s*extent\\b)", "std::extent", {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_cv\\b)", "std::add_cv", {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_const\\b)", "std::add_const", {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_pointer\\b)", "std::add_pointer",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_volatile\\b)", "std::add_volatile",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_lvalue_reference\\b)",
            "std::add_lvalue_reference", {"type_traits"}},
        {"(\\bstd\\s*::\\s*add_rvalue_reference\\b)",
            "std::add_rvalue_reference", {"type_traits"}},
        {"(\\bstd\\s*::\\s*make_signed\\b)", "std::make_signed",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*make_unsigned\\b)", "std::make_unsigned",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_cv\\b)", "std::remove_cv", {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_const\\b)", "std::remove_const",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_volatile\\b)", "std::remove_volatile",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_reference\\b)", "std::remove_reference",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_pointer\\b)", "std::remove_pointer",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_extent\\b)", "std::remove_extent",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*remove_all_extents\\b)", "std::remove_all_extents",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*decay\\b)", "std::decay", {"type_traits"}},
        {"(\\bstd\\s*::\\s*enable_if\\b)", "std::enable_if", {"type_traits"}},
        {"(\\bstd\\s*::\\s*conditional\\b)", "std::conditional",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*integral_constant\\b)", "std::integral_constant",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*common_type\\b)", "std::common_type",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*underlying_type\\b)", "std::underlying_type",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*underlying_type_t\\b)", "std::underlying_type_t",
            {"type_traits"}},
        {"(\\bstd\\s*::\\s*result_of\\b)", "std::result_of", {"type_traits"}},
        // cstring
        {"(\\bstd\\s*::\\s*(mem((set)|(cpy)|(move)))\\b)", "std::\\2",
            {"cstring"}},
        // utility
        {"(\\bstd\\s*::\\s*swap\\b)", "std::swap", {"utility"}},
        {"(\\bstd\\s*::\\s*move\\b)", "std::move", {"utility"}},
        {"(\\bstd\\s*::\\s*forward\\b)", "std::forward", {"utility"}},
        {"(\\bstd\\s*::\\s*declval\\b)", "std::declval", {"utility"}},
        {"(\\bstd\\s*::\\s*pair\\b)", "std::pair", {"utility"}},
        {"(\\bstd\\s*::\\s*make_pair\\b)", "std::make_pair", {"utility"}},
        // exception
        {"(\\bstd\\s*::\\s*exception_ptr\\b)", "std::exception_ptr",
            {"exception"}},
        {"(\\bstd\\s*::\\s*terminate\\b)", "std::terminate", {"exception"}},
        // algorithm
        {"(\\bstd\\s*::\\s*swap_ranges\\b)", "std::swap_ranges", {"algorithm"}},
        {"(\\bstd\\s*::\\s*iter_swap\\b)", "std::iter_swap", {"algorithm"}},
        // atomic
        {"(\\bstd\\s*::\\s*atomic\\b)", "std::atomic", {"atomic"}},
        {"(\\bstd\\s*::\\s*(memory_order_*)\\b)", "std::\\2", {"atomic"}},
        // random
        {"(\\bstd\\s*::\\s*(minstd_rand(0)?)\\b)", "std::\\2", {"random"}},
        {"(\\bstd\\s*::\\s*(mt19937(_64)?)\\b)", "std::\\2", {"random"}},
        {"(\\bstd\\s*::\\s*random_device\\b)", "std::random_device",
            {"random"}},
        {"(\\bstd\\s*::\\s*uniform_int_distribution\\b)",
            "std::uniform_int_distribution", {"random"}},
        {"(\\bstd\\s*::\\s*uniform_real_distribution\\b)",
            "std::uniform_real_distribution", {"random"}},
        {"(\\bstd\\s*::\\s*exponential_distribution\\b)",
            "std::exponential_distribution", {"random"}},
        // system_error
        {"(\\bstd\\s*::\\s*error_code\\b)", "std::error_code",
            {"system_error"}},
        {"(\\bstd\\s*::\\s*error_condition\\b)", "std::error_condition",
            {"system_error"}},
        {"(\\bstd\\s*::\\s*error_category\\b)", "std::error_category",
            {"system_error"}},
        {"(\\bstd\\s*::\\s*system_error\\b)", "std::system_error",
            {"system_error"}},
        // c++20
        {"(\\bstd\\s*::\\s*cmp_equal\\b)", "std::cmp_equal", {"utility"}},
        {"(\\bstd\\s*::\\s*cmp_less\\b)", "std::cmp_less", {"utility"}},
        {"(\\bstd\\s*::\\s*cmp_less_equal\\b)", "std::cmp_less_equal",
            {"utility"}},
        {"(\\bstd\\s*::\\s*cmp_greater\\b)", "std::cmp_greater", {"utility"}},
        {"(\\bstd\\s*::\\s*cmp_greater_equal\\b)", "std::cmp_greater_equal",
            {"utility"}},
        {"(\\bstd\\s*::\\s*cmp_not_equal\\b)", "std::cmp_not_equal",
            {"utility"}},
        {"(\\bstd\\s*::\\s*in_range\\b)", "std::in_range", {"utility"}},
        // boost
        {"(\\bhpx\\s*::\\s*intrusive_ptr\\b)", "hpx::intrusive_ptr",
            {"hpx/modules/memory.hpp"}},
        {"(\\bhpx\\s*::\\s*util\\s*::\\s*from_string\\b)",
            "hpx::util::from_string", {"hpx/modules/format.hpp"}},
        {"(\\bhpx\\s*::\\s*util\\s*::\\s*to_string\\b)", "hpx::util::to_string",
            {"hpx/modules/format.hpp"}},
        // macros
        {"(\\bHPX_PP_CAT\\b)", "HPX_PP_CAT",
            {"hpx/modules/preprocessor.hpp", "hpx/preprocessor/cat.hpp"}},
        {"(\\bHPX_PP_EXPAND\\b)", "HPX_PP_EXPAND",
            {"hpx/modules/preprocessor.hpp", "hpx/preprocessor/expand.hpp"}},
        {"(\\bHPX_PP_NARGS\\b)", "HPX_PP_NARGS",
            {"hpx/modules/preprocessor.hpp", "hpx/preprocessor/nargs.hpp"}},
        {"(\\bHPX_PP_STRINGIZE\\b)", "HPX_PP_STRINGIZE",
            {"hpx/modules/preprocessor.hpp", "hpx/preprocessor/stringize.hpp"}},
        {"(\\bHPX_PP_STRIP_PARENS\\b)", "HPX_PP_STRIP_PARENS",
            {"hpx/modules/preprocessor.hpp",
                "hpx/preprocessor/strip_parens.hpp"}},
        //
        {"(\\HPX_ASSERT\\b)", "HPX_ASSERT", {"hpx/assert.hpp"}},
        {"(\\HPX_ASSERT_MSG\\b)", "HPX_ASSERT_MSG", {"hpx/assert.hpp"}},
        {"(\\HPX_THROW_EXCEPTION\\b)", "HPX_THROW_EXCEPTION",
            {"hpx/modules/errors.hpp"}},
        {"(\\HPX_THROWS_IF\\b)", "HPX_THROWS_IF", {"hpx/modules/errors.hpp"}},
        {"(\\HPX_THROW_BAD_ALLOC\\b)", "HPX_THROW_BAD_ALLOC",
            {"hpx/modules/errors.hpp"}},
        {"(\\HPX_THROWS_BAD_ALLOC_IF\\b)", "HPX_THROWS_BAD_ALLOC_IF",
            {"hpx/modules/errors.hpp"}},
        //
        // std concepts
        {"(\\bstd\\s*::\\s*convertible_to\\b)", "std::convertible_to",
            {"concepts"}},
        {"(\\bstd\\s*::\\s*destructible\\b)", "std::destructible",
            {"concepts"}},
        {"(\\bstd\\s*::\\s*derived_from\\b)", "std::derived_from",
            {"concepts"}},
        {"(\\bstd\\s*::\\s*integral\\b)", "std::integral", {"concepts"}},
        {"(\\bstd\\s*::\\s*invocable\\b)", "std::invocable", {"concepts"}},
        {"(\\bstd\\s*::\\s*regular_invocable\\b)", "std::regular_invocable",
            {"concepts"}},
        {"(\\bstd\\s*::\\s*same_as\\b)", "std::same_as", {"concepts"}},
        //
        // std iterator
        {"(\\bstd\\s*::\\s*input_iterator\\b)", "std::input_iterator",
            {"iterator"}},
        {"(\\bstd\\s*::\\s*output_iterator\\b)", "std::output_iterator",
            {"iterator"}},
        {"(\\bstd\\s*::\\s*forward_iterator\\b)", "std::forward_iterator",
            {"iterator"}},
        {"(\\bstd\\s*::\\s*bidirectional_iterator\\b)",
            "std::bidirectional_iterator", {"iterator"}},
        {"(\\bstd\\s*::\\s*random_access_iterator\\b)",
            "std::random_access_iterator", {"iterator"}},
        {"(\\bstd\\s*::\\s*sentinel_for\\b)", "std::sentinel_for",
            {"iterator"}},
        {"(\\bstd\\s*::\\s*next\\b)", "std::next", {"iterator"}},
        {"(\\bstd\\s*::\\s*prev\\b)", "std::prev", {"iterator"}},
        // std ranges
        {"(\\bstd\\s*::\\s*ranges\\s*::\\srange\\b)", "std::ranges::range",
            {"ranges"}},
        {"(\\bstd\\s*::\\s*ranges\\s*::\\s*input_range\\b)",
            "std::ranges::input_range", {"ranges"}},
        {"(\\bstd\\s*::\\s*ranges\\s*::\\s*bidirectional_range\\b)",
            "std::ranges::bidirectional_range", {"ranges"}},
        {"(\\bstd\\s*::\\s*ranges\\s*::\\s*iterator_t\\b)",
            "std::ranges::iterator_t", {"ranges"}},
        {nullptr, nullptr, {nullptr}}};

    //  include_check constructor  -------------------------------------------//

    include_check::include_check()
      : m_errors(0)
    {
        // C/C++ source code...
        register_signature(".c");
        register_signature(".cpp");
        register_signature(".cu");
        register_signature(".cxx");
        register_signature(".h");
        register_signature(".hpp");
        register_signature(".hxx");
        register_signature(".inc");
        register_signature(".ipp");
        register_signature(".ixx");
        register_signature(".cppm");

        for (names_includes const* names_it = &names[0];
            names_it->name_regex != nullptr; ++names_it)
        {
            std::string rx(names_it->name_regex);
            rx += "|"    // or (ignored)
                  "("
                  "//[^\\n]*"    // single line comments (//)
                  "|"
                  "/\\*.*?\\*/"    // multi line comments (/**/)
                  "|"
                  "\"([^\"\\\\]|\\\\.)*\""    // string literals
                  ")";
            regex_data.push_back(names_regex_data(names_it, rx));
        }
    }

    //  inspect ( C++ source files )  ---------------------------------------//

    void include_check::inspect(string const& library_name,
        path const& full_path,     // example: c:/foo/boost/filesystem/path.hpp
        string const& contents)    // contents of file to be inspected
    {
        std::string::size_type p = contents.find("hpxinspect:"
                                                 "noinclude");
        if (p != string::npos)
        {
            // ignore this directive here (it is handled below) if it is followed
            // by a ':'
            if (p == contents.size() - 20 ||
                (contents.size() > p + 20 && contents[p + 20] != ':'))
            {
                return;
            }
        }

        // first, collect all #includes in this file
        std::set<std::string> includes;

        boost::sregex_iterator cur(
            contents.begin(), contents.end(), include_regex),
            end;

        for (; cur != end; ++cur /*, ++m_errors*/)
        {
            auto m = *cur;
            if (m[1].matched)
                includes.insert(std::string(m[1].first, m[1].second));
            else if (m[2].matched)
                includes.insert(std::string(m[2].first, m[2].second));
        }

        // if one of the includes is <hpx/hpx.hpp> assume all is well
        if (includes.find("hpx/hpx.hpp") != includes.end())
            return;

        // if one of the includes starts with <hpx/include/...> assume all is
        // well as well
        constexpr char const* prefix = "hpx/include/";
        if (auto it = includes.lower_bound(prefix);
            it != includes.end() && it->find(prefix) == 0)
        {
            return;
        }

        // for all given names, check whether corresponding include was found
        std::set<std::string> checked_includes;
        std::set<std::string> found_names;
        for (names_regex_data const& d : regex_data)
        {
            boost::sregex_iterator cur(
                contents.begin(), contents.end(), d.pattern),
                end;
            for (/**/; cur != end; ++cur)
            {
                auto m = *cur;
                if (m[1].matched)
                {
                    // avoid checking the same include twice
                    auto checked_includes_it =
                        checked_includes.find(m.format(d.data->include[0]));
                    if (checked_includes_it != checked_includes.end())
                        continue;

                    // avoid errors to be reported twice
                    std::string found_name(m[1].first, m[1].second);
                    if (found_names.find(found_name) != found_names.end())
                        continue;
                    found_names.insert(found_name);

                    std::string tag("hpxinspect:"
                                    "noinclude:" +
                        found_name);
                    if (contents.find(tag) != string::npos)
                        continue;

                    bool found = false;
                    for (auto const& inc : d.data->include)
                    {
                        auto include_it = includes.find(m.format(inc));
                        if (include_it != includes.end())
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        // include is missing
                        auto it = contents.begin();
                        auto match_it = m[1].first;
                        auto line_start = it;

                        string::size_type line_number = 1;
                        for (/**/; it != match_it; ++it)
                        {
                            if (string::traits_type::eq(*it, '\n'))
                            {
                                ++line_number;
                                line_start = it + 1;    // could be end()
                            }
                        }

                        ++m_errors;
                        error(library_name, full_path,
                            string(name()) + " missing #include (" +
                                m.format(d.data->include[0]) + ") for symbol " +
                                m.format(d.data->name) + " on line " +
                                linelink(full_path,
                                    hpx::util::to_string(line_number)));
                    }
                    checked_includes.insert(m.format(d.data->include[0]));
                }
            }
        }
    }

}}    // namespace boost::inspect
