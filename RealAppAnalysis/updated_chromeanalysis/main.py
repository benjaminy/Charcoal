import parseprofile as parser
import cpuprofileparser as cpuparser
import durationeventsanalyzer as duranalyzer
import profileanalyzer
import utils
import pprint



def main():
    pp = pprint.PrettyPrinter(indent=2)

    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json")


    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpuparser.cpuprofile(profile))
    categorized_profile = parser.parseprofile(profile)
    print profileanalyzer.event_types(categorized_profile)

    #obtain durationevents from thread of interest
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    durations = duranalyzer.get_durations(durationevents)
    (xs, ys) = duranalyzer.data2(durations)
    duranalyzer.create_graph("testgraph.png", xs, ys)
    #pp.pprint(durations)






if __name__ == '__main__':
    main()
