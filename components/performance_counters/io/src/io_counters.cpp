//  Copyright (c) 2015 Maciej Brodowicz
//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/components/performance_counters/io/io_counters.hpp>

#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/object.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_uint.hpp>

#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#if defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

// type to store parser output
BOOST_FUSION_DEFINE_STRUCT((hpx) (performance_counters) (io), proc_io,
    (std::uint64_t, riss)(std::uint64_t, wiss)(std::uint64_t, rsysc)(
        std::uint64_t, wsysc)(std::uint64_t, rstor)(std::uint64_t, wstor)(
        std::uint64_t, wcanc))

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality, We register the module dynamically
// as no executable links against it.
HPX_REGISTER_COMPONENT_MODULE_DYNAMIC()

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters { namespace io {
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    // grammar
    template <typename I>
    struct proc_io_parser : qi::grammar<I, proc_io(), ascii::space_type>
    {
        proc_io_parser()
          : proc_io_parser::base_type(start)
        {
            using qi::lit;

            start %= lit("rchar:") >> uint64_t_ >> lit("wchar:") >> uint64_t_ >>
                lit("syscr:") >> uint64_t_ >> lit("syscw:") >> uint64_t_ >>
                lit("read_bytes:") >> uint64_t_ >> lit("write_bytes:") >>
                uint64_t_ >> lit("cancelled_write_bytes:") >> uint64_t_;
        }

        qi::rule<I, proc_io(), ascii::space_type> start;
        qi::uint_parser<std::uint64_t> uint64_t_;
    };

    ///////////////////////////////////////////////////////////////////////////
    void parse_proc_io(proc_io& pio)
    {
#if defined(HPX_HAVE_UNISTD_H)
        pid_t pid = getpid();
#else
        pid_t pid = 0;
#endif
        std::string fn = hpx::util::format("/proc/{1}/io", pid);
        std::ifstream ins(fn);

        if (!ins.is_open())
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "hpx::performance_counters::io::parse_proc_io",
                hpx::util::format("failed to open /proc/{1}/io", pid));

        typedef boost::spirit::basic_istream_iterator<char> iterator;
        iterator it(ins), end;
        proc_io_parser<iterator> p;

        if (!qi::phrase_parse(it, end, p, ascii::space, pio))
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "hpx::performance_counters::io::parse_proc_io",
                hpx::util::format("failed to parse /proc/{1}/io", pid));
    }

    std::uint64_t get_pio_riss(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.riss;
    }

    std::uint64_t get_pio_wiss(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.wiss;
    }

    std::uint64_t get_pio_rsysc(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.rsysc;
    }

    std::uint64_t get_pio_wsysc(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.wsysc;
    }

    std::uint64_t get_pio_rstor(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.rstor;
    }

    std::uint64_t get_pio_wstor(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.wstor;
    }

    std::uint64_t get_pio_wcanc(bool)
    {
        proc_io pio;
        parse_proc_io(pio);
        return pio.wcanc;
    }

    ///////////////////////////////////////////////////////////////////////////
    void register_counter_types()
    {
        namespace pc = hpx::performance_counters;
        pc::install_counter_type("/runtime/io/read_bytes_issued", &get_pio_riss,
            "number of bytes read by process (aggregate of count "
            "arguments passed to read() call or its analogues)",
            "bytes", pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/write_bytes_issued",
            &get_pio_wiss,
            "number of bytes the process has caused or shall cause to be "
            "written (aggregate of count arguments passed to write() call or "
            "its analogues)",
            "bytes", pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/read_syscalls", &get_pio_rsysc,
            "number of system calls that perform I/O reads", "",
            pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/write_syscalls", &get_pio_wsysc,
            "number of system calls that perform I/O writes", "",
            pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/read_bytes_transferred",
            &get_pio_rstor,
            "number of bytes retrieved from storage by I/O operations", "bytes",
            pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/write_bytes_transferred",
            &get_pio_wstor,
            "number of bytes transferred to storage by I/O operations", "bytes",
            pc::counter_type::monotonically_increasing);
        pc::install_counter_type("/runtime/io/write_bytes_cancelled",
            &get_pio_wcanc,
            "number of bytes accounted by write_bytes_transferred that has not "
            "been ultimately stored due to truncation or deletion",
            "bytes", pc::counter_type::monotonically_increasing);
    }

    bool get_startup(
        hpx::startup_function_type& startup_func, bool& pre_startup)
    {
#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__gnu_linux__)
        startup_func = register_counter_types;
        pre_startup = true;
        return true;
#else
        return false;
#endif
    }
}}}    // namespace hpx::performance_counters::io

// register component's startup function
HPX_REGISTER_STARTUP_MODULE_DYNAMIC(hpx::performance_counters::io::get_startup)
