#!/usr/bin/env python3

import sys
import os
import re
import json
import merger
import numpy
import matplotlib.pyplot as plt
import datetime

class Object:
    pass

class GrowingList( list ):
    def __getitem__( self, index ):
        if index >= len(self):
            self.extend( [0] * ( index + 1 - len( self ) ) )
        return list.__getitem__( self, index )

    def __setitem__( self, index, value ):
        if index >= len( self ):
            self.extend( [0] * ( index + 1 - len( self ) ) )
        list.__setitem__( self, index, value )

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
        try:
            ev.name = misc[ "name" ]
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
            ev.name = misc[ "name" ]
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

def fancyPlot( name, ns1, ns2, splitx, splity ):
    fig = plt.figure()
    plt.title( name )
    plt.xscale( "log" )
    host = fig.add_subplot(111)

    n1 = len( ns1 )
    ys1 = list( range( n1 ) )
    # for i in range( n1 ):
    #     ys1[ i ] = ys1[ i ] / n1
    host.plot( ns1, ys1 )
    if ns2 is not None:
        n2 = len( ns2 )
        ys2 = list( range( n2 ) )
        # for i in range( n2 ):
        #     ys2[ i ] = ys2[ i ] / n2
        plotable = host.twinx() if splitx else host
        plotable = plotable.twiny() if splity else plotable
        plotable.set_xscale( "log" )
        plotable.plot( ns2, ys2 )
    # plt.ylabel('some numbers')
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

def combineStats( s1, s2 ):
    s1.children_per_cont.extend ( s2.children_per_cont )
    s1.pchildren_per_edge.extend( s2.pchildren_per_edge )
    s1.macro_lengths.extend     ( s2.macro_lengths )
    s1.micro_lengths.extend     ( s2.micro_lengths )
    s1.before_counts.extend     ( s2.before_counts )
    s1.after_counts.extend      ( s2.after_counts )
    s1.uninterrupted_gaps.extend( s2.uninterrupted_gaps )
    s1.interrupted_gaps.extend  ( s2.interrupted_gaps )

