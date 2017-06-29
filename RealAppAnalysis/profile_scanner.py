from os import listdir, getcwd, walk
from os.path import isfile, isdir, join
from json import load
import datautil
    
def getCPUProfileNodes(data): return data[-1]["args"]["data"]["cpuProfile"]["nodes"]
def log(str, indent = 1): print(("\t" * indent) + str) 

def onAllDataInDir(root_dir, func):
    isDataFile = lambda filename: filename.endswith(".json")
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
                    
            func(data)         
    
def examineProfileAccuracy(data):
    cpu_profile = data[-1]
    fcall_count_in_cpuprofile = len(cpu_profile["args"]["data"]["cpuProfile"]["nodes"])
    fcall_count_in_data = 0
    
    for event in data:
        if event["name"] == "FunctionCall": 
            fcall_count_in_data += 1
    
    print("Number of function calls")
    print("CPU Profile: %d Data: %d" % (fcall_count_in_cpuprofile, fcall_count_in_data))
    
    
def compareCPUProfileToFunctionEvents(data):
    cpu_profile_nodes = getCPUProfileNodes(data)
    cpu_profile_fnames = [node["callFrame"]["functionName"] for node in cpu_profile_nodes]
    functions = datautil.filterEvents(data, "name", "FunctionCall")
    #print(functions)
    func_events_names = [function["args"]["data"]["functionName"] for function in functions if function["args"]]
    
    cpu_set = set(cpu_profile_fnames)
    event_set = set(func_events_names)
    
    intersection = cpu_set.intersection(event_set)
    log("Intersection: " + str(intersection))
    log("Count: %d" % (len(intersection)), indent = 2)

    dif = cpu_set.difference(event_set)
    log("Not in Function Events: " + str(dif))
    log("Count: %d" % (len(dif)), indent = 2)
    
    dif = event_set.difference(cpu_set)
    log("Not in Profile: " + str(dif))
    log("Count: %d" % (len(dif)), indent = 2)
    #functions = datautil.filterEvents(data, "name", "FunctionCall")

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
onAllDataInDir(main, compareCPUProfileToFunctionEvents)