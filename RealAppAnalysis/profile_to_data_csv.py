import datautil
from datautil import filterEvents, log, parseCmdLnArgs, categorizeByValue, getJSONDataFromFile, _flagged
from csv import DictWriter
from os.path import join
import json
import sys

def main(argv):
    opts, args = parseCmdLnArgs(argv, "mdo", ["manual", "debug", "outdir"], _usage)
    debug = _flagged(opts, "-d", "--debug")
    
    if not args: profile = getJSONDataFromFile(None, find = True)    
    else: profile = getJSONDataFromFile(args[0])
    if not profile: return
    
    profile_by_pids = categorizeByValue(profile, "pid")
    
    if _flagged(opts, "-m", "--manual"):
        pids = list(profile_by_pids.keys())
        if debug: log(pids, indent = 3, tag = "Process ID") 
        pid = _inputKey(profile_by_pids)
        tid = _inputKey(categorizeByValue(profile_by_pids))
    
    #Use the process of the CPU Profile     
    else: 
        cpu_profile = profile[-1]
        pid = cpu_profile["pid"]
        tid = cpu_profile["tid"]
    
    if debug: datautil.log(pid, tag = "Process Selected")
    if debug: datautil.log(tid, tag = "Thread Selected")
    filtered_data = filterEvents(filterEvents(profile, "pid", pid), "tid", tid)
    if debug: log(len(filtered_data), indent = 3, tag = "Number of events")
    processed_data = toFuncData(filtered_data)
    if debug: log(len(processed_data), indent = 3, tag = "Processe data")
    outdir = ""
    filename = argv[-1]
    
    if _flagged(opts, "-o", "--outdir"):
        outdir = opt[1]
        filename = filename.split("\\")[-1]

    out_filename = filename + ".csv"
    out_filepath = join(outdir, out_filename)
    dataToCSV(processed_data, out_filepath)
    if debug: log(out_filepath, indent = 3, tag = "Out")

def toFuncData(data):
    functions = datautil.filterEvents(data, "name", "FunctionCall")
    condensed_func_data = functionSummary(functions)
    return condensed_func_data

def dataToCSV(data, filepath):   
    '''Takes JSON data and converts it into a .csv file in an
    excel dialect'''
    
    with open(filepath, 'w') as csvfile:
        fields_from_data_sample = data[0].keys()
        out = DictWriter(csvfile, 
                         fieldnames = fields_from_data_sample, 
                         delimiter = ",", 
                         dialect = 'excel')
        out.writeheader()
        for event in data:
            out.writerow(event)

def functionSummary(fcall_events):
    function_stack = []
    funcnames_and_durations = []
    def is_beginning(fcall_event): return fcall_event["ph"] == "B"
    def is_end(fcall_event): return fcall_event["ph"] == "E"
    
    for fcall_event in fcall_events:
        if(is_beginning(fcall_event)): function_stack.append(fcall_event)
            
        elif(is_end(fcall_event)):
            if not function_stack: continue
            start_fcall = function_stack.pop()
            start_time = float(start_fcall["ts"])
            end_time = float(fcall_event["ts"])
            dur = end_time - start_time
            funcnames_and_durations.append({"name": getFunctionName(start_fcall), 
                                            "start time": start_time, 
                                            "end time": end_time, 
                                            "duration": dur})
            
    return funcnames_and_durations        
    
def getFunctionName(fcall_event):
    name = fcall_event["args"]["data"]["functionName"]
    return name

def _inputKey(profile_by_pids):
    valid_key = False
    while not valid_key:
        key = int(input("Key: "))
        valid_key = key in profile_by_pids
        
    return key
    
def _usage():
    log("Options:", indent = 1)
    
    def _log(flag, explanation): 
        log(explanation, indent = 1, tag = "-" + flag)
        
    _log("m , --manual", "Toggles manual selection of data, including \n\
                       finding a data file and selecting which \n\
                       process to extract data")
    
    _log("d, --debug", "Displays information about the data as it is processed")
    
    _log("args", "<inputfile><module function>")
    
if __name__ == "__main__":
    main(sys.argv[1:])