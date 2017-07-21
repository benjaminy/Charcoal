# This module defines all the functions that can be used to map
# over profiles.

from datautil import trimDict

def compareCPUProfileToFunctionEvents(data, debug = False):
    
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
    
    if debug:
        report_printer = pprint.PrettyPrinter(indent = 1)
        report_printer.pprint(report)
        
    return report

def countFCalls(data):
    cpu_profile = data[-1]
    fcall_count_in_cpuprofile = len(cpu_profile["args"]["data"]["cpuProfile"]["nodes"])
    fcall_count_in_data = 0
    
    for event in data:
        if event["name"] == "FunctionCall": 
            fcall_count_in_data += 1
    
    print("Number of function calls")
    print("CPU Profile: %d Data: %d" % (fcall_count_in_cpuprofile, fcall_count_in_data))