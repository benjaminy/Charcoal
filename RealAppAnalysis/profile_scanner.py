from os import listdir, getcwd, walk
from os.path import isfile, isdir, join
from json import load
from parseprofile import parseprofile
import datautil
import pprint
from datautil import trimDict, filterEvents, log, splitList

def getCPUProfileNodes(cpu_profile): return cpu_profile["args"]["data"]["cpuProfile"]["nodes"]


def formatFCallEvent(fcall_event):
    trimDict(fcall_event, "ts", "ph", "cat") 
    fcall_event["name"] = fcall_event["args"]["data"]["functionName"]
    del(fcall_event["args"])
    
def formatCPUNode(node, pid = 0, tid = 0): 
    node["pid"] = pid
    node["tid"] = tid
    node["name"] = node["callFrame"]["functionName"]
    trimDict(node, "callFrame", "hitCount", "id", "children", "positionTicks")

def onAllDataInDir(root_dir, func):
    def isDataFile(filename): return filename.endswith(".json")
    results = {}
    for (filepath, sub_directories, files) in walk(root_dir):
        print("Filepath: " + filepath)
        print("Subdirectories: " + str(sub_directories))
        print("Files: " + str(files))
        for file in files:
            if(isDataFile(file)):
                print(file)
                try:
                    with open((join(filepath, file)), 'r') as data_file: 
                        data = load(data_file)
                except:
                    print("Error in processing data file: " + file)  
                    data = []    
    
            results.update({file: func(data)})
    
    return results
                     
def examineProfileAccuracy(data):
    cpu_profile = data[-1]
    fcall_count_in_cpuprofile = len(cpu_profile["args"]["data"]["cpuProfile"]["nodes"])
    fcall_count_in_data = 0
    
    for event in data:
        if event["name"] == "FunctionCall": 
            fcall_count_in_data += 1
    
    print("Number of function calls")
    print("CPU Profile: %d Data: %d" % (fcall_count_in_cpuprofile, fcall_count_in_data))
    
    
def compareCPUProfileToFunctionEvents2(data, debug = False):
    """Work in progress"""
    try:
        cpu_profile = data[-1]
        
    except:
        print("CPU Profile not Found")
        return

    cpup_nodes = getCPUProfileNodes(cpu_profile)
    
    for node in cpup_nodes: formatCPUNode(node, pid = cpu_profile["pid"], tid = cpu_profile["tid"])
    
    fcall_events = filterEvents(filterEvents(data, "name", "FunctionCall"), "ph", "B")
    for fcall_event in fcall_events: formatFCallEvent(fcall_event)
        
    intersection = []
    
    for fce in fcall_events:
        if fce in cpup_nodes:
            intersection.append(fce)
            cpup_nodes.remove(fce)
            fcall_events.remove(fce)
                        
    if debug:
        log(intersection, indent = 1, tag = "Intersection")
        log(fcall_events, indent = 1, tag = "Event Only")
        log(cpup_nodes, indent = 1, tag = "CPU Profile Only")
        
    report = {}
    
    meta_events = filterEvents(data, "ph", "M")
    
    def categorizeByPIDTID(events):
        events = splitList(events, "pid")
        for pid, thread in events.items():
            events[pid] = splitList(thread, "tid")

        return events
    
    meta_events = categorizeByPIDTID(meta_events)
                
    def meta(event): return meta_events[event["pid"]][event["tid"]]
    
    meta_intersection = meta(cpu_profile)
    intersection.insert(0, meta_intersection)
    cpup_nodes.insert(0, meta_intersection)
    report.update({"Intersection": intersection})
    report.update({"CPU Only": cpup_nodes})
    
    fcall_events = categorizeByPIDTID(fcall_events)
    for process in fcall_events.values():
        for tid, thread_events in process.items():
            sample_event = thread_events[0]
            meta_thread = meta(sample_event)
            thread_events.insert(0, meta_thread)
            
    report.update({"Events Only": fcall_events})
    
    report_printer = pprint.PrettyPrinter(indent = 1)
    report_printer.pprint(report)
    
    return report

def compareCPUProfileToFunctionEvents(data):
    cpu_profile_nodes = getCPUProfileNodes(data)
    cpu_profile_fnames = [node["callFrame"]["functionName"] for node in cpu_profile_nodes]
    functions = filterEvents(data, "name", "FunctionCall")
    #print(functions)
    func_events_names = [function["args"]["data"]["functionName"] for function in functions if function["args"]]
    
    cpu_set = set(cpu_profile_fnames)
    event_set = set(func_events_names)
    
    intersection = cpu_set.intersection(event_set)
    log(intersection, tag = "Intersection")
    log(len(intersection), indent = 2, tag = "Count" )

    dif = cpu_set.difference(event_set)
    log(dif, tag = "Not in Function Events")
    log(len(dif), tag = "Count", indent = 2)
    
    dif = event_set.difference(cpu_set)
    log("Not in Profile: " + str(dif))
    log("Count: %d" % (len(dif)), indent = 2)
    #functions = filterEvents(data, "name", "FunctionCall")

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
        print("Nodes: "                + str(len(cpu_profile)))
        print("High level functions: " + str(len(high_level_functions)))
        print("Root children: "        + str(len(IDS_of_high_level_functions)))
        print("High level functions "  + str(high_level_functions))
    
    return high_level_functions
    
main = getcwd() + "/sample traces"
test = main + "/test page"
results = onAllDataInDir(main + "/wikipedia", compareCPUProfileToFunctionEvents2)

def examineParser(results_from_parseprofile):
    def print_keys(dict, tag = ""): 
        print(tag + str(list(dict.keys())))

    log(list(results_from_parseprofile), tag = "Files: ")
    for file, val in results_from_parseprofile.items():
        log(file, indent = 1, tag = "File")
        for pid, val2 in val.items():
            log(pid, indent = 2, tag = "pid")
            for tid, val3 in val2.items():
                log(tid, indent = 3, tag = "tid")
                for ph, val4 in val3.items():
                    log(ph, indent = 4, tag = "event type")
                    for e in val4:
                        log(e, indent = 4)
    print("\n\n")