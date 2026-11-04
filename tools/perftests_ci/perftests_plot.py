#!/usr/bin/env python3
#
# Copyright (c) 2024 Vedant Nimje
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import io
import os
import sys
import json
import matplotlib.pyplot as plt
from PIL import Image
import scipy
import scipy.stats
import numpy as np
import seaborn as sns

sns.set_style("ticks",{'axes.grid' : True})

# Report, and comment on a pull request about, only the measurements that got
# more than 5% faster or slower (or have no reference). Set this to True to
# report every measurement, and comment on every pull request, while testing
# the CI reports. A build triggered by the pull request comment "retest lsu perf
# full" reports every measurement as well (HPX_PERFTESTS_REPORT_ALL, set by
# .jenkins/lsu-perftests/entry.sh).
report_unchanged = False
if os.environ.get("HPX_PERFTESTS_REPORT_ALL", "") not in ("", "0"):
    report_unchanged = True

def classify(lower, upper):

    if upper < lower:
        return classify(upper, lower)

    # large uncertainty
    if upper - lower > 0.1:
        return '??'
        
    if -0.01 <= lower <= 0 <= upper <= 0.01:
        return '='
    if -0.02 <= lower <= 0 <= upper <= 0.02:
        return '(=)'

    # probably no change, but quite large uncertainty
    if -0.05 <= lower <= 0 <= upper <= 0.05:
        return '?'

    # faster
    if -0.01 <= lower <= 0.0:
        return '(+)'
    if -0.05 <= lower <= -0.01:
        return '+'
    if -0.1 <= lower <= -0.05:
        return '++'
    if lower <= -0.1:
        return '+++'

    # slower
    if 0.01 >= upper >= 0.0:
        return '(-)'
    if 0.05 >= upper >= 0.01:
        return '-'
    if 0.1 >= upper >= 0.05:
        return '--'
    if upper >= 0.1:
        return '---'

    # no idea
    return '???'

def median_statistic(sample1, sample2, axis=-1):
    median1 = np.median(sample1, axis=axis)
    median2 = np.median(sample2, axis=axis)
    return (median1 - median2)

def execution_medians(test):
    # The median of the timings of each execution of the benchmark. The timings
    # of one execution depend on each other, most of the variation is between
    # the executions, so the confidence interval of the comparison is computed
    # from these medians; bootstrapping all timings as if they were independent
    # makes it far too narrow, and noise shows up as a change. A result without
    # the number of timings per execution (recorded by the perftest driver)
    # counts every timing as an execution of its own.
    series = np.asarray(test["series"], dtype=float)
    sizes = test.get("executions")
    if not sizes or sum(sizes) != len(series):
        return series
    bounds = np.cumsum(sizes)[:-1]
    return np.array([np.median(part) for part in np.split(series, bounds)
                     if len(part) > 0])

def time_unit(seconds):
    # the unit that shows the given time with one to three integer digits
    for unit, factor in (("s", 1), ("ms", 1e3), ("\u00b5s", 1e6)):
        if seconds * factor >= 1:
            return unit, factor
    return "ns", 1e9

def save_png(fig, path):
    # The plots use only a few colors, a 64 color palette keeps them
    # visually unchanged at about a third of the size of an RGBA png. The
    # pull request comment shows them 500 pixels wide, so the figures are
    # small and saved at a higher resolution to keep the text readable there
    buffer = io.BytesIO()
    fig.savefig(buffer, format="png", bbox_inches="tight", dpi=160)
    plt.close(fig)
    buffer.seek(0)
    # Pillow 9.1 added Image.Quantize and deprecated the module constants
    quantize = getattr(Image, "Quantize", Image)
    with Image.open(buffer) as image:
        image.convert("RGB").quantize(
            colors=64, method=quantize.MEDIANCUT).save(
                path, optimize=True)

def write_info(result_path, reference_path, curr_path):
    # Write what the results and the reference were measured with (as added by
    # the perftest driver) into info.html next to the report. References that
    # weren't recorded by the driver show "unknown". The commit isn't shown:
    # a pull request is built merged into master, a commit that exists only on
    # the CI node; the report names the commit of the pull request instead.
    def load(path):
        try:
            with open(path, 'r') as f:
                return json.loads(f.read())
        except (OSError, ValueError):
            return {}

    current, reference = load(result_path), load(reference_path)

    def value(data, section, key):
        text = str(data.get(section, {}).get(key) or "unknown")
        return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

    rows = (("Run date", "environment", "datetime"),
            ("Host", "environment", "hostname"),
            ("Compiler", "environment", "compiler"))
    with open(f'{curr_path}/info.html', "w") as info_file:
        info_file.write("<table><tr><th>Property</th><th>Reference</th><th>Current</th></tr>")
        for name, section, key in rows:
            info_file.write("<tr><td>{}</td><td>{}</td><td>{}</td></tr>".format(
                name, value(reference, section, key), value(current, section, key)))
        info_file.write("</table>")

def plot_report(result_path, reference_path, output_prefix):
    # Compare one benchmark with its reference: append its rows to the
    # index.html next to output_prefix and write the plots as
    # <output_prefix>_<i>.png. Returns whether the comparison could be made.
    # Without a reference (reference_path is None), the plots show only the
    # current results, which can become the reference of this node.
    with open(result_path, 'r') as f1:
        json_obj1 = json.loads(f1.read())
    json_obj2 = None
    if reference_path is not None:
        with open(reference_path, 'r') as f2:
            json_obj2 = json.loads(f2.read())

    curr_path = '/'.join(output_prefix.split('/')[:-1])
    report_name = output_prefix.split('/')[-1]

    # zip(strict=True) needs Python 3.10, the CI runs 3.9
    if json_obj2 is not None and \
            len(json_obj1["outputs"]) != len(json_obj2["outputs"]):
        print(f"Tests are not the same: {report_name}")
        return False

    references = json_obj2["outputs"] if json_obj2 is not None else \
        [None] * len(json_obj1["outputs"])

    with open(f'{curr_path}/index.html', "a+") as html_file, \
            open(f'{curr_path}/changed.txt', "a+") as changed_file:
        header_flag = True
        i = 0
        for test1, test2 in zip(json_obj1["outputs"], references):
            if test2 is not None and test1["name"] != test2["name"]:
                print(f"Tests are not the same: {report_name}")
                return False
            flag = True

            median1 = np.median(test1["series"])
            median2 = np.median(test2["series"]) if test2 is not None else None

            fig, ax = plt.subplots(figsize=(5, 2.6))

            # A few outliers would stretch the axis (and the density estimate)
            # so far that both distributions end up in a single bar, so plot
            # only the central part of each series: without the lowest and
            # highest 1%, and without the values further than 3 interquartile
            # ranges from the quartiles (more than 1% of the values of a
            # series with stalls, such as the latency reports). The legend
            # says how many values are left out.
            unit, factor = time_unit(median2 if test2 is not None else median1)

            def central(series, label):
                series = np.asarray(series)
                low, q1, q3, high = np.percentile(series, [1, 25, 75, 99])
                low = max(low, q1 - 3 * (q3 - q1))
                high = min(high, q3 + 3 * (q3 - q1))
                values = series[(series >= low) & (series <= high)]
                # a series of two values has both outside the percentiles
                if len(values) == 0:
                    values = series
                values = values * factor
                outside = len(series) - len(values)
                if outside:
                    label = f"{label} ({outside} of {len(series)} not shown)"
                return values, label

            current, label = central(test1["series"], "current")
            plotted = [(current, "C1", label)]
            if test2 is not None:
                baseline, label = central(test2["series"], "baseline")
                plotted.insert(0, (baseline, "C0", label))
            # Common bins as narrow as the narrower of the bins each series
            # would get on its own, so that a series isn't a single bar if it
            # lies far from the other one. A constant series gets bins of
            # width 1 (numpy widens its range to that), so only the other
            # series count; if all are constant, the bins span all values.
            low = min(values.min() for values, _, _ in plotted)
            high = max(values.max() for values, _, _ in plotted)
            widths = [np.diff(np.histogram_bin_edges(values, "auto"))[0]
                for values, _, _ in plotted if values.max() > values.min()]
            if high > low and widths:
                count = int(np.ceil((high - low) / min(widths)))
                bins = np.linspace(low, high, max(1, min(count, 500)) + 1)
            else:
                bins = np.histogram_bin_edges(
                    np.concatenate([values for values, _, _ in plotted]), "auto")
            # densities, as the baseline and the current run may have a
            # different number of values; the density estimate covers only
            # the plotted values (cut=0), instead of extending beyond them
            for values, color, label in plotted:
                sns.histplot(values, bins=bins, stat="density", kde=True,
                    kde_kws={"cut": 0}, color=color, label=label, ax=ax)
            if test2 is not None:
                ax.axvline(median2 * factor, label='baseline median', color='k')
            ax.axvline(median1 * factor, label='current median', color='g')
            ax.set_xlim(bins[0], bins[-1])
            ax.set_xlabel(f"time [{unit}]")
            ax.legend(fontsize=7)
            ax.set_title(f'{test1["name"]}\n{test1["executor"]}', fontsize=9)

            plot_name = f"{report_name}_{i}.png"
            save_png(fig, f"{output_prefix}_{i}.png")

            if test2 is not None:
                # classify expects the change of the current time relative
                # to the baseline, so a positive interval means slower
                data = (execution_medians(test1) / median2,
                        execution_medians(test2) / median2)
                # The percentile interval, the default BCa method needs a
                # jackknife over all values, which takes minutes for a result
                # without the timings per execution
                res = scipy.stats.bootstrap(data, median_statistic,
                    n_resamples=1000, method="percentile")
                lower, upper = res.confidence_interval
                classification = classify(lower, upper)
                percentage = "{:.2f} %".format(
                    ((median2 - median1) / median2) * 100)
            else:
                classification = "no reference"
                percentage = "n/a"

            # Only a measurement that got more than 5% faster or slower (++,
            # +++, --, ---), or has no reference, is reported and listed in
            # changed.txt, entry.sh comments on the pull request if there is
            # any. Smaller changes ((+), +, (-), -) are mostly noise, and
            # uncertain results (?, ??, ???) aren't reported either.
            reported = report_unchanged or test2 is None or \
                '++' in classification or '--' in classification
            if reported:
                changed_file.write("{} {} {}\n".format(
                    test1["name"], test1["executor"], classification))
                if header_flag:
                    html_file.writelines("<tr><th scope=\"row\" colspan=\"4\">{}</th></tr>".format(report_name))
                    header_flag = False
                if flag:
                    html_file.writelines("<tr><th>{}</th>".format(test1["name"]))
                    flag = False
                html_file.writelines("<td>{}</td>".format(test1["executor"].replace('<', '&lt;').replace('>', '&gt;')))
                html_file.writelines("<td>{}</td>".format(percentage))
                html_file.writelines("<td>{}</td>".format(classification))
                if not flag:
                    html_file.writelines("</tr>")
                # The plot gets a row of its own, as GitHub shrinks a table
                # column with an image to the room left by the other columns,
                # and is collapsed to keep the report short. The plot is
                # relative to the report, comment_github.sh points it to the
                # uploaded copy
                html_file.writelines("<tr><td colspan=\"4\"><details><summary>Plot</summary><img src=\"{}\" width=\"500\"></details></td></tr>".format(plot_name))
            i += 1

    return True


def same_measurements(result_path, reference_path):
    # Whether a result has the same measurements (names and executors, in the
    # same order) as its reference
    def measurements(path):
        with open(path, 'r') as f:
            return [(output["name"], output.get("executor"))
                    for output in json.load(f)["outputs"]]
    return measurements(result_path) == measurements(reference_path)


if len(sys.argv) < 4 or (len(sys.argv) - 1) % 3 != 0:
    print("Usage: python perftests_plot.py [path_to_result.json] [path_to_reference.json] [perftest_name] ...")
    exit(1)

# All reports are plotted in one process, so that the imports above, which
# take a few seconds, are paid once. A benchmark without a result (because it
# wasn't run) is skipped, a failed comparison doesn't stop the others.
failed = False
info_written = False
args = sys.argv[1:]
for k in range(0, len(args), 3):
    result_path, reference_path, output_prefix = args[k:k + 3]
    if not os.path.exists(result_path):
        print(f"No result, skipping: {result_path}")
        continue
    curr_path = '/'.join(output_prefix.split('/')[:-1])
    # The host and the compiler are the same for all results of a run, the run
    # date is the one of the first result
    if not info_written:
        write_info(result_path, reference_path, curr_path)
        info_written = True
    # The references are kept per node, the node this ran on may not have
    # any yet. This is reported (and commented on), but is not a failure: the
    # results are reported and plotted on their own, and are listed in
    # no_reference.txt, so that batch.sh makes new references of that node
    # from this run. A benchmark whose measurements changed (one was added,
    # removed or renamed) has no reference for them either.
    if not os.path.exists(reference_path) or \
            not same_measurements(result_path, reference_path):
        if os.path.exists(reference_path):
            print(f"The measurements differ from the reference: {reference_path}")
        else:
            print(f"No reference: {reference_path}")
        plot_report(result_path, None, output_prefix)
        with open(f'{curr_path}/no_reference.txt', "a+") as no_reference_file:
            no_reference_file.write(
                "{}\n".format(os.path.basename(result_path)))
        continue
    if not plot_report(result_path, reference_path, output_prefix):
        failed = True

exit(1 if failed else 0)
