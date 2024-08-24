import sys
import json
import matplotlib.pyplot as plt
import scipy
import scipy.stats
import numpy as np
import seaborn as sns
from math import ceil

sns.set_style("ticks",{'axes.grid' : True})

def classify(lower, upper):
    if upper - lower > 0.1:
        return '??'
        
    if -0.01 <= lower <= 0 <= upper <= 0.01:
        return '='
    if -0.02 <= lower <= upper <= 0.02:
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

rng = np.random.default_rng()

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
    category = []
    samples = []
    
    header_flag = True
    
    i = 0
    for test1, test2 in zip(json_obj1["outputs"], json_obj2["outputs"]):
        if test1["name"] == test2["name"]:
            flag = True
            category.append("baseline")
            test_names.append(test2["name"] + " (baseline),\n" + test2["executor"])
            samples.append(test2["series"])
            test_names.append(test1["name"] + ",\n" + test1["executor"])
            category.append("current")
            samples.append(test1["series"])
            
            data = (test2["series"] / np.median(test1["series"]), test1["series"] / np.median(test1["series"]))
            res = scipy.stats.bootstrap(data, median_statistic, method='basic', random_state=rng)
            
            median2 = np.median(test2["series"])
            median1 = np.median(test1["series"])
                
            plt.figure(figsize=(8, 4))
                
            sns.kdeplot(test2["series"], fill=True, label='baseline')
            sns.kdeplot(test1["series"], fill=True, label='current')
            plt.axvline(median2, label='baseline median', color='k')
            plt.axvline(median1, label='current median', color='g')
            plt.legend()
            plt.suptitle(f'{test1["name"]}, \n{test1["executor"]}')
            
            plt.tight_layout() 
            plt.savefig(f"{sys.argv[3]}_{i}.png")
                
            percentage_diff = ((median2 - median1) / median2) * 100
            
            lower, upper = res.confidence_interval
            
            if ('=' not in classify(lower, upper)):
                if header_flag:
                    html_file.writelines("<tr><th scope=\"row\" colspan=\"4\">{}</th></tr>".format(sys.argv[3].split('/')[-1]))
                    header_flag = False
                if flag:
                    html_file.writelines("<tr><th>{}</th>".format(test1["name"]))
                    flag = False
                html_file.writelines("<td>{}</td>".format(test1["executor"].replace('<', '&lt;').replace('>', '&gt;')))
                html_file.writelines("<td>{:.2f} %</td>".format(percentage_diff))
                html_file.writelines("<td>{}</td>".format(classify(lower, upper)))
                if not flag:
                    html_file.writelines("</tr>")
        else:
            print("Tests are not the same")
            exit(1)
        i += 1
    
    html_file.close()
    