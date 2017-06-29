from datautil import findFile
from datautil import filterEvents
from datautil import toTxt
import json

def printKeys(dict):
    try:
        print(dict.keys())
    except:
        print("Not a Dict Object")
    
def printList(list, indent = 0):
    spacing = ""
    for i in range(indent): spacing += " "
    
    for elem in list:
        try:
            elem[0]
            printList(list)
        except:
            print(spacing + str(elem))

manual = True
if(manual): data_path = findFile()
else:       data_path = 'C:/Users/Miguel Guerrero/git/Charcoal/RealAppAnalysis/sample traces/test page/simple action.json'

with open(data_path, "r") as json_file:
    data = json.load(json_file)
    
cpu_profile = filterEvents(data, "name", "CpuProfile")
print("Length of in: %d" % len(cpu_profile))
focus = cpu_profile[0]["args"]["data"]["cpuProfile"]["nodes"]
print("Length: %d" % len(focus))

#print(focus)


    
    