import sys
import json
import matplotlib.pyplot as plt
import scipy
import scipy.stats
import numpy as np
import seaborn as sns
from math import ceil

sns.set_style("ticks",{'axes.grid' : True})

def mean_statistic(sample1, sample2, axis=-1):
    mean1 = np.mean(sample1, axis=axis)
    mean2 = np.mean(sample2, axis=axis)
    return (mean1 - mean2) / mean1

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
    n = ceil(len(json_obj1["outputs"]) / 2)
    fig, ax = plt.subplots(n, 2, figsize=(16, 3 * n), sharey=False)
    plt.subplots_adjust(hspace=0.3)
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
            
            data = (test2["series"], test1["series"])
            res = scipy.stats.bootstrap(data, mean_statistic, method='basic', random_state=rng)
            
            mean2 = np.mean(test2["series"])
            mean1 = np.mean(test1["series"])
            
            if n != 1:
                curr_plot = ax[i % n, i // n]
            else:
                curr_plot = ax[i]
                
            sns.kdeplot(test2["series"], fill=True, ax=curr_plot, label='baseline')
            sns.kdeplot(test1["series"], fill=True, ax=curr_plot, label='current')
            curr_plot.axvline(mean2, label='baseline mean', color='k')
            curr_plot.axvline(mean1, label='current mean', color='g')
            curr_plot.legend()
            curr_plot.set_title(f'{test1["name"]}, {test1["executor"]}')
                
            percentage_diff = ((mean2 - mean1) / mean2) * 100
            
            lower, upper = res.confidence_interval
            
            if not (-0.02 <= lower <= 0 <= upper <= 0.02 or -0.01 <= lower <= 0.0 or 0.01 >= upper >= 0.0):
                if header_flag:
                    html_file.writelines("<tr><th scope=\"row\" colspan=\"5\">{}</th></tr>".format(sys.argv[3].split('/')[-1]))
                    header_flag = False
                if flag:
                    html_file.writelines("<tr><th>{}</th>".format(test1["name"]))
                    flag = False
                html_file.writelines("<td>{}</td>".format(test1["executor"].replace('<', '&lt;').replace('>', '&gt;')))
                html_file.writelines("<td>{:.2f} %</td>".format(percentage_diff))
                html_file.writelines("<td>{:.5f}</td>".format(abs(res.standard_error/np.mean(res.bootstrap_distribution))))
                if not flag:
                    html_file.writelines("</tr>")
        else:
            print("Tests are not the same")
            exit(1)
        i += 1
    
    html_file.close()

    plt.tight_layout()    
    [fig.delaxes(a) for a in ax.flatten() if not a.has_data()]
    plt.savefig(sys.argv[3] + ".png", dpi=150)
    