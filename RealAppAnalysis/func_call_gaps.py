import tkinter
from datautil import findFile, \
                     toTxt, \
                     parseCmdLnArgs,\
                     filterEvents, \
                     readCSV, \
                     log, \
                     _flaggedRetArg
                     
from csv import DictReader
import sys

_DEFAULT_THRESHOLD = 1000
_DEFAULT_CLUSTER_SIZE = 2

def main(argv):
    opts, args = parseCmdLnArgs(argv, "s:t:", ["cluster-size=", "gap-threshold="])
    
    raw_data = _loadFile(args)
    clusters = findClusters(raw_data, 
                            min_cluster_size = _handleSizeFlag(opts),
                            threshold = _handleThresholdFlag(opts))

    log(len(clusters), indent = 2, tag = "Clusters found")

    return clusters
    
def _handleThresholdFlag(opts):
    threshold = _flaggedRetArg(opts, "-t", "--gap-threshold")
    if threshold: 
        return float(threshold)
    else: 
        return _DEFAULT_THRESHOLD
    
def _handleSizeFlag(opts):
    size = _flaggedRetArg(opts, "-s", "--cluser-size")
    if size: 
        return int(size)
    else:
        return _DEFAULT_CLUSTER_SIZE
    
def _loadFile(args):
    if not args:
        filepath = findFile()
    else:
        filepath = args[0]

    return readCSV(filepath)
        
def findClusters(func_data, 
                 min_cluster_size = _DEFAULT_CLUSTER_SIZE, 
                 threshold = _DEFAULT_THRESHOLD):
    clusters = []
    cluster = [func_data[0]]
    
    def registCluster(): 
        if len(cluster) >= min_cluster_size: 
            clusters.append(cluster) 
            
    for func_cur in func_data[1:]:
        func_prev = cluster[-1]
        gap = float(func_cur["start time"]) - float(func_prev["end time"])
        if(gap <= threshold): 
            cluster.append(func_cur)
            
        else: registCluster()
               
    #In case for loop ends while a cluster is growing
    registCluster()
    
    return clusters
    
if __name__ == "__main__":
    main(sys.argv[1:])