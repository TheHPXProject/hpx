//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/asio.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/batch_environments/batch_environment.hpp>
#include <hpx/batch_environments/detail/alps_environment.hpp>
#include <hpx/batch_environments/detail/flux_environment.hpp>
#include <hpx/batch_environments/detail/pbs_environment.hpp>
#include <hpx/batch_environments/detail/pjm_environment.hpp>
#include <hpx/batch_environments/detail/slurm_environment.hpp>

#include <hpx/asio/asio_util.hpp>

#include <asio/io_context.hpp>
#include <asio/ip/host_name.hpp>
#include <asio/ip/tcp.hpp>

#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util::detail { namespace {

    struct batch_environment final : batch_environment_base
    {
        explicit batch_environment(std::vector<std::string>& nodelist,
            bool have_mpi = false, bool debug = false, bool enable = true);

        ~batch_environment() override = default;

        std::string init_from_nodelist(std::vector<std::string> const& nodes,
            std::string const& agas_host, bool have_tcp) override;

        std::size_t retrieve_number_of_threads() const noexcept override;

        std::size_t retrieve_number_of_localities() const noexcept override;

        std::size_t retrieve_node_number() const noexcept override;

        std::string host_name() const override;

        std::string host_name(std::string const& def_hpx_name) const override;

        std::string agas_host_name(std::string const& def_agas) const override;

        std::size_t agas_node() const noexcept override;

        bool found_batch_environment() const noexcept override;

        std::string get_batch_name() const override;

        std::string agas_node_;
        std::size_t agas_node_num_;
        std::size_t node_num_;
        std::size_t num_threads_;
        std::size_t num_localities_;
        std::string batch_name_;
        bool debug_;

#if defined(HPX_HAVE_PARCELPORT_TCP)
        using node_map_type = std::map<::asio::ip::tcp::endpoint,
            std::pair<std::string, std::size_t>>;

        node_map_type nodes_;
#endif
    };

    batch_environment::batch_environment(std::vector<std::string>& nodelist,
        bool have_mpi, bool debug, bool enable)
      : agas_node_num_(0)
      , node_num_(static_cast<std::size_t>(-1))
      , num_threads_(static_cast<std::size_t>(-1))
      , num_localities_(static_cast<std::size_t>(-1))
      , debug_(debug)
    {
        if (!enable)
            return;

        struct onexit
        {
            explicit onexit(batch_environment const& env) noexcept
              : env_(env)
            {
            }

            onexit(onexit const&) = delete;
            onexit(onexit&&) = delete;

            onexit& operator=(onexit const&) = delete;
            onexit& operator=(onexit&&) = delete;

            ~onexit()
            {
                if (env_.debug_)
                {
                    std::cerr << "batch_name: " << env_.batch_name_
                              << std::endl;
                    std::cerr << "num_threads: " << env_.num_threads_
                              << std::endl;
                    std::cerr << "node_num_: " << env_.node_num_ << std::endl;
                    std::cerr << "num_localities: " << env_.num_localities_
                              << std::endl;
                }
            }

            batch_environment const& env_;
        };

        onexit _(*this);

        batch_environments::detail::alps_environment const alps_env(
            nodelist, debug);
        if (alps_env.valid())
        {
            batch_name_ = "ALPS";
            num_threads_ = alps_env.num_threads();
            num_localities_ = alps_env.num_localities();
            node_num_ = alps_env.node_num();
            return;
        }

        batch_environments::detail::pjm_environment const pjm_env(
            nodelist, have_mpi, debug);
        if (pjm_env.valid())
        {
            batch_name_ = "PJM";
            num_threads_ = pjm_env.num_threads();
            num_localities_ = pjm_env.num_localities();
            node_num_ = pjm_env.node_num();
            return;
        }

        batch_environments::detail::flux_environment const flux_env;
        if (flux_env.valid())
        {
            batch_name_ = "FLUX";
            num_localities_ = flux_env.num_localities();
            node_num_ = flux_env.node_num();
            return;
        }

        batch_environments::detail::slurm_environment const slurm_env(
            nodelist, debug);
        if (slurm_env.valid())
        {
            batch_name_ = "SLURM";
            num_threads_ = slurm_env.num_threads();
            num_localities_ = slurm_env.num_localities();
            node_num_ = slurm_env.node_num();
            return;
        }

        batch_environments::detail::pbs_environment const pbs_env(
            nodelist, have_mpi, debug);
        if (pbs_env.valid())
        {
            batch_name_ = "PBS";
            num_threads_ = pbs_env.num_threads();
            num_localities_ = pbs_env.num_localities();
            node_num_ = pbs_env.node_num();
            return;
        }
    }

    // This function returns true if a batch environment was found.
    bool batch_environment::found_batch_environment() const noexcept
    {
        return !batch_name_.empty();
    }

    // this function initializes the map of nodes from the given a list of nodes
    std::string batch_environment::init_from_nodelist(
        std::vector<std::string> const& nodes, std::string const& agas_host,
        [[maybe_unused]] bool have_tcp)
    {
        if (debug_)
            std::cerr << "got node list" << std::endl;

        std::string nodes_list;
        bool found_agas_host = false;

#if defined(HPX_HAVE_NETWORKING)
        ::asio::io_context io_service;

        std::size_t agas_node_num = 0;
        for (std::string const& s : nodes)
        {
            if (!s.empty())
            {
                if (debug_)
                    std::cerr << "extracted: '" << s << "'" << std::endl;

#if defined(HPX_HAVE_PARCELPORT_TCP)
                if (!found_agas_host &&
                    ((agas_host.empty() && nodes_.empty()) || s == agas_host))
                {
                    found_agas_host = true;
                    agas_node_ = s;
                    agas_node_num_ = agas_node_num;
                }

                if (have_tcp)
                {
                    ::asio::ip::tcp::endpoint ep =
                        util::resolve_hostname(s, 0, io_service);

                    if (0 == nodes_.count(ep))
                    {
                        if (debug_)
                            std::cerr << "incrementing agas_node_num"
                                      << std::endl;
                        ++agas_node_num;
                    }

                    std::pair<std::string, std::size_t>& data = nodes_[ep];
                    if (data.first.empty())
                        data.first = s;
                    ++data.second;
                }
#else
                if (!found_agas_host && (agas_host.empty() || s == agas_host))
                {
                    found_agas_host = true;
                    agas_node_ = s;
                    agas_node_num_ = agas_node_num;
                }
#endif
                nodes_list += s + ' ';
            }
        }
#endif

        // if an AGAS host is specified, it needs to be in the list of nodes
        // participating in this run
        if (!agas_host.empty() && !found_agas_host)
        {
            throw hpx::detail::command_line_error("Requested AGAS host (" +
                agas_host + ") not found in node list");
        }

        if (debug_)
        {
            if (!agas_node_.empty())
            {
                std::cerr << "using AGAS host: '" << agas_node_
                          << "' (node number " << agas_node_num_ << ")"
                          << std::endl;
            }

#if defined(HPX_HAVE_PARCELPORT_TCP)
            std::cerr << "Nodes from nodelist:" << std::endl;
            node_map_type::const_iterator const end = nodes_.end();
            // clang-format off
            for (node_map_type::const_iterator it = nodes_.begin(); it != end;
                 ++it)
            {
                std::cerr << (*it).second.first << ": " << (*it).second.second
                          << " (" << (*it).first << ")" << std::endl;
            }
            // clang-format on
#endif
        }
        HPX_UNUSED(nodes);
        return nodes_list;
    }

    // The number of threads is either one (if no PBS/SLURM information was
    // found), or it is the same as the number of times this node has been
    // listed in the node file. Additionally, this takes into account the number
    // of tasks run on this node.
    std::size_t batch_environment::retrieve_number_of_threads() const noexcept
    {
        return num_threads_;
    }

    // The number of localities is either one (if no PBS information was found),
    // or it is the same as the number of distinct node names listed in the node
    // file. In case of SLURM we can extract the number of localities from the
    // job environment.
    std::size_t batch_environment::retrieve_number_of_localities()
        const noexcept
    {
        return num_localities_;
    }

    // Try to retrieve the node number from the PBS/SLURM environment
    std::size_t batch_environment::retrieve_node_number() const noexcept
    {
        return node_num_;
    }

    std::string batch_environment::host_name() const
    {
        std::string hostname = ::asio::ip::host_name();
        if (debug_)
            std::cerr << "asio host_name: " << hostname << std::endl;
        return hostname;
    }

    std::string batch_environment::host_name(
        [[maybe_unused]] std::string const& def_hpx_name) const
    {
#if defined(HPX_HAVE_PARCELPORT_TCP)
        std::string host = nodes_.empty() ? def_hpx_name : host_name();
#else
        std::string host = host_name();
#endif
        if (debug_)
            std::cerr << "host_name: " << host << std::endl;
        return host;
    }

    // We either select the first host listed in the node file or a given host
    // name to host the AGAS server.
    std::string batch_environment::agas_host_name(
        std::string const& def_agas) const
    {
        std::string host = agas_node_.empty() ? def_agas : agas_node_;
        if (debug_)
            std::cerr << "agas host_name: " << host << std::endl;
        return host;
    }

    std::size_t batch_environment::agas_node() const noexcept
    {
        return agas_node_num_;
    }

    // Return a string containing the name of the batch system
    std::string batch_environment::get_batch_name() const
    {
        return batch_name_;
    }
}}    // namespace hpx::util::detail

