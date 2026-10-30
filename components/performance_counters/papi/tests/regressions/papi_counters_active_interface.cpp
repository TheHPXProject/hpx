//  Copyright (c) 2013 Maciej Brodowicz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx_start.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/program_options.hpp>

#include <papi.h>

///////////////////////////////////////////////////////////////////////////////
char const* counter_name = "/papi{locality#0/worker-thread#0}/PAPI_SR_INS";
size_t const nstores = 1000000;

///////////////////////////////////////////////////////////////////////////////
inline bool close_enough(double m, double ex, double perc)
{
    return 100.0 * fabs(m - ex) / ex <= perc;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map&)
{
    hpx::start_active_counters();

    // perform n stores, active counter
    size_t volatile i;
    for (i = 0; i < nstores; i = i + 1)
        ;

    hpx::evaluate_active_counters();
    // stop the counter w/o resetting
    hpx::stop_active_counters();

    // perform n stores (should be uncounted)
    for (i = 0; i < nstores; i = i + 1)
        ;
    // get value and reset, and start again
    hpx::evaluate_active_counters(true);
    hpx::start_active_counters();

    // perform 2*n stores, counted from 0 (or close to it)
    for (i = 0; i < 2 * nstores; i = i + 1)
        ;
    hpx::evaluate_active_counters();

    // perform another n stores, counted from 0 (or close to it)
    hpx::reset_active_counters();
    for (i = 0; i < nstores; i = i + 1)
        ;
    hpx::evaluate_active_counters();

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int check_(int fd)
{
    std::string out;
    std::vector<double> cnt;
    while (true)
    {
        char buf[1024];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0)
        {
            out += std::string(buf, buf + n);
            size_t pos = out.find("invalid");
            if (pos != out.npos)
                return 0;

            pos = out.find(counter_name);
            if (pos != out.npos)
            {    // counter name found
                out.erase(0, pos);
                pos = out.find('\n');
                if (pos != out.npos)
                {    // this is complete line; extract counter value
                    size_t cpos = out.rfind(',', pos);
                    std::cerr << out.substr(0, pos + 1);
                    std::string value = out.substr(cpos + 1, pos - cpos - 1);
                    if (value == "invalid")
                    {
                        cnt.push_back(-1.0);
                    }
                    else
                    {
                        cnt.push_back(hpx::util::from_string<double>(value));
                    }
                    if (cnt.size() == 5)
                        break;
                    out.erase(0, pos + 1);
                }
            }
        }
        else
            throw std::runtime_error(
                "truncated input; didn't get all counter values");
    }
    // bail out if perf counter isn't available ...
    if (close_enough(cnt[0], -1.0, 5.0))
        return 0;

    // since printing affects the counts, the relative error bounds need to be
    // increased compared to the "basic_functions" test
    bool pass = close_enough(cnt[0], nstores, 5.0) && (cnt[1] >= cnt[0]) &&
        close_enough(cnt[0], cnt[1], 5.0) &&
        close_enough(cnt[2], 2.0 * cnt[0], 5.0) &&
        close_enough(cnt[3], cnt[0], 5.0);

    std::cerr << (pass ? "PASSED" : "FAILED") << ".\n";

    return pass ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
    {
        std::cerr << "PAPI library initialization failed\n";
        return 1;
    }
    int const event_status = PAPI_query_event(PAPI_SR_INS);
    if (event_status == PAPI_ENOEVNT)
    {
        std::cout << "SKIP: PAPI_SR_INS is unavailable on this machine\n";
        return 0;
    }
    if (event_status != PAPI_OK)
    {
        std::cerr << "PAPI_SR_INS query failed: " << PAPI_strerror(event_status)
                  << '\n';
        return 1;
    }

    // Prepare to grab the output stream.
    int pipefd[2];
    if (pipe(pipefd) != 0 || dup2(pipefd[1], STDOUT_FILENO) < 0)
        throw std::runtime_error("could not create pipe to stdout");

    // Add the required counter command line option.
    char** opt = new char*[argc + 2];
    for (int i = 0; i < argc; i++)
        opt[i] = argv[i];
    char copt[256];
    snprintf(copt, 256, "--hpx:print-counter=%s", counter_name);
    opt[argc] = copt;
    opt[argc + 1] = nullptr;

    // Run test in HPX domain.
    hpx::start(argc + 1, opt);

    // Collect and process the output.
    int rc = check_(pipefd[0]);

    hpx::stop();
    return rc;
}
