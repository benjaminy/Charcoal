import datautil
import os
from functools import reduce

trace_events = datautil.getJSONData()['traceEvents'];
v8_events = datautil.filterEvents(lambda event: "v8" in event["cat"], trace_events);

print(v8_events);
#durAvg = reduce(lambda e1, e2: e1["dur"] + e2["dur"], v8_events);

base = 0;
print(v8_events[0]);
for event in v8_events:
    if "dur" in event: base += event['dur'];
    
avg = base / len(v8_events);
print(avg);

print(os.getcwd());
datautil.toCSV(v8_events, v8_events[0].keys(), os.getcwd() + "//test.csv");