import matplotlib.pyplot as pyplot
from csv import DictReader
import datautil
import csv

def readCSVFuncData(funcdata_csv):
    data = DictReader(funcdata_csv)
    func_data = []
    for row in data: func_data.append(row)
    return func_data

def getDensityData(durations, fcall_total_duration):
    xs = []
    ys = []
    cumulative_time = 0.0
    percent = 0.0
    
    for dur in durations:
        #Credit to Blake
        xs.append(dur)
        cumulative_time += dur
        percent = cumulative_time / fcall_total_duration
        print("Cumulative: %d Total: %d" % (cumulative_time, fcall_total_duration))
        ys.append(percent)
        
    return (xs, ys)

def getFrequencyData(durations, fcall_total_duration):
    
    ys = []
    num = 0
    percent = 0.0
    
    for dur in durations:
        num += 1
        percent = float(num) / float(fcall_total_duration)
        ys.append(percent)
        
    return ys

def graph(xs, ys, ys2):
    
    x_max = 100000.0
    x_min = 0.0
    
    y_max = 1.05
    y_min = -0.05
    
    _, plot_one = pyplot.subplots()
    plot_one.plot(xs, ys2, "b.")
    plot_one.set_xlabel("Function call duration (μs)")
    plot_one.set_ylabel("Cumulative Percentage of Total Function Duration")
    plot_one.set_xscale("log")
    plot_one.set_ylim([y_min, y_max])
    plot_one.set_xlim([x_min, x_max])

    plot_two = plot_one.twinx()
    plot_two.plot(xs, ys, "r.")
    plot_two.set_ylabel("Cumulative Percentage of Duration Time", color = 'b')
    plot_two.set_xscale("log")
    plot_two.set_xscale('log')
    plot_two.set_ylim([y_min, y_max])
    plot_two.set_xlim([x_min, x_max])
    
    pyplot.show()
    
    
def main():
    with open(datautil.findFile(), 'r') as funcdata_csv:
        data = readCSVFuncData(funcdata_csv)
    
    fcall_total_duration = float(data[-1]["end time"]) - float(data[0]["start time"])
    print(fcall_total_duration)
    durations_string_repr = datautil.extract(data, "duration")
    just_func_durations = [float(duration) for duration in durations_string_repr]
    
    sorted_durations = sorted(just_func_durations)
    datautil.toTxt(sorted_durations, "sorted_durations")
    xs, ys = getDensityData(sorted_durations, fcall_total_duration)
    ys2 = getFrequencyData(sorted_durations, fcall_total_duration)
    graph(xs, ys, ys2)
    
main()