namespace hpx::util {

    batch_environment::batch_environment(std::vector<std::string>& nodelist,
        bool have_mpi, bool debug, bool enable)
      : data_(std::make_unique<detail::batch_environment>(
            nodelist, have_mpi, debug, enable))
    {
    }

    // this function initializes the map of nodes from the given (space
    // separated) list of nodes
    std::string batch_environment::init_from_nodelist(
        std::vector<std::string> const& nodes, std::string const& agas_host,
        bool have_tcp) const
    {
        return data_->init_from_nodelist(nodes, agas_host, have_tcp);
    }

    // The number of threads is either one (if no PBS information was
    // found), or it is the same as the number of times this node has been
    // listed in the node file.
    std::size_t batch_environment::retrieve_number_of_threads() const noexcept
    {
        return data_->retrieve_number_of_threads();
    }

    // The number of localities is either one (if no PBS information was
    // found), or it is the same as the number of distinct node names listed
    // in the node file.
    std::size_t batch_environment::retrieve_number_of_localities()
        const noexcept
    {
        return data_->retrieve_number_of_localities();
    }

    // Try to retrieve the node number from the PBS environment
    std::size_t batch_environment::retrieve_node_number() const noexcept
    {
        return data_->retrieve_node_number();
    }

    std::string batch_environment::host_name() const
    {
        return data_->host_name();
    }

    std::string batch_environment::host_name(
        std::string const& def_hpx_name) const
    {
        return data_->host_name(def_hpx_name);
    }

    // We either select the first host listed in the node file or a given
    // host name to host the AGAS server.
    std::string batch_environment::agas_host_name(
        std::string const& def_agas) const
    {
        return data_->agas_host_name(def_agas);
    }

    // The AGAS node number represents the number of the node which has been
    // selected as the AGAS host.
    std::size_t batch_environment::agas_node() const noexcept
    {
        return data_->agas_node();
    }

    // The function will analyze the current environment and return true if
    // it finds sufficient information to deduce its running as a batch job.
    bool batch_environment::found_batch_environment() const noexcept
    {
        return data_->found_batch_environment();
    }

    // Return a string containing the name of the batch system
    std::string batch_environment::get_batch_name() const
    {
        return data_->get_batch_name();
    }

}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
