import re

countAPIs = 0
apis = []

def log(log, indent = 1, tag = ""):
    log = str(log)
    if tag: log = tag + ": " + log
    print("\t" * indent + log)

badges = {" icon-beaker\n": "Experimental", 
          " None\n": "Standard", 
          " icon-trash\n": "Obsolete",
          " icon-warning-sign\n" : "Non-standardize",
          " icon-thumbs-down-alt\n": "Deprecated"}

def parseAPIHeader(line):
    API = {}
    #Hacking is fun...
    name_and_badge = line[5:].replace("(", "").replace(")", "").replace("\'", "").split(",")
    
    #Using [1:] to get rid of space at index 0...
    API["name"] = name_and_badge[0][1:]
    API["status"] = badges[name_and_badge[1]]
    return API
    
with open("out.txt", "r") as file:
    current_api = []
    for line in file:
        if "API" in line:
            name_and_status = parseAPIHeader(line)
            current_api = [line]
            apis.append(current_api)
        
        current_api.append(line)

log(len(apis), indent = 0, tag = "Total API Count")

log("Async APIs")
async_apis = []
for api in apis:
    def removeTag(name): return name[6:]
    name = removeTag(api[0])
    print_buffer = [name]
    async_methods = []
    for i, method in enumerate(api):
        if "Async" in method:
            async_method = api[i - 1] #Comes before indicator
            async_methods.append((async_method, method))
            print_buffer.append(method)
            print_buffer.append(async_method)
            
    if async_methods:
        print(api)
        async_apis.append({"name": api[5:], "methods": async_methods})
        #log(print_buffer[0], indent = 1)
        index_of_methods = 1;
        #for line in print_buffer[index_of_methods:]: log(line, indent = 0)
        
log(len(async_apis), indent = 0, tag = "Async API Count")

            
def methodsStats(async_apis):
    explicit = 0
    callbacks = 0
    promise = 0
    for api in async_apis:
        for m in api["methods"]:
            if "async" in m[1]: explicit += 1
            if "callback" in m[1]: callbacks += 1
            if "promise" in m[1]: promise += 1

    log(explicit + callbacks + promise, tag = "Total Async Methods")
    log(explicit, indent = 2, tag = "Async")
    log(callbacks, indent = 2, tag = "Callbacks")
    log(promise, indent = 2, tag = "Promise")       
    
methodsStats(async_apis)