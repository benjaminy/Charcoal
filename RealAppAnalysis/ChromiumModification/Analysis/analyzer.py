#!/usr/bin/env python3

import sys
import os
import re
import json
import merger
import numpy
import matplotlib.pyplot as plt

def timestamp( trace_entry ):
    try:
        rv = trace_entry[ "ts" ]
    except:
        rv = trace_entry[ 0 ]
    # print( "%s  --  %s --  %s" % ( trace_entry, rv, type( rv ) ) )
    return rv

def parseBeginsAndEnds( basepath ):
    begins_ends = []
    for fname in os.listdir( basepath ):
        path = os.path.join( basepath, fname )
        trace = []
        with open( path ) as f:
            for line in f:
                if "CHARCOAL_BEGIN_RECORDING_TRACE" in line:
                    try:
                        j = json.loads( line )
                        print( j )
                        begins_ends.append( ( "B", j[ 0 ] ) )
                    except:
                        print( "json parse failed!!! %s" % line )
                        sys.exit()
                if "CHARCOAL_END_RECORDING_TRACE" in line:
                    try:
                        j = json.loads( line )
                        print( j )
                        begins_ends.append( ( "E", j[ 0 ] ) )
                    except:
                        print( "json parse failed!!! %s" % line )
                        sys.exit()

    recording_ranges = []
    def k( be ):
        return be[ 1 ]
    begins_ends.sort( key=k )
    last_begin = None
    for b_or_e in begins_ends:
        if last_begin is None:
            if b_or_e[ 0 ] is "B":
                last_begin = b_or_e[ 1 ]
            elif b_or_e[ 0 ] is "E":
                print( "Weird end without begin" )
            else:
                print( "WHOA! %s" % b_or_e[ 0 ] )
                exit()
        else:
            if b_or_e[ 0 ] is "B":
                print( "Weird double begin" )
            elif b_or_e[ 0 ] is "E":
                recording_ranges.append( ( last_begin, b_or_e[ 1 ] ) )
                last_begin = None
            else:
                print( "WHOA! %s" % b_or_e[ 0 ] )
                exit()
    print( recording_ranges )
    return recording_ranges

