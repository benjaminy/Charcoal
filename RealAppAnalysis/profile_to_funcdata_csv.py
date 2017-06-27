import datautil
import csv
import json

manual_input = True

def main():
    with open(datautil.findFile(), 'r') as json_data: 
        trace_profile = json.load(json_data)
    profile_by_pids = datautil.splitList(trace_profile, "pid")
    pids = list(profile_by_pids.keys())
    print(pids)
    
    if(manual_input):
        pid = inputKey(profile_by_pids)
    else:            
        pid = guessProcessOfInterest(profile_by_pids)
    
    process_of_interest = profile_by_pids[pid]
    
    functions = datautil.filterEvents(process_of_interest, "name", "FunctionCall")
    condensed_func_data = functionSummary(functions)
    print(condensed_func_data)
    
    fcall_total_duration = datautil.runtime(functions)    
    print("Cumulative Function Call Duration: " + str(fcall_total_duration))
    toCSV(condensed_func_data, "data_out.csv")
    
    #datautil.toTxt(process_of_interest, "test_out", subdir = "out")
    datautil.toTxt(functions, "test_functions_out", subdir = "out")
    #datautil.toTxt(durations, "test_fdur_out", subdir = "out")

def toCSV(data, filepath):
    '''Takes JSON data and converts it into a .csv file in an
    excel dialect'''
    with open(filepath, 'w') as csvfile:
        fields_from_data_sample = data[0].keys()
        out = csv.DictWriter(csvfile, fieldnames = fields_from_data_sample, delimiter = ",", dialect = 'excel')
        out.writeheader()
        for event in data:
            out.writerow(event)

def getJSONDataFromFile():
    with open(datautil.findFile(),'r') as jsonfile:
        data = json.load(jsonfile)
    
    return data;

def functionSummary(fcall_events):
    
    function_stack = []
    funcnames_and_durations = []
    begins_fcall = lambda fcall: fcall["ph"] == "B"
    ends_fcall = lambda fcall: fcall["ph"] == "E"
    
    for fcall in fcall_events:
        if(begins_fcall(fcall)): function_stack.append(fcall)
            
        elif(ends_fcall(fcall)):
            start_fcall = function_stack.pop()
            start_time = float(start_fcall["ts"])
            end_time = float(fcall["ts"])
            dur = end_time - start_time
            funcnames_and_durations.append({"name": getFunctionName(start_fcall), "start time": start_time, "end time": end_time, "duration": dur})
        
        else:
            pass
            
    return funcnames_and_durations        
    
def getFunctionName(fcall_event):
    name = fcall_event["args"]["data"]["functionName"]
    return name

def guessProcessOfInterest(events_by_process):
    '''Returns the pid with the most function calls. Used as a rough estimator for processes of interest,
    but no guarantee'''
    
    max_fcalls = -1
    weird_coincidence = list(events_by_process.keys())[-2]
    likely_pid = weird_coincidence
    
    for pid in events_by_process:
        fcall_count = 0
        process = events_by_process[pid]
        
        for event in process:
            if event["name"] == "FunctionCall": 
                fcall_count += 1

        if fcall_count > max_fcalls: 
            max_fcalls = fcall_count
            likely_pid = pid
        
        print("Process: %d Function Count: %d" % (pid, fcall_count))
            
    return likely_pid

def inputKey(profile_by_pids):
    valid_key = False
    while not valid_key:
        key = int(input("Key: "))
        valid_key = key in profile_by_pids
        
    return key

main()
