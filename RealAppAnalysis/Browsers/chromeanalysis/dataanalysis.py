import csv
import json
import matplotlib.pyplot as plt
from csv import DictReader
import os
import utils


def generate_graph(input_csvfile):
    with open(input_csvfile, 'r') as funcdata_csv:
        data = readCSVFuncData(funcdata_csv) #list of dictionairies from csv file, which contains the condenced form of relevant event objects

    #fcall_total_duration = runtime(data)
    start_times = normalize( sorted( extract(data, "start time") ) ) #--> return list of the values of key "duration"
    simple_graph( utils.newpath("outputgraphs/", input_csvfile, "simple.png"), start_times)

    durations = sorted( extract(data, "duration") ) #--> return list of the values of key "duration"
    total_durationtime = cum_functionduration(data)
    xs, ys = getDensityData(durations, total_durationtime)
    ys2 = getFrequencyData(durations, total_durationtime)
    create_graph(utils.newpath("outputgraphs/", input_csvfile, ".png" ), xs, ys, ys2)

    gaps = gaps_list(data)
    print gaps_count(gaps)

    print (runtime(data)) / (gaps_cumulative(gaps) + cum_functionduration(data))

    if ( runtime(data) == gaps_cumulative(gaps) + cum_functionduration(data) ):
        print "match"
    else:
        print "problem"

    print gaps_cumulative(gaps) / runtime(data)





def readCSVFuncData(funcdata_csv):
    data = DictReader(funcdata_csv) #interpret the dictionairy of the csv file --> returns an iterable object list of dictionairy
    func_data = []
    for row in data: func_data.append(row)
    return func_data # --> a list of dictionairies

def getDensityData(durations, fcall_total_duration):
    xs = []
    ys = []
    cumulative_time = 0.0
    percent = 0.0

    for dur in durations:
        xs.append(dur)
        cumulative_time += dur
        percent = cumulative_time / fcall_total_duration
        ys.append(percent)

    return (xs, ys)

def getFrequencyData(durations, fcall_total_duration):

    ys = []
    length = len(durations)

    for i in range(1, length+1):
        ys.append(float(i) / float(length))

    return ys

def normalize(start_times):
    modified = []
    start = start_times[0]
    for time in start_times:
        modified.append(time - start)
    return modified

def runtime(events):
    starttime = events[0]["start time"];
    endtime = events[-1]["end time"];
    return float(endtime) - float(starttime);

def cum_functionduration(data):
    durations = extract(data, "duration")
    sum=0
    for dur in durations:
        sum += dur
    return sum

def gaps_list(functions_csv):
    gaps = []
    for i in range(0, len(functions_csv) - 1):

        f1 = functions_csv[i]
        f2 = functions_csv[i + 1]
        f1_end = float(f1["end time"])
        f2_start = float(f2["start time"])

        if(f1_end != f2_start):
            gap = f2_start - f1_end
            gaps.append(gap)
    return gaps


def gaps_count(gaps):
    count=0
    for gap in gaps:
        count+=1
    return count

def gaps_cumulative(gaps):
    cum_time=0
    for gap in gaps:
        cum_time += gap
    return cum_time

def idle_percent(cum_gaptime, cum_funcduration):
    return ( cum_gaptime / (cum_gaptime + cum_funcduration) )

def simple_graph(path, xs):
    ys=[]
    for i in range(len(xs)):
        ys.append(0)

    plt.figure(2)
    plt.plot(xs, ys, 'ro')
    plt.axis([0, 1000, 0, 1])
    plt.savefig(path)


def create_graph(path, xs, ys, ys2):
    plt.figure(1)

    x_max = 100000.0
    x_min = 0.0

    y_max = 1.05
    y_min = -0.05

    _, plot_one = plt.subplots()
    plot_one.plot(xs, ys2, "b.")
    plot_one.set_xlabel("Function call duration (microsec)")
    plot_one.set_ylabel("Cumulative Percentage of Total Function Duration")
    plot_one.set_xscale("log")
    plot_one.set_ylim([y_min, y_max])
    plot_one.set_xlim([x_min, x_max])

    plot_two = plot_one.twinx()
    plot_two.plot(xs, ys, "r.")
    plot_two.set_ylabel("Cumulative Percentage of Duration Time", color = 'b')
    plot_two.set_xscale("log")
    plot_two.set_ylim([y_min, y_max])
    plot_two.set_xlim([x_min, x_max])

    plt.savefig(path)


def extract(events, attr):
    return [float(event[attr]) for event in events if attr in event];
