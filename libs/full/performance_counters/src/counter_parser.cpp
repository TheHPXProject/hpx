//  Copyright (c) 2020 Agustin Berge
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/performance_counters/counter_parser.hpp>
#include <hpx/performance_counters/counters.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#include <boost/spirit/home/x3/auxiliary.hpp>
#include <boost/spirit/home/x3/char.hpp>
#include <boost/spirit/home/x3/core.hpp>
#include <boost/spirit/home/x3/directive.hpp>
#include <boost/spirit/home/x3/nonterminal.hpp>
#include <boost/spirit/home/x3/numeric.hpp>
#include <boost/spirit/home/x3/operator.hpp>
#include <boost/spirit/home/x3/string.hpp>

#include <string>

BOOST_FUSION_ADAPT_STRUCT(
    hpx::performance_counters::instance_name, name_, index_, basename_)

BOOST_FUSION_ADAPT_STRUCT(
    hpx::performance_counters::instance_elements, parent_, child_, subchild_)

BOOST_FUSION_ADAPT_STRUCT(hpx::performance_counters::path_elements, object_,
    instance_, counter_, parameters_)

namespace {
    ///
    ///    /objectname{parentinstancename#parentindex/instancename#instanceindex}
    ///       /countername#parameters
    ///    /objectname{parentinstancename#*/instancename#*}/countername#parameters
    ///    /objectname{/basecounter}/countername,parameters
    ///
    namespace x3 = boost::spirit::x3;

    x3::rule<class path_parser, hpx::performance_counters::path_elements> const
        path_parser = "path_parser";
    x3::rule<class instance, hpx::performance_counters::instance_elements> const
        instance = "instance";
    x3::rule<class parent, hpx::performance_counters::instance_name> const
        parent = "parent";
    x3::rule<class child, hpx::performance_counters::instance_name> const
        child = "child";
    x3::rule<class subchild, hpx::performance_counters::instance_name> const
        subchild = "subchild";
    x3::rule<class raw_uint, std::string> const raw_uint = "raw_uint";

    auto const path_parser_def =
        -x3::lit(hpx::performance_counters::counter_prefix) >> '/' >>
        +~x3::char_("/{#@") >> -instance >> -('/' >> +~x3::char_("#}@")) >>
        -('@' >> +x3::char_);

    auto const instance_def = '{' >> parent >> -('/' >> child) >>
        -('/' >> subchild) >> '}';

    auto const parent_def = &x3::lit('/') >> x3::raw[path_parser] >>
            x3::attr("-1") >> x3::attr(true)
        // base counter
        | +~x3::char_("#/}") >>
            ('#' >> raw_uint           // counter parent-instance name
                | -x3::string("#*")    // counter parent-instance skeleton name
                ) >>
            x3::attr(false);

    auto const child_def = +~x3::char_("#/}") >>
        (x3::char_('#') >> +~x3::char_("/}")    // counter instance name
            | -x3::string("#*")    // counter instance skeleton name
            ) >>
        x3::attr(false);

    auto const subchild_def = +~x3::char_("#}") >>
        ('#' >> raw_uint           // counter (sub-)instance name
            | -x3::string("#*")    // counter (sub-)instance skeleton name
            ) >>
        x3::attr(false);

    auto const raw_uint_def = x3::raw[x3::uint_];

    BOOST_SPIRIT_DEFINE(    //-V801
        path_parser, instance, parent, child, subchild, raw_uint)

}    // namespace

namespace hpx::performance_counters {

    bool parse_counter_name(std::string const& name, path_elements& elements)
    {
        // parse the full name
        std::string::const_iterator begin = name.begin();
        return x3::parse(begin, name.end(), path_parser, elements) &&
            begin == name.end();
    }
}    // namespace hpx::performance_counters
