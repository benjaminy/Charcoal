import parseprofile as parser
import cpuprofileparser as cpuparser
import durationeventsanalyzer as duranalyzer
import profileanalyzer
import utils
import pprint
import os


def main():
    for jsonprofile in os.listdir("jsonprofiles"):
        print jsonprofile
        profile = utils.load_profile_from_file("jsonprofiles/" + jsonprofile)
        ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpuparser.cpuprofile(profile))
        categorized_profile = parser.parseprofile(profile)
        durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
        print("the number of durationevents is: " + str( len(durationevents) ) )
        durations = duranalyzer.get_durations(durationevents)

        (xs, ys) = duranalyzer.data1(durations)
        (xs2, ys2) = duranalyzer.data2(durations)
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,".png"), xs, ys )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"2.png"), xs2, ys2 )


    '''pp = pprint.PrettyPrinter(indent=2)

    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json")


    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpuparser.cpuprofile(profile))
    categorized_profile = parser.parseprofile(profile)
    print profileanalyzer.event_types(categorized_profile)

    #obtain durationevents from thread of interest
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    durations = duranalyzer.get_durations(durationevents)
    (xs, ys) = duranalyzer.data2(durations)
    duranalyzer.create_graph("testgraph.png", xs, ys)
    #pp.pprint(durations)'''






if __name__ == '__main__':
    main()
