import datautil
import os

getKeyManually = False;
trace_profile = datautil.getJSONDataFromFile();
print(trace_profile.keys());

if(getKeyManually):
    key = input("Key: ");
    datautil.toTxt(trace_profile[key], "out_" + key);
else:
    key = "meta"
    datautil.toTxt(trace_profile[key], "out_" + key);