def printIles( name, items, iles ):
    L = len( items )
    print( "%s %10d " % ( name, L ), end="" )
    if L < 1:
        print( "EMPTY" )
        return
    for ( N, D ) in iles:
        print( "%8.0f " % items[ N * L // D ], end="" )
    print( "" )

def parseProcessInfo( path ):
    processes = {}
    r = re.compile( "\"CHARCOAL_PROCESS_INFO\s+(A|T)\s+(\d+)\s+(\w+)\"" )
    with open( path ) as f:
        for line in f:
            result = r.search( line )
            if result is None:
                continue
            id = int( result.group( 2 ) )
            kind = result.group( 3 )
            try:
                if processes[ id ] != kind:
                    print( "WRONG PROCESS KIND %s %s" % ( kind, processes[ id ] ) )
                    exit()
            except:
                processes[ id ] = kind
    print( processes )
    render_processes = set()
    for key, value in processes.items():
        if value == "renderer":
            render_processes.add( key )
    print( render_processes )
    return render_processes

def stacker( trace ):
    global_container = {
        "task"   : None,
        "events" : []
    }
    container_stack = [ global_container ]
    mi_loop_stack = []
    max_depth = 1
    call_depths = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ]
    micro_depths = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ]
    macro_lengths = []
    micro_lengths = []
    before_counts = []
    after_counts = []
    gaps = []
    pc_good = 0
    pc_bad = 0

    task_reg_map = {}

    def printStack():
        print( "STACK ", end=" " )
        for x in container_stack:
            print( x[ "task" ], end=" - " )
        print( "" )

    line_count = 0

    for line in trace:
        line_count += 1
        container = container_stack[ -1 ]
        max_depth = max( max_depth, len( container_stack ) )
        source = line[ 3 ]

        if source == "macro":
            task = line[ 4 ]
            kind = line[ 5 ]
            misc = line[ 6 ]

            if kind == "scheduled":
                call_depths[ len( container_stack ) ] += 1
                if container != global_container:
                    task_reg_map[ task ] = ( container, line )
                container[ "events" ].append( line )
            elif kind == "ctor":
                container = {
                    "task"   : task,
                    "begin"  : line,
                    "events" : []
                }
                container[ "events" ].append( container )
                container_stack.append( container )
                try:
                    ( reg, reg_line ) = task_reg_map[ task ]
                    gap = line[ 0 ] - reg[ "end" ][ 0 ]
                    gaps.append( gap )
                    pc_good += 1
                    try:
                        recurring = reg_line[ 6 ][ "recurring" ]
                    except:
                        recurring = False
                    if gap < 400:
                        if recurring:
                            pass # print( "R - %s" % reg[ "begin" ] )
                        else:
                            pass # print( "I - %s" % reg_line )
                except:
                    pc_bad += 1
                if len( container_stack ) > 3:
                    pass # printStack()
            elif kind == "dtor":
                container = container_stack.pop()
                if task != container[ "task" ]:
                    print( "TASK MISMATCH '%s' '%s'" % ( task, container[ "task" ] ) )
                    exit()
                macro_lengths.append( line[ 0 ] - container[ "begin" ][ 0 ] )
                container[ "end" ] = line
                if misc[ "recurring" ]:
                    task_reg_map[ task ] = ( container, line )
            elif kind == "canceled" or kind == "all_canceled":
                container[ "events" ].append( line )
            else:
                print( "WEIRD MACRO TASK KIND %s" % kind )
                exit()

        elif source == "micro":
            kind = line[ 4 ]

            if kind.startswith( "start" ):
                container = {
                    "task"   : "@ micro",
                    "begin"  : line,
                    "events" : []
                }
                container[ "events" ].append( container )
                container_stack.append( container )
                if len( container_stack ) > 3:
                    pass #printStack()
                micro_depths[ line[ 5 ][ "num_tasks" ] ] += 1
            elif kind.startswith( "done" ):
                container = container_stack.pop()
                if "@ micro" != container[ "task" ]:
                    print( "MICROTASK MISMATCH %s %s" % ( container[ "task" ], line ) )
                    exit()
                container[ "end" ] = line
            elif kind == "enq":
                container[ "events" ].append( line )
            elif kind == "before_loop":
                mi_loop_stack.append( line )
                before_counts.append( line[ 5 ][ "num_tasks" ] )
            elif kind == "after_loop":
                try:
                    begin = mi_loop_stack.pop()
                    micro_lengths.append( line[ 0 ] - begin[ 0 ] )
                except:
                    print( "WEIRD DANGLING END OF MICRO LOOP" )
                after_counts.append( line[ 5 ][ "count" ] )
            else:
                print( "WEIRD MICRO TASK KIND %s" % kind )
                exit()

        elif source == "exec":
            container[ "events" ].append( line )

        else:
            print( "WEIRD SOURCE %s" %source )
            exit()

    print( "w00t? %d %d %d" % ( max_depth, len( global_container[ "events" ] ), len( container_stack ) ) )
    print( "CALL %s" % call_depths )
    print( "MICRO %s" % micro_depths )
    macro_lengths.sort()
    micro_lengths.sort()
    before_counts.sort()
    after_counts.sort()
    gaps.sort()
    printIles( "AL", macro_lengths, [ (1,10),  (1,2),   (9,10), (95,100), (99,100) ] )
    printIles( "IL", micro_lengths, [ (1,10),  (1,2),   (9,10), (95,100), (99,100) ] )
    printIles( "GA", gaps,          [ (1,100), (5,100), (1,10),  (1,2),    (9,10) ] )
    printIles( "BC", before_counts, [ (1,10),  (1,2),   (9,10), (95,100), (99,100) ] )
    printIles( "CC", after_counts,  [ (1,10),  (1,2),   (9,10), (95,100), (99,100) ] )

    print( "Good: %d - Bad: %d" % ( pc_good, pc_bad ) )

    ys = list( range( len( micro_lengths ) ) )
    for i in range( len( ys ) ):
        ys[ i ] = ys[ i ] / len( micro_lengths )
    plt.plot( micro_lengths, ys )
    # plt.ylabel('some numbers')
    plt.xscale( "log" )
    plt.show()

    print( "." )

    return global_container

