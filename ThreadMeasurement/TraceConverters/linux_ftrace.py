#!/usr/bin/env python

'''This program takes an input file name and an output file name as
parameters.  The input file is assumed to be in the Linux Ftrace .dat
format.  This program reads the input file and converts it into the
thread measurement project's simple sqlite format.'''

import argparse, subprocess, tempfile, re, sqlite3, os

def dat_to_txt(in_name, out_name):
    try:
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

def init_sqlite_file(c):
    tables = []
    tables.append(('event_kinds',
        ['id INTEGER PRIMARY KEY', 'name TEXT']))
    tables.append(('processes',
        ['id INTEGER PRIMARY KEY', 'name TEXT', 'friendly_name TEXT']))
    tables.append(('threads',
        ['id INTEGER PRIMARY KEY', 'name TEXT', 'process INTEGER',
         'FOREIGN KEY(process) REFERENCES processes(id)']))
    evs_table = []
    evs_table.append('id INTEGER PRIMARY KEY')
    evs_table.append('thread INTEGER')
    evs_table.append('core INTEGER')
    evs_table.append('kind INTEGER')
    evs_table.append('timestamp INTEGER')
    evs_table.append('FOREIGN KEY(thread) REFERENCES threads(id)')
    evs_table.append('FOREIGN KEY(kind) REFERENCES event_kinds(id)')
    tables.append(('events', evs_table ))

    for (name, columns) in tables:
        cmd = 'CREATE TABLE '+name+'\n    ('
        first = True
        for column in columns:
            if not first:
                cmd += ', '
            cmd += column
            first = False
        cmd += ')'
        print cmd
        c.execute(cmd)

    c.execute("INSERT INTO event_kinds VALUES (1,'start')")
    c.execute("INSERT INTO event_kinds VALUES (2,'stop')")


def translate_ftrace_to_generic():
    argp = argparse.ArgumentParser(description='''Ftrace to generic trace translator''')
    argp.add_argument('dat_file', metavar='T',
                      help='Name of the input ftrace dat file')
    argp.add_argument('sql_file', metavar='G',
                      help='Name of the output sqlite3 file')
    args = argp.parse_args()

    temp = tempfile.NamedTemporaryFile(mode='w+b')

    dat_to_txt(args.dat_file, temp.name)

    try:
        os.remove(args.sql_file)
    except:
        pass  # Really doesn't matter
    conn = sqlite3.connect(args.sql_file)
    c = conn.cursor()

    init_sqlite_file(c)

    threads = {}
    processes = {}
    threads[0] = 0
    pat = re.compile('-.*\[(.*)\]\D*(\d*)\.(\d*).*sched_switch\D*(\d*).*==>\D*(\d*)\S*\s+(.*)')
    event_id = 1
    process_id = 1
    for line in temp:
        m = pat.search(line)
        if m:
            cpu_txt       = m.group(1)
            time_sec_txt  = m.group(2)
            time_usec_txt = m.group(3)
            from_pid_txt  = m.group(4)
            to_pid_txt    = m.group(5)
            to_task_name  = m.group(6)
            cpu       = int(cpu_txt)
            time_sec  = int(time_sec_txt)
            time_usec = int(time_usec_txt)
            from_pid  = int(from_pid_txt)
            to_pid    = int(to_pid_txt)
            time = ((time_sec * 1000000) + time_usec) * 1000
            print('CPU:%d stamp:%d from:%8d to:%8d  to_name:%s' %
                  (cpu, time, from_pid, to_pid, to_task_name))
            if from_pid != 0 and (from_pid in threads):
                c.execute("INSERT INTO events VALUES (%d, %d, %d, 2, %d)" %
                          (event_id, from_pid, cpu, time))
                event_id += 1
            if not to_pid in threads:
                if not to_task_name in processes:
                    c.execute("INSERT INTO processes VALUES (%d, '%s', '%s')" %
                              (process_id, to_task_name, to_task_name))
                    processes[to_task_name] = process_id
                    process_id += 1
                to_task_id = processes[to_task_name]
                c.execute("INSERT INTO threads VALUES (%d, '%d', %d)" %
                          (to_pid, to_pid, to_task_id))
                threads[to_pid] = to_task_name
                    
            if to_pid != 0:
                c.execute("INSERT INTO events VALUES (%d, %d, %d, 1, %d)" %
                          (event_id, to_pid, cpu, time))
                event_id += 1
        else:
            print 'NO MATCH',line,

    # Save (commit) the changes
    conn.commit()

    # We can also close the connection if we are done with it.
    # Just be sure any changes have been committed or they will be lost.
    conn.close()

if __name__ == '__main__':
    translate_ftrace_to_generic()
