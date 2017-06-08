import tkinter
from functools import reduce;
from tkinter.filedialog import askopenfilename
import json

root = tkinter.Tk();
root.withdraw();

with open(askopenfilename(),'r') as jsonfile:
    data = json.load(jsonfile)

trace_events = data['traceEvents'];

v8_events = list(filter(lambda event: "v8" in event["cat"], trace_events));

def splitEvents(events, attr):
    categorized_events = {};
    for event in events:
        cur_event_key = event[attr];
        if(cur_event_key not in categorized_events):
            val = [event];
            categorized_events[cur_event_key] = val;
            #print(categorized_events);
        else: 
            val = categorized_events[cur_event_key];
        
        val.append(event);
    
    return categorized_events;

print(v8_events);
#durAvg = reduce(lambda e1, e2: e1["dur"] + e2["dur"], v8_events);

base = 0;
print(v8_events[0]);
for event in v8_events:
    if "dur" in event: base += event['dur'];
    
avg = base / len(v8_events);
print(avg);