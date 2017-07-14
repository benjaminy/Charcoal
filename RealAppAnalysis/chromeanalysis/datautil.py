'''Utility functions to search, filter, and manipulate output data of Chrome's event tracer'''

import tkinter
from tkinter.filedialog import askopenfilename
import json
import csv
import os;

def findFile():
    #Gets rid of a GUI element native to tkinter
    root = tkinter.Tk();
    root.withdraw();
    return askopenfilename()

def splitList(events, attr):
    '''Split a set of events into a set of lists whose elements differ 
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


def filterEvents(events, attr, val): return [event for event in events if event[attr] == val]
def extract(events, attr): return [event[attr] for event in events if attr in event];

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
        
def runtime(events, attr = "ts"):
    '''Gets the runtime of a list events. This assumes events were derived
    from chrome's profiler, which is already in sorted order'''
    starttime = events[0][attr];
    endtime = events[-1][attr];
    return endtime - starttime;

def readCSVFuncData(funcdata_csv):
    return list(csv.DictReader(funcdata_csv))
