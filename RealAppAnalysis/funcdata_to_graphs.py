import matplotlib.pyplot as pyplot
from csv import DictReader
from datautil import findFile, parseCmdLnArgs, _flagged
import datautil
import sys

def main(argv):
    opts, args = parseCmdLnArgs(argv,"hos:", ["help",  "outdir=", "show"])
    
    _handleHelpFlag(opts)
    graph_data = _formatData(_loadFile(args))
    fcall_total_duration = sum(graph_data)
    
    xs, ys = getCumulativeDurationPercentages(graph_data, fcall_total_duration)
    ys2 = getFunctionAccumulation(graph_data)
    graph(xs, ys, ys2)
    
def readCSV(dict_csv):
    return list(DictReader(dict_csv))

def _handleHelpFlag(opts):
    if _flagged(opts, "-h", "--help"):
        _usage()
        sys.exit(2)

def _handleOutputFlag(opts):
    pass

def _loadfile(args):
    if not args:
        with open(findFile(), 'r') as funcdata_csv:
            return readCSV(funcdata_csv)
    else:
        filepath = args[0]
        return readCSV(filepath)
    
def _formatData(data):
    durations_string_repr = datautil.extract(data, "duration")
    just_func_durations = [float(duration) for duration in durations_string_repr]
    return sorted(just_func_durations)

def getCumulativeDurationPercentages(durations, fcall_total_duration):
    xs = []
    ys = []
    cumulative_time = 0.0
    percent = 0.0
    
    for dur in durations:
        #Credit to Blake
        xs.append(dur)
        cumulative_time += dur
        percent = cumulative_time / fcall_total_duration
        ys.append(percent)
                
    return (xs, ys)

def getFunctionAccumulation(durations):
    
    ys = []
    num = 0
    percent = 0.0
    num_of_durations = len(durations)
    for dur in durations:
        num += 1
        percent = float(num) / float(num_of_durations)
        ys.append(percent)
        
    return ys

def graph(xs, ys, ys2):
    
    x_max = 1000000.0
    x_min = -1000.0
    
    y_max = 1.05
    y_min = -0.05
    
    _, plot_one = pyplot.subplots()
    plot_one.plot(xs, ys2, "b.")
    plot_one.set_xlabel("Function call duration (μs)")
    plot_one.set_ylabel("Cumulative Percentages of Total Function Duration")
    plot_one.set_xscale("log")
    plot_one.set_ylim([y_min, y_max])
    plot_one.set_xlim([x_min, x_max])

    plot_two = plot_one.twinx()
    plot_two.plot(xs, ys, "r.")
    plot_two.set_ylabel("Cumulative Percentage of Function Duration", color = 'b')
    plot_two.set_xscale("log")
    plot_two.set_ylim([y_min, y_max])
    plot_two.set_xlim([x_min, x_max])
    
    pyplot.show()
    
if __name__ == "__main__":
    main(sys.argv[1:])