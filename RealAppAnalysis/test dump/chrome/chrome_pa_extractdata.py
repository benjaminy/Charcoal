import datautil
import matplotlib.pyplot as pyplot

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

def getCallDurationDensityData(durations):
    xs = []
    ys = []
    cumulative_time = 0.0
    percent = 0.0
    
    for dur in durations:
        #Credit to Blake
        xs.append(float(dur))
        cumulative_time += float(dur)
        percent = float(cumulative_time) / float(fcall_total_duration)
        ys.append(percent)
        
    return (xs, ys)

def getDurationPercentageData(durations):
    ys = []
    num = 0
    percent = 0.0
    
    for dur in durations:
        num += 1
        percent = float(num) / float(fcall_total_duration)
        ys.append(percent)
        
    return ys

def graph(xs, ys, ys2):
    
    figure, axis_one = pyplot.subplots()
    axis_one.plot(xs, ys2, "b.")
    axis_one.set_xlabel("Function call duration (μs)")
    axis_one.set_ylabel("Cumulative Percentage of Total Function Duration")
    axis_one.set_xscale("log")
    
    axis_two = axis_one.twinx()
    axis_two.plot(xs, ys, "r.")
    axis_two.set_ylabel("Cumulative Percentage of Duration Time", color = 'b')
    axis_two.set_xscale("log")
    
    pyplot.show()


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

global fcall_total_duration 
fcall_total_duration = datautil.runtime(functions)
durations = sorted(getDurations(functions))

print("Cumulative Function Call Duration: " + str(fcall_total_duration))
print("Durations: \n" + str(durations))

datautil.toTxt(process_of_interest, "test_out", subdir = "out")
datautil.toTxt(functions, "test_functions_out", subdir = "out")
datautil.toTxt(durations, "test_fdur_out", subdir = "out")

data = getCallDurationDensityData(durations)
data2 = getDurationPercentageData(durations)
graph(data[0], data[1], data2)

#pyplot.plot(data[0], data[1])
#pyplot.xlabel("Function call duration (μs)") 
#pyplot.xscale('log')
