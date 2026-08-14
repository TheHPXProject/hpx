//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/string_util.hpp>

#include <hpx/batch_environments/detail/slurm_environment.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/home/x3/char.hpp>
#include <boost/spirit/home/x3/core.hpp>
#include <boost/spirit/home/x3/nonterminal.hpp>
#include <boost/spirit/home/x3/numeric.hpp>
#include <boost/spirit/home/x3/operator.hpp>
#include <boost/spirit/home/x3/string.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

    using range_type = std::vector<std::string>;

    using ranges_type = std::vector<range_type>;

    using nodename_type =
        boost::fusion::vector<std::string, boost::optional<ranges_type>>;

    void construct_nodelist(
        std::vector<std::string>& nodes, std::vector<nodename_type> const& p)
    {
        std::vector<std::string> tmp_nodes;

        for (nodename_type const& value : p)
        {
            std::string const& prefix = boost::fusion::at_c<0>(value);
            boost::optional<ranges_type> const& ranges =
                boost::fusion::at_c<1>(value);
            bool push_now = tmp_nodes.empty();
            if (ranges)
            {
                for (range_type const& range : *ranges)
                {
                    if (range.size() == 1)
                    {
                        std::string s(prefix);
                        s += range[0];
                        if (push_now)
                        {
                            tmp_nodes.push_back(s);
                        }
                        else
                        {
                            for (std::string& node : tmp_nodes)
                            {
                                node += s;
                            }
                        }
                    }
                    else
                    {
                        std::size_t begin =
                            hpx::util::from_string<std::size_t>(range[0]);
                        std::size_t end =
                            hpx::util::from_string<std::size_t>(range[1]);
                        if (begin > end)
                            std::swap(begin, end);

                        std::vector<std::string> vs;

                        for (std::size_t i = begin; i <= end; ++i)
                        {
                            std::string s(prefix);
                            std::size_t dec = 10;
                            // pad with zeros
                            for (std::size_t j = 0; j < range[0].length() - 1;
                                ++j)
                            {
                                if (i < dec)
                                {
                                    s += "0";
                                }
                                dec *= 10;
                            }
                            s += std::to_string(i);
                            if (push_now)
                            {
                                tmp_nodes.push_back(s);
                            }
                            else
                            {
                                vs.push_back(s);
                            }
                        }
                        if (!push_now)
                        {
                            std::vector<std::string> tmp;
                            std::swap(tmp, tmp_nodes);
                            for (std::string s : tmp)
                            {
                                for (std::string const& s2 : vs)
                                {
                                    s += s2;
                                    tmp_nodes.push_back(s);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (push_now)
                {
                    tmp_nodes.push_back(prefix);
                }
                else
                {
                    for (std::string& node : tmp_nodes)
                    {
                        node += prefix;
                    }
                }
            }
        }
        nodes.insert(nodes.end(), tmp_nodes.begin(), tmp_nodes.end());
    }

    namespace x3 = boost::spirit::x3;

    x3::rule<class prefix, std::string> prefix = "prefix";
    x3::rule<class range_str, std::string> range_str = "range_str";
    x3::rule<class range, range_type> range = "range";
    x3::rule<class ranges, ranges_type> ranges = "ranges";
    x3::rule<class ranges, nodename_type> nodename = "nodename";
    x3::rule<class hostlist, std::vector<std::string>> hostlist = "hostlist";
    x3::rule<class nodelist, std::vector<std::string>> nodelist = "nodelist";

    // grammar definition
    auto const prefix_def = +(x3::print - (x3::char_("[") | x3::char_(",")));

    auto const range_str_def =
        +(x3::print - (x3::char_("]") | x3::char_(",") | x3::char_("-")));

    auto const range_def = range_str % '-';

    auto const ranges_def = x3::lit("[") >> (range % ',') >> x3::lit("]");

    auto const nodename_def = prefix >> -ranges;

    auto const bind_construct_nodelist = [](auto& ctx) {
        auto& nodes = x3::_val(ctx);
        auto const& nodenames = x3::_attr(ctx);
        construct_nodelist(nodes, nodenames);
    };
    auto const hostlist_def = (+nodename)[bind_construct_nodelist];

    auto const nodelist_def = hostlist % ',';

    BOOST_SPIRIT_DEFINE(
        prefix, range_str, range, ranges, nodename, hostlist, nodelist)

}    // namespace

namespace hpx::util::batch_environments::detail {

    slurm_environment::slurm_environment(
        std::vector<std::string>& nodelist, bool debug)
      : node_num_(0)
      , num_threads_(0)
      , num_tasks_(0)
      , num_localities_(0)
      , valid_(false)
    {
        char* node_num = std::getenv("SLURM_PROCID");
        valid_ = node_num != nullptr;
        if (valid_)
        {
            // Initialize our node number
            node_num_ = from_string<std::size_t>(node_num);

            // Retrieve number of localities
            retrieve_number_of_localities(debug);

            // Retrieve number of tasks
            retrieve_number_of_tasks(debug);

            // Get the list of nodes
            if (nodelist.empty())
            {
                retrieve_nodelist(nodelist, debug);
            }

            // Determine how many threads to use
            retrieve_number_of_threads();
        }
    }

    void slurm_environment::retrieve_number_of_localities(bool debug)
    {
        char* total_num_tasks = std::getenv("SLURM_STEP_NUM_TASKS");
        if (total_num_tasks)
        {
            num_localities_ = from_string<std::size_t>(total_num_tasks);
        }
        else
        {
            if (debug)
            {
                std::cerr
                    << "SLURM_STEP_NUM_TASKS not found: set num_localities to 1"
                    << std::endl;
            }
            num_localities_ = 1;
        }
    }

    void slurm_environment::retrieve_number_of_tasks(bool debug)
    {
        char* slurm_step_tasks_per_node =
            std::getenv("SLURM_STEP_TASKS_PER_NODE");
        if (slurm_step_tasks_per_node)
        {
            std::vector<std::string> tokens;
            hpx::string_util::split(tokens, slurm_step_tasks_per_node,
                hpx::string_util::is_any_of(","));

            char* slurm_node_id = std::getenv("SLURM_NODEID");
            HPX_ASSERT(slurm_node_id != nullptr);
            if (slurm_node_id)
            {
                std::size_t node_id = from_string<std::size_t>(slurm_node_id);
                std::size_t task_count = 0;
                for (auto& token : tokens)
                {
                    std::size_t paren_pos = token.find_first_of('(');
                    if (paren_pos != std::string::npos)
                    {
                        HPX_ASSERT(token[paren_pos + 1] == 'x');
                        HPX_ASSERT(token[token.size() - 1] == ')');
                        std::size_t begin = paren_pos + 2;
                        std::size_t end = token.size() - 1;
                        task_count += from_string<std::size_t>(
                            token.substr(paren_pos + 2, end - begin));
                    }
                    else
                    {
                        task_count += 1;
                    }

                    if (task_count > node_id)
                    {
                        num_tasks_ = from_string<std::size_t>(
                            token.substr(0, paren_pos));
                        break;
                    }
                }
                HPX_ASSERT(num_tasks_);
            }
        }
        else
        {
            if (debug)
            {
                std::cerr
                    << "SLURM_STEP_TASKS_PER_NODE not found: set num_tasks to 1"
                    << std::endl;
            }
            num_tasks_ = 1;
        }
    }

    void slurm_environment::retrieve_nodelist(
        std::vector<std::string>& nodes, bool debug)
    {
        char* slurm_nodelist_env = std::getenv("SLURM_STEP_NODELIST");
        if (slurm_nodelist_env)
        {
            if (debug)
            {
                std::cerr << "SLURM nodelist found (SLURM_STEP_NODELIST): "
                          << slurm_nodelist_env << std::endl;
            }

            std::string nodelist_str(slurm_nodelist_env);
            std::string::iterator begin = nodelist_str.begin();
            std::string::iterator end = nodelist_str.end();

            if (!x3::parse(begin, end, nodelist, nodes) || begin != end)
            {
                if (debug)
                {
                    std::cerr << "failed to parse SLURM nodelist "
                                 "(SLURM_STEP_NODELIST): "
                              << slurm_nodelist_env << std::endl;
                }
            }
        }
    }

    void slurm_environment::retrieve_number_of_threads()
    {
        char* slurm_cpus_per_task = std::getenv("SLURM_CPUS_PER_TASK");
        if (slurm_cpus_per_task)
            num_threads_ = from_string<std::size_t>(slurm_cpus_per_task);
        else
        {
            char* slurm_job_cpus_on_node =
                std::getenv("SLURM_JOB_CPUS_PER_NODE");
            HPX_ASSERT(slurm_job_cpus_on_node != nullptr);
            if (slurm_job_cpus_on_node)
            {
                std::vector<std::string> tokens;
                hpx::string_util::split(tokens, slurm_job_cpus_on_node,
                    hpx::string_util::is_any_of(","));

                char* slurm_node_id = std::getenv("SLURM_NODEID");
                HPX_ASSERT(slurm_node_id != nullptr);
                if (slurm_node_id)
                {
                    std::size_t node_id =
                        from_string<std::size_t>(slurm_node_id);
                    std::size_t task_count = 0;
                    for (auto& token : tokens)
                    {
                        std::size_t paren_pos = token.find_first_of('(');
                        if (paren_pos != std::string::npos)
                        {
                            HPX_ASSERT(token[paren_pos + 1] == 'x');
                            HPX_ASSERT(token[token.size() - 1] == ')');
                            std::size_t begin = paren_pos + 2;
                            std::size_t end = token.size() - 1;
                            task_count += from_string<std::size_t>(
                                token.substr(paren_pos + 2, end - begin));
                        }
                        else
                        {
                            task_count += 1;
                        }
                        if (task_count > node_id)
                        {
                            num_threads_ = from_string<std::size_t>(
                                               token.substr(0, paren_pos)) /
                                num_tasks_;
                            break;
                        }
                    }
                    HPX_ASSERT(num_threads_);
                }
            }
        }
    }
}    // namespace hpx::util::batch_environments::detail
