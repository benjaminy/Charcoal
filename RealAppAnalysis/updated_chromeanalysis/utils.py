import json

def trim_dic(dic, *argv):
    for arg in argv:
        del dic[arg]
    return dic

def load_profile_from_file(filepath):
    with open(filepath) as json_data:
        profile = json.load(json_data)
    return profile
