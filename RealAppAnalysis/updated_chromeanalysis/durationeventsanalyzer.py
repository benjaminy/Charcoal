import matplotlib.pyplot as plt
import profileparser
import utils
import cpuprofileparser as cpuparser
import pprint
import math


''' Takes as input a list of durationevents ASSUMED TO BE SORTED BY START TIME, belonging to a certain thread and process, and performs analysis on it.
NOTE: Some parts of the analysis are fundamentally flawed if complete events should be taken into account.

Prove the following:
Many applications are composed of tons of extremely short callbacks.
Sometimes there are chains of callbacks that typically execute in short succession.
If some other action happened in the middle of such a chain, it could cause a concurrency bug.

durationevents = [
{
  { args':
     {
       tileData':
         { layerId': 93,
           sourceFrameNumber': 311,
           tileId':
           {
             u'id_ref':
             u'0x7f860ad387b0'
           },

         tileResolution':
         u'HIGH_RESOLUTION'
         }
     },
   'cat': u'cc,disabled-by-default-devtools.timeline',
   'dur': '532',
   'end time': 27781510672,
   'name': u'RasterTask',
   'start time': 27781510140,
   'tts': 100325},
},

{}, {}...]

'''

def main():
    pp = pprint.PrettyPrinter(indent=2)
    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/updated_chromeanalysis/jsonprofiles/facebook.json")
    cpu_profile = cpuparser.cpuprofile(profile)
    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpu_profile)


    categorized_profile = profileparser.parseprofile(profile)
    '''Get events of interest, i.e. durationevents that are functioncalls'''
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    fcalls = get_fcalls_only(durationevents)
    fcalls = utils.sort_by_attribute(fcalls, "start time")

    start_times = get_starttimes(fcalls)
    gaps_times = get_gaptimes(fcalls)

    clusters = find_clusters(fcalls, 1000)

    clusters = filter_by_cluster_length( clusters, 3 )
    print "Number of clusters with min size 3: " + str(len(clusters))

    print "CLUSTER LENGTHS"
    sizes = get_clusters_lengths(clusters)
    print "Average: " + str(sum(sizes)/len(sizes))
    print "Standard deviation: " + str(stdev(sizes))
    clusters_durations = get_clusters_durations(clusters)
    print "CLUSTER DURATIONS"
    print "Average cluster length: " + str(sum(clusters_durations)/len(clusters_durations))
    print "Standard deviation: " + str(stdev(clusters_durations))


def get_fcalls_only(durationevents):
    fcalls = []
    for durevent in durationevents:
        if durevent["name"] == "FunctionCall":
            fcalls.append(durevent)
    return fcalls

def get_durations(durationevents):
    durations = []
    for durationevent in durationevents:
        durations.append(durationevent["dur"])
    return sorted(durations)

def get_funcnames(durationevents):
    func_names = []
    for durevent in durationevents:
        if durevent["name"] == "FunctionCall":
            func_names.append(durevent["args"]["data"]["functionName"])
    return set(func_names)

def get_starttimes(durationevents_sorted):
    start_times = []
    for durationevent in durationevents_sorted:
        start_times.append(durationevent["start time"])
    print "IS SORTED (start times)? " + str(start_times == sorted(start_times))
    return normalize_starttimes( start_times )

def get_gaptimes(durationevents_sorted):
    gaps = []
    for i in range(0, len(durationevents_sorted) - 1):

        f1 = durationevents_sorted[i]
        f2 = durationevents_sorted[i + 1]
        f1_end = float(f1["end time"])
        f2_start = float(f2["start time"])

        if(f1_end != f2_start):
            gap = f2_start - f1_end
            gaps.append(gap)
    return sorted(gaps)

def total_functiontime(durations):
    sum=0
    for dur in durations:
        sum += dur
    return sum

def total_idletime(gaps):
    cum_time=0
    for gap in gaps:
        cum_time += gap
    return cum_time

def idletime_percentage(total_gaptime, total_functiontime):
    return ( total_gaptime / (total_gaptime + total_functiontime) )

def distributiondata(start_times):
    ys = []
    number_of_calls = len( start_times )
    for i in range( number_of_calls ):
        ys.append( float(i+1)/float(number_of_calls) )
    return (start_times, ys)

def normalize_starttimes(start_times):
    normalized_start_times = []
    for stime in start_times:
        normalized_start_times.append(float(stime)-float(start_times[0]))
    return normalized_start_times

def simple_timeline_data(normalized_start_times):
    xs = []
    ys = []
    for start_time in normalized_start_times:
        xs.append( start_time )
        ys.append(0)
    return (xs, ys)


'''One of the lines is the number of events/function calls with duration less than X,
normalized to the total number of function calls in the trace. '''
def data1(durations):
    xs = []
    ys = []

    number_of_durevents = len(durations)
    for i in range( number_of_durevents ):
        xs.append( durations[i] )
        ys.append( float(i+1)/float(number_of_durevents) )

    return (xs, ys)

'''The other line is the total duration (sum) of all the event/function calls with duration less than X,
normalized to the total duration of all function calls in the trace. '''
def data2(durations):

    xs = []
    ys = []

    total_duration = total_functiontime(durations)
    cumulative_time = 0.0

    for dur in durations:
        cumulative_time += dur
        xs.append( dur )
        ys.append( float(cumulative_time)/float(total_duration) )

    return (xs, ys)

def find_clusters(durationevents_sorted, threshold = 1000000):
    clusters = []
    current_cluster = []
    current_cluster_is_empty = True

    for i in range(len(durationevents_sorted) - 1):
        f1 = durationevents_sorted[i]

        if ( current_cluster_is_empty ):
            current_cluster.append( f1 )
            current_cluster_is_empty = False

        f2 = durationevents_sorted[i + 1]

        if( (f2["start time"]-f1["end time"]) < threshold ):
            current_cluster.append( f2 )

        else:
            clusters.append(current_cluster)
            if (i < len(durationevents_sorted)-2):
                current_cluster = []
                current_cluster_is_empty = True

    clusters.append(current_cluster)
    return clusters



def stdev(l):
    mean = sum(l)/len(l)
    s = 0
    for integer in l:
        s += (integer - mean)**2
    return math.sqrt(s/len(l))

def filter_by_cluster_length(clusters, minimum_cluster_length):
    filtered_clusters = []
    for cluster in clusters:
        if len(cluster) >= minimum_cluster_length:
            filtered_clusters.append( cluster )
    return filtered_clusters


def get_clusters_distr(clusters):
    xs = []
    ys = []
    for i in range(0, len(clusters)):
        xs.append(i+1)
        ys.append( len( clusters[i] ) )

    return (xs, ys)

def get_clusters_lengths(clusters):
    clustersx = []
    for cluster in clusters:
        clustersx.append( len(cluster) )
    return clustersx

def get_clusters_durations(clusters):
    clustersx = []
    for cluster in clusters:
        clustersx.append( cluster[-1]["end time"]-cluster[0]["start time"] )
    return clustersx

def get_clusters_fnames(clusters):
    clustersx = []
    for cluster in clusters:
        clusterx = []
        for fcallevent in cluster:
            clusterx.append( fcallevent["args"]["data"]["functionName"] )
        clustersx.append(clusterx)
    return clustersx


def create_graph(filepath, xs, ys, xmax, ymax, logscale = False):
    plt.plot(xs, ys, "ro")
    if logscale:
        plt.xscale("log")
    plt.axis([0, xmax, 0, ymax])
    plt.savefig(filepath)
    plt.clf()

if __name__ == '__main__':
    main()