def showStats( s ):
    s.children_per_cont.sort()
    s.pchildren_per_edge.sort()
    s.macro_lengths.sort()
    s.micro_lengths.sort()
    s.before_counts.sort()
    s.after_counts.sort()
    s.uninterrupted_gaps.sort()
    s.interrupted_gaps.sort()

    printIles( "AL", s.macro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "IL", s.micro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "GA", s.uninterrupted_gaps, [ 0.01, 0.05,  0.1,  0.5, 0.9 ] )
    printIles( "BC", s.before_counts, [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "CC", s.after_counts,  [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )

    fancyPlot( "gaps", s.uninterrupted_gaps, s.interrupted_gaps, False, False )
    fancyPlot( "children", s.children_per_cont, s.pchildren_per_edge, False, False )
    fancyPlot( "lengths", s.macro_lengths, s.micro_lengths, True, False )
    fancyPlot( "micros", s.micro_lengths, s.after_counts, False, True )

def stacker( trace ):
    all_continuations = []
    most_recent_task = None
    global_continuation = Object()
    global_continuation.tkid = None
    global_continuation.events = []
    continuation_stack = [ global_continuation ]
    max_depth = 1
    call_depths = GrowingList()
    micro_depths = GrowingList()
    macro_lengths = []
    micro_lengths = []
    before_counts = []
    after_counts = []
    interrupted_gaps = []
    uninterrupted_gaps = []
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
                call_depths[ len( continuation_stack ) ] += 1
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
                        if most_recent_task != parent_cont.begin.tkid:
                            interrupted_gaps.append( gap )
                        else:
                            uninterrupted_gaps.append( gap )
                        pc_good += 1
                    except:
                        missing_end += 1
                else:
                    missing_parent += 1
                if len( continuation_stack ) > 3:
                    pass # printStack()

            elif ev.kind == "dtor":
                most_recent_task = ev.tkid
                blah = continuation_stack.pop()
                # print( "BLAH %s" % vars( blah ) )
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
                call_depths[ len( continuation_stack ) ] += 1
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
                        if most_recent_task != parent_cont.begin.tkid:
                            interrupted_gaps.append( gap )
                        else:
                            uninterrupted_gaps.append( gap )
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
                micro_depths[ ev.num_tasks ] += 1

            elif ev.kind.startswith( "done" ):
                pass

            elif ev.kind == "after_loop":
                continuation_stack.pop()
                if curr_cont.begin.tkid != "micro":
                    print( "MICROTASK MISMATCH %s\n%s" % ( vars( curr_cont.begin ), vars( ev ) ) )
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

    children_per_cont = []
    pchildren_per_edge = []
    for continuation in all_continuations:
        n = len( continuation.children )
        children_per_cont.append( n )
        for i in range( n ):
            pchildren_per_edge.append( n )

    print( "w00t? %d %d" % ( max_depth, len( global_continuation.events ) ) )
    print( "CALL %s" % call_depths )
    print( "MICRO %s" % micro_depths )

    print( "Good: %d - Missing Parent: %d - Missing End: %d" %
           ( pc_good, missing_parent, missing_end ) )

    print( "." )

    stats = Object()
    stats.children_per_cont = children_per_cont
    stats.pchildren_per_edge = pchildren_per_edge
    stats.macro_lengths = macro_lengths
    stats.micro_lengths = micro_lengths
    stats.before_counts = before_counts
    stats.after_counts = after_counts
    stats.uninterrupted_gaps = uninterrupted_gaps
    stats.interrupted_gaps = interrupted_gaps

    return stats

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

def analyze( basepath, ranges, render_processes, stats ):
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
        ts1 = datetime.datetime.now()
        print( "Starting process: %s" % id )
        try:
            paths = id_path_map[ id ]
        except:
            print( "No files for pid %d" % id )
            continue

        merged = merger.mergeTraces( merger.parseTraces( paths ) )
        print( "lines: %d" % ( len( merged ) ) )

        for tid, trace in splitByThread( merged ).items():
            print( "THREAD %s" % tid )
            s = stacker( trace )
            combineStats( stats, s )
            # tracer( trace )
        ts2 = datetime.datetime.now()
        print( "[TS] Done with process %s: %s" % ( id, ( ts2 - ts1 ) ) )


def analyzeDir( base_dir, stats ):
    ts1 = datetime.datetime.now()
    recording_ranges = parseBeginsAndEnds( os.path.join( base_dir, "Traces" ) )
    recording_ranges = [ ( 0, 9999999999999 ) ] # HACK
    ts2 = datetime.datetime.now()
    print( "[TS] begin-end ranges: %s" % ( ts2 - ts1 ) )
    render_processes = parseProcessInfo( os.path.join( base_dir, "stderr.txt" ) )
    ts3 = datetime.datetime.now()
    print( "[TS] process info: %s" % ( ts3 - ts2 ) )
    analyze( os.path.join( base_dir, "Traces" ), recording_ranges, render_processes, stats )
    ts4 = datetime.datetime.now()
    print( "[TS] kit and caboodle: %s" % ( ts4 - ts3 ) )

def main():
    traces_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "./Traces"
    stats = Object()
    stats.children_per_cont = []
    stats.pchildren_per_edge = []
    stats.macro_lengths = []
    stats.micro_lengths = []
    stats.before_counts = []
    stats.after_counts = []
    stats.uninterrupted_gaps = []
    stats.interrupted_gaps = []

    for fname in os.listdir( traces_dir ):
        path = os.path.join( traces_dir, fname )
        analyzeDir( path, stats )

    showStats( stats )

if __name__ == "__main__":
    # execute only if run as a script
    main()



# par1 = host.twinx()
# par2 = host.twinx()

# host.set_xlim(0, 2)
# host.set_ylim(0, 2)
# par1.set_ylim(0, 4)
# par2.set_ylim(1, 65)

# host.set_xlabel("Distance")
# host.set_ylabel("Density")
# par1.set_ylabel("Temperature")
# par2.set_ylabel("Velocity")

# # color1 = plt.cm.viridis(0)
# # color2 = plt.cm.viridis(0.5)
# # color3 = plt.cm.viridis(.9)

# p1, = host.plot([0, 1, 2], [0, 1, 2], color=color1,label="Density")
# p2, = par1.plot([0, 1, 2], [0, 3, 2], color=color2, label="Temperature")
# p3, = par2.plot([0, 1, 2], [50, 30, 15], color=color3, label="Velocity")

# lns = [p1, p2, p3]
# host.legend(handles=lns, loc='best')

# # right, left, top, bottom
# par2.spines['right'].set_position(('outward', 60))
# # no x-ticks
# # par2.xaxis.set_ticks([])
# # Sometimes handy, same for xaxis
# #par2.yaxis.set_ticks_position('right')

# host.yaxis.label.set_color(p1.get_color())
# par1.yaxis.label.set_color(p2.get_color())
# par2.yaxis.label.set_color(p3.get_color())

# plt.savefig("pyplot_multiple_y-axis.png", bbox_inches='tight')
