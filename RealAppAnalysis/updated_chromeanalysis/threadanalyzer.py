import profileanalyzer
import utils
import parseprofile as parser

'''

Takes as input a dictionairy of events :

{ "M":[metaevents], "D":[durationevents], "X":[completeevents], "I":[instantevents],...}

Belonging to a given thread.

'''
def main():
    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/sample.json")
    categorized_events = parser.parseprofile( profile  )
    (pid, tid) = profileanalyzer.thread_with_most_durationevents(categorized_events)
    print types_of_events(categorized_events[pid][tid])
    for pid, subdic in categorized_events.items():
        for tid, subsubdic in subdic.items():
            types_of_events(subsubdic)


def event_types_count(dic_of_events):
    ''' { "M": 34, "D":45, "X":65, "I":67 }'''
    dic = {}
    for key, value in dic_of_events.items():
        dic[key] = len(value)
    return dic


if __name__ == '__main__':
    main()
