# Author: Ethan Bogdan
# Date: Fall 2013
#
# Purpose: Data analysis for CS97 research project; includes tools for parsing Instruments output
#          and calculting Thread-Level Parallelism


import sys, os

def main():
    if len(sys.argv) != 4:
        print "Incorrect usage:\nTLP.py [trace file] [app name] [is background idle?]"
        exit()
        
    if not os.path.exists(sys.argv[1]):
        print "Invalid file specified: "+sys.argv[1]
        exit()
        
    slices = readTimeSlices(sys.argv[1])
    core0, core1 = splitSlicesByCore(slices)
    
    convertTimestampsToDurations(core0)
    convertTimestampsToDurations(core1)
    
    core0 = insertIdleTimes(core0, 1.1)
    core1 = insertIdleTimes(core1, 1.1)
    
    handleIdleTime(core0, sys.argv[2], int(sys.argv[3]))
    handleIdleTime(core1, sys.argv[2], int(sys.argv[3]))
        
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
