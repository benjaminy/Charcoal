#!/usr/bin/env python3

import sys
import subprocess

def usage():
    print( "Give me a command that expects a number" )

min_val = 1
max_val = None

def done():
    if max_val is None:
        return False
    diff = max_val - min_val
    return diff < 10 or min_val / diff > 100

cmd = sys.argv[ 1 ]

while not done():
    test = min_val * 2 if max_val is None else ( max_val + min_val ) / 2
    exit_code = subprocess.call( [ cmd, str( test ) ] )
    if exit_code == 0:
        min_val = test
    else:
        max_val = test
    print( "Min %10d  Max %10d" %( min_val, max_val ) )
