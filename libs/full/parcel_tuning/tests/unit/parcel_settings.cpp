//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The rules of the parcel area with the measurements of the parcelport study
// (chapter 17, result 5: two V100 nodes over ConnectX-3, a std::vector<int>
// as argument and result of an action, medians in ms): with the UCX conduit
// moving the host chunks the TCP parcelport is faster from 1 MiB on, the MPI
// parcelport gains nothing. The rules must keep host chunks on for TCP, turn
// them off for MPI and choose a threshold of 1 MiB.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcel_tuning.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace tuning = hpx::tuning;

namespace {

    constexpr std::size_t mib = std::size_t(1) << 20;

    tuning::summary exactly(double const us)
    {
        tuning::summary s;
        s.count = 50;
        s.min = s.q1 = s.median = s.q3 = s.p90 = s.max = us;
        return s;
    }

    tuning::transfer_point point(std::size_t const size, double const get_ms,
        double const set_ms, double const conduit_chunks)
    {
        tuning::transfer_point p;
        p.size = size;
        p.get_us = exactly(get_ms * 1000.0);
        p.set_us = exactly(set_ms * 1000.0);
        p.conduit_chunks_per_round = conduit_chunks;
        return p;
    }

    // chapter 17, result 5
    tuning::parcelport_measurements tcp()
    {
        return {"tcp",
            {point(mib, 0.73, 0.75, 0), point(4 * mib, 2.83, 2.79, 0),
                point(16 * mib, 13.4, 12.4, 0), point(64 * mib, 71.2, 71.6, 0)},
            {point(mib, 0.53, 0.71, 1), point(4 * mib, 1.87, 2.44, 1),
                point(16 * mib, 9.61, 10.6, 1),
                point(64 * mib, 61.0, 66.7, 1)}};
    }

    tuning::parcelport_measurements mpi()
    {
        return {"mpi",
            {point(mib, 0.49, 0.62, 0), point(4 * mib, 1.97, 2.58, 0),
                point(16 * mib, 9.56, 11.0, 0), point(64 * mib, 58.4, 66.1, 0)},
            {point(mib, 0.49, 0.64, 1), point(4 * mib, 2.05, 2.44, 1),
                point(16 * mib, 9.63, 11.1, 1),
                point(64 * mib, 61.1, 69.1, 1)}};
    }

    tuning::tuned_setting const* find(
        tuning::parcel_rules_result const& r, std::string const& key)
    {
        auto const it = std::find_if(r.settings.begin(), r.settings.end(),
            [&](tuning::tuned_setting const& s) { return s.key == key; });
        return it == r.settings.end() ? nullptr : &*it;
    }

    bool reports(tuning::parcel_rules_result const& r, std::string const& text)
    {
        return std::any_of(
            r.report.begin(), r.report.end(), [&](std::string const& line) {
                return line.find(text) != std::string::npos;
            });
    }

    void test_faster_from()
    {
        tuning::parcelport_measurements const t = tcp();
        HPX_TEST_EQ(tuning::faster_from(t.conduit, t.plain, 0.05), mib);
        HPX_TEST_EQ(
            tuning::faster_from(t.plain, t.conduit, 0.05), std::size_t(0));

        // faster at the large sizes only
        tuning::parcelport_measurements const m = mpi();
        std::vector<tuning::transfer_point> later = m.plain;
        later[2] = point(16 * mib, 8.0, 9.0, 1);
        later[3] = point(64 * mib, 50.0, 55.0, 1);
        HPX_TEST_EQ(tuning::faster_from(later, m.plain, 0.05), 16 * mib);

        // faster at a smaller size again doesn't help: the size in between
        // that isn't faster (4 MiB) ends the range
        later[0] = point(mib, 0.3, 0.3, 1);
        HPX_TEST_EQ(tuning::faster_from(later, m.plain, 0.05), 16 * mib);

        // overlapping quartiles are not faster
        std::vector<tuning::transfer_point> noisy = t.conduit;
        for (auto& p : noisy)
        {
            p.get_us.q3 = p.get_us.median * 2.0;
            p.set_us.q3 = p.set_us.median * 2.0;
        }
        HPX_TEST_EQ(tuning::faster_from(noisy, t.plain, 0.05), std::size_t(0));
    }

