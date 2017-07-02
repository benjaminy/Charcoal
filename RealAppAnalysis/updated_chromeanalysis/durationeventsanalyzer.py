
''' Takes as input a list of durationevents and performs analysis on it.
The functions are fundamentally flawed if complete events are to be considered.'''

def main():
    pass

def normalize(initial_ts):
    normalized_times = []
    start = start_times[0]
    for starttime in starttimes:
        normalized_times.append(starttime - initial_ts)
    return normalized_times

def total_runtime(events):
    starttime = events[0]["start time"];
    endtime = events[-1]["end time"];
    return float(endtime) - float(starttime);

def total_functiontime(durationevents):
    sum=0
    for durevent in durationevents:
        sum += durevent["duration"]
    return sum

def gaps_list(durationevents):
    gaps = []
    for i in range(0, len(durationevents) - 1):

        f1 = durationevents[i]
        f2 = durationevents[i + 1]
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

def gaps_cumulativetime(gaps):
    cum_time=0
    for gap in gaps:
        cum_time += gap
    return cum_time

def idle_percent(total_gaptime, total_functiontime):
    return ( total_gaptime / (total_gaptime + total_functiontime) )


def density_data(durationevents, total_functiontime):
    xs = []
    ys = []
    cumulative_time = 0.0
    percent = 0.0

    for durevent in durationevents:
        duration = durevent["duration"]
        xs.append(duration)
        cumulative_time += dur
        ys.append(cumulative_time / total_functiontime)

    return (xs, ys)

def frequency_data(durationevents, total_functiontime):

    ys = []
    number_of_calls = len(durationevents)

    for i in range(1, length+1):
        ys.append(float(i) / float(number_of_calls))

    return ys


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

if __name__ == '__main__':
    main()
