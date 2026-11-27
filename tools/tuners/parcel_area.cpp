//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The parcel area of hpx_tune (--parcel): host data through the TCP and MPI
// parcelports, with and without the UCX device conduit (the first version of
// the parcel area, design/parcel.md of TheHPXProject/hpx-tuner-tool). Each
// configuration is measured in a child of two localities, started through
// hpxrun.py (or --launcher); the child is hpx_tune itself, run with
// --child=parcel:<parcelport>:<conduit>, the only part of the tool that starts
// the HPX runtime.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/parcel_tuning.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"
#include "child.hpp"
#include "parcel_area.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(_WIN32)
#include <unistd.h>    // environ
#endif

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace hpx_tune_detail {

    // the core fingerprint of the locality it runs on, for a child to compare
    // its two localities' nodes
    std::vector<std::pair<std::string, std::string>> host_entries()
    {
        std::vector<std::pair<std::string, std::string>> entries;
        for (tuning::fingerprint_entry const& e :
            tuning::describe_host(hpx::threads::get_topology()).entries)
        {
            entries.emplace_back(e.key, e.value);
        }
        return entries;
    }
}    // namespace hpx_tune_detail

HPX_PLAIN_ACTION(hpx_tune_detail::host_entries, hpx_tune_host_entries_action)

namespace hpx_tune {

    namespace {

        ///////////////////////////////////////////////////////////////////////////
        // The parcel area: host data through the TCP and MPI parcelports, with
        // and without the UCX device conduit (the first version of the parcel
        // area, design/parcel.md of TheHPXProject/hpx-tuner-tool). Each
        // configuration is measured in a child of two localities, started
        // through a launcher (or by the tool itself, two processes on this node,
        // for the TCP parcelport); the child is this program, run with
        // --child=parcel:<parcelport>:<conduit>, and it reports its
        // measurements on its standard output.

        // a measurement with about three significant digits: 15626, 55.2, 1.31
        std::string number(double const value)
        {
            double const magnitude = value < 0 ? -value : value;
            int const decimals = magnitude >= 100.0 ? 0 :
                magnitude >= 10.0                   ? 1 :
                                                      2;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << value;
            return os.str();
        }

        std::string describe(tuning::summary const& s, char const* unit)
        {
            return number(s.median) + " " + unit + " (quartiles " +
                number(s.q1) + " to " + number(s.q3) + ")";
        }

    }    // namespace

    po::options_description parcel_options()
    {
        po::options_description desc("Options of the parcel area (--parcel)");
        // clang-format off
        desc.add_options()
            ("hpxrun", po::value<std::string>(),
                "hpxrun.py, which starts the localities of a child (default: "
                "next to this program, ../../bin seen from it (the build "
                "tree's bin directory from <build>/<config>/bin), the build "
                "tree's bin directory, the installation's)")
            ("runwrapper", po::value<std::string>(),
                "hpxrun.py's run wrapper: none (the processes on this node), "
                "srun or mpi (default: inside a Slurm allocation none for "
                "TCP on one node, as HPX gives localities on one host "
                "different ports only when it starts them itself, srun "
                "otherwise; outside an allocation none for TCP and mpi "
                "for MPI)")
            ("wrapper-args-same-node", po::value<std::string>(),
                "srun's arguments for the same node ({threads} is replaced "
                "by --child-threads; default: \"-N 1 -c {threads} "
                "--overlap --cpu-bind=none\" (HPX binds its threads "
                "itself); inside an allocation, the tool's own step "
                "needs at least that many CPUs per task)")
            ("wrapper-args-two-nodes", po::value<std::string>(),
                "srun's arguments for two nodes (default: \"-N 2 -c "
                "{threads} --overlap --cpu-bind=none\")")
            ("launcher", po::value<std::string>(),
                "instead of hpxrun.py: how to start a child of two "
                "localities, {} replaced by its command line, {nodes} by "
                "the number of nodes and {threads} by --child-threads")
            ("placement", po::value<std::vector<std::string>>(),
                "same-node or two-nodes; may be given more than once "
                "(default: same-node, and two-nodes inside a Slurm "
                "allocation of at least two nodes)")
            ("parcel-configurations", po::value<std::vector<std::string>>(),
                "the parcelports to measure, tcp or mpi; may be given more "
                "than once (default: those compiled in)")
            ("sizes", po::value<std::string>()->default_value("1K:64M"),
                "the transfer sizes as <from>:<to> (bytes, or with the "
                "suffixes K and M), the powers of 2 between them")
            ("rounds", po::value<std::size_t>()->default_value(50),
                "measured rounds per size and direction")
            ("passes", po::value<std::size_t>()->default_value(2),
                "how many times every configuration is measured, in a new "
                "random order each time; the rules use the median of the "
                "passes and leave out sizes at which they differ by more "
                "than 10%")
            ("seed", po::value<unsigned>(),
                "the seed of the random order of the children (default: "
                "random, written into the file's comments)")
            ("progress",
                "print a line for every size a child has measured, while it "
                "runs (--verbose prints them too)")
            ("child-threads", po::value<std::size_t>()->default_value(4),
                "worker threads per locality of a child")
            ("child-timeout", po::value<std::size_t>()->default_value(600),
                "seconds after which a child is stopped (0: never; uses "
                "the timeout command where it exists)")
            ("child", po::value<std::string>(),
                "internal: run as a child, <area>:<configuration>");
        // clang-format on
        return desc;
    }

