# Header stuff

# Trace file should be CSV formatted.  Each field should be enclosed in
# double quotes and no quotes should appear within each line.  The
# fields (in the order in which they much appear)
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

next_id = 1
process_ids = {}
processes = {}
threads = {}

def get_process(process_name):
    global next_id
    global process_ids
    global processes

    if process_name in process_ids:
        process = processes[process_ids[process_name]]
    else:
        process = Process()
        process.id = next_id
        next_id = next_id + 1
        process_ids[process_name] = process.id
        process.name = process_name
        process.threads = []
        process.thread_ids = {}
        processes[process.id] = process
    return process

def get_thread(process, thread_name):
    global next_id
    global threads

    if thread_name in process.thread_ids:
        thread = threads[process.thread_ids[thread_name]]
    else:
        thread = Thread()
        thread.id = next_id
        next_id = next_id + 1
        process.thread_ids[thread_name] = thread.id
        process.threads.append(thread)
        thread.process = process
        thread.events = []
        threads[thread.id] = thread
    return thread

def parse_timestamp(row_csv):
    timestamp_s  = int(row_csv[0])
    timestamp_ms = int(row_csv[1])
    timestamp_us = int(row_csv[2])
    timestamp_ns = int(row_csv[3])
    # XXX: numerical overflow? I guess Python will use 64 bits automagically
    timestamp = timestamp_s * 1000 + timestamp_ms
    timestamp = timestamp * 1000 + timestamp_us
    return timestamp * 1000 + timestamp_ns

def parse_event_kind(name):
    if name == "create":
        event_kind = EVKIND_CREATE
    elif name == "destroy":
        event_kind = EVKIND_DESTROY
    elif name == "start":
        event_kind = EVKIND_START
    elif name == "destroy":
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

    next_process_id = 1
    next_thread_id = 1

    for row_csv in trace_reader:
        # print row_csv
        process = get_process(row_csv[4])
        thread  = get_thread(process, row_csv[5])
        core_id = int(row_csv[6])
        timestamp = parse_timestamp(row_csv)
        event_kind = parse_event_kind(row_csv[7])
        event = (timestamp, event_kind)

        if process.name in process_dict:
            thread_dict = process_dict[process_name]
            if thread_name in thread_dict:
                thread = thread_dict[thread_name]
                prev_event = thread[len(thread)-1]
                thread.append(event)
                # check compatibility of event and prev_event
            else:
                thread_dict[thread_name] = [event]
        else:
            process_dict[process_name] = {thread_name : [event]}

    #for line in core0:
    #    print line
    #
    #print
    #for line in core1:
    #    print line
        
    core0 = consolidateDurations(core0)
    core1 = consolidateDurations(core1)
    
    TLP = calculateTLP(core0, core1)
    print "TLP:", TLP
    
    
    
def readTimeSlices(cpuFile):
    infile = open(cpuFile, 'r')
    timeSlices = []
    for line in infile.readlines()[1:]:
        line = line.split(',')
        
        timePieces = line[1].split(':')
        timePieces[1] = ''.join([timePieces[1][0:6],timePieces[1][7:10]])
        time = int(timePieces[0][1:])*60+float(timePieces[1])
        
        timeSlices.append([time, int(line[3][1:-1]), line[4][1:-1]])
    
    infile.close()
    return timeSlices
    
    
def splitSlicesByCore(slices):
    core0 = []
    core1 = []
    
    for slice in slices:
        if slice[1] == 0:
            core0.append(slice)
        else:
            core1.append(slice)
    
    return [core0, core1]


def convertTimestampsToDurations(slices):
    for i in range(len(slices)-1):
        slices[i][0] = round(1000*(slices[i+1][0]-slices[i][0])/1.024, 2)
        #divide by 2^10/1000 = 1.024 to compensate for systematic sampling error
    slices[len(slices)-1][0] = 1.0
    
    
def insertIdleTimes(slices, threshold):
    newSlices = []
    for i in range(len(slices)):
        nextSlice = list(slices[i])
        
        if nextSlice[0] > threshold:
            nextSlice[0] = 1.0
            idleSlice = [round(slices[i][0]-1.0, 2), slices[i][1], "idle"]
            newSlices.append(nextSlice)
            newSlices.append(idleSlice)
            
        else:
            newSlices.append(nextSlice)
            
    return newSlices
    

def handleIdleTime(slices, mainProcess, mode):
    if mode == 1:
        for slice in slices:
            if slice[2] != mainProcess:
                slice[2] = 'idle'
    
    else:
        for slice in slices:
            if slice[2] != 'idle':
                slice[2] = mainProcess


def consolidateDurations(slices):
    duration = 0
    currentProcess = slices[0][2]
    consolidatedSlices = []
    
    for slice in slices:
        if slice[2] == currentProcess:
            duration += slice[0]
            
        else:
            consolidatedSlices.append([round(duration, 2), slice[1], currentProcess])
            currentProcess = slice[2]
            duration = slice[0]

    consolidatedSlices.append([round(duration, 2), slice[1], currentProcess])
    return consolidatedSlices


def calculateTLP(core0, core1):
    index0 = 0
    index1 = 0
    
    time0 = 0
    time1 = 0
    currentTime = 0
    
    total1 = 0
    total2 = 0
    
    while index0 < len(core0) and index1 < len(core1):
        nextCore0Time = time0+core0[index0][0] #absolute time of end of next section
        nextCore1Time = time1+core1[index1][0]
        
        numberActive = (core0[index0][2] != 'idle') + (core1[index1][2] != 'idle')
                
        if nextCore0Time < nextCore1Time:
            if numberActive > 0:
                if numberActive == 1:
                    total1 += nextCore0Time-currentTime
                else:
                    total2 += nextCore0Time-currentTime
                    
            currentTime = nextCore0Time
            time0 = nextCore0Time
            index0 += 1
                        
        else:
            if numberActive > 0:
                if numberActive == 1:
                    total1 += nextCore1Time-currentTime
                else:
                    total2 += nextCore1Time-currentTime
                    
            currentTime = nextCore1Time
            time1 = nextCore1Time
            index1 += 1
            
            if nextCore0Time == nextCore1Time:
                time0 = nextCore0Time
                index0 += 1
                            
    print total1, total2
    return (total1+2*total2)/(total1+total2)
    
main()
