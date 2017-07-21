import csv
import json
import os
import utils


def generate_csv(inputprofile_filename):

    with open(inputprofile_filename) as json_data:
        trace_profile = json.load(json_data)


    profile_by_pids = splitList(trace_profile, "pid")
    pids = list(profile_by_pids.keys())

    pid = process_most_func_calls(profile_by_pids)
    #if(pid == cpu_profile_pid(trace_profile)):
    #    print("profile and cpup consistent")
    #else:
    #    print("NOT CONSISTENT")

    process_of_interest = profile_by_pids[pid]
    functions = filterEvents(process_of_interest, "name", "FunctionCall")
    #returns a list with the event objects satisfying the following conditions
    # a) correct process id
    # b) has the name = "functioncall"

    condensed_func_data = functionSummary(functions)

    #head, tail = os.path.split(inputprofile_filename)
    toCSV(condensed_func_data, utils.newpath( "csvfiles/", inputprofile_filename, ".csv") ) #"csvfiles/" + tail + ".csv"


def cpu_profile_pid(profile):
    cpu_profile = profile[-1]
    return cpu_profile["pid"]

def cpu_profile_tid(profile):
    cpu_profile = profile[-1]
    return cpu_profile["tid"]

def splitList(events, attr):
    '''Split a set of events into set of lists whose elements differ
    by the attribute argument'''

    ''' dictionary with "attr" mapping to lists with events '''
    categorized_events = {};

    for event in events:
        category_key = event[attr];

        if(category_key not in categorized_events):
            #initialize a key : value pair
            #( pid:[event1, event2, ..., event x], pid:[...] ... )
            categorized_events[category_key] = []

        categorized_events[category_key].append(event)

    return categorized_events;



def filterEvents(events, attr, val):
   return [event for event in events if event[attr] == val]


def toCSV(data, filename):
    '''Takes a list of dictionaires and converts it into a .csv file '''
    with open(filename, 'w') as csvfile:
        fields = data[0].keys() #list of keys pid, tid....
        out = csv.DictWriter(csvfile, fieldnames = fields, delimiter = ",", dialect = 'excel') #dicwriter part of the csv module
        out.writeheader() #write fieldnames
        for event in data:
            out.writerow(event)


def functionSummary(fcall_events):
    function_stack = []
    funcnames_and_durations = []
    begins_fcall = lambda fcall: fcall["ph"] == "B" #return a function which returnstrue if the json event object (input) has event type B
    ends_fcall = lambda fcall: fcall["ph"] == "E" #complement, E

    for fcall in fcall_events:
        if( begins_fcall( fcall ) ):
            function_stack.append(fcall)

        else:

            if( len(function_stack) > 0 ):
                start_fcall = function_stack.pop()
                start_time = float(start_fcall["ts"])
                end_time = float(fcall["ts"])
                dur = end_time - start_time
                funcnames_and_durations.append({"name": getFunctionName(start_fcall), "start time": start_time, "end time": end_time, "duration": dur})

    return funcnames_and_durations

def getFunctionName(fcall_event):
    name = fcall_event["args"]["data"]["functionName"]
    return name

def process_most_func_calls(events_by_process):
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

        #print("Process: %d Function Count: %d" % (pid, fcall_count))
    #print "likelypid: " + str(likely_pid)

    return likely_pid