def tracer( trace ):
    uid_internal = { "value" : 0 }
    def uidGen():
        uid_internal[ "value" ] = uid_internal[ "value" ] + 1
        return uid_internal[ "value" ]

    macro_stack = []
    task_uid_map = {}
    parent_of = {}

    for line in trace:
        ts     = line[ 0 ]
        source = line[ 3 ]

        if source == "macro":
            task   = line[ 4 ]
            kind   = line[ 5 ]
            misc   = line[ 6 ]
            try:
                ( uid, def_line ) = task_uid_map[ task ]
            except:
                uid = None
                def_line = None
            # print( "U D %s %s" % ( uid, def_line ) );

            if kind == "scheduled":
                if uid is not None:
                    print( "SCHEDULED WHEN ALREADY IN MAP %s %s" % ( ts - def_line[ 0 ], line ) )
                    continue
                uid = uidGen()
                task_uid_map[ task ] = ( uid, line )
                if len( macro_stack ) > 0:
                    # print( "PARENT A %s %s" %( uid, macro_stack[ -1 ] ) )
                    parent_of[ uid ] = macro_stack[ -1 ]
                else:
                    print( "WEIRD floating scheduled %s" % line )
            elif kind == "ctor":
                if len( macro_stack ) > 0:
                    print( "WEIRD NESTED tasks %s   %s" % ( line, macro_stack ) )
                    if uid != macro_stack[ -1 ]:
                        print( "NESTED TASKS DIFFERENT" )
                        exit()
                if uid is None:
                    print( "MISSED THE SCHEDULED %s" % line )
                    uid = uidGen()
                    task_uid_map[ task ]( uid, line )
                macro_stack.append( uid )
            elif kind == "dtor":
                if len( macro_stack ) < 1:
                    print( "EMPTY STACK %s" % line )
                    exit()
                if( uid != macro_stack.pop() ):
                    print( "WRONG TASK ON STACK %s" % line )
                    exit()
                if( len( macro_stack ) > 0 ):
                    continue
                del task_uid_map[ task ]
                if misc[ "recurring" ]:
                    new_uid = uidGen()
                    task_uid_map[ task ] = ( new_uid, line )
                    # print( "PARENT B %s %s" %( new_uid, uid ) )
                    parent_of[ new_uid ] = uid
            elif kind == "canceled":
                if uid is None:
                    print( "CANCELED AFTER D'TOR? %s" % line )
                else:
                    print( "canceled %s" % uid )
            elif kind == "all_canceled":
                print( "ALL CANCELED" )
            else:
                print( "WEIRD MACRO TASK KIND %s" % kind )
                exit()

        elif source == "micro":
            kind   = line[ 4 ]
            misc   = line[ 5 ]
            if len( macro_stack ) > 0 and kind != "enq":
                print( "MICRO INSIDE MACRO %s" % line )
                # "enq" "start_callback" "start_jsfun" "startpromise_resolve" "start_promise_react" "done_bail" "done"

        elif source == "exec":
            kind   = line[ 4 ]
            misc   = line[ 5 ]
            pass

        else:
            print( "WEIRD SOURCE %s" %source )
            exit()

def bloop( trace ):
    macro_tasks = {}

    def append( task, kind ):
        try:
            macro_tasks[ task ].append( kind )
        except:
            kinds = [ kind ]
            macro_tasks[ task ] = kinds

    for line in trace:
        ts     = line[ 0 ]
        source = line[ 3 ]
        if source != "macro":
            continue
        task   = line[ 4 ]
        kind   = line[ 5 ]
        misc   = line[ 6 ]
        if kind == "dtor":
            kind = "dtor " + ( "R" if misc[ "recurring" ] else "N" )
        append( task, kind )

    print( "YAY" )
    for task, kinds in macro_tasks.items():
        # if kinds[ -1 ] == "dtor N":
        #     continue
        # if kinds[ -1 ] != "canceled":
        #     continue
        print( "%s - %s" % ( task, kinds ) )
#        print( "%s - %s" % ( task, kinds[ -1 ] ) )
#        if len( kinds ) > 7:
#            print( "%s - %s" % ( task, kinds ) )

def splitByThread( trace ):
    traces = {}

    def append( tid, line ):
        try:
            traces[ tid ].append( line )
        except:
            trace = [ line ]
            traces[ tid ] = trace

    for line in trace:
        append( line[ 2 ], line )

    return traces

def analyze( basepath, ranges, render_processes ):
    id_path_map = {}
    def add( id, fname ):
        path = os.path.join( basepath, fname )
        try:
            id_path_map[ id ].add( path )
        except:
            paths = { path }
            id_path_map[ id ] = paths

    r = re.compile( "p(c|i)_(\d+)\." )
    for fname in os.listdir( basepath ):
        result = r.search( fname )
        if result is None:
            continue
        id = int( result.group( 2 ) )
        add( id, fname )

    for id in render_processes:
        print( id )
        try:
            paths = id_path_map[ id ]
        except:
            print( "No files for pid %d" % id )
            continue

        merged = merger.mergeTraces( merger.parseTraces( paths ) )
        print( "lines: %d" % ( len( merged ) ) )

        for tid, trace in splitByThread( merged ).items():
            print( "THREAD %s" % tid )
            stacker( trace )
            # tracer( trace )

def main():
    recording_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "."
    recording_ranges = parseBeginsAndEnds(
        os.path.join( recording_dir, "Traces" ) )
    render_processes = parseProcessInfo( os.path.join( recording_dir, "stderr.txt" ) )
    analyze( os.path.join( recording_dir, "Traces" ), recording_ranges, render_processes )

if __name__ == "__main__":
    # execute only if run as a script
    main()
