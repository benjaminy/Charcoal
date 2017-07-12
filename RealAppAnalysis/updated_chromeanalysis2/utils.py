import json
import os

def trim_dic(dic, *argv):
    for arg in argv:
        del dic[arg]
    return dic

def load_profile_from_file(filepath):
    with open(filepath) as json_data:
        profile = json.load(json_data)
    return profile

def newfilepath(newhead, inputfilename, newextension):
    _, tail = os.path.split(inputfilename)
    newtail = tail.split('.')[0] + newextension
    return newhead + newtail

def sort_by_attribute(list_of_dic, attribute):
    return sorted(list_of_dic, key=lambda k: k[attribute])

def log(val, indent = 1, tag = ""):
    if tag: tag = str(tag) + ": "
    print(("\t" * indent) + tag + str(val))
