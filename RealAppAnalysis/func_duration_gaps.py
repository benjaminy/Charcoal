import tkinter
from tkinter.filedialog import askopenfilename
from csv import DictReader

def findFile():
    #Gets rid of a GUI element native to tkinter
    root = tkinter.Tk();
    root.withdraw();
    return askopenfilename()

def readCSVFuncData(funcdata_csv):
    data = DictReader(funcdata_csv)
    func_data = []
    for row in data: func_data.append(row)
    return func_data

data_csv = findFile()
with open(data_csv, "r") as funcdata_csv:
    data = readCSVFuncData(funcdata_csv) 


pos = 0
cumulutive_gap = 0

for i in range(0, len(data) - 2):

    cumulative_duration = 0
    f1 = data[i]
    f2 = data[i + 1]
    f1_end = float(f1["end time"])
    f2_start = float(f2["start time"])
    if(f1_end != f2_start):
        print("Discontinuity at: %d" % i)
        gap = f2_start - f1_end
        print("Gap: %f" % gap)
        cumulutive_gap += gap
        cumulative_duration = f1["duration"] + f2["duration"]


print("Function Duration: %f" % cumulative_duration)
print("Total gap time: %f" % cumulutive_gap)