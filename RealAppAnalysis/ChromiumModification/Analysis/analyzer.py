#!/usr/bin/env python3

import sys
import os
import re
import json
import merger
import numpy
import matplotlib.pyplot as plt

class Object:
    pass

def lineToEvent( line ):
    ev        = Object()
    ev.ts     = line[ 0 ]
    ev.pid    = line[ 1 ]
    ev.tid    = line[ 2 ]
    ev.source = line[ 3 ]
    if ev.source == "macro":
        ev.tkid = line[ 4 ]
        ev.kind = line[ 5 ]
        misc = line[ 6 ]
        try:
            ev.ctx = misc[ "ctx" ]
        except:
            pass
        try:
            ev.ctx_ptr = misc[ "ctx_ptr" ]
        except:
            pass
        try:
            ev.step = misc[ "step" ]
        except:
            pass
        try:
            ev.recurring = misc[ "recurring" ]
        except:
            pass
        try:
            ev.name = misc[ "name" ]
        except:
            pass
    elif ev.source == "micro":
        ev.tkid = "micro"
        ev.kind = line[ 4 ]
        misc = line[ 5 ]
        try:
            ev.props = misc[ "props" ]
        except:
            pass
        try:
            ev.num_tasks = misc[ "num_tasks" ]
        except:
            pass
        try:
            ev.callback = misc[ "callback" ]
        except:
            pass
        try:
            ev.ctx = misc[ "ctx" ]
        except:
            pass
        try:
            ev.ctxdesc = misc[ "ctxdesc" ]
        except:
            pass
        try:
            ev.count = misc[ "count" ]
        except:
            pass
    elif ev.source == "exec":
        ev.kind = line[ 4 ]
        misc = line[ 5 ]
        try:
            ev.ctx = misc[ "ctx" ]
        except:
            pass
        try:
            ev.ctxdesc = misc[ "ctxdesc" ]
        except:
            pass
        try:
            ev.ctor = misc[ "ctor" ]
        except:
            pass
        try:
            ev.has_exn = misc[ "has_exn" ]
        except:
            pass
        try:
            ev.throwOnAllowed = misc[ "throwOnAllowed" ]
        except:
            pass
        try:
            ev.ctx = misc[ "ctx" ]
        except:
            pass
    else:
        print( "UNKNOWN TASK SOURCE: %s" % ev.source )
        exit()
    return ev

def timestamp( trace_entry ):
    try:
        rv = trace_entry[ "ts" ]
    except:
        rv = trace_entry[ 0 ]
    # print( "%s  --  %s --  %s" % ( trace_entry, rv, type( rv ) ) )
    return rv

def fancyPlot( ns1, ns2 ):
    n1 = len( ns1 )
    ys1 = list( range( n1 ) )
    # for i in range( n1 ):
    #     ys1[ i ] = ys1[ i ] / n1
    plt.plot( ns1, ys1 )
    if ns2 is not None:
        n2 = len( ns2 )
        ys2 = list( range( n2 ) )
        # for i in range( n2 ):
        #     ys2[ i ] = ys2[ i ] / n2
        plt.plot( ns2, ys2 )
    # plt.ylabel('some numbers')
    plt.xscale( "log" )
    plt.show()

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
    for p in iles:
        print( "%8.0f " % items[ int( L * p ) ], end="" )
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

def increment( table, key ):
    try:
        table[ key ] = table[ key ] + 1
    except:
        table[ key ] = 1

