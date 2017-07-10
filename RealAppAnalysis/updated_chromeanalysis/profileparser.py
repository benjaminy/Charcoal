import json
import pprint
import utils

'''
Parse the JSON file from a chrome performance analyzer profile into a Python datastructure with the following layout:

    {
    "2435" :
        {
        "774": { "M":[metaevents], "D":[durationevents], "X":[completeevents], "I":[instantevents],...} ,
        "776": { "M":[metaevents], "D":[durationevents], [...], [...]},
        "675": { "M":[metaevents], "D":[durationevents], [...], [...]},
        ...
        }

    "5425" :
        {
        "345": { "M":[metaevents], "D":[durationevents], [...], [...]} ,
        "745": { "M":[metaevents], "D":[durationevents], [...], [...]},
        "623": { "M":[metaevents], "D":[durationevents], [...], [...]}, ......
        ...
        }
        ......
    }
'''

def main():
    pp = pprint.PrettyPrinter(indent=2)
    profile = utils.load_profile_from_file("jsonprofiles/sample.json")
    categorized_events = parseprofile( profile  )
    pp.pprint(categorized_events)



def parseprofile(profile):

    events = durationevents(profile) + profile

    '''[ events ] --> { "pid:[ events ], ..."}'''
    categorized_events = categorize_as_dic(events, "pid")

    '''{ "pid:[ events ], ..."} --> { "pid:{ "tid:[ events ], tid:[ events ], ..."}, ..."}'''
    for key, value in categorized_events.items():
        newvalue = categorize_as_dic(value, "tid")
        categorized_events[key] = newvalue

    '''{ "pid:{ "tid:[ events ], tid:[ events ], ..."}, ..."} --> { "pid:{ "tid:{ "D": [...], 'X':[...], 'I':[...] " }, ..., ..."}, ..."}'''
    for pid, subdic in categorized_events.items():
        for tid, value in subdic.items():
            newvalue = categorize_as_dic(value, "ph")

            categorized_events[pid][tid] = newvalue

    return categorized_events



def categorize_as_dic(events, attribute):
    '''Categorizes a list of events according to specified attributeibute.
    [ events ] --> { "attribute1:[ events ], attribute2:[ events ], attribute3:[ events ]..."} '''

    categorized_events = {}

    for event in events:
        category_key = event[attribute];

        if(category_key not in categorized_events):
            categorized_events[category_key] = []

        categorized_events[category_key].append(utils.trim_dic(event, attribute))

    if(attribute == "ph"):
        if 'B' in categorized_events:
            del categorized_events['B']
            del categorized_events['E']

    return categorized_events


def durationevents(events):
    stack = []
    durationevents = []

    for event in events:
        eventtype = event["ph"]

        if( eventtype == "B" ):
            stack.append(event.copy())

        if( eventtype == "E"):
            if stack:
                durationevent = stack.pop()
                duration = event["ts"] - durationevent["ts"]
                durationevent["dur"] = duration
                durationevent["start time"] = durationevent.pop("ts")
                durationevent["end time"] = event["ts"]
                durationevent["ph"] = "Dx"
                durationevents.append( durationevent )

    return durationevents


if __name__ == '__main__':
    main()