    namespace {

        std::size_t parse_size(std::string const& text)
        {
            std::size_t pos = 0;
            std::size_t value = std::stoull(text, &pos);
            if (pos < text.size())
            {
                char const suffix = text[pos];
                if (suffix == 'K' || suffix == 'k')
                    value <<= 10;
                else if (suffix == 'M' || suffix == 'm')
                    value <<= 20;
                else if (suffix == 'G' || suffix == 'g')
                    value <<= 30;
                else
                    throw std::invalid_argument("bad size: " + text);
            }
            return value;
        }

        std::vector<std::size_t> parse_sizes(std::string const& text)
        {
            auto const colon = text.find(':');
            std::size_t const from = parse_size(text.substr(0, colon));
            std::size_t const to = colon == std::string::npos ?
                from :
                parse_size(text.substr(colon + 1));
            std::vector<std::size_t> sizes;
            for (std::size_t size = 1; size <= to; size *= 2)
            {
                if (size >= from)
                    sizes.push_back(size);
            }
            return sizes;
        }

        // the --hpx:ini settings of a child's configuration
        std::vector<std::string> child_settings(
            std::string const& parcelport, std::string const& conduit)
        {
            std::vector<std::string> ini;
            bool const tcp = parcelport == "tcp";
#if defined(HPX_HAVE_PARCELPORT_TCP)
            ini.push_back(
                std::string("hpx.parcel.tcp.enable=") + (tcp ? "1" : "0"));
            if (tcp)
                ini.push_back("hpx.parcel.tcp.priority=1000");
#endif
#if defined(HPX_HAVE_PARCELPORT_MPI)
            ini.push_back(
                std::string("hpx.parcel.mpi.enable=") + (tcp ? "0" : "1"));
            if (!tcp)
                ini.push_back("hpx.parcel.mpi.priority=1000");
#endif
#if defined(HPX_HAVE_PARCELPORT_LCI)
            ini.push_back("hpx.parcel.lci.enable=0");
#endif
#if defined(HPX_HAVE_PARCELPORT_LCW)
            ini.push_back("hpx.parcel.lcw.enable=0");
#endif
#if defined(HPX_HAVE_PARCELPORT_GASNET)
            ini.push_back("hpx.parcel.gasnet.enable=0");
#endif
            ini.push_back("hpx.parcel.bootstrap=" + parcelport);

            if (conduit == "none")
            {
                ini.push_back("hpx.parcel.device_conduit=none");
                ini.push_back("hpx.parcel.conduit_host_chunks=0");
            }
            else
            {
                // every chunk of the zero-copy threshold or larger through the
                // conduit, so that the curve shows where it pays off
                ini.push_back("hpx.parcel.device_conduit=" + conduit);
                ini.push_back("hpx.parcel.conduit_host_chunks=1");
                ini.push_back(
                    "hpx.parcel." + parcelport + ".conduit_host_chunks=1");
                ini.push_back("hpx.parcel.conduit_host_threshold=8192");
            }
            return ini;
        }

        tuning::summary parse_summary(std::istream& is)
        {
            tuning::summary s;
            is >> s.count >> s.min >> s.q1 >> s.median >> s.q3 >> s.p90 >>
                s.max;
            return s;
        }