    void test_v100()
    {
        tuning::parcel_rules_result const r =
            tuning::derive_parcel_settings({tcp(), mpi()});

        tuning::tuned_setting const* t =
            find(r, "hpx.parcel.tcp.conduit_host_chunks");
        HPX_TEST(t != nullptr);
        if (t != nullptr)
        {
            HPX_TEST_EQ(t->value, std::string("1"));
            HPX_TEST(t->is_default);
        }

        tuning::tuned_setting const* m =
            find(r, "hpx.parcel.mpi.conduit_host_chunks");
        HPX_TEST(m != nullptr);
        if (m != nullptr)
        {
            HPX_TEST_EQ(m->value, std::string("0"));
            HPX_TEST(!m->is_default);
        }

        tuning::tuned_setting const* threshold =
            find(r, "hpx.parcel.conduit_host_threshold");
        HPX_TEST(threshold != nullptr);
        if (threshold != nullptr)
        {
            HPX_TEST_EQ(threshold->value, std::to_string(mib));
            HPX_TEST(threshold->is_default);
        }

        // rule 1 only reports: no priority is written
        HPX_TEST(find(r, "hpx.parcel.tcp.priority") == nullptr);
        HPX_TEST(find(r, "hpx.parcel.mpi.priority") == nullptr);
        HPX_TEST(reports(r, "rule 1 (reported only)"));
        HPX_TEST(reports(r, "1 MiB mpi"));
        HPX_TEST(reports(r, "4 MiB to 16 MiB tcp with the conduit"));
    }

    void test_conduit_unused()
    {
        // the conduit's curve didn't go through the conduit (HPX without
        // host chunks through it, or the destination not reachable): nothing
        // is written for that parcelport
        tuning::parcelport_measurements t = tcp();
        for (auto& p : t.conduit)
        {
            p.conduit_chunks_per_round = 0;
        }
        tuning::parcelport_measurements m = mpi();
        for (auto& p : m.conduit)
        {
            p.conduit_chunks_per_round = 0;
        }

        tuning::parcel_rules_result const r =
            tuning::derive_parcel_settings({t, m});
        HPX_TEST(r.settings.empty());
        HPX_TEST(reports(r, "rule 2 (tcp): faster with the conduit, but"));
        HPX_TEST(reports(r, "rule 2 (mpi): the conduit didn't move"));
        HPX_TEST(reports(r, "rule 3: no parcelport"));
    }

    void test_missing_and_wrong()
    {
        tuning::parcelport_measurements t = tcp();
        t.conduit.clear();
        tuning::parcelport_measurements m = mpi();
        m.plain[0].errors = 1;

        tuning::parcel_rules_result const r =
            tuning::derive_parcel_settings({t, m});
        HPX_TEST(r.settings.empty());
        HPX_TEST(reports(r, "rule 2 (tcp): no measurement with the conduit"));
        HPX_TEST(reports(r, "rule 2 (mpi): data arrived wrong"));
    }

    void test_unstable()
    {
        // TCP faster from 1 MiB, but the passes disagreed at 16 MiB: no
        // decision for TCP; MPI's decision (at 64 MiB) isn't touched by an
        // unstable size below it
        tuning::parcelport_measurements t = tcp();
        t.unstable = {16 * mib};
        tuning::parcelport_measurements m = mpi();
        m.unstable = {4 * mib};

        tuning::parcel_rules_result const r =
            tuning::derive_parcel_settings({t, m});
        HPX_TEST(find(r, "hpx.parcel.tcp.conduit_host_chunks") == nullptr);
        HPX_TEST(reports(r, "rule 2 (tcp): the passes disagree at 16 MiB"));
        HPX_TEST(find(r, "hpx.parcel.mpi.conduit_host_chunks") != nullptr);
        HPX_TEST(find(r, "hpx.parcel.conduit_host_threshold") == nullptr);

        // the largest size unstable: no decision either way
        tuning::parcelport_measurements t2 = tcp();
        t2.conduit.pop_back();
        t2.plain.pop_back();
        t2.unstable = {64 * mib};
        tuning::parcel_rules_result const r2 =
            tuning::derive_parcel_settings({t2});
        HPX_TEST(find(r2, "hpx.parcel.tcp.conduit_host_chunks") == nullptr);
    }

