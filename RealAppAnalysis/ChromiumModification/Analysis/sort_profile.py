#!/usr/bin/env python3

import sys
import re

def main():
    lines_in = sys.stdin.readlines()
    lines_out = []
    r = re.compile( "\{\"pid\":\d+,\"tid\":\d+,\"ts\":(\d+),.*" )

    for line in lines_in:
        result = r.search( line )
        if result is None:
            lines_out.append( ( 999999999999999, line ) )
        else:
            lines_out.append( ( int( result.group( 1 ) ), line ) )

    lines_out.sort( key=lambda l: l[0] )

    for line in lines_out:
        print( line[ 1 ], end="" )

# execute only if run as a script
if __name__ == "__main__":
    main()
