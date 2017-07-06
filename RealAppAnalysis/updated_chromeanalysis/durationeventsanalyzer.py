import matplotlib.pyplot as plt

''' Takes as input a list of durationevents, belonging to a certain thread and process, and performs analysis on it.
NOTE: Some parts of the analysis are fundamentally flawed if complete events should be taken into account.

Prove the following:
Many applications are composed of tons of extremely short callbacks.
Sometimes there are chains of callbacks that typically execute in short succession.
If some other action happened in the middle of such a chain, it could cause a concurrency bug.

durationevents = [
{
  { args':
     {
       tileData':
         { layerId': 93,
           sourceFrameNumber': 311,
           tileId':
           {
             u'id_ref':
             u'0x7f860ad387b0'
           },

         tileResolution':
         u'HIGH_RESOLUTION'
         }
     },
   'cat': u'cc,disabled-by-default-devtools.timeline',
   'dur': '532',
   'end time': 27781510672,
   'name': u'RasterTask',
   'start time': 27781510140,
   'tts': 100325},
},

{}, {}...]

'''

def main():
    pass




def functioncalls_only(durationevents):
    fcalls = []
    for durevent in durationevents:
        if durevent["name"] == "FunctionCall":
            fcalls.append(durevent)
    return fcalls


def get_durations(durationevents):
    durations = []
    for durationevent in durationevents:
        durations.append(durationevent["dur"])
    return sorted(durations)

def get_starttimes(durationevents):
    starttimes = []
    for durationevent in durationevents:
        starttimes.append(durationevent["start time"])
    return sorted(starttimes)

def get_funcnames(durationevents):
    func_names = []
    for durevent in durationevents:
        if durevent["name"] == "FunctionCall":
            func_names.append(durevent["args"]["data"]["functionName"])
    return set(func_names)


def sort_events_according_to_starttime(durationevents):
    pass

def total_runtime(sorted_durationevents):
    starttime = sorted_durationevents[0]["start time"];
    endtime = sorted_durationevents[-1]["end time"];
    return endtime-starttime;


def gaptimes(sorted_durationevents):
    gaps = []
    for i in range(0, len(sorted_durationevents) - 1):

        f1 = sorted_durationevents[i]
        f2 = sorted_durationevents[i + 1]
        f1_end = float(f1["end time"])
        f2_start = float(f2["start time"])

        if(f1_end != f2_start):
            gap = f2_start - f1_end
            gaps.append(gap)
    return sorted(gaps)

def total_functiontime(durations):
    sum=0
    for dur in durations:
        sum += dur
    return sum

def total_idletime(gaps):
    cum_time=0
    for gap in gaps:
        cum_time += gap
    return cum_time

def idletime_percentage(total_gaptime, total_functiontime):
    return ( total_gaptime / (total_gaptime + total_functiontime) )

def distributiondata(start_times):
    ys = []
    number_of_calls = len( start_times )
    for i in range( number_of_calls ):
        ys.append( float(i+1)/float(number_of_calls) )

    return (start_times, ys)

def normalize_starttimes(start_times):
    normalized_starttimes = []
    for stime in start_times:
        normalized_starttimes.append(float(stime)-float(start_times[0]))
    return normalized_starttimes

def simpledata(start_times):
    xs = []
    ys = []

    first = start_times[0]

    for stime in start_times:
        xs.append(float(stime)-float(start_times[0]))
    for i in xrange( len(start_times) ):
        ys.append(0)

    return (xs, ys)


'''One of the lines is the number of events/function calls with duration less than X,
normalized to the total number of function calls in the trace. '''
def data1(durations):
    xs = []
    ys = []

    number_of_durevents = len(durations)
    for i in range( number_of_durevents ):
        xs.append( durations[i] )
        ys.append( float(i+1)/float(number_of_durevents) )

    #print ys
    return (xs, ys)

'''The other line is the total duration (sum) of all the event/function calls with duration less than X,
normalized to the total duration of all function calls in the trace. '''
def data2(durations):

    xs = []
    ys = []

    total_duration = total_functiontime(durations)
    cumulative_time = 0.0

    for dur in durations:
        cumulative_time += dur
        xs.append( dur )
        ys.append( float(cumulative_time)/float(total_duration) )

    return (xs, ys)



def create_graph(filepath, xs, ys, xmax):
    plt.plot(xs, ys, "ro")
    plt.axis([0, xmax, -0.05, 1.05])
    plt.savefig(filepath)
    plt.clf()

if __name__ == '__main__':
    main()
