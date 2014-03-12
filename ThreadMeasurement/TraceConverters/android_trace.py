#!/usr/bin/env python

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This program traces system-wide activity on Android devices and outputs trace
results in text, sqlite3, and dat files. 

Sample usage:
  $ python android_trace.py --time=30 -o TestTrace

  Above command will trace system for 30 seconds, and will output 
  'TestTrace.txt', 'TestTrace.sqlite3', and 'TestTrace.dat'.

  You can also run the program with default values:
  $ python android_trace.py 

  This will run the program the with default time of 5 seconds and default 
  filename of 'trace'. 
"""

import errno, optparse, os, select, subprocess, sys, time, zlib, re, sqlite3
import thread_study_utils as utils

def add_adb_serial(command, serial):
  if serial != None:
    command.insert(1, serial)
    command.insert(1, '-s')


def parse_command_line_args():
  parser = optparse.OptionParser()
  parser.add_option('-o', dest='output_file', help='write HTML to FILE',
                    default='trace', metavar='FILE')
  parser.add_option('-t', '--time', dest='trace_time', type='int',
                    help='trace for N seconds', metavar='N')
  parser.add_option('-b', '--buf-size', dest='trace_buf_size', type='int',
                    default=3000, help='use a trace buffer size of N KB', metavar='N')
  options, args = parser.parse_args()
  return options


def trace_with_adb(options):
  atrace_args = ['adb', 'shell', 'atrace', '-z']
  atrace_args.append('-f') # add option of cpu frequency changes trace
  atrace_args.append('-i') # add option of cpu idle events trace
  atrace_args.append('-s') # add option of cpu scheduler trace 
  
  if options.trace_time is not None:
    if options.trace_time > 0:
      atrace_args.extend(['-t', str(options.trace_time)])
    else:
      parser.error('the trace time must be a positive number')
  if options.trace_buf_size is not None:
    if options.trace_buf_size > 0:
      atrace_args.extend(['-b', str(options.trace_buf_size)])
    else:
      parser.error('the trace buffer size must be a positive number')

  script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))

  filename = options.output_file
  txt_filename = filename + ".txt"
  dat_filename = filename + ".dat"

  trace_started = False
  leftovers = ''
  adb = subprocess.Popen(atrace_args, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
  dec = zlib.decompressobj()
  
  while True:
    ready = select.select([adb.stdout, adb.stderr], [], [adb.stdout, adb.stderr])
    if adb.stderr in ready[0]:
      err = os.read(adb.stderr.fileno(), 4096)
      sys.stderr.write(err)
      sys.stderr.flush()
    if adb.stdout in ready[0]:
      out = leftovers + os.read(adb.stdout.fileno(), 4096)
      out = out.replace('\r\n', '\n')
      if out.endswith('\r'):
        out = out[:-1]
        leftovers = '\r'
      else:
        leftovers = ''
      if not trace_started:
        lines = out.splitlines(True)
        out = ''
        for i, line in enumerate(lines):
          # Read the text portion of the output and watch for the 'TRACE:' 
          # marker that indicates the start of the trace data.
          if line == 'TRACE:\n':
            sys.stdout.write("downloading trace...")
            sys.stdout.flush()
            out = ''.join(lines[i+1:])
            txt_file = open(txt_filename, 'w')
            dat_file = open(dat_filename, 'w')
            trace_started = True
            break
          elif 'TRACE:'.startswith(line) and i == len(lines) - 1:
            leftovers = line + leftovers
          else:
            sys.stdout.write(line)
            sys.stdout.flush()
      if len(out) > 0:
        dat_file.write(out) # writes binary dat format file
        out = dec.decompress(out)
      txt_out = out.replace('\n', '\\n\\\n')
      if len(txt_out) > 0:
        txt_file.write(txt_out)
    result = adb.poll()
    if result is not None:
      break
  if result != 0:
    print >> sys.stderr, 'adb returned error code %d' % result
  elif trace_started:
    txt_out = dec.flush().replace('\n', '\\n\\\n').replace('\r', '')
    if len(txt_out) > 0:
      txt_file.write(txt_out)
    dat_file.close()
    print " done\n\n    wrote file://%s/%s\n" % (os.getcwd(), options.output_file)
  else:
    print >> sys.stderr, ('An error occured while capturing the trace.  Output ' +
      'file was not written.')


def translate_trace_to_generic(filename):
  
  txt_file = open(filename + '.txt', 'r')
  sql_filename = filename + '.sqlite3'
  
  try:
      os.remove(sql_filename)
  except:
      pass  # Really doesn't matter
  conn = sqlite3.connect(sql_filename)
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
  for line in txt_file:
      m = pat.search(line)
      if not m:
          print 'NO MATCH'
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

  txt_file.close()


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
  options = parse_command_line_args()
  trace_with_adb(options)
  translate_trace_to_generic(options.output_file)
