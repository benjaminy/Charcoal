import matplotlib.pyplot as pyplot
from csv import DictReader
from datautil import findFile, parseCmdLnArgs, _flagged, _flaggedRetArg, log, extract, readCSV
import sys
from os.path import join
 
def main(argv):
    opts, args = parseCmdLnArgs(argv,"ho:sdg:", ["help",  "outdir=", "show", "debug"])
    _handleHelpFlag(opts)
    _selectGraph()
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
    
def _selectGraph():
    options = [("Scatter Plot", scatterPlot),
               ("Cumulative Distribution", cumulativeDistribution)]
              
    for index, opt in enumerate(options): log(options[index][0], tag = index + 1)
    plot = int(input("Select a plot: ")) - 1
    
    func_index = 1
    return options[plot][func_index]

def scatterPlot():
    pass

def cumulativeDistribution():
    pass
    
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
    if not args:
        filepath = findFile()
        args.insert(0, filepath)
    
    else: filepath = args[0]
    
    data = readCSV(filepath)
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
    
    x_max = 10000.0
    x_min = 0.0
    
    y_max = 1.05
    y_min = -0.05
    
    figure, plot_one = pyplot.subplots()
    plot_one.set_xlabel("Function call duration (μs)")
    plot_one.set_ylabel("Cumulative Percentages of Total Function Duration", color = 'b')
    plot_one.set_xscale("log")
    plot_one.set_ylim(y_min, y_max)
    plot_one.plot(xs, ys2, "b.")

    plot_two = plot_one.twinx()
    plot_two.set_ylabel("Cumulative Percentage of Function Duration", color = 'r')
    plot_two.set_xscale("log")
    plot_two.set_ylim(y_min, y_max)
    plot_two.plot(xs, ys, "r.")

    
    if show: pyplot.show()
    return pyplot

if __name__ == "__main__":
    main(sys.argv[1:])