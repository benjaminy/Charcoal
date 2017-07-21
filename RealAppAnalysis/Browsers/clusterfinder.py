from datautil import findFile, \
                     toTxt, \
                     parseCmdLnArgs,\
                     filterEvents, \
                     readCSV, \
                     log, \
                     _flaggedRetArg
                     
from csv import DictReader
import sys
from _operator import itemgetter

_DEFAULT_THRESHOLD = 1000
_DEFAULT_FUNCDURATION_THRESHOLD = _DEFAULT_THRESHOLD * 2
_DEFAULT_CLUSTER_SIZE = 1

def main(argv):
    opts, args = parseCmdLnArgs(argv, "s:t:", ["cluster-size=", "gap-threshold="])
    
    raw_data = _loadFile(args)
    
    if False: clusters = findClusters(raw_data, 
                            min_cluster_size = _handleSizeFlag(opts),
                            threshold = _handleThresholdFlag(opts))
    
    
    else: clusters = find_clusters(raw_data, threshold = 1000)
    
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
        
def findClusters(functions 
                 , min_cluster_size = _DEFAULT_CLUSTER_SIZE
                 , threshold = _DEFAULT_THRESHOLD
                 , func_duration_threshold = _DEFAULT_FUNCDURATION_THRESHOLD):
    
    clusters = []
    cluster = [functions[0]]
    
    def registCluster(): 
        if len(cluster) >= min_cluster_size: 
            clusters.append(cluster) 
            
    for func_cur in functions[1:]:
        if not cluster: 
            cluster.append(func_cur) 
            continue
        
        func_prev = cluster[-1]
        #if float(func_cur["duration"]) <= func_duration_threshold:
        gap = float(func_cur["start time"]) - float(func_prev["end time"])
        if(gap <= threshold): 
            cluster.append(func_cur)
                
        else: 
            registCluster()
            cluster = []
               
    #In case for loop ends while a cluster is growing
    registCluster()
    
    return clusters

def find_clusters(durationevents_sorted, threshold = 1000):
    clusters = []
    current_cluster = []

    for i in range(len(durationevents_sorted) - 1):
        f1 = durationevents_sorted[i]

        if not current_cluster:
            current_cluster.append( f1 )
            continue

        f2 = durationevents_sorted[i + 1]

        if( (float(f2["start time"]) - float(f1["end time"])) < threshold ):
            current_cluster.append( f2 )

        else:
            clusters.append(current_cluster)
            if (i < len(durationevents_sorted)-2):
                current_cluster = []

    clusters.append(current_cluster)
    return clusters

if __name__ == "__main__":
    main(sys.argv[1:])