        // one child's result
        struct child_result
        {
            bool complete = false;
            bool host_chunk_support = false;
            // the core fingerprints of the child's two localities
            std::vector<std::pair<std::string, std::string>> here;
            std::vector<std::pair<std::string, std::string>> peer;
            std::vector<tuning::transfer_point> points;
        };

        child_result parse_child(std::vector<std::string> const& lines)
        {
            child_result r;
            for (std::string const& line : lines)
            {
                std::istringstream is(line);
                std::string kind;
                is >> kind;
                if (kind == "point")
                {
                    tuning::transfer_point p;
                    is >> p.size;
                    p.get_us = parse_summary(is);
                    p.set_us = parse_summary(is);
                    is >> p.conduit_chunks_per_round >> p.errors;
                    if (is)
                        r.points.push_back(p);
                }
                else if (kind == "host_chunk_support")
                {
                    int v = 0;
                    is >> v;
                    r.host_chunk_support = v != 0;
                }
                else if (kind == "here" || kind == "peer")
                {
                    std::string key, value;
                    is >> key;
                    std::getline(is >> std::ws, value);
                    (kind == "here" ? r.here : r.peer).emplace_back(key, value);
                }
                else if (kind == "end")
                {
                    r.complete = true;
                }
            }
            return r;
        }

        struct parcel_run_options
        {
            std::string self;
            std::string python;
            std::string hpxrun;
            std::string runwrapper;    // empty: by parcelport and placement
            bool in_slurm = false;
            std::string wrapper_args_same_node;
            std::string wrapper_args_two_nodes;
            bool wrapper_args_given = false;
            std::string launcher;    // instead of hpxrun.py
            unsigned port = 7911;    // the next child's port
            std::string sizes;
            std::size_t rounds = 50;
            std::size_t threads = 4;
            std::size_t timeout = 600;
            bool verbose = false;
            bool progress = false;
            std::size_t child = 0;       // the child being measured, from 1
            std::size_t children = 0;    // the children to measure
        };

        // inside a Slurm job step: "env -u SLURM_... " for every variable of the
        // step but SLURM_JOB_ID, so that an srun started by the child's launcher
        // sees the job's allocation, not the step's; empty otherwise
        // inside a job step srun sees only the step's nodes: the arguments of
        // env that remove the step's variables (all SLURM_ ones but the job's
        // number), empty outside a step
        std::vector<std::string> job_environment()
        {
            std::vector<std::string> env;
#if !defined(_WIN32)
            if (std::getenv("SLURM_STEP_ID") == nullptr ||
                std::getenv("SLURM_JOB_ID") == nullptr)
            {
                return env;
            }
            env.emplace_back("env");
            for (char** e = environ; *e != nullptr; ++e)
            {
                std::string const entry(*e);
                std::string const name = entry.substr(0, entry.find('='));
                if (name.rfind("SLURM_", 0) == 0 && name != "SLURM_JOB_ID")
                {
                    env.emplace_back("-u");
                    env.push_back(name);
                }
            }
#endif
            return env;
        }

        std::string as_text(std::vector<std::string> const& argv)
        {
            std::string text;
            for (std::string const& arg : argv)
                text += (text.empty() ? "" : " ") + in_quotes(arg);
            return text;
        }

