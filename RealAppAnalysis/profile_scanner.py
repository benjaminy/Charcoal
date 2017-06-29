from os import listdir, getcwd, walk
from os.path import isfile, isdir, join
from json import load
import datautil
    
def getCPUProfile(data): return data[-1]["args"]["data"]["cpuProfile"]["nodes"]

def onAllDataInDir(root_dir, func):
    isDataFile = lambda filename: filename.endswith(".json")
    for (filepath, sub_directories, files) in walk(root_dir):
        print("Filepath: " + filepath)
        print("Subdirectories: " + str(sub_directories))
        print("Files: " + str(files))
        for file in files:
            if(isDataFile(file)):
                try:
                    with open((join(filepath, file)), 'r') as data_file: 
                        data = load(data_file)
                        print(file)
                        func(data)
                except:
                    print("Error in processing data file: " + file)               
    
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
    cpu_profile = getCPUProfile(data)
    cpu_profile_fnames = [node["callFrame"]["functionName"] for node in cpu_profile]
    functions = datautil.filterEvents(data, "name", "FunctionCall")
    print(functions)
    events_fnames = [function["args"]["data"]["functionName"] for function in functions]
    print(events_fnames)
        
    #functions = datautil.filterEvents(data, "name", "FunctionCall")

def getTopLevelFunctions(cpu_profile):
    '''Returns all the functions that are not called by other functions from a CPU_Profile
       Ex:
           Let there be functions a, b, abd c
           If function A calls B, and function B calls C during runtime
           This function would return A
    '''
    print("Nodes: " + str(len(cpu_profile)))
    root = cpu_profile[0]
    IDS_of_high_level_functions = root["children"]
    high_level_functions = [function for function in cpu_profile if function["id"] in IDS_of_high_level_functions]
    print(len(high_level_functions))
    print(len(IDS_of_high_level_functions))
    print(high_level_functions)
    
onAllDataInDir(getcwd() + "/sample traces/test page", compareCPUProfileToFunctionEvents)