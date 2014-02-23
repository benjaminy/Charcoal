#!/usr/bin/env python

'''This program takes an input file name and an output file name as
parameters.  The input file is assumed to be in the Linux Ftrace .dat
format.  This program reads the input file and converts it into the
thread measurement project's simple sqlite format.'''

import argparse, subprocess, tempfile

def translate_ftrace_to_generic():
    argp = argparse.ArgumentParser(description='''Ftrace to generic trace translator''')
    argp.add_argument('dat_file', metavar='T', type=file,
                      help='Name of the input ftrace dat file')
    argp.add_argument('sql_file', metavar='G', type=file,
                      help='Name of the output sqlite3 file')
    args = argp.parse_args()

    temp = tempfile.NamedTemporaryFile()

    try:
        cmd = ['sudo', 'trace-cmd', 'report', '-i', args.dat_file, '>', temp.name ]
        return_code = subprocess.call(cmd)

        if return_code != 0:
            print 'Bad return code '+return_code
            raise Exception()
    except:
        print 'Command just flat out failed'
        raise

if __name__ == '__main__':
    translate_ftrace_to_generic()
