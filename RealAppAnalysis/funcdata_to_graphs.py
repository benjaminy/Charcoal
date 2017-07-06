import matplotlib.pyplot as pyplot
from csv import DictReader
from datautil import findFile, parseCmdLnArgs, _flagged, _flaggedRetArg, log, extract
import sys
from os.path import join
from _operator import index
from matplotlib.testing.jpl_units.Duration import Duration

def main(argv):
    opts, args = parseCmdLnArgs(argv,"ho:sd", ["help",  "outdir=", "show", "debug"])
    _handleHelpFlag(opts)
    
    graph_data = _formatData(_loadFile(args))
    xs, ys = getCumulativeDurationPercentages(graph_data, sum(graph_data))
    ys2 = getFunctionAccumulation(graph_data)

    show_graph = _flagged(opts, "-s", "--show")
    figure = graph(xs, ys, ys2, show = show_graph)
    filepath = _handleOutFlag(opts, args)
    figure.savefig(filepath, indent = 2, format = "png")
    
    debug = _flagged(opts, "-d", "--debug")
    if debug:
        log(opts, indent = 2, tag = "opts")
        log(args, indent = 2, tag = "args")
        log(filepath, indent = 2, tag = "Output")
    
def _handleHelpFlag(opts):
    if _flagged(opts, "-h", "--help"):
        _usage()
        sys.exit(2)
        
def _handleOutFlag(opts, args):
    index_extension = -3
    filename = args[0][:index_extension] + "png" 
    outdir = _flaggedRetArg(opts, "-o", "--outdir")
    if outdir:
        filename = filename.split("//")[-1]        
    return join(outdir, filename)

def _loadFile(args):
    def readCSV(dict_csv): return list(DictReader(dict_csv))
    if not args:
        file = findFile()
        args.insert(0, file)
        with open(file, 'r') as funcdata_csv:
            data = readCSV(funcdata_csv)
    else:
        filepath = args[0]
        data = readCSV(filepath)
        
    print(list(data[0]))
    return data
    
def _formatData(data):
    durations_string_repr = extract(data, "duration")
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

def _usage():
    pass

def graph(xs, ys, ys2, show = False, outdir = ""):
    
    x_max = 1000000.0
    x_min = 0.0
    
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
    
    if show: pyplot.show()
    return pyplot
    
if __name__ == "__main__":
    main(sys.argv[1:])