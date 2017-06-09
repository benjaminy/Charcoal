'''Utility functions to search, filter, and manipulate output data of Chrome's event tracer'''

import tkinter
from tkinter.filedialog import askopenfilename
import json
import csv;

def getJSONData():
    '''Returns the JSON data in a file'''
    #Gets rid of a GUI element native to tkinter
    root = tkinter.Tk();
    root.withdraw();
    
    with open(askopenfilename(),'r') as jsonfile:
        #Data includes keys 'traceEvents' and metadata
        data = json.load(jsonfile)
    
    return data;

def splitEvents(events, attr):
    '''Split a set of events into set of event lists which differ 
    by the attribute argument'''
    categorized_events = {};
    for event in events:
        cur_event_key = event[attr];
        if(cur_event_key not in categorized_events):
            val = [event];
            categorized_events[cur_event_key] = val;
            #print(categorized_events);
        else: 
            val = categorized_events[cur_event_key];
        
        val.append(event);
    
    return categorized_events;

def filterEvents(func, events):
    '''Filters events using the provided functions.'''
    return list(filter(func, events));


def toCSV(json_data, common_attributes, filepath):
    '''Takes JSON data and converts it into a .csv file in an
    excel dialect'''
    with open(filepath, 'w') as csvfile:
        out = csv.DictWriter(csvfile, common_attributes, delimiter=' ', dialect='excel');
        for event in json_data:
            out.writerow(event);
        
    