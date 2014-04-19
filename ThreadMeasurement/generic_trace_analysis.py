# TODO: Header stuff

# This program reads trace files that are CSV formatted.  Each field
# should be enclosed in double quotes and no quotes should appear within
# a field.  The fields (in the order in which they much appear)
# - Timestamp
#   - seconds
#   - milliseconds
#   - microseconds
#   - nanoseconds
# - Process name
# - Thread name
# - Processor (i.e. core)
# - Kind of event (create, destroy, start, stop)

EVKIND_CREATE  = 1
EVKIND_DESTROY = 2
EVKIND_START   = 3
EVKIND_STOP    = 4

import argparse, csv

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

def main():
    argp = argparse.ArgumentParser(description="""Thread analyzer command line parser.  The format is: ...
        Hello""")
    argp.add_argument('processes', metavar='P', type=int, nargs='*',
                      help='List of process IDs to consider part of the application')
    argp.add_argument('--background_app', dest='does_background_count', action='store_true',
                      help="If present, consider all processes to be part of the application")
    argp.add_argument('trace_file', metavar='T', type=file,
                      help='Name of the input trace file')
    args = argp.parse_args()

    trace_reader = csv.reader(args.trace_file)
    # It seems possible that the events will not be in chronological
    # order, so do a shallow parse then sort them by timestamp
    raw_events = []
    for row_csv in trace_reader:
        # print row_csv
        raw_event              = RawEvent()
        raw_event.timestamp    = parse_timestamp(row_csv)
        raw_event.process_name = row_csv[4]
        raw_event.thread_name  = row_csv[5]
        raw_event.core         = int(row_csv[6])
        raw_event.kind         = parse_event_kind(row_csv[7])
        raw_events.append(raw_event)

    raw_events_chronological = sorted(raw_events, cmp=compare_timestamps)

    active_intervals = []

    event_counter  = 0
    timestamp_min  = raw_events_chronological[0].timestamp
    timestamp_max  = raw_events_chronological[-1].timestamp
    timestamp_prev = 0
    cpu_time_total = 0
    tlp_accum      = {}
    processors     = {}
    for raw_event in raw_events_chronological:
        process         = get_process(raw_event.process_name)
        thread          = get_thread(process, raw_event.thread_name)
        event           = Event()
        event.timestamp = raw_event.timestamp
        event.kind      = raw_event.kind
        event.core_id   = raw_event.core
        event.thread    = thread

        active_core_count = len(processors)
        if event_counter != 0:
            interval_len = event.timestamp - timestamp_prev
            if not active_core_count in tlp_accum:
                tlp_accum[active_core_count] = 0
            tlp_accum[active_core_count] += interval_len

        if event.kind == EVKIND_STOP:
            if event.core_id in processors:
                # TODO: check that it's the right threads
                del processors[event.core_id]
            else:
                # weird, but okay I guess
                pass

        if event.kind == EVKIND_START:
            processors[event.core_id] = event

        if event.kind == EVKIND_CREATE and len(thread.events) > 0:
            raise Exception('event before create', thread.name)

        if len(thread.events) > 0:
            prev_event = thread.events[len(thread.events) - 1]
            if event.kind == EVKIND_STOP:
                active_intervals.append((prev_event, event))
                cpu_time = event.timestamp - prev_event.timestamp
                cpu_time_total = cpu_time_total + cpu_time
                thread.cpu_time = thread.cpu_time + cpu_time
                p = thread.process
                if p != prev_event.thread.process:
                    raise Exception('process mismatch', p.name)
                p.cpu_time = p.cpu_time + cpu_time

        thread.events.append(event)
        timestamp_prev = event.timestamp
        event_counter = event_counter + 1

    tlp = calculate_tlp(tlp_accum)


    for interval in active_intervals:
        pass

    for id, process in processes.items():
        print process.name, " ", (process)

def calculate_tlp(data):
    tlp_numerator = 0
    tlp_denominator = 0
    for core_count, time in data.items():
        tlp_numerator += time * core_count
        if core_count > 0:
            tlp_denominator += time
        print core_count, time

    print "TLP:", float(tlp_numerator) / tlp_denominator

main()
