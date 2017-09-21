#!/usr/bin/env python3

import os
import json

def merge( basepath ):

    traces = []

    for fname in os.listdir( basepath ):
        path = os.path.join( basepath, fname )
        trace = []
        with open( path ) as f:
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

    merged = []

    while len( traces ) > 0:
        earliest = ( 0, traces[ 0 ][ 0 ] )
        for i in range( 1, len( traces ) ):
            # print( len( traces[ i ] ) )
            if traces[ i ][ 0 ][ 0 ] < earliest[ 1 ][ 0 ]:
                earliest = ( i, traces[ i ][ 0 ] )
        # print( earliest[ 1 ] )
        merged.append( earliest[ 1 ] )
        traces[ earliest[ 0 ] ].pop( 0 )
        if len( traces[ earliest[ 0 ] ] ) < 1:
            traces.pop( earliest[ 0 ] )

    return merged

def test():
    merged = merge( "Traces" )
    print( len( merged ) )
    for te in merged:
        print( te )

test()
