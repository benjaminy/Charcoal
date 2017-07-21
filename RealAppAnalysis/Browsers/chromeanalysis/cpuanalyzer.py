from os import listdir, getcwd, walk
from os.path import isfile, isdir, join
from json import load
import datautil

# justify that cpu profiler is good
# generate report with the functions shared between cpu and event tracer
# in addition to including pid and tid, metadata is to be included
# overall picture of how cpu raltes to overall profile
# --> can we just look at the cpu profile or not?


#
#
#
#

def getCPUProfileNodes(data):
    return data[-1]["args"]["data"]["cpuProfile"]["nodes"]

def log(str, indent = 1):
    print ("\t" * indent) + str


# include in each event object: pid, tid, functionname, ---- the timestamp, duration
def trimFunctionCallEvent(fcall_event):
    del( fcall_event["ts"] )
    del( fcall_event["ph"] )
    del( fcall_event["cat"] )
    fcall_event["name"] = fcall_event["args"]["data"]["functionName"]
    del(fcall_event["args"])


# include in each event object: pid, tid, functionname
def trimCPUNode(node, pid = 0, tid = 0):
    node["pid"] = pid
    node["tid"] = tid
    node["name"] = node["callFrame"]["functionName"]
    del(node["callFrame"])
    del(node["hitCount"])
    del(node["id"])
    del(node["children"])


def onAllDataInDir(root_dir, func):

    def isDataFile(filename):
        return filename.endswith(".json")

    for (filepath, sub_directories, files) in walk(root_dir):
        print("Filepath: " + filepath)
        print("Subdirectories: " + str(sub_directories))
        print("Files: " + str(files))

        for file in files:
            if(isDataFile(file)):
                print(file)
                try:
                    with open( (join(filepath, file) ), 'r') as data_file:
                        data = load(data_file)
                except:
                    print("Error in processing data file: " + file)
                    data = []

            func(data)

def examineProfileAccuracy(data):
    cpu_profile = data[-1]
    fcall_count_in_cpuprofile = len(cpu_profile["args"]["data"]["cpuProfile"]["nodes"])
    fcall_count_in_data = 0

    for event in data:
        if event["name"] == "FunctionCall":
            fcall_count_in_data += 1

    print("Number of function calls")
    print("CPU Profile: %d Data: %d" % (fcall_count_in_cpuprofile, fcall_count_in_data))


def compareCPUProfileToFunctionEvents2(data):
    # allows for a more inclusive report
    """Work in progress"""
    cpu_profile = data[-1]

    cpup_nodes = getCPUProfileNodes(data)
    for node in cpup_nodes:
        trimCPUNode(node, pid = cpu_profile["pid"], tid = cpu_profile["tid"])

    fcall_events = datautil.filterEvents(data, "name", "FunctionCall")
    for fcall_event in fcall_events:
        trimFunctionCallEvent(fcall_event)

    #comparison
    # find the functions that are in both cpu and events
    # cpup_nodes = { { "pid:xx", "tid:yy", "name":zz },{},{} }
    # fcall_events = { { "pid:xx", "tid:yy", "name":zz },{},{} }

    events = set(fcall_events)
    nodes = set(cpup_nodes)
    intersection = events.intersection(nodes)
    in_eventtrace_only = events.difference(nodes)
    in_cpu_only = nodes.difference(events)

    #metadataevents = filterEvents(data, "cat", "__metadata")
    ''' { (1275, 776, ): ( metadata, [ intersection ], [ in event ],  ), (1275, 776): [ metaeventobject, [durationevents] ], (1275, "776, metadata): [ durationevents ] (1275, "776, metadata): []  }'''
    ''' '''

    metadataevents_by_pid = splitList(metadataevents, "pid")
    '''{ "1483" : [ .... ], 1324: [...] ..... }'''
    ''' { "1483" : { "775": metaevent, "673" : metaevent, "564": metaevent....... }, "1483" : { "775": metaevent, "673" : metaevent, "564": metaevent....... }        ......} '''

    metadataevents_by_pid[ cpu_profile["pid"] ]
    metadata_by_pid_and_tid = filterEvents( metadataevents_by_pid, "tid", cpu_profile["tid"] )

    formatted_cpu_only = {(cpu_profile["pid"], cpu_profile["tid"]): metadataevents_by_pid_and_tid + in_cpu_only}







def compareCPUProfileToFunctionEvents(data):
    #looks only at functionnames
    cpu_profile_nodes = getCPUProfileNodes(data)
    cpu_profile_fnames = [node["callFrame"]["functionName"] for node in cpu_profile_nodes]

    functions = datautil.filterEvents(data, "name", "FunctionCall")
    #print(functions)
    func_events_names = [ function["args"]["data"]["functionName"] for function in functions if function["args"] ]

    cpu_set = set(cpu_profile_fnames)
    event_set = set(func_events_names)

    intersection = cpu_set.intersection(event_set)
    log( "Intersection: " + str(intersection) )
    log( "Count: %d" % ( len(intersection) ), indent = 2 )

    dif = cpu_set.difference(event_set)
    log("In cpuprofile but not larger profile: " + str(dif))
    log("Count: %d" % (len(dif)), indent = 2)

    dif = event_set.difference(cpu_set)
    log("In larger profile but not cpuprofile: " + str(dif))
    log("Count: %d" % (len(dif)), indent = 2)
    #functions = datautil.filterEvents(data, "name", "FunctionCall")



def getTopLevelFunctions(cpu_profile, debug = False):
    '''
    Returns all the functions that are not called by other functions from a CPU_Profile
       Ex:
           Let there be functions a, b, and c
           If function A calls B, and function B calls C during runtime
           This function would return A
    '''

    root = cpu_profile[0]
    high_level_functions = [function for function in cpu_profile if function["id"] in root["children"]]

    if debug:
        print( "Nodes: "                + str(len(cpu_profile)))
        print("High level functions: " + str(len(high_level_functions)))
        print("Root children: "        + str(len(IDS_of_high_level_functions)))
        print("High level functions "  + str(high_level_functions))

    return high_level_functions

main = getcwd() + "/sample traces"
test = main + "/test page"
onAllDataInDir(main, compareCPUProfileToFunctionEvents)
