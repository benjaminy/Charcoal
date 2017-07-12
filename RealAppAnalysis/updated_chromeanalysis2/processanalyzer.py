import profileanalyzer
import utils
import profileparser as parser

def main():
    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/sample.json")
    categorized_events = parser.profileparser( profile  )
    (pid, tid) = profileanalyzer.thread_with_most_durationevents(categorized_events)
    

def thread_with_most_durationevents(categorized_events):
    thread_of_interst = (0, 0)
    max_number_of_durationevents = -1

    for pid, subdic in categorized_events.items():
        for tid, subsubdic in subdic.items():
            number_of_durationevents = 0
            if 'D' in subsubdic:
                number_of_durationevents += len(subsubdic['D'])
        if( number_of_durationevents > max_number_of_durationevents ):
            max_number_of_durationevents = number_of_durationevents
            thread_of_interst = (pid, tid)

    return thread_of_interst




if __name__ == '__main__':
    main()
