# TODO: Header stuff

# This program reads trace files that are CSV formatted.  Each field
# should be enclosed in double quotes and no quotes should appear within
# each line.  The fields (in the order in which they much appear)
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
        event_kind = EVKIND_CREATE
    elif name == "destroy":
        event_kind = EVKIND_DESTROY
    elif name == "start":
        event_kind = EVKIND_START
    elif name == "stop":
        event_kind = EVKIND_STOP
    else:
        raise Exception('weird event kind', name)

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

    active_intervals = []

    running_thread_count = 0

    event_counter = 0
    timestamp_min = 0
    timestamp_max = 0
    cpu_time_total = 0
    for row_csv in trace_reader:
        # print row_csv
        process         = get_process(row_csv[4])
        thread          = get_thread(process, row_csv[5])
        event           = Event()
        event.timestamp = parse_timestamp(row_csv)
        event.kind      = parse_event_kind(row_csv[7])
        event.core_id   = int(row_csv[6])
        event.thread    = thread

        if event_counter == 0 or event.timestamp < timestamp_min:
            timestamp_min = event.timestamp
        if event_counter == 0 or event.timestamp > timestamp_max:
            timestamp_max = event.timestamp

        if event.kind == EVKIND_CREATE and len(thread.events) > 0:
            raise Exception('event before create', thread.name)

        if event.kind == EVKIND_START:
            if running_thread_count == 0:
                pass
            running_thread_count = running_thread_count + 1

        if len(thread.events) > 0:
            prev_event = thread.events[len(thread.events) - 1]
            if event.kind == EVKIND_STOP:
                active_intervals.append(prev_event, event)
                cpu_time = event.timestamp - prev_event.timestamp
                cpu_time_total = cpu_time_total + cpu_time
                thread.cpu_time = thread.cpu_time + cpu_time
                p = thread.process
                if p != prev_thread.process:
                    raise Exception('process mismatch', p.name)
                p.cpu_time = p.cpu_time + cpu_time

        thread.events.append(event)
        event_counter = event_counter + 1        

    for interval in active_intervals:
        pass

    for id, process in processes.items():
        print process.name, " ", (process)

main()
