'''Utility functions to search, filter, and manipulate output data of Chrome's event tracer'''

import tkinter
from tkinter.filedialog import askopenfilename
import json
import csv
import os;

def getJSONDataFromFile():
    '''Returns the JSON data in a file'''
    #Gets rid of a GUI element native to tkinter
    root = tkinter.Tk();
    root.withdraw();
    
    with open(askopenfilename(),'r') as jsonfile:
        #Data includes keys 'traceEvents' and metadata
        data = json.load(jsonfile)
    
    return data;

def splitList(events, attr):
    '''Split a set of events into set of lists whose elements differ 
    by the attribute argument'''
    categorized_events = {};
    for event in events:
        category_key = event[attr];
        if(category_key not in categorized_events):       
            #Init a key : value pair 
            val = [];
            categorized_events[category_key] = val;
            #print(categorized_events);
        else: 
            val = categorized_events[category_key];
        
        val.append(event);
    
    return categorized_events;


def filterEvents(events, attr, val):
   return [event for event in events if event[attr] == val]

def extract(events, attr):
    return [event[attr] for event in events if attr in event];

def toTxt(data, name, subdir = ""):
    filepath = ""
    
    if(not subdir == ""):
        filepath = os.getcwd() + "\\" + subdir
        if not os.path.exists(filepath):
            os.makedirs(filepath)
        filepath += "\\" 
        
    with open(filepath + name + ".txt", "w") as out:
        for element in data:
            out.write(str(element) + "\n");
        
def toCSV(json_data, common_attributes, filepath):
    '''Takes JSON data and converts it into a .csv file in an
    excel dialect'''
    with open(filepath, 'w') as csvfile:
        out = csv.DictWriter(csvfile, common_attributes, delimiter = ",", dialect = 'excel');
        out.writeheader(common_attributes);
        for event in json_data:
            out.writerow(event);
            
def runtime(events):
    '''Gets the runtime of a list events. This assumes events were derived
    from chrome's profiler, which is already in sorted order'''
    starttime = events[0]["ts"];
    endtime = events[len(events) - 1]["ts"];
    return endtime - starttime;


def getDurations(duration_events):
    function_stack = []
    durations = []
    begins_fcall = lambda event: event["ph"] == "B"
    ends_fcall = lambda event: event["ph"] == "E"
    for event in duration_events:
        if(begins_fcall(event)):
            function_stack.append(event)
            
        elif(ends_fcall(event)):
            startFCall = function_stack.pop()
            
            if(ends_fcall(startFCall)):
                raise Exception("Unexpected event: Not a duration event")
            
            else:
                start_time = float(startFCall["ts"])
                end_time = float(event["ts"])
                dur = end_time - start_time
                durations.append(dur)
        
        else:
            pass
            
    return durations