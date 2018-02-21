#!/usr/bin/env python3

import sys
import os
import re
import json
import merger
import datetime
import pathlib

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

global_exec_depths = GrowingList()

def lineToEvent( line ):
    ev        = Object()
    ev.ts     = line[ 0 ]
    ev.pid    = line[ 1 ]
    ev.tid    = line[ 2 ]
    ev.source = line[ 3 ]
    def extractFields( source, names ):
        for name in names:
            try:
                setattr( ev, name, source[ name ] )
            except:
                pass

    if ev.source == "macro":
        ev.tkid = line[ 4 ]
        ev.kind = line[ 5 ]
        extractFields( line[ 6 ], [ "ctx", "ctx_ptr", "step", "recurring", "name" ] )
    elif ev.source == "micro":
        ev.tkid = "micro"
        ev.kind = line[ 4 ]
        extractFields( line[ 5 ], [ "props", "num_tasks", "callback", "ctx", "ctxdesc", "count", "name" ] )
    elif ev.source == "exec":
        ev.kind = line[ 4 ]
        extractFields( line[ 5 ], [ "ctx", "ctxdesc", "ctor", "has_exn", "throwOnAllowed", "name" ] )
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

def parseBeginsAndEnds( dir ):
    begins_ends = []
    for fname in os.listdir( dir ):
        with open( os.path.join( dir, fname ) ) as f:
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

    begins_ends.sort( key=( lambda be: be[ 1 ] ) )
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
    all_continuations = []
    global_continuation = Object()
    global_continuation.tkid = None
    global_continuation.events = []
    continuation_stack = [ global_continuation ]
    exec_depths = GrowingList()

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
        ev = lineToEvent( line )
        curr_cont = continuation_stack[ -1 ]
        curr_cont.events.append( ev )

        if ev.source == "macro":
            if ev.kind == "scheduled":
                if curr_cont != global_continuation:
                    parent_of[ ev.tkid ] = ( curr_cont, ev )

            elif ev.kind == "ctor":
                exec_depths[ len( continuation_stack ) ] += 1
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.end      = None
                next_cont.parent   = None
                next_cont.sched_ev = None
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                continuation_stack.append( next_cont )
                curr_cont.events.append( next_cont )
                if ev.tkid in parent_of:
                    ( parent_cont, sched_ev ) = parent_of[ ev.tkid ]
                    parent_cont.children.append( next_cont )
                    next_cont.parent = parent_cont
                    next_cont.sched_ev = sched_ev

            elif ev.kind == "dtor":
                continuation_stack.pop()
                if ev.tkid != curr_cont.begin.tkid:
                    print( "TASK MISMATCH '%s' '%s'" % ( ev.tkid, curr_cont.begin.tkid ) )
                    exit()
                curr_cont.end = ev

                if hasattr( ev, "recurring" ):
                    if ev.recurring:
                        if curr_cont.sched_ev is None:
                            ev.name = "RECURRING"
                        else:
                            if curr_cont.sched_ev.name.startswith( "RECURRING" ):
                                ev.name = curr_cont.sched_ev.name
                            else:
                                ev.name = "RECURRING - %s" % curr_cont.sched_ev.name
                        parent_of[ ev.tkid ] = ( curr_cont, ev )
                else:
                    pass

            elif ev.kind == "canceled" or ev.kind == "all_canceled":
                pass

            else:
                print( "WEIRD MACRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "micro":
            if ev.kind == "enq":
                if curr_cont != global_continuation and parent_of[ "micro" ] == None:
                    ev.name = "MICRO"
                    parent_of[ "micro" ] = ( curr_cont, ev )

            elif ev.kind == "before_loop":
                exec_depths[ len( continuation_stack ) ] += 1
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.end      = None
                next_cont.parent   = None
                next_cont.sched_ev = None
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                continuation_stack.append( next_cont )
                curr_cont.events.append( next_cont )
                if parent_of[ "micro" ] != None:
                    ( parent_cont, sched_ev ) = parent_of[ "micro" ]
                    parent_cont.children.append( next_cont )
                    curr_cont.parent = parent_cont
                    curr_cont.sched_ev = sched_ev

            elif ev.kind.startswith( "start" ):
                pass

            elif ev.kind.startswith( "done" ):
                pass

            elif ev.kind == "after_loop":
                continuation_stack.pop()
                if curr_cont.begin.tkid != "micro":
                    print( "MICRO MISMATCH %s\n%s" % ( vars( curr_cont.begin ), vars( ev ) ) )
                    exit()
                curr_cont.end = ev
                parent_of[ "micro" ] = None

            else:
                print( "WEIRD MICRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "exec":
            curr_cont.events.append( ev )

        else:
            print( "WEIRD SOURCE %s" %source )
            exit()

    print( "Exec Depths %s" % exec_depths )
    for i in range( len( exec_depths ) ):
        global_exec_depths[ i ] += exec_depths[ i ]

    return all_continuations

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

def analyze( dir, ranges, render_processes ):
    all_continuations = {}
    print( "======================== analyze %s ======================== %s" % ( dir, render_processes ) )
    id_path_map = {}
    def add( id, fname ):
        path = os.path.join( dir, fname )
        try:
            id_path_map[ id ].add( path )
        except:
            paths = { path }
            id_path_map[ id ] = paths

    r = re.compile( "p(c|i)_(\d+)\." )
    for fname in os.listdir( dir ):
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
            continuations = stacker( trace )
            all_continuations[ ( id, tid ) ] = continuations
            # tracer( trace )
        ts2 = datetime.datetime.now()
        print( "[TS] Done with process %s: %s" % ( id, ( ts2 - ts1 ) ) )

    return all_continuations

def analyzeDir( base_dir ):
    print( "======================== analyzeDir %s ========================" % base_dir )
    ts1 = datetime.datetime.now()
    recording_ranges = parseBeginsAndEnds( os.path.join( base_dir, "Traces" ) )
    recording_ranges = [ ( 0, 9999999999999 ) ] # HACK
    ts2 = datetime.datetime.now()
    print( "[TS] begin-end ranges: %s" % ( ts2 - ts1 ) )
    render_processes = parseProcessInfo( os.path.join( base_dir, "stderr.txt" ) )
    ts3 = datetime.datetime.now()
    print( "[TS] process info: %s" % ( ts3 - ts2 ) )
    trees = analyze(
        os.path.join( base_dir, "Traces" ), recording_ranges, render_processes )
    ts4 = datetime.datetime.now()
    print( "[TS] kit and caboodle: %s" % ( ts4 - ts3 ) )
    return trees

def normalizeEv( ev ):
    if ev == None:
        return None
    misc = {}
    for attr_name in [ "recurring", "num_tasks", "count", "name" ]:
        if hasattr( ev, attr_name ):
            misc[ attr_name ] = getattr( ev, attr_name )
    return [ ev.ts, ev.source, ev.kind, misc ]

def dumpTree( f, tree ):
    for cont in tree:
        begin    = normalizeEv( cont.begin )
        end      = normalizeEv( cont.end )
        sched_ev = normalizeEv( cont.sched_ev )
        parent   = None if cont.parent == None else cont.parent.begin.ts
        children = list( map( lambda c: c.begin.ts, cont.children ) )
        # dropping "events"
        f.write( "%s\n" % json.dumps( [ begin, end, sched_ev, parent, children ] ) )


def dumpTrees( tree_path, trees ):
    for ( ( pid, tid ), tree ) in trees.items():
        path = os.path.join( tree_path, "p%d_t%d.json" % ( pid, tid ) )
        with open( path, "a" ) as f:
            dumpTree( f, tree )

def main():
    traces_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "./Traces"
    trees_dir  = sys.argv[ 2 ] if len( sys.argv ) > 2 else "./Trees"

    for fname in os.listdir( traces_dir ):
        if fname == ".DS_Store":
            continue
        path = os.path.join( traces_dir, fname )
        trees = analyzeDir( path )
        tree_path = os.path.join( trees_dir, fname )
        pathlib.Path( tree_path ).mkdir( parents=True, exist_ok=True )
        dumpTrees( tree_path, trees )

    print( "Global exec depths: %s" % global_exec_depths )

# execute only if run as a script
if __name__ == "__main__":
    main()
