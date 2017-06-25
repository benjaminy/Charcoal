import datautil
import csv

trace_profile = datautil.getJSONDataFromFile()
profile_by_pids = datautil.splitList(trace_profile, "pid")
pids = profile_by_pids.keys()
print(pids)

valid_key = False
while not valid_key:
    key = int(input("Key: "))
    valid_key = key in profile_by_pids
        
process_of_interest = profile_by_pids[key]
functions = datautil.filterEvents(process_of_interest, "name", "FunctionCall")
formatted = [{"name": func["name"], "args": func["args"] } for func in functions if func["ph"] == "B"]
print(formatted)
global fcall_total_duration 
fcall_total_duration = datautil.runtime(functions)
durations = datautil.getDurations(functions)

print("Cumulative Function Call Duration: " + str(fcall_total_duration))

#datautil.toTxt(process_of_interest, "test_out", subdir = "out")
#datautil.toTxt(functions, "test_functions_out", subdir = "out")
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

toCSV(functions, "data_out.csv")

def getFunctionName(fcall_event):