def stacker( trace ):
    all_continuations = []
    most_recent_task = None
    global_continuation = Object()
    global_continuation.tkid = None
    global_continuation.events = []
    continuation_stack = [ global_continuation ]
    max_depth = 1
    call_depths = {}
    micro_depths = {}
    macro_lengths = []
    micro_lengths = []
    before_counts = []
    after_counts = []
    interrupted_gaps = []
    gaps = []
    pc_good = 0
    missing_parent = 0
    missing_end = 0

    parent_of = {}
    parent_of[ "micro" ] = None

    def printStack():
        print( "STACK ", end=" " )
        for x in continuation_stack:
            print( x[ "task" ], end=" - " )
        print( "" )

    line_count = 0

    for line in trace:
        line_count += 1
        max_depth = max( max_depth, len( continuation_stack ) )

        ev = lineToEvent( line )
        curr_cont = continuation_stack[ -1 ]
        curr_cont.events.append( ev )

        if ev.source == "macro":
            if ev.kind == "scheduled":
                increment( call_depths, len( continuation_stack ) )
                if curr_cont != global_continuation:
                    parent_of[ ev.tkid ] = ( curr_cont, ev )

            elif ev.kind == "ctor":
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                curr_cont.events.append( next_cont )
                continuation_stack.append( next_cont )
                if ev.tkid in parent_of:
                    ( parent_cont, parent_ev ) = parent_of[ ev.tkid ]
                    parent_cont.children.append( next_cont )
                    try:
                        gap = ev.ts - parent_cont.end.ts
                        gaps.append( gap )
                        if most_recent_task != parent_cont.begin.tkid:
                            interrupted_gaps.append( gap )
                        pc_good += 1
                    except:
                        missing_end += 1
                else:
                    missing_parent += 1
                if len( continuation_stack ) > 3:
                    pass # printStack()

            elif ev.kind == "dtor":
                most_recent_task = ev.tkid
                continuation_stack.pop()
                if ev.tkid != curr_cont.begin.tkid:
                    print( "TASK MISMATCH '%s' '%s'" % ( ev.tkid, curr_cont.begin.tkid ) )
                    exit()
                macro_lengths.append( ev.ts - curr_cont.begin.ts )
                curr_cont.end = ev
                if ev.recurring:
                    parent_of[ ev.tkid ] = ( curr_cont, ev )

            elif ev.kind == "canceled" or ev.kind == "all_canceled":
                pass

            else:
                print( "WEIRD MACRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "micro":
            if ev.kind == "enq":
                increment( call_depths, len( continuation_stack ) )
                if curr_cont != global_continuation and parent_of[ "micro" ] == None:
                    parent_of[ "micro" ] = ( curr_cont, ev )

            elif ev.kind == "before_loop":
                before_counts.append( ev.num_tasks )
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                curr_cont.events.append( next_cont )
                continuation_stack.append( next_cont )
                if parent_of[ "micro" ] != None:
                    ( parent_cont, parent_ev ) = parent_of[ "micro" ]
                    parent_cont.children.append( next_cont )
                    try:
                        gap = ev.ts - parent_cont.end.ts
                        gaps.append( gap )
                        if most_recent_task != parent_cont.begin.tkid:
                            interrupted_gaps.append( gap )
                        pc_good += 1
                    except:
                        missing_end += 1
                else:
                    missing_parent += 1
                if len( continuation_stack ) > 3:
                    pass # printStack()

            elif ev.kind.startswith( "start" ):
                if len( continuation_stack ) > 3:
                    pass #printStack()
                increment( micro_depths, ev.num_tasks )

            elif ev.kind.startswith( "done" ):
                pass

            elif ev.kind == "after_loop":
                continuation_stack.pop()
                if curr_cont.begin.tkid != "micro":
                    print( "MICROTASK MISMATCH %s %s" % ( curr_cont[ "task" ], ev ) )
                    exit()
                curr_cont.end = ev
                micro_lengths.append( ev.ts - curr_cont.begin.ts )
                after_counts.append( ev.count )
                parent_of[ "micro" ] = None
                most_recent_task = ev.tkid

            else:
                print( "WEIRD MICRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "exec":
            curr_cont.events.append( ev )

        else:
            print( "WEIRD SOURCE %s" %source )
            exit()

    num_children = []
    num_pchildren = []
    for continuation in all_continuations:
        n = len( continuation.children )
        num_children.append( n )
        for i in range( n ):
            num_pchildren.append( n )

    print( "w00t? %d %d %d" % ( max_depth, len( global_continuation.events ), len( continuation_stack ) ) )
    print( "CALL %s" % call_depths )
    print( "MICRO %s" % micro_depths )
    num_children.sort()
    num_pchildren.sort()
    macro_lengths.sort()
    micro_lengths.sort()
    before_counts.sort()
    after_counts.sort()
    gaps.sort()
    interrupted_gaps.sort()
    printIles( "AL", macro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "IL", micro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "GA", gaps,          [ 0.01, 0.05,  0.1,  0.5, 0.9 ] )
    printIles( "BC", before_counts, [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "CC", after_counts,  [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )

    print( "Good: %d - Missing Parent: %d - Missing End: %d" %
           ( pc_good, missing_parent, missing_end ) )

    # fancyPlot( gaps, interrupted_gaps )
    fancyPlot( num_children, num_pchildren )
    # fancyPlot( macro_lengths, micro_lengths )
    # fancyPlot( micro_lengths, after_counts )

    print( "." )

    return global_continuation

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
        print( "File: %s %s" % ( fname, result ) )
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
    recording_ranges = [ ( 0, 9999999999999 ) ]
    print( "parsed begins and ends" )
    render_processes = parseProcessInfo( os.path.join( recording_dir, "stderr.txt" ) )
    analyze( os.path.join( recording_dir, "Traces" ), recording_ranges, render_processes )

if __name__ == "__main__":
    # execute only if run as a script
    main()