    void test_threshold_at_least_zero_copy()
    {
        // faster from the smallest size measured, below the zero-copy
        // threshold: the threshold is the zero-copy threshold
        tuning::parcelport_measurements t;
        t.parcelport = "tcp";
        t.plain = {point(4096, 0.1, 0.1, 0), point(8192, 0.2, 0.2, 0),
            point(65536, 0.5, 0.5, 0)};
        t.conduit = {point(4096, 0.05, 0.05, 1), point(8192, 0.1, 0.1, 1),
            point(65536, 0.2, 0.2, 1)};

        tuning::parcel_rules_result const r =
            tuning::derive_parcel_settings({t});
        tuning::tuned_setting const* threshold =
            find(r, "hpx.parcel.conduit_host_threshold");
        HPX_TEST(threshold != nullptr);
        if (threshold != nullptr)
        {
            HPX_TEST_EQ(threshold->value, std::string("8192"));
            HPX_TEST(!threshold->is_default);
        }
    }

    void test_sections()
    {
        std::vector<tuning::ini_section> const sections =
            tuning::to_sections(std::vector<tuning::tuned_setting>{
                {"hpx.parcel.tcp.conduit_host_chunks", "1", "a", true},
                {"hpx.parcel.conduit_host_threshold", "1048576", "b", true},
                {"hpx.parcel.mpi.conduit_host_chunks", "0", "c", false},
                {"hpx.parcel.tcp.zero_copy_serialization_threshold", "8192",
                    "d", true}});
        HPX_TEST_EQ(sections.size(), std::size_t(3));
        if (sections.size() == 3)
        {
            HPX_TEST_EQ(sections[0].name, std::string("hpx.parcel.tcp"));
            HPX_TEST_EQ(sections[0].settings.size(), std::size_t(2));
            HPX_TEST_EQ(sections[1].name, std::string("hpx.parcel"));
            HPX_TEST_EQ(sections[2].name, std::string("hpx.parcel.mpi"));
        }
    }

    void test_combine_passes()
    {
        tuning::parcelport_measurements const t = tcp();

        // one pass: as it is
        tuning::combined_curve const one = tuning::combine_passes({t.plain});
        HPX_TEST_EQ(one.points.size(), t.plain.size());
        HPX_TEST(one.unstable.empty());

        // three passes that agree within 10%: the median of each value
        std::vector<tuning::transfer_point> second = t.plain;
        std::vector<tuning::transfer_point> third = t.plain;
        for (auto& p : second)
        {
            p.get_us = exactly(p.get_us.median * 1.04);
            p.set_us = exactly(p.set_us.median * 1.04);
        }
        for (auto& p : third)
        {
            p.get_us = exactly(p.get_us.median * 0.98);
            p.set_us = exactly(p.set_us.median * 0.98);
        }
        tuning::combined_curve const three =
            tuning::combine_passes({t.plain, second, third});
        HPX_TEST_EQ(three.points.size(), t.plain.size());
        HPX_TEST(three.unstable.empty());
        if (!three.points.empty())
        {
            HPX_TEST_EQ(
                three.points[0].get_us.median, t.plain[0].get_us.median);
            HPX_TEST_EQ(three.points[0].get_us.count, std::size_t(150));
        }

        // a pass that disagrees at one size by more than 10%: that size is
        // left out and listed
        second[1].get_us = exactly(second[1].get_us.median * 1.5);
        tuning::combined_curve const disagree =
            tuning::combine_passes({t.plain, second, third});
        HPX_TEST_EQ(disagree.points.size(), t.plain.size() - 1);
        HPX_TEST_EQ(disagree.unstable.size(), std::size_t(1));
        if (!disagree.unstable.empty())
        {
            HPX_TEST_EQ(disagree.unstable[0], 4 * mib);
        }

        // a size missing from one pass is left out, not unstable
        std::vector<tuning::transfer_point> const shorter(
            t.plain.begin(), t.plain.end() - 1);
        tuning::combined_curve const missing =
            tuning::combine_passes({t.plain, shorter});
        HPX_TEST_EQ(missing.points.size(), t.plain.size() - 1);
        HPX_TEST(missing.unstable.empty());

        // wrong data in any pass stays visible
        std::vector<tuning::transfer_point> wrong = t.plain;
        wrong[0].errors = 2;
        tuning::combined_curve const with_errors =
            tuning::combine_passes({t.plain, wrong});
        HPX_TEST(!with_errors.points.empty() &&
            with_errors.points[0].errors == std::size_t(2));
    }
}    // namespace

int main()
{
    test_faster_from();
    test_combine_passes();
    test_v100();
    test_conduit_unused();
    test_missing_and_wrong();
    test_threshold_at_least_zero_copy();
    test_unstable();
    test_sections();
    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif
