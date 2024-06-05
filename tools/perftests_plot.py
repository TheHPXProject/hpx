import sys
import subprocess
import json
import matplotlib.pyplot as plt

if len(sys.argv) != 4:
    print("Usage: python perftests_plot.py [path_to_first_result.json] [path_to_second_result.json] [perftest_name]")
else:
    f1 = open(sys.argv[1], 'r')
    f2 = open(sys.argv[2], 'r')
    json_obj1 = json.loads(f1.read())
    json_obj2 = json.loads(f2.read())

    test_names = []
    samples = []

    for test in json_obj["outputs"]:
        test_names.append(test["name"] + ",\n" + test["executor"])
        samples.append(test["series"])

    fig = plt.figure(figsize=(20, 6))
    ax = fig.add_subplot()
    bp = ax.boxplot(samples, showfliers=False)
    plt.setp(ax.set_xticklabels(test_names), fontsize=7)
    plt.ylabel("Execution time")
    plt.savefig(sys.argv[3] + ".jpg")
    