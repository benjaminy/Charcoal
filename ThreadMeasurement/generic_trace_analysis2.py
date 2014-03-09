#!/usr/bin/env python

# TODO: Header stuff

# This program reads trace files that are sqlite formatted.  Schema
# described elsewhere

import argparse, sqlite3
import matplotlib.pyplot as plt

class Process(object):
    pass

class Thread(object):
    pass

class RawEvent(object):
    pass

class Event(object):
    pass

class Processor(object):
    pass

next_id     = 1
process_ids = {}
processes   = {}
threads     = {}

def get_fresh_id():
    global next_id
    id = next_id
    next_id = next_id + 1
    return id

def get_process(process_name):
    global process_ids
    global processes

    if process_name in process_ids:
        process = processes[process_ids[process_name]]
    else:
        process                   = Process()
        process.id                = get_fresh_id()
        process_ids[process_name] = process.id
        process.name              = process_name
        process.threads           = []
        process.thread_ids        = {}
        process.cpu_time          = 0
        processes[process.id]     = process
    return process

def get_thread(process, thread_name):
    global threads

    if thread_name in process.thread_ids:
        thread = threads[process.thread_ids[thread_name]]
    else:
        thread                          = Thread()
        thread.id                       = get_fresh_id()
        thread.name                     = thread_name
        process.thread_ids[thread_name] = thread.id
        thread.process                  = process
        thread.events                   = []
        thread.cpu_time                 = 0
        threads[thread.id]              = thread
        process.threads.append(thread)
    return thread

def parse_timestamp(row_csv):
    timestamp_s  = int(row_csv[0])
    timestamp_ms = int(row_csv[1])
    timestamp_us = int(row_csv[2])
    timestamp_ns = int(row_csv[3])
    # XXX: numerical overflow? I guess Python will use 64 bits automagically
    timestamp = timestamp_s * 1000 + timestamp_ms
    timestamp = timestamp   * 1000 + timestamp_us
    return      timestamp   * 1000 + timestamp_ns

def parse_event_kind(name):
    if name == "create":
        return EVKIND_CREATE
    elif name == "destroy":
        return EVKIND_DESTROY
    elif name == "start":
        return EVKIND_START
    elif name == "stop":
        return EVKIND_STOP
    else:
        raise Exception('weird event kind', name)

def compare_timestamps(x,y):
    return x.timestamp - y.timestamp

def is_app_proc( proc, processes ):
    return proc in processes or (1 > len(processes))

def is_app_thread( thread, processes ):
    return is_app_proc(thread[1], processes)

