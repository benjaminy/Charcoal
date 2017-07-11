from datautil import *
from pprint import PrettyPrinter
import math 
def main(data):
    log(list(data.keys()), tag = "Files processed")
    
    cur_dir = ""
    for filepath, cluster_list in data.items():
        if(filepath[0] != cur_dir): 
            log(filepath[0])
            cur_dir = filepath[0]
            
        log(filepath[1], indent = 2)
        num_clusters = len(cluster_list)
        if not num_clusters: 
            log(None, indent = 3)
            print("\n")
            continue 
        
        pp = PrettyPrinter(indent = 2)
        #pp.pprint(cluster_list)
        
        log(len(cluster_list), indent = 4, tag = "Clusters Found")
        sizes, durations = maps(cluster_list, len, clusterDurations)  
        
        printStats(sorted(sizes), "Cluster Sizes")
        printStats(sorted(durations), "Cluster Durations")
        print("\n")
    
def maps(vals, *funcs):
    results = []
    for f in funcs:
        results.append([f(val) for val in vals])
            
    return tuple(results)

def printStats(vals, label, i = 4):
    if label: log(label, indent = i - 1)
    def _log(vals, t): log(vals, indent = i, tag = t)
    _log(range(vals), "Range")
    _log(avg(vals), "Average")
    _log(stdev(vals),"Standard Deviation")
    
def range(values):
    return values[-1] - values[0]       
 
def clusterDurations(clusters):
    return float(clusters[-1]["end time"]) - float(clusters[0]["start time"])

def avg(values):
    return sum(values) / len(values)

def stdev(values):
    total = sum(values)
    size = len(values)
    avg = float(total / size)
    
    accum = 0
    for val in values:
        dev = (val - avg)
        accum += dev*dev
        
    result = math.sqrt(accum / size)
    return result

if __name__ == "__main__":
    main(None)