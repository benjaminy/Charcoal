import cpuprofileparser as cpuparser
import profileparser as parser
import durationeventsanalyzer as duranalyzer
import processanalyzer
import utils

def main():
    #profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json")
    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/wikipedia.json")
    cpu_profile = cpuparser.cpuprofile(profile)
    print cpuparser.process_and_thread_ids(cpu_profile)

    categorized_events = parser.profileparser( profile  )

    print process_with_most_durationevents(categorized_events)
    print processanalyzer.thread_with_most_durationevents(categorized_events)

    print event_types(categorized_events)

def analyze_profile(filepath):
    (_, _, jsonprofile) = filepath.split('/')
    profile = utils.load_profile_from_file(filepath)


    '''Retrieve pid and tid of interest, i.e. that of the CPU profile event'''
    cpu_profile = cpuparser.cpuprofile(profile)
    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpu_profile)


    categorized_profile = parser.parseprofile(profile)
    '''Get events of interest, i.e. durationevents that are functioncalls'''
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    fcalls = duranalyzer.get_fcalls_only(durationevents)
    fcalls = utils.sort_by_attribute(fcalls, "start time")

    get_cluster_data(fcalls)
    #generate_graphs(fcalls, jsonprofile)


def get_cluster_data(fcalls):
    clusters = duranalyzer.find_clusters(fcalls, 1000)

    clusters = duranalyzer.filter_by_cluster_length( clusters, 3 )
    print "Number of clusters with min size 3: " + str(len(clusters))

    print "CLUSTER LENGTHS"
    sizes = duranalyzer.get_clusters_lengths(clusters)
    print "Average: " + str(sum(sizes)/len(sizes))
    print "Standard deviation: " + str(duranalyzer.stdev(sizes))
    clusters_durations = duranalyzer.get_clusters_durations(clusters)
    print "CLUSTER DURATIONS"
    print "Average cluster length: " + str(sum(clusters_durations)/len(clusters_durations))
    print "Standard deviation: " + str(duranalyzer.stdev(clusters_durations))
    print ""

def generate_graphs(fcalls, jsonprofile):

    '''Retrieve interesting data from function call events'''
    start_times = duranalyzer.get_starttimes(fcalls)
    durations = duranalyzer.get_durations(fcalls)
    gaptimes = duranalyzer.get_gaptimes(fcalls)
    clusters = duranalyzer.find_clusters(fcalls, 500)


    '''Graph data'''
    (xs1, ys1) = duranalyzer.data1(durations)
    (xs3, ys3) = duranalyzer.data1(gaptimes)
    (xs5, ys5) = duranalyzer.distributiondata(start_times)
    (xs0, ys0) = duranalyzer.clusters_distr(clusters)

    duranalyzer.create_graph( utils.newfilepath("newgraphs/", jsonprofile ,"_cluster_sizes.png"), xs0, ys0, len(clusters), 50 )
    duranalyzer.create_graph( utils.newfilepath( "newgraphs/", jsonprofile ,"_duration_data.png"), xs1, ys1, durations[-1], 1, True )
    duranalyzer.create_graph( utils.newfilepath( "newgraphs/", jsonprofile ,"_gaps_data.png"), xs3, ys3, gaptimes[-1], 1, True )
    duranalyzer.create_graph( utils.newfilepath( "newgraphs/", jsonprofile ,"_timeline_distribution.png"), xs5, ys5, start_times[-1], 1)


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


def event_types(categorized_events):

    eventtypes=[]
    for pid, subdic in categorized_events.items():
        for tid, subsubdic in subdic.items():
            for eventtype, events in subsubdic.items():
                if eventtype not in eventtypes:
                    eventtypes.append(eventtype)

    return eventtypes

if __name__ == '__main__':
    main()
