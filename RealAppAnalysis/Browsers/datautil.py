'''Utility functions to search, filter, and manipulate output data of Chrome's event tracer'''

import tkinter
from tkinter.filedialog import askopenfilename
import json
import csv
import os;
from getopt import getopt, GetoptError
import sys

def findFile():
    #Gets rid of a GUI element native to tkinter
    root = tkinter.Tk();
    root.withdraw();
    
    return askopenfilename()

def categorizeByValue(dict_list, key, trim = False):
    '''Categorizes a list of dictionary objects by sorting them
       into lists whose elements share a common value'''
    categorized_items = {};
    for dict in dict_list:
        category_key = dict[key];
        
        if(category_key not in categorized_items):       
            val = [];
            categorized_items[category_key] = val;
        else: 
            val = categorized_items[category_key];
        
        val.append(dict);
        
        #Removes the attribute of the item, since it may be inferred
        #by the category in which the item is placed
        if trim: del dict[key]
    
    return categorized_items;

def filterEvents(events, attr, val):
   return [event for event in events if attr in event and event[attr] == val]

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
        
def runtime(events, attr = "ts"):
    '''Gets the runtime of a list events. This assumes events were derived
    from chrome's profiler, which is already in sorted order'''
    starttime = events[0][attr];
    endtime = events[-1][attr];
    return endtime - starttime;

def trimDict(dict, direct = True, *attrs):
    """
    Removes key:value pairs from a dict
        If a direct trim, the provided attributes will be removed from the dict
        If an indirect trim, the provided attributes will be kept, and all other 
        attributes will be deleted
    """
    if direct:
        for attr in attrs:
            if attr in dict: 
                del(dict[attr])
    else:
        for key in dict: 
            if key not in attrs:
                del(dict[key])
                
    return dict
    
def log(val, indent = 1, tag = ""):
    if tag: tag = str(tag) + ": "
    print(("\t" * indent) + tag + str(val))
   
def parseCmdLnArgs(argv, short_flags, long_flags = [], usage = None):
    try: return getopt(argv, short_flags, long_flags)
    except GetoptError:
        log("Unrecognized option")
        if usage: usage()
        sys.exit()
         
def findFileFromCWD(filename, root_subdir = ""):
    '''Searches the current working directory for the specified file and returns
    its file path.
    
    If provided a root subdirectory, the search will begin in that
       directory if it exists in the the current working directory'''
    for filepath, sub_directories, files in os.walk(root_dir):
        for file in files:
            if file == filename: return join(filepath, file)

def getJSONDataFromFile(file, find = False):
    '''Returns the the data from a .json file.
       If find is toggled, the user is prompt to find the file through a
       file explorer'''
    
    if find: file = findFile()
    try:
        with open(file,'r') as jsonfile:
            data = json.load(jsonfile)
    except:
        return None
    
    return data;

def readCSV(path): 
     with open(path, 'r') as fcsv:
        return list(csv.DictReader(fcsv))
    
def _flagged(opts, *flags):
    '''Checks for whether any of the flags are found in the argumented options'''
    for opt in opts:
        for f in flags:
            if f in opt:
                return True
            
    return False

def _flaggedRetArg(opts, *flags):
    '''Checks for whether any of the flags are found in the argumented options and 
    returns the argument of a found flag, or an empty string'''

    for opt in opts:
        for f in flags:
            if f in opt: 
                return opt[1]
                
    return ""              
