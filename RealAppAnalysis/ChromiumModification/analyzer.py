#!/usr/bin/env python3

import sys
import os
import re
import json
import merger

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
                uid = task_uid_map[ task ]
            except:
                uid = None

            if kind == "scheduled":
                if uid is not None:
                    print( "SCHEDULED WHEN ALREADY IN MAP %s" % line )
                    exit()
                uid = uidGen()
                task_uid_map[ task ] = uid
                if len( macro_stack ) > 0:
                    print( "PARENT A %s %s" %( uid, macro_stack[ -1 ] ) )
                    parent_of[ uid ] = macro_stack[ -1 ]
                else:
                    print( "WEIRD floating scheduled %s" % line )
            elif kind == "ctor":
                if len( macro_stack ) > 0:
                    print( "WEIRD NESTED tasks %s   %s" % ( line, macro_stack ) )
                if uid is None:
                    print( "MISSED THE SCHEDULED %s" % line )
                    uid = uidGen()
                    task_uid_map[ task ] = uid
                macro_stack.append( uid )
            elif kind == "dtor":
                if len( macro_stack ) < 1:
                    print( "EMPTY STACK %s" % line )
                    exit()
                if( uid != macro_stack.pop() ):
                    print( "WRONG TASK ON STACK %s" % line )
                    exit()
                try:
                    del task_uid_map[ task ]
                except:
                    print( "FAILED TO DELETE TASK %s" % line )
                    exit()
                if misc[ "recurring" ]:
                    new_uid = uidGen()
                    task_uid_map[ task ] = new_uid
                    print( "PARENT B %s %s" %( new_uid, uid ) )
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
            tracer( trace )

def main():
    recording_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "."
    recording_ranges = parseBeginsAndEnds(
        os.path.join( recording_dir, "Traces" ) )
    render_processes = parseProcessInfo( os.path.join( recording_dir, "stderr.txt" ) )
    analyze( os.path.join( recording_dir, "Traces" ), recording_ranges, render_processes )

if __name__ == "__main__":
    # execute only if run as a script
    main()
