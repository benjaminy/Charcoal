#!/usr/bin/env python3

import os
import json

def timestamp( trace_entry ):
    try:
        rv = trace_entry[ "ts" ]
    except:
        rv = trace_entry[ 0 ]
    # print( "%s  --  %s --  %s" % ( trace_entry, rv, type( rv ) ) )
    return rv

def allFiles( basepath ):
    paths = set()
    for fname in os.listdir( basepath ):
        paths.add( os.path.join( basepath, fname ) )
    return paths

def parseTraces( paths ):
    traces = []

    for path in paths:
        trace = []
        with open( path ) as f:
            try:
                pre_trace = json.load( f )
                print( "whole file JSON parsing WORKED! %s %s" % ( path, type( pre_trace ) ) )
                for line in pre_trace:
                    try:
                        line[ "ts" ]
                        trace.append( line )
                    except:
                        print( "No ts %s" % type( line ) )
                        pass
                def k( trace_line ):
                    return trace_line[ "ts" ]
                trace.sort( key=k )
            except:
                print( "whole file JSON parsing DID NOT WORK! %s" % path )
                f.seek( 0 )
                for line in f:
                    # print line
                    try:
                        j = json.loads( line )
                        # print( j )
                        trace.append( j )
                    except:
                        pass
        if len( trace ) > 0:
            traces.append( trace )

    return traces

def mergeTraces( traces ):
    merged = []
    print( "%d Traces" % ( len( traces ) ) )
    while len( traces ) > 0:
        earliest = ( 0, traces[ 0 ][ 0 ] )
        for i in range( 1, len( traces ) ):
            # print( len( traces[ i ] ) )
            if timestamp( traces[ i ][ 0 ] ) < timestamp( earliest[ 1 ] ):
                earliest = ( i, traces[ i ][ 0 ] )
        idx = earliest[ 0 ]
        trace_line = earliest[ 1 ]
        # print( "%s -- %s" % ( idx, trace_line ) )
        merged.append( trace_line )
        traces[ idx ].pop( 0 )
        if len( traces[ idx ] ) < 1:
            traces.pop( idx )

    return merged

def mergeFiles( paths ):
    return mergeTraces( parseTraces( paths ) )

def test():
    merged = mergeTraces( parseTraces( allFiles( "Traces" ) ) )
    print( len( merged ) )
    for te in merged:
        print( te )

if __name__ == "__main__":
    # execute only if run as a script
    test()