        // starts one child and returns its measurements
        child_result run_child(parcel_run_options& o,
            std::string const& parcelport, std::string const& conduit,
            std::string const& placement)
        {
            std::string const threads = std::to_string(o.threads);
            std::vector<std::string> args = {
                "--sizes=" + o.sizes, "--rounds=" + std::to_string(o.rounds)};
            for (std::string const& s : child_settings(parcelport, conduit))
                args.push_back("--hpx:ini=" + s);
            std::string const area = "parcel:" + parcelport + ":" + conduit;

            // a launcher template is a command line (run by the shell), the
            // default is hpxrun.py's arguments (no shell)
            std::string command;
            std::vector<std::string> argv;
            if (!o.launcher.empty())
            {
                args.push_back("--hpx:threads=" + threads);
                command = replace_all(o.launcher, "{nodes}",
                    placement == "two-nodes" ? "2" : "1");
                command = replace_all(command, "{threads}", threads);
                command = replace_all(command, "{}", child_command(area, args));
            }
            else
            {
                // inside a Slurm allocation srun, except for TCP on one node:
                // there hpxrun.py starts the processes itself, as HPX gives the
                // localities on one host different ports only then; outside an
                // allocation the processes on this node for TCP, mpiexec for MPI
                std::string wrapper = o.runwrapper;
                if (wrapper.empty())
                {
                    if (parcelport == "tcp" && placement == "same-node")
                        wrapper = "none";
                    else if (o.in_slurm)
                        wrapper = "srun";
                    else
                        wrapper = parcelport == "mpi" ? "mpi" : "none";
                }
                std::string wrapper_args = placement == "two-nodes" ?
                    o.wrapper_args_two_nodes :
                    o.wrapper_args_same_node;
                // Open MPI and MPICH under srun need PMIx
                if (wrapper == "srun" && parcelport == "mpi" &&
                    !o.wrapper_args_given)
                {
                    wrapper_args += " --mpi=pmix";
                }
                // inside a job step: start the child as a step of the job,
                // with the step's variables removed
                if (wrapper == "srun")
                {
                    argv = job_environment();
                    if (!argv.empty())
                    {
                        wrapper_args += std::string(" --jobid=") +
                            std::getenv("SLURM_JOB_ID");
                    }
                }
#if !defined(_WIN32)
                if (o.timeout != 0)
                {
                    argv.insert(argv.end(),
                        {"timeout", "-k", "10", std::to_string(o.timeout)});
                }
#endif
                // a port of its own for every child: a port just closed may not
                // be free yet
                argv.insert(argv.end(),
                    {o.python, o.hpxrun, o.self, "-l", "2", "-t", threads, "-p",
                        parcelport, "-r", wrapper, "-d",
                        std::to_string(o.port)});
                o.port += 2;
                if (!wrapper_args.empty() && wrapper == "srun")
                {
                    argv.emplace_back("-w");
                    argv.push_back(
                        replace_all(wrapper_args, "{threads}", threads));
                }
                argv.emplace_back("--");
                argv.push_back("--child=" + area);
                argv.insert(argv.end(), args.begin(), args.end());
                // processes started by hpxrun.py itself must not take their
                // locality numbers from a batch environment (inside a Slurm step
                // both would be locality 0, on the same port)
                if (wrapper == "none")
                    argv.emplace_back("--hpx:ignore-batch-env");
                command = as_text(argv);
            }
#if !defined(_WIN32)
            if (!o.launcher.empty() && o.timeout != 0)
            {
                command = "timeout -k 10 " + std::to_string(o.timeout) + " " +
                    command;
            }
#endif
            if (o.verbose)
                std::cout << "parcel: " << command << "\n";

            // progress: the sizes as the child measures them
            std::string const what = "parcel: [" + std::to_string(o.child) +
                "/" + std::to_string(o.children) + "] " + placement + ", " +
                parcelport + ", conduit " + conduit;
            auto const on_line = [&](std::string const& line) {
                if (!(o.progress || o.verbose) ||
                    line.rfind(" progress ", 0) != 0)
                    return;
                std::istringstream is(line.substr(10));
                std::size_t size = 0;
                double get_us = 0.0;
                double set_us = 0.0;
                is >> size >> get_us >> set_us;
                std::cout << what << ": " << size << " bytes, get "
                          << number(get_us) << " us, set " << number(set_us)
                          << " us\n"
                          << std::flush;
            };
            try
            {
                return parse_child(argv.empty() ?
                        run_command(command, "parcel", on_line).lines :
                        run_program(argv, "parcel", on_line).lines);
            }
            catch (std::exception const& e)
            {
                std::cerr << "hpx_tune: " << e.what() << "\n";
                return child_result{};
            }
        }

        // hpxrun.py: given, next to this program, in the build tree's bin
        // directory seen from <build>/<config>/bin (where multi-configuration
        // generators put this program), the build tree's, the installation's
        std::string find_hpxrun(po::variables_map const& vm)
        {
            if (vm.count("hpxrun"))
                return vm["hpxrun"].as<std::string>();

            std::string const self = hpx::util::get_executable_filename();
            std::string const self_dir =
                self.substr(0, self.find_last_of("/\\") + 1);
            std::vector<std::string> candidates = {
                self_dir + "hpxrun.py", self_dir + "../../bin/hpxrun.py"};
#if defined(HPX_TUNE_HPXRUN_BUILD)
            candidates.push_back(HPX_TUNE_HPXRUN_BUILD);
#endif
#if defined(HPX_TUNE_HPXRUN_INSTALL)
            candidates.push_back(HPX_TUNE_HPXRUN_INSTALL);
#endif
            for (std::string const& c : candidates)
            {
                if (std::ifstream(c).good())
                    return c;
            }
            return "";
        }

