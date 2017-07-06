import cpuprofileparser as cpuparser
import parseprofile as parser
import processanalyzer
import utils

def main():
    #profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json")
    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/wikipedia.json")
    cpu_profile = cpuparser.cpuprofile(profile)
    print cpuparser.process_and_thread_ids(cpu_profile)

    categorized_events = parser.parseprofile( profile  )

    print process_with_most_durationevents(categorized_events)
    print processanalyzer.thread_with_most_durationevents(categorized_events)

    print event_types(categorized_events)



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
