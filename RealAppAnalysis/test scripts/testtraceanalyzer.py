import datautil
import os
from functools import reduce

trace_profile = datautil.getJSONDataFromFile();
trace_events = trace_profile['traceEvents'];
duration = trace_events[len(trace_events) - 1]["ts"] - trace_events[0]["ts"];
print(duration);
events_by_pid = datautil.splitEvents(trace_events, "pid");
data = [event for event in events_by_pid[15540] if "v8" in event["cat"]];
rt = datautil.runtime(data);
print(rt);
#v8_events = datautil.filterEvents(lambda event: "v8" in event["cat"], trace_events);
v8_events = [event for event in trace_events if "v8" in event["cat"]];
datautil.toCSV(v8_events, v8_events[0].keys(), os.getcwd() + "//test.csv");

