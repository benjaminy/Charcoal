'''Utility functions to search, filter, and manipulate output data of Chrome's event tracer'''

import tkinter
from tkinter.filedialog import askopenfilename
import json
import csv;
from statistics import mean;
from statistics import median;
from statistics import mode;
from functools import reduce;
from operator import itemgetter

def getJSONDataFromFile():
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


def filterEvents(func, events):
    '''Filters events by the provided function.'''
    return list(filter(func, events));
    


def getBasicStats(events, attr):
    values = extractAttrValues(events, attr);
    return {"mean": mean(values), "median": median(data), "mode": mode(data)};

def meanAttrValue(events, attr):
    sum = 0;
    for event in events:
        if attr in event:
            sum += event[attr];
    mean = sum / len(events)
    return mean;

def medianAttrValue(events, attr):
    values = extractAttrValues(events, attr);
    return median(values);

def modeAttrValue(events, attr):
    values = extractAttrValues(events, attr);
    return median(values);

def extract(events, attr):
    return [event[attr] for event in events if attr in event];

def toCSV(json_data, common_attributes, filepath):
    '''Takes JSON data and converts it into a .csv file in an
    excel dialect'''
    with open(filepath, 'w') as csvfile:
        out = csv.DictWriter(csvfile, common_attributes, delimiter = ",", dialect='excel');
        for event in json_data:
            out.writerow(event);
            
def sortByAttribute(events, attr):
    return sorted(events, key=itemgetter(attr));

def runtime(events):
    '''Gets the runtime of a list events. This assumes events were derived
    from chrome's profiler, which is already in sorted order'''
    starttime = events[0]["ts"];
    endtime = events[len(events) - 1]["ts"];
    return endtime - starttime;

