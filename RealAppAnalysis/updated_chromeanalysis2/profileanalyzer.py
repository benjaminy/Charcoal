import cpuprofileparser as cpuparser
import profileparser as parser
import durationeventsanalyzer as duranalyzer
import processanalyzer
import utils
import os

def main():
    pass

def analyze_profile(filepath, newroot):
    profile = utils.load_profile_from_file(filepath)



    '''Retrieve pid and tid of interest, i.e. that of the CPU profile event'''
    cpu_profile = cpuparser.cpuprofile(profile)
    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpu_profile)
    print cpu_pid
    print cpu_tid

    fcalls2 = [event for event in profile if event["pid"]==cpu_pid and event["tid"]==cpu_tid]

    print "BLABLABLABLA: " + str(len([event for event in fcalls2 if event["name"]=="FunctionCall"])/2)

    categorized_profile = parser.parseprofile(profile)
    '''Get events of interest, i.e. durationevents that are functioncalls'''
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    fcalls = duranalyzer.get_fcalls_only(durationevents)
    fcalls = utils.sort_by_attribute(fcalls, "start time")


    print "NUMBER OF FUNCTIONCALLS: " + str(len(fcalls))
    #for f in fcalls:
    #    print f


    #tail, profile_name = os.path.split(filepath)
    #root, subdir = os.path.split(tail)
    #name, extension = profile_name.split(".")

    get_cluster_data(fcalls)
    #generate_graphs( fcalls, os.path.join(newroot, subdir, name) )
    print ""
    print ""
    print ""

def get_cluster_data(fcalls):
    clusters = duranalyzer.find_clusters(fcalls, 1000)
    #clusters = duranalyzer.findClusters2(fcalls)


    clusters = duranalyzer.filter_by_cluster_length( clusters, 3 )
    #print duranalyzer.get_clusters_fnames(clusters)
    print "Number of clusters with min size 3: " + str(len(clusters))

    print "CLUSTER LENGTHS"
    sizes = duranalyzer.get_clusters_lengths(clusters)
    if ( len(sizes) ):
        print "Average: " + str(float(sum(sizes))/float(len(sizes)))
    print "Standard deviation: " + str(duranalyzer.stdev(sizes))
    clusters_durations = duranalyzer.get_clusters_durations(clusters)
    print "CLUSTER DURATIONS"
    if ( len(clusters_durations) ):
        print "Average cluster duration: " + str(float(sum(clusters_durations))/float(len(clusters_durations)))
    print "Standard deviation: " + str(duranalyzer.stdev(clusters_durations))
    print ""

def generate_graphs(fcalls, filepath):

    '''Retrieve data from function call events'''
    start_times = duranalyzer.get_starttimes(fcalls)
    durations = duranalyzer.get_durations(fcalls)
    gaptimes = duranalyzer.get_gaptimes(fcalls)
    clusters = duranalyzer.find_clusters(fcalls, 500)


    '''Graph data'''
    (xs1, ys1) = duranalyzer.data1(durations)
    (xs3, ys3) = duranalyzer.data1(gaptimes)
    (xs5, ys5) = duranalyzer.distributiondata(start_times)
    (xs0, ys0) = duranalyzer.get_clusters_distr(clusters)

    duranalyzer.create_graph( filepath + "/_cluster_sizes.png", xs0, ys0, len(clusters), max(clusters) )
    duranalyzer.create_graph( filepath + "/duration_data.png", xs1, ys1, durations[-1], 1, True )
    duranalyzer.create_graph( filepath + "/_gaps_data.png", xs3, ys3, gaptimes[-1], 1, True )
    duranalyzer.create_graph( filepath + "/_timeline_distribution.png", xs5, ys5, start_times[-1], 1)


def process_with_most_durationevents(categorized_events):
    process_of_interst = 0
    max_number_of_durationevents = -1

    for pid, subdic in categorized_events.items():
        number_of_durationevents = 0
        for tid, subsubdic in subdic.items():
            if 'D' in subsubdic:
                number_of_durationevents += len(subsubdic['D'])
        if( number_of_durationevents > max_number_of_durationevents ):
            max_number_of_durationevents = number_of_durationevents
            process_of_interst = pid

    return process_of_interst


def get_event_types(categorized_events):

    eventtypes=[]
    for pid, subdic in categorized_events.items():
        for tid, subsubdic in subdic.items():
            for eventtype, events in subsubdic.items():
                if eventtype not in eventtypes:
                    eventtypes.append(eventtype)

    return eventtypes

if __name__ == '__main__':
    main()
