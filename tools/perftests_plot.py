import sys
import json
import matplotlib.pyplot as plt
import scipy
import scipy.stats
import numpy as np
import os 

if len(sys.argv) != 4:
    print("Usage: python perftests_plot.py [path_to_first_result.json] [path_to_second_result.json] [perftest_name]")
else:
    f1 = open(sys.argv[1], 'r')
    f2 = open(sys.argv[2], 'r')
    
    curr_path = '/'.join(sys.argv[3].split('/')[:-1])
    
    html_file = open(f'{curr_path}/index.html', "a+")
    
    json_obj1 = json.loads(f1.read())
    json_obj2 = json.loads(f2.read())

    test_names = []
    samples = []
    
    header_flag = True

    for test1, test2 in zip(json_obj1["outputs"], json_obj2["outputs"]):
        if test1["name"] == test2["name"]:
            flag = True
            test_names.append(test2["name"] + " (baseline),\n" + test2["executor"])
            samples.append(test2["series"])
            test_names.append(test1["name"] + ",\n" + test1["executor"])
            samples.append(test1["series"])
            ks_stat, pvalue = scipy.stats.ks_2samp(test1["series"], test2["series"])
            
            mean2 = np.mean(test2["series"])
            mean1 = np.mean(test1["series"])
            
            alpha = 1e-7
            percentage_diff = ((mean2 - mean1) / mean2) * 100
            
            if pvalue < alpha:
                if header_flag:
                    html_file.writelines("<h3>{}</h3>".format(sys.argv[3].split('/')[-1]))
                    html_file.writelines("<ol>")
                    header_flag = False
                if flag:
                    html_file.writelines("<li><b>{}</b>".format(test1["name"]))
                    flag = False
                if mean1 < mean2:
                    html_file.writelines(", {}: Performance is better by {:.2f} % (KS-stat: {})</li>".format(test2["executor"].replace('<', '&lt;').replace('>', '&gt;'), percentage_diff, ks_stat))
                else:
                    html_file.writelines(", {}: Performance is worse by {:.2f} % (KS-stat: {})</li>".format(test2["executor"].replace('<', '&lt;').replace('>', '&gt;'), -percentage_diff, ks_stat))
            if not flag:
                html_file.writelines("</li>")
        else:
            print("ERROR")
            exit(1)
            
    if not header_flag:
        html_file.writelines("</ol>")

    fig = plt.figure(figsize=(25, 16))
    ax = fig.add_subplot()
    bp = ax.boxplot(samples, showfliers=False)
    
    html_file.close()
    
    plt.setp(ax.set_xticklabels(test_names), fontsize=7, rotation=30, horizontalalignment='right')
    plt.ylabel("Execution time")
    plt.grid(True)
    plt.savefig(sys.argv[3] + ".png")
    