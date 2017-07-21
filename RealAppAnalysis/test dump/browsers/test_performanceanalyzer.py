import datautil
from functools import reduce

trace_profile = datautil.getJSONDataFromFile();
pids = datautil.splitList(trace_profile, "pid");
timeline = [event for event in trace_profile if event["cat"] == "devtools.timeline"];
timeline_functions = [fcall for fcall in timeline if fcall["name"] == "FunctionCall"];
datautil.toTxt(timeline, "test_output");
datautil.toTxt(timeline_functions, "test_outputf");
