'''A few helper functions for the Thread study.'''

def init_sqlite_file(c):
    ''' Set up the tables. '''
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

def ensure_thread_in_db(tid, proc_name, friendly_name,
                        threads, processes, c, process_id):
    if not tid in threads:
        if not proc_name in processes:
            c.execute("INSERT INTO processes VALUES (%d, '%s', '%s')" %
                      (process_id, proc_name, friendly_name))
            processes[proc_name] = process_id
            process_id += 1
        to_task_id = processes[proc_name]
        c.execute("INSERT INTO threads VALUES (%d, '%d', %d)" %
                  (tid, tid, to_task_id))
        threads[tid] = proc_name
    return process_id