        // the number of nodes in a Slurm hostlist ("medusa[14-15],anvil" is 3)
        std::size_t count_hosts(std::string const& list)
        {
            std::size_t count = 0;
            std::size_t depth = 0;
            std::string range;
            bool in_name = false;
            auto const end_range = [&]() {
                auto const dash = range.find('-');
                count += dash == std::string::npos ?
                    1 :
                    std::stoul(range.substr(dash + 1)) -
                        std::stoul(range.substr(0, dash)) + 1;
                range.clear();
            };
            for (char const c : list)
            {
                if (c == '[')
                {
                    ++depth;
                    in_name = false;
                }
                else if (c == ']')
                {
                    end_range();
                    --depth;
                }
                else if (depth != 0)
                {
                    if (c == ',')
                        end_range();
                    else
                        range += c;
                }
                else if (c == ',')
                {
                    if (in_name)
                        ++count;
                    in_name = false;
                }
                else
                {
                    in_name = true;
                }
            }
            if (in_name)
                ++count;
            return count;
        }

        // the nodes of the Slurm allocation, also inside a job step (where
        // SLURM_NNODES and SLURM_JOB_NODELIST describe the step); 0 outside one
        std::size_t slurm_nodes()
        {
            if (char const* value = std::getenv("SLURM_JOB_NUM_NODES"))
                return std::stoul(value);
            if (char const* list = std::getenv("SLURM_NODELIST"))
                return count_hosts(list);
            return 0;
        }

    }    // namespace

    namespace {

        std::string json_curve(std::vector<tuning::transfer_point> const& c)
        {
            std::vector<std::string> points;
            points.reserve(c.size());
            for (tuning::transfer_point const& p : c)
            {
                points.push_back(tuning::json_object({
                    {"size", std::to_string(p.size)},
                    {"get_us", tuning::to_json(p.get_us)},
                    {"set_us", tuning::to_json(p.set_us)},
                    {"conduit_chunks_per_round",
                        tuning::json_number(p.conduit_chunks_per_round)},
                    {"errors", std::to_string(p.errors)},
                }));
            }
            return tuning::json_array(points);
        }

        // the parcel area's object in the report (--report): every pass,
        // the combined curves and the rules' results, by placement
        class report_writer
        {
        public:
            void begin_placement(std::string const& placement)
            {
                placement_ = placement;
                configurations_.clear();
            }

            void configuration(std::string const& parcelport,
                std::string const& conduit,
                std::vector<std::vector<tuning::transfer_point>> const& passes,
                tuning::combined_curve const& combined)
            {
                std::vector<std::string> unstable;
                for (std::size_t const size : combined.unstable)
                    unstable.push_back(std::to_string(size));
                std::vector<std::string> passes_json;
                for (auto const& pass : passes)
                    passes_json.push_back(json_curve(pass));
                configurations_.push_back(tuning::json_object({
                    {"parcelport", tuning::json_string(parcelport)},
                    {"conduit", tuning::json_string(conduit)},
                    {"passes", tuning::json_array(passes_json)},
                    {"combined", json_curve(combined.points)},
                    {"unstable", tuning::json_array(unstable)},
                }));
            }

            void end_placement(tuning::parcel_rules_result const& rules)
            {
                std::vector<std::string> settings;
                for (tuning::tuned_setting const& s : rules.settings)
                    settings.push_back(tuning::to_json(s));
                std::vector<std::string> report;
                for (std::string const& line : rules.report)
                    report.push_back(tuning::json_string(line));
                placements_.push_back(tuning::json_object({
                    {"placement", tuning::json_string(placement_)},
                    {"configurations", tuning::json_array(configurations_)},
                    {"settings", tuning::json_array(settings)},
                    {"report", tuning::json_array(report)},
                }));
            }

            std::string text(std::size_t passes, unsigned seed) const
            {
                return tuning::json_object({
                    {"passes", std::to_string(passes)},
                    {"seed", std::to_string(seed)},
                    {"placements", tuning::json_array(placements_)},
                });
            }

        private:
            std::string placement_;
            std::vector<std::string> configurations_;
            std::vector<std::string> placements_;
        };
    }    // namespace

