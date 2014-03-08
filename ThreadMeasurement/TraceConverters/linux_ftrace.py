#!/usr/bin/env python

'''This program takes an input file name and an output file name as
parameters.  The input file is assumed to be in the Linux Ftrace .dat
format.  This program reads the input file and converts it into the
thread measurement project's simple sqlite format.'''

import argparse, subprocess, tempfile, re, sqlite3, os
import thread_study_utils as utils

def translate_ftrace_to_generic():
    args = parse_command_line_args()
    if args.skip_report:
        txt_file = open(args.dat_file, 'r')
    else:
        txt_file = tempfile.NamedTemporaryFile(mode='w+b')
        dat_to_txt(args.dat_file, txt_file.name)
    try:
        os.remove(args.sql_file)
    except:
        pass  # Really doesn't matter
    conn = sqlite3.connect(args.sql_file)
    c = conn.cursor()
    utils.init_sqlite_file(c)

    pat = re.compile('-.*\[(.*)\]\D*(\d*)\.(\d*).*sched_switch\D*(\d*).*==>\D*(\d*)\S*\s+(.*)')
    min_time = min_timestamp(txt_file, pat)
    txt_file.seek(0)
    threads = {}
    processes = {}
    # Assumption: The pid of the idle task is 0
    threads[0] = 0
    event_id = 1
    process_id = 1
    for line in text_file:
        m = pat.search(line)
        if not m:
            print 'NO MATCH',line,
            continue
        cpu           = int(m.group(1))
        time_sec      = int(m.group(2))
        time_usec     = int(m.group(3))
        from_pid      = int(m.group(4))
        to_pid        = int(m.group(5))
        to_task_name  = m.group(6)
        time_ns_abs = ((time_sec * 1000000) + time_usec) * 1000
        time_ns = time_ns_abs - min_time
        print('CPU:%d stamp:%12d from:%8d to:%8d  to_name:%s' %
              (cpu, time_ns, from_pid, to_pid, to_task_name))

        # XXX Add core/thread sanity checking

        # Deal with the thread that's stopping
        if from_pid != 0 and (from_pid in threads):
            c.execute("INSERT INTO events VALUES (%d, %d, %d, 2, %d)" %
                      (event_id, from_pid, cpu, time_ns))
            event_id += 1
        # Deal with the thread that's starting
        process_id = utils.ensure_thread_in_db(
            to_pid, to_task_name, to_task_name, threads, processes, c, process_id)
        if to_pid != 0:
            c.execute("INSERT INTO events VALUES (%d, %d, %d, 1, %d)" %
                      (event_id, to_pid, cpu, time_ns))
            event_id += 1

    # Save (commit) the changes
    conn.commit()

    # We can also close the connection if we are done with it.
    # Just be sure any changes have been committed or they will be lost.
    conn.close()

def parse_command_line_args():
    argp = argparse.ArgumentParser(description='''Ftrace to generic trace translator''')
    argp.add_argument('dat_file', metavar='T',
                      help='Name of the input ftrace dat file')
    argp.add_argument('sql_file', metavar='G',
                      help='Name of the output sqlite3 file')
    argp.add_argument('--skip-report', action='store_true')
    return argp.parse_args()

def dat_to_txt(in_name, out_name):
    ''' dat_to_txt is just a thin wrapper around `trace-cmd report` '''
    try:
        # Doesn't seem like the most elegant way to invoke another
        # program, but it works
        cmd = ['trace-cmd report -i '+in_name+' > '+out_name]
        print cmd
        return_code = subprocess.call(cmd, shell=True)

        if return_code != 0:
            print out_name
            print 'Bad return code '+str(return_code)
            raise Exception()
    except:
        print 'Command just flat out failed'
        raise

def min_timestamp(lines, pat):
    min_time = -1
    for line in lines:
        m = pat.search(line)
        if not m:
            continue
        time_sec      = int(m.group(2))
        time_usec     = int(m.group(3))
        time_ns = ((time_sec * 1000000) + time_usec) * 1000
        if (min_time < 0) or (time_ns < min_time):
            min_time = time_ns
    return min_time

if __name__ == '__main__':
    translate_ftrace_to_generic()