def main():
    argp = argparse.ArgumentParser(description="""Thread analyzer command line parser.  The format is: ...
        Hello""")
    argp.add_argument('trace_file', metavar='T',
                      help='Name of the input trace file')
    argp.add_argument('processes', metavar='P', type=int, nargs='*',
                      help='List of process IDs to consider part of the application')
    argp.add_argument('--background_app', dest='does_background_count', action='store_true',
                      help="If present, consider all processes to be part of the application")
    args = argp.parse_args()

    conn = sqlite3.connect(args.trace_file)
    c = conn.cursor()

    cores = {}
    active_intervals = []

    processes = {}
    for process in c.execute('SELECT * FROM processes'):
        pid    = process[0]
        pname  = process[1]
        pfname = process[2]
        processes[pid] = (pname, pfname, 0.0)

    threads = {}
    for thread in c.execute('SELECT * FROM threads'):
        tid   = thread[0]
        tname = thread[1]
        pid   = thread[2]
        threads[tid] = (tname, pid)

    active_interval_count = 0
    total_run_time = 0
    app_run_time = 0
    non_app_run_time = 0
    for event in c.execute('SELECT * FROM events ORDER BY timestamp, kind DESC, id'):
        event_id   = event[0]
        thread_id  = event[1]
        core       = event[2]
        event_kind = event[3]
        timestamp  = event[4]
        if event_kind == 2:
            if core in cores:
                duration = timestamp - cores[core]
                thread = threads[thread_id]
                proc = processes[thread[1]]
                total_run_time += duration
                processes[thread[1]] = (proc[0], proc[1], proc[2] + duration)
                if is_app_thread( thread, args.processes ):
                    active_interval_count += 1
                    app_run_time += duration
                    active_intervals.append((duration, thread_id))
                else:
                    non_app_run_time += duration
                del cores[core]
        elif event_kind == 1:
            cores[core] = timestamp
        # print event

    print "Active interval count:", active_interval_count
    if 0 < len(args.processes):
        print 'Ignored processes account for %.6f%% of total CPU time' % (100.0 * non_app_run_time / total_run_time)

    def cmp_snd_third((pid1, (a1, b1, c1)), (pid2, (a2, b2, c2))):
        diff = c1 - c2
        if diff < 0:
            return -1
        elif diff > 0:
            return 1
        else:
            return 0

    for (pid, proc) in sorted(processes.items(), cmp=cmp_snd_third):
        if is_app_proc( pid, args.processes ):
            print pid, proc, ('APP %6.2f' % (100.0 * proc[2] / app_run_time))
        else:
            print pid, proc, ('NON-APP %6.2f' % (100.0 * proc[2] / non_app_run_time))

    # The default comparison function should be fine, because we only
    # care about the first field of the tuple
    active_intervals_sorted = sorted(active_intervals)

    ai_count = 0
    cumm_run_time = 0
    just_active_intervals = []
    pct_run_time = []
    pct_active_intervals = []
    for (ai,t) in active_intervals_sorted:
        ai_count += 1
        cumm_run_time += ai
        pct_active = 100.0*ai_count     /active_interval_count
        pct_run    = 100.0*cumm_run_time/app_run_time
        # print ("%5.1f  %5.1f  %12d" % (pct_active, pct_run, ai)),
        # (tname, pid) = threads[t]
        # (pname, pname2) = processes[pid]
        # print ('%8d %20s'% (t,pname))
        just_active_intervals.append(ai)
        pct_run_time.append(pct_run)
        pct_active_intervals.append(pct_active)

    if True:
        fig = plt.figure()
        ax = fig.add_subplot(1,1,1)
        line,  = ax.plot(just_active_intervals, pct_run_time, color='blue', lw=2)
        line2, = ax.plot(just_active_intervals, pct_active_intervals, color='red', lw=2)
        ax.set_xscale('log')
    else:
        plt.plot(just_active_intervals, pct_run_time, 'ro', just_active_intervals, pct_active_intervals, 'b-')
        # plt.axis([0, 6, 0, 20])
    plt.show()


    # # It seems possible that the events will not be in chronological
    # # order, so do a shallow parse then sort them by timestamp
    # raw_events = []
    # for row_csv in trace_reader:
    #     # print row_csv
    #     raw_event              = RawEvent()
    #     raw_event.timestamp    = parse_timestamp(row_csv)
    #     raw_event.process_name = row_csv[4]
    #     raw_event.thread_name  = row_csv[5]
    #     raw_event.core         = int(row_csv[6])
    #     raw_event.kind         = parse_event_kind(row_csv[7])
    #     raw_events.append(raw_event)

    # raw_events_chronological = sorted(raw_events, cmp=compare_timestamps)

    # active_intervals = []

    # event_counter  = 0
    # timestamp_min  = raw_events_chronological[0].timestamp
    # timestamp_max  = raw_events_chronological[-1].timestamp
    # timestamp_prev = 0
    # cpu_time_total = 0
    # tlp_accum      = {}
    # processors     = {}
    # for raw_event in raw_events_chronological:
    #     process         = get_process(raw_event.process_name)
    #     thread          = get_thread(process, raw_event.thread_name)
    #     event           = Event()
    #     event.timestamp = raw_event.timestamp
    #     event.kind      = raw_event.kind
    #     event.core_id   = raw_event.core
    #     event.thread    = thread

    #     active_core_count = len(processors)
    #     if event_counter != 0:
    #         interval_len = event.timestamp - timestamp_prev
    #         if not active_core_count in tlp_accum:
    #             tlp_accum[active_core_count] = 0
    #         tlp_accum[active_core_count] += interval_len

    #     if event.kind == EVKIND_STOP:
    #         if event.core_id in processors:
    #             # TODO: check that it's the right threads
    #             del processors[event.core_id]
    #         else:
    #             # weird, but okay I guess
    #             pass

    #     if event.kind == EVKIND_START:
    #         processors[event.core_id] = event

    #     if event.kind == EVKIND_CREATE and len(thread.events) > 0:
    #         raise Exception('event before create', thread.name)

    #     if len(thread.events) > 0:
    #         prev_event = thread.events[len(thread.events) - 1]
    #         if event.kind == EVKIND_STOP:
    #             active_intervals.append((prev_event, event))
    #             cpu_time = event.timestamp - prev_event.timestamp
    #             cpu_time_total = cpu_time_total + cpu_time
    #             thread.cpu_time = thread.cpu_time + cpu_time
    #             p = thread.process
    #             if p != prev_event.thread.process:
    #                 raise Exception('process mismatch', p.name)
    #             p.cpu_time = p.cpu_time + cpu_time

    #     thread.events.append(event)
    #     timestamp_prev = event.timestamp
    #     event_counter = event_counter + 1

    # tlp = calculate_tlp(tlp_accum)


    # for interval in active_intervals:
    #     pass

    # for id, process in processes.items():
    #     print process.name, " ", (process)
    conn.close()

def calculate_tlp(data):
    tlp_numerator = 0
    tlp_denominator = 0
    for core_count, time in data.items():
        tlp_numerator += time * core_count
        if core_count > 0:
            tlp_denominator += time
        print core_count, time

    print "TLP:", float(tlp_numerator) / tlp_denominator

if __name__ == '__main__':
    main()


