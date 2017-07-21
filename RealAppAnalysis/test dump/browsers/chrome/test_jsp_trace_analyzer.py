import datautil
import os
from functools import reduce

trace_profile = datautil.getJSONDataFromFile();
print(trace_profile.keys());
nodes = trace_profile["nodes"];
datautil.toTxt(nodes, "test_nodes");

start = trace_profile["startTime"];
end = trace_profile["endTime"];