    // The parcel area: phases 3 and 4 of the design, the first version.
    area_result tune_parcel(area_context const& context)
    {
        po::variables_map const& vm = context.vm;
        char const* const argv0 = context.executable;
        bool const verbose = context.verbose;
        tuning::host_fingerprint& fingerprint = context.fingerprint;

        parcel_run_options o;
        o.self = hpx::util::get_executable_filename(argv0);
        o.sizes = vm["sizes"].as<std::string>();
        o.rounds = vm["rounds"].as<std::size_t>();
        o.threads = vm["child-threads"].as<std::size_t>();
        o.timeout = vm["child-timeout"].as<std::size_t>();
        o.verbose = verbose;
        o.progress = vm.count("progress") != 0;

        bool const in_slurm = std::getenv("SLURM_JOB_ID") != nullptr;
        if (vm.count("launcher"))
        {
            o.launcher = vm["launcher"].as<std::string>();
        }
        else
        {
            o.python = HPX_TUNE_PYTHON;
            o.hpxrun = find_hpxrun(vm);
            if (o.hpxrun.empty())
            {
                std::cerr << "hpx_tune: can't find hpxrun.py (see --hpxrun)\n";
                return area_result{};
            }
            o.in_slurm = in_slurm;
            if (vm.count("runwrapper"))
                o.runwrapper = vm["runwrapper"].as<std::string>();
            o.wrapper_args_given = vm.count("wrapper-args-same-node") != 0 ||
                vm.count("wrapper-args-two-nodes") != 0;
            o.wrapper_args_same_node = vm.count("wrapper-args-same-node") ?
                vm["wrapper-args-same-node"].as<std::string>() :
                "-N 1 -c {threads} --overlap --cpu-bind=none";
            o.wrapper_args_two_nodes = vm.count("wrapper-args-two-nodes") ?
                vm["wrapper-args-two-nodes"].as<std::string>() :
                "-N 2 -c {threads} --overlap --cpu-bind=none";
        }

        std::vector<std::string> placements;
        if (vm.count("placement"))
        {
            placements = vm["placement"].as<std::vector<std::string>>();
        }
        else
        {
            placements.push_back("same-node");
            if (in_slurm && slurm_nodes() >= 2)
                placements.push_back("two-nodes");
        }

        std::vector<std::string> parcelports;
        if (vm.count("parcel-configurations"))
        {
            parcelports =
                vm["parcel-configurations"].as<std::vector<std::string>>();
        }
        else
        {
#if defined(HPX_HAVE_PARCELPORT_TCP)
            parcelports.push_back("tcp");
#endif
#if defined(HPX_HAVE_PARCELPORT_MPI)
            parcelports.push_back("mpi");
#endif
        }

        std::vector<std::string> conduits = {"none"};
#if defined(HPX_HAVE_DEVICE_CONDUIT_UCX)
        conduits.push_back("ucx");
#endif

        std::size_t const passes =
            (std::max) (vm["passes"].as<std::size_t>(), std::size_t(1));
        unsigned const seed = vm.count("seed") ? vm["seed"].as<unsigned>() :
                                                 std::random_device{}();

        // every configuration once per pass, each pass in a new random order
        // (changes of the machine during the run then spread over all
        // configurations rather than biasing one)
        struct job
        {
            std::string placement;
            std::string parcelport;
            std::string conduit;
        };
        std::vector<job> configurations;
        for (std::string const& placement : placements)
            for (std::string const& pp : parcelports)
                for (std::string const& conduit : conduits)
                    configurations.push_back(job{placement, pp, conduit});

        std::vector<job> order;
        std::mt19937 random(seed);
        for (std::size_t pass = 0; pass != passes; ++pass)
        {
            std::vector<job> this_pass = configurations;
            std::shuffle(this_pass.begin(), this_pass.end(), random);
            order.insert(order.end(), this_pass.begin(), this_pass.end());
        }
        o.children = order.size();

        area_result result;
        result.comments.push_back("parcel: " + std::to_string(passes) +
            " passes, the children in random order (--seed " +
            std::to_string(seed) + ")");
        bool host_chunk_support = false;
        bool peer_recorded = false;
        std::vector<std::string> measured_names;

        // the passes of each configuration, by placement, parcelport, conduit
        std::map<std::string, std::vector<std::vector<tuning::transfer_point>>>
            measured_passes;
        auto const key = [](job const& j) {
            return j.placement + ":" + j.parcelport + ":" + j.conduit;
        };

        for (job const& j : order)
        {
            ++o.child;
            std::cout << "parcel: [" << o.child << "/" << o.children
                      << "] measuring " << j.placement << ", " << j.parcelport
                      << ", conduit " << j.conduit << "\n"
                      << std::flush;
            child_result const r =
                run_child(o, j.parcelport, j.conduit, j.placement);
            std::string const what = "parcel: " + j.placement + ", " +
                j.parcelport + ", conduit " + j.conduit;
            if (!r.complete || r.points.empty())
            {
                result.comments.push_back(
                    what + ": the child failed or gave no measurements");
                std::cout << what << ": failed\n";
                continue;
            }
            host_chunk_support = host_chunk_support || r.host_chunk_support;
            if (std::find(measured_names.begin(), measured_names.end(),
                    key(j)) == measured_names.end())
            {
                measured_names.push_back(key(j));
            }

            // the other node, where it differs from locality 0's
            if (j.placement == "two-nodes" && !peer_recorded)
            {
                peer_recorded = true;
                for (auto const& [name, value] : r.peer)
                {
                    auto const same = std::find_if(r.here.begin(), r.here.end(),
                        [&](auto const& h) { return h.first == name; });
                    if (same == r.here.end() || same->second != value)
                    {
                        fingerprint.add(
                            "parcel_peer_" + name, value, name != "host_name");
                    }
                }
            }
            if (verbose)
            {
                for (tuning::transfer_point const& p : r.points)
                {
                    std::cout << what << ", " << p.size << " bytes: get "
                              << describe(p.get_us, "us") << ", set "
                              << describe(p.set_us, "us") << ", "
                              << number(p.conduit_chunks_per_round)
                              << " conduit chunks per round\n";
                }
            }
            measured_passes[key(j)].push_back(r.points);
        }

        // per placement: the passes combined, then the rules
        std::vector<tuning::tuned_setting> written;
        report_writer report;
        for (std::string const& placement : placements)
        {
            std::vector<tuning::parcelport_measurements> measured;
            report.begin_placement(placement);
            for (std::string const& pp : parcelports)
            {
                tuning::parcelport_measurements m;
                m.parcelport = pp;
                for (std::string const& conduit : conduits)
                {
                    auto const it =
                        measured_passes.find(key(job{placement, pp, conduit}));
                    if (it == measured_passes.end())
                        continue;
                    tuning::combined_curve const combined =
                        tuning::combine_passes(it->second);
                    if (!combined.unstable.empty())
                    {
                        std::string sizes;
                        for (std::size_t const size : combined.unstable)
                        {
                            sizes += (sizes.empty() ? "" : ", ") +
                                std::to_string(size);
                        }
                        std::string const line = "parcel: " + placement + ", " +
                            pp + ", conduit " + conduit +
                            ": the passes differ by more than 10% at " + sizes +
                            " bytes, left out";
                        result.comments.push_back(line);
                        std::cout << line << "\n";
                    }
                    report.configuration(pp, conduit, it->second, combined);
                    (conduit == "none" ? m.plain : m.conduit) = combined.points;
                    m.unstable.insert(m.unstable.end(),
                        combined.unstable.begin(), combined.unstable.end());
                }
                measured.push_back(HPX_MOVE(m));
            }

            tuning::parcel_rules_result const rules =
                tuning::derive_parcel_settings(measured);
            for (std::string const& line : rules.report)
            {
                result.comments.push_back("parcel: " + placement + ": " + line);
                std::cout << "parcel: " << placement << ": " << line << "\n";
            }
            for (tuning::tuned_setting const& s : rules.settings)
            {
                std::cout << "parcel: " << placement << ": " << s.key << " = "
                          << s.value << " (" << s.reason
                          << (s.is_default ? ", HPX's default" : "") << ")\n";
            }
            report.end_placement(rules);

            // one value per setting for all destinations until the parcel
            // layer has settings per destination class: the two-node
            // placement's where it was measured
            if (written.empty() || placement == "two-nodes")
                written = rules.settings;
        }

        result.report = report.text(passes, seed);

        // the parcel layer's part of the fingerprint
        std::string compiled;
        for (char const* pp : {
#if defined(HPX_HAVE_PARCELPORT_TCP)
                 "tcp",
#endif
#if defined(HPX_HAVE_PARCELPORT_MPI)
                 "mpi",
#endif
#if defined(HPX_HAVE_PARCELPORT_LCI)
                 "lci",
#endif
#if defined(HPX_HAVE_PARCELPORT_LCW)
                 "lcw",
#endif
#if defined(HPX_HAVE_PARCELPORT_GASNET)
                 "gasnet",
#endif
                 ""})
        {
            if (*pp != '\0')
                compiled += (compiled.empty() ? "" : ",") + std::string(pp);
        }
        fingerprint.add("parcel_parcelports", compiled);
        std::string compiled_conduits;
        for (std::string const& c : conduits)
        {
            if (c != "none")
            {
                compiled_conduits += (compiled_conduits.empty() ? "" : ",") + c;
            }
        }
        fingerprint.add("parcel_conduits",
            compiled_conduits.empty() ? "none" : compiled_conduits);
        std::string measured_list;
        for (std::string const& m : measured_names)
            measured_list += (measured_list.empty() ? "" : ",") + m;
        fingerprint.add("parcel_measured", measured_list, false);

        if (!host_chunk_support)
        {
            result.comments.push_back(
                "parcel: this HPX doesn't move host chunks through a device "
                "conduit (hpx.parcel.conduit_host_chunks is unknown)");
        }
        result.sections = tuning::to_sections(written);
        return result;
    }

    namespace {

        // the child: measures one configuration from locality 0 to locality 1
        // and writes the measurements to its standard output
        int child_main(po::variables_map& vm)
        {
            std::string const child_prefix = child_line_prefix("parcel");
            std::vector<hpx::id_type> const remotes =
                hpx::find_remote_localities();
            if (remotes.empty())
            {
                std::cout << child_prefix << " error no remote locality\n";
                return hpx::finalize();
            }

            tuning::transfer_options options;
            options.sizes = parse_sizes(vm["sizes"].as<std::string>());
            options.rounds = vm["rounds"].as<std::size_t>();

            std::cout << child_prefix << " host_chunk_support "
                      << (hpx::get_config_entry(
                              "hpx.parcel.conduit_host_chunks", "") != "" ?
                                 1 :
                                 0)
                      << "\n";

            // a line per size as soon as it is measured, for the tool's progress
            options.progress = [&child_prefix](
                                   tuning::transfer_point const& p) {
                std::cout << child_prefix << " progress " << p.size << " "
                          << p.get_us.median << " " << p.set_us.median << "\n"
                          << std::flush;
            };
            std::vector<tuning::transfer_point> const points =
                tuning::measure_transfers(remotes.front(), options);

            auto const write = [](std::ostream& os, tuning::summary const& s) {
                os << " " << s.count << " " << s.min << " " << s.q1 << " "
                   << s.median << " " << s.q3 << " " << s.p90 << " " << s.max;
            };
            for (tuning::transfer_point const& p : points)
            {
                std::ostringstream os;
                os << std::setprecision(9) << child_prefix << " point "
                   << p.size;
                write(os, p.get_us);
                write(os, p.set_us);
                os << " " << p.conduit_chunks_per_round << " " << p.errors;
                std::cout << os.str() << "\n";
            }
            for (auto const& [key, value] : hpx_tune_detail::host_entries())
                std::cout << child_prefix << " here " << key << " " << value
                          << "\n";
            for (auto const& [key, value] :
                hpx_tune_host_entries_action()(remotes.front()))
            {
                std::cout << child_prefix << " peer " << key << " " << value
                          << "\n";
            }
            std::cout << child_prefix << " end\n" << std::flush;
            return hpx::finalize();
        }

    }    // namespace

    int run_as_child(int argc, char* argv[])
    {
        po::options_description desc("hpx_tune child");
        desc.add(parcel_options());
        hpx::init_params params;
        params.desc_cmdline = desc;
        return hpx::init(child_main, argc, argv, params);
    }
}    // namespace hpx_tune

// after the idle area (whose measurements the children would disturb) and
// the chunk area (the shorter one)
HPX_TUNE_REGISTER_AREA(parcel,
    hpx_tune::area{"parcel", "tune the parcel layer's settings", 20,
        &hpx_tune::parcel_options, &hpx_tune::tune_parcel,
        &hpx_tune::run_as_child})

#endif
