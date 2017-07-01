import json
import pprint

'''
Parse the JSON file from a chrome performance analyzer profile into a Python datastructure of the following layout:

    {
    "2435" :
        {
        "774": ( metaevent, [durationevents], [], []) ,
        "776": ( metaevent, [durationevents], [], []),
        "675": ( metaevent, [durationevents], [], []), ......
        }....

    "5425" :
        {
        "345": ( metaevent, [durationevents], [], []) ,
        "745": ( metaevent, [durationevents], [], []),
        "623": ( metaevent, [durationevents], [], []), ......
        }
        ......
    }
'''

def main():
    parseprofile( load_profile_from_file("profiles/sample.json") )

def load_profile_from_file(filepath):
    with open(filepath) as json_data:
        profile = json.load(json_data)
    return profile

def cpuprofile_event(profile):
    return profile[-1]

def parseprofile(profile):

    events = durationevents(profile) + profile

    '''[ events ] --> { "pid:[ events ], ..."}'''
    categorized_events = categorize_as_dic(events, "pid")

    '''{ "pid:[ events ], ..."} --> { "pid:{ "tid:[ events ], tid:[ events ], ..."}, ..."}'''
    for key, value in categorized_events.items():
        newvalue = categorize_as_dic(value, "tid")
        categorized_events[key] = newvalue

    '''{ "pid:{ "tid:[ events ], tid:[ events ], ..."}, ..."} --> { "pid:{ "tid:{ "durationevents": [...], 'X':[...], 'I':[...] " }, ..., ..."}, ..."}'''
    for pid, subdic in categorized_events.items():
        for tid, value in subdic.items():
            newvalue = categorize_as_dic(value, "ph")
            categorized_events[pid][tid] = newvalue


    pp = pprint.PrettyPrinter(indent=2)
    pp.pprint(categorized_events)

    return categorized_events



def categorize_as_dic(events, attribute):
    '''Categorizes a list of events according to specified attributeibute.
    [ events ] --> { "attribute1:[ events ], attribute2:[ events ], attribute3:[ events ]..."} '''

    categorized_events = {}

    for event in events:
        category_key = event[attribute];

        if(category_key not in categorized_events):
            categorized_events[category_key] = []

        categorized_events[category_key].append(trim_event(event, attribute))

    if(attribute == "ph"):
        if 'B' in categorized_events:
            del categorized_events['B']
            del categorized_events['E']

    return categorized_events


def categorize_as_tuple(events, attribute):
    ''' [ events ] -> ( eventtype1 ], [ eventtype2 ], ... ) '''
    return tuple( categorize_as_dic(events, attribute).values() )


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
                durationevent["dur"] = str(duration)
                durationevent["ph"] = "durationevents"
                durationevents.append( durationevent )

    return durationevents

def trim_event(event, *argv):
    for arg in argv:
        del event[arg]
    return event


if __name__ == '__main__':
    main()
