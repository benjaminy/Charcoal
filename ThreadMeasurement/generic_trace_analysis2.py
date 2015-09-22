#!/usr/bin/env python

# TODO: Header stuff

# This program reads trace files that are sqlite formatted.  Schema
# described elsewhere

import argparse, sqlite3
import matplotlib.pyplot as plt

def main():
    args = read_command_line_args()
    (db_connection, db_cursor) = init_db(args.trace_file)

    cores            = {}
    processes        = {}
    threads          = {}
    app_processes    = []
    app_threads      = []
    read_process_table(db_cursor, processes)
    read_thread_table (db_cursor, threads, processes)

    for pid in args.processes:
        p = processes[pid]
        app_processes.append(p)
        app_threads += p.threads

    prev_timestamp        = -1
    tlp_app               = {}
    tlp_non_app           = {}
    for event_sql in db_cursor.execute('SELECT * FROM events ORDER BY timestamp, kind DESC, id'):
        event   = read_event(event_sql)
        thread  = threads[event.tid]
        process = processes[thread.pid]

        do_tlp_accounting(tlp_app, tlp_non_app, prev_timestamp, event,
                          cores, threads, app_threads)

        try:
            min_timestamp = min(min_timestamp, event.timestamp)
            max_timestamp = max(max_timestamp, event.timestamp)
        except NameError:
            min_timestamp = event.timestamp
            max_timestamp = event.timestamp

        if event.kind == 2:
            if event.core in cores:
                duration = event.timestamp - cores[event.core].timestamp
                thread.run_time += duration
                process.run_time += duration
                thread.intervals.append(duration)
                process.intervals.append(duration)
                del cores[event.core]
            else:
                print "Warning: No thread on core at end event", event
        elif event.kind == 1:
            if event.core in cores:
                print "Warning: Thread on core at begin event", event
            cores[event.core] = event
        else:
            print event
            raise Exception("Mystery event")
        prev_timestamp = event.timestamp

    print "Time range:", ((max_timestamp - min_timestamp)/1000000)

    calculate_tlp(tlp_app)

    app_run_time      = 0
    non_app_run_time  = 0
    app_intervals     = []
    non_app_intervals = []
    for (pid, p) in processes.items():
        if in_or_empty(p, app_processes):
            app_run_time += p.run_time
            for t in p.threads:
                def add_t(i):
                    return (i,t)
                app_intervals += map(add_t, t.intervals)
        else:
            non_app_run_time += p.run_time
            for t in p.threads:
                def add_t(i):
                    return (i,t)
                non_app_intervals += map(add_t, t.intervals)
    total_run_time  = app_run_time + non_app_run_time
    total_intervals = app_intervals + non_app_intervals

    print "App run time: %12d (%.2f%%)  App intervals: %12d (%.2f%%)" % (app_run_time, 100.0 * app_run_time / total_run_time, len(app_intervals), 100.0 * len(app_intervals) / len(total_intervals))

    if 0 < len(app_processes):
        print 'Ignored processes account for %.6f%% of total CPU time' % (100.0 * non_app_run_time / total_run_time)

    def cmp_proc_time((pid1, p1), (pid2, p2)):
        return cmp(p1.run_time, p2.run_time)

    print "Processes, sorted by run time"
    for (pid, proc) in sorted(processes.items(), cmp=cmp_proc_time):
        if in_or_empty(proc, app_processes):
            print proc, ('APP %6.2f' % (100.0 * proc.run_time / app_run_time))
        else:
            print proc, ('NON-APP %6.2f' % (100.0 * proc.run_time / non_app_run_time))

    def cmp_thread_time((tid1, t1), (tid2, t2)):
        diff1 = cmp(t1.process.run_time, t2.process.run_time)
        if diff1 == 0:
            return cmp(t1.run_time, t2.run_time)
        return diff1

    print "Threads, sorted by containing process's run time"
    for (tid, thread) in sorted(threads.items(), cmp=cmp_thread_time):
        print thread, ('%.4f' % (100.0 * thread.run_time / total_run_time))

    # The default comparison function should be fine, because we only
    # care about the first field of the tuple
    app_intervals_sorted = sorted(app_intervals)

    ai_count = 0
    cumm_run_time = 0
    just_active_intervals = []
    pct_run_time = []
    pct_active_intervals = []
    for (ai,t) in app_intervals_sorted:
        ai_count += 1
        cumm_run_time += ai
        pct_active = 100.0*ai_count     /len(app_intervals)
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
    db_connection.close()

# end of main()

def do_tlp_accounting(tlp_app, tlp_non_app, prev, event, cores, threads, app_threads):
    if prev != -1:
        duration = event.timestamp - prev
        active_cores_app = 0
        active_cores_non_app = 0
        for (core, cevent) in cores.items():
            if in_or_empty(threads[cevent.tid], app_threads):
                active_cores_app += 1
            else:
                active_cores_non_app += 1

        def dict_incr_default_0(dict, key, incr_val):
            try:
                dict[key] += incr_val
            except KeyError:
                dict[key] = incr_val
        dict_incr_default_0(tlp_app, active_cores_app, duration)
        dict_incr_default_0(tlp_non_app, active_cores_non_app, duration)

def in_or_empty(thing, things):
    return (thing in things) or not things

class Process(object):
    def __str__(self):
        return '{id=%8d, nt=%4d, rt=%12d, n=%30s, fn=%30s}' %(self.pid, len(self.threads), self.run_time, self.name, self.fname)
    def __unicode__(self):
        return u__str__(self)

def read_process_table(db_cursor, processes):
    for process_sql in db_cursor.execute('SELECT * FROM processes'):
        process           = Process()
        process.pid       = process_sql[0]
        process.name      = process_sql[1]
        process.fname     = process_sql[2]
        process.threads   = []
        # The run_time and intervals fields are redundant with the
        # same-named fields in the Thread objects.  They are here for
        # performance and readability reasons.
        process.run_time  = 0
        process.intervals = []
        processes[process.pid] = process

class Thread(object):
    def __str__(self):
        i = len(self.intervals)
        return '{id=%8d, n=%40s, p=%8d, rt=%12d, i=%12d, avg=%9d}' %(self.tid, self.name, self.pid, self.run_time, i, self.run_time / i if i > 0 else 0)
    def __unicode__(self):
        return u__str__(self)

def read_thread_table(db_cursor, threads, processes):
    for thread_sql in db_cursor.execute('SELECT * FROM threads'):
        thread           = Thread()
        thread.tid       = thread_sql[0]
        thread.name      = thread_sql[1]
        thread.pid       = thread_sql[2]
        thread.process   = processes[thread.pid]
        thread.run_time  = 0
        thread.intervals = []
        threads[thread.tid] = thread
        thread.process.threads.append(thread)

class Event(object):
    def __str__(self):
        return '{id=%8d, t=%4d, c=%2d, k=%d, ts=%12d}' %(self.eid, self.tid, self.core, self.kind, self.timestamp)
    def __unicode__(self):
        return u__str__(self)

def read_event(event_sql):
    e = Event()
    e.eid       = event_sql[0]
    e.tid       = event_sql[1]
    e.core      = event_sql[2]
    e.kind      = event_sql[3]
    e.timestamp = event_sql[4]
    return e

def calculate_tlp(tlp):
    print tlp
    final_tlp = 0
    tlp_time_that_counts = 0
    for (p, t) in tlp.items():
        final_tlp += p * t
        if p > 0:
            tlp_time_that_counts += t
    print 'TLP: %.2f'% (1.0 * final_tlp / tlp_time_that_counts)


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


def read_command_line_args():
    argp = argparse.ArgumentParser(description="""Thread analyzer command line parser.  The format is: ...
        Hello""")
    argp.add_argument('trace_file', metavar='T',
                      help='Name of the input trace file')
    argp.add_argument('processes', metavar='P', type=int, nargs='*',
                      help='List of process IDs to consider part of the application')
    argp.add_argument('--background_app', dest='does_background_count', action='store_true',
                      help="If present, consider all processes to be part of the application")
    return argp.parse_args()

def init_db(f):
    conn = sqlite3.connect(f)
    return (conn, conn.cursor())

if __name__ == '__main__':
    main()


