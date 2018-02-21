#!/usr/bin/env python3

import sys
import os
import re
import json
import merger
import numpy
import matplotlib.pyplot as plt
import datetime
from bisect import bisect_left

from functools import partial

class Infix(object):
    def __init__(self, func):
        self.func = func
    def __or__(self, other):
        return self.func(other)
    def __ror__(self, other):
        return Infix(partial(self.func, other))
    def __call__(self, v1, v2):
        return self.func(v1, v2)

@Infix
def o( f, g ):
    return lambda x: g( f( x ) )

class Object:
    pass

def dictToEv( d ):
    if d == None:
        return None
    ev = Object()
    ev.ts     = d[ 0 ]
    ev.source = d[ 1 ]
    ev.kind   = d[ 2 ]
    for k, v in d[ 3 ].items():
        setattr( ev, k, v )
    return ev

def lineToCont( j ):
    cont = Object()
    cont.begin        = dictToEv( j[ 0 ] )
    cont.end          = dictToEv( j[ 1 ] )
    cont.sched_ev     = dictToEv( j[ 2 ] )
    cont.parent_ts    = j[ 3 ]
    cont.children_tss = j[ 4 ]
    return cont

def eatTreeFile( f ):
    conts = []
    for line in f:
        ( json.loads |o| lineToCont |o| conts.append )( line )
    conts.sort( key=lambda c: c.begin.ts )
    keys = [ c.begin.ts for c in conts ]
    def lookup( ts ):
        i = bisect_left( keys, ts )
        if keys[ i ] != ts:
            print( "ERROR: Timestamp indexing thing" )
            sys.exit()
        return conts[ i ]
    position = 0
    for cont in conts:
        cont.pos = position
        position += 1
        cont.parent = None if cont.parent_ts is None else lookup( cont.parent_ts )
        del cont.parent_ts
        cont.children = []
        for child_ts in cont.children_tss:
            cont.children.append( lookup( child_ts ) )
        del cont.children_tss
    return conts

def analyzeDurs( cont, stats ):
    dur = cont.end.ts - cont.begin.ts
    recur = ( cont.sched_ev is not None ) and hasattr( cont.sched_ev, "recurring" ) and cont.sched_ev.recurring
    if recur:
        stats.durs_recur.append( dur )
    else:
        stats.durs_nrecur.append( dur )

def analyzeGaps( cont, stats ):
    if cont.parent is None:
        return
    gap = cont.begin.ts - cont.parent.end.ts
    if cont.sched_ev is None:
        stats.gaps_weird.append( gap )
    elif hasattr( cont.sched_ev.recurring ) and cont.sched_ev.recurring:
        stats.gaps_recur.append( gap )
    else:
        if cont.pos == parent.pos + 1:
            stats.gaps_uninterrupted.append( gap )
        else:
            stats.gaps_interrupted.append( gap )
        stats.gaps[ cont.sched_ev.name ].append( gap )

def analyzeChains( cont, stats ):
    ancestor = cont
    for depth in range( 2, 5 ):
        if ( ancestor.parent is None ) or ( ancestor.sched_ev is None ):
            return
        if hasattr( ancestor.sched_ev, "recurring" ) and ancestor.sched_ev.recurring:
            return
        stats.chains[ depth ].append( cont.end.ts - ancestor.parent.begin.ts )
        ancestor = ancestor.parent

def analyzeBranching( cont, stats ):
    stats.branching.append( len( cont.children ) )
    if cont.parent is not None:
        stats.pbranching.append( len( cont.parent.children ) )

def analuzeConcurrency( tree, stats ):
    liveConts = {}
    for cont in tree:

def analyzeSubtrees( cont, stats ):
    limits = [ 100, 1000, 10000 ]
    for limit in limits:
        def helper( descendant ):
            if descendant.end.ts > cont.begin.ts + limit:
                return None
            cont = Object()

            concat = lambda x, y: x + y
            subs = reduce( concat, map( helper, descendant.children ), [] )
            if non-empty ...

def analyzeTree( tree, stats ):
    for cont in tree:
        analyzeDurs( cont, stats )
        analyzeGaps( cont, stats )
        analyzeChains( cont, stats )
        analyzeBranching( cont, stats )
        analyzeSubtrees( cont, stats )

def fancyPlot( name, data ):
    if len( data ) < 1:
        return

    colors = [ plt.cm.viridis( 0.05 ) ]
    if len( data ) > 1:
        incr = 0.9 / ( len( data ) - 1 )
        for i in range( 1, len( data ) ):
            colors.append( plt.cm.viridis( 0.05 + ( incr * i ) ) )

    # csfont = {'fontname':'Comic Sans MS'}
    # hfont = {'fontname':'Helvetica'}

    fig = plt.figure( figsize=( 4, 3 ) )

    # plt.title( 'title', **csfont )
    # plt.xlabel( 'xlabel', **hfont )

    # plt.title( name )
    plt.xscale( "log" )
    host = fig.add_subplot(111)

    color_idx = 0
    for stuff in data:
        ns = stuff[ 0 ]
        splitx = stuff[ 1 ] if len( stuff ) > 1 else False
        splity = stuff[ 2 ] if len( stuff ) > 2 else False
        n = len( ns )
        ys = list( range( n ) )
        # for i in range( n ):
        #     ys[ i ] = ys[ i ] / n
        plotable = host.twinx() if splitx else host
        plotable = plotable.twiny() if splity else plotable
        plotable.set_xscale( "log" )
        # plotable.plot( ns, ys, color=colors[ color_idx ] )
        lines = plotable.plot( ns, ys )
        if color_idx == 0:
            plt.setp( lines, color="black" )
        elif color_idx == 1:
            plt.setp( lines, color="black" )
            plt.setp( lines, linewidth="2.0" )
            plt.setp( lines, linestyle="--" )
            plt.setp( lines, dashes=( 4, 2 ) )
        else:
            plt.setp( lines, color="black" )
            plt.setp( lines, linewidth="2.0" )
            plt.setp( lines, linestyle=":" )
            # plt.setp( lines, dashes=( 3, 3 ) )
        color_idx += 1

    if name is "gaps":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 500000, y1, y2 ) )

    if name is "lengths":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 1000000, y1, y2 ) )

    if False:
        plt.show()
    else:
        plt.savefig( "%s.pdf" % name, bbox_inches='tight' )

def printIles( name, items, iles ):
    L = len( items )
    print( "%s %10d " % ( name, L ), end="" )
    if L < 1:
        print( "EMPTY" )
        return
    for p in iles:
        print( "%8.0f " % items[ int( L * p ) ], end="" )
    print( "" )

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

def combineStats( s1, s2 ):
    s1.children_per_cont.extend ( s2.children_per_cont )
    s1.pchildren_per_edge.extend( s2.pchildren_per_edge )
    s1.macro_lengths.extend     ( s2.macro_lengths )
    s1.recur_lengths.extend     ( s2.recur_lengths )
    s1.not_recur_lengths.extend ( s2.not_recur_lengths )
    s1.micro_lengths.extend     ( s2.micro_lengths )
    s1.before_counts.extend     ( s2.before_counts )
    s1.after_counts.extend      ( s2.after_counts )
    s1.chain2_lengths.extend    ( s2.chain2_lengths )
    s1.chain3_lengths.extend    ( s2.chain3_lengths )
    s1.chain4_lengths.extend    ( s2.chain4_lengths )
    s1.spans.extend             ( s2.spans )
    for key, value in s2.api_kind.items():
        try:
            s1.api_kind[ key ] += value
        except:
            s1.api_kind[ key ] = value
    for key, value in s2.gaps.items():
        try:
            s1.gaps[ key ].extend( value )
        except:
            s1.gaps[ key ] = value

def showStats( s ):
    all_lengths = s.macro_lengths[:]
    all_lengths.extend( s.micro_lengths )
    allnr_lengths = s.not_recur_lengths[:]
    allnr_lengths.extend( s.micro_lengths )

    s.macro_lengths.sort()
    s.recur_lengths.sort()
    s.not_recur_lengths.sort()
    s.micro_lengths.sort()
    all_lengths.sort()
    allnr_lengths.sort()
    s.children_per_cont.sort()
    s.pchildren_per_edge.sort()
    s.before_counts.sort()
    s.after_counts.sort()
    s.chain2_lengths.sort()
    s.chain3_lengths.sort()
    s.chain4_lengths.sort()
    s.spans.sort()
    shorties1m = {}
    shorties100u = {}
    def incrShorty1m( sched_name ):
        try:
            shorties1m[ sched_name ] += 1
        except:
            shorties1m[ sched_name ] = 1
    def incrShorty100u( sched_name ):
        try:
            shorties100u[ sched_name ] += 1
        except:
            shorties100u[ sched_name ] = 1

    aggregate_gaps = []
    for sched_name, value in s.gaps.items():
        s.gaps[ sched_name ].sort()
        for gap in value:
            if gap > 1000:
                break
            incrShorty1m( sched_name )
            if gap > 100:
                continue
            incrShorty100u( sched_name )

        # print( "GAPS \"%s\" %d" % ( sched_name, len( value ) ) )
        # if sched_name not in [ "interrupted", "uninterrupted", "RECURRING - RECURRING", "RECURRING - SendRequest" ]:
        #     aggregate_gaps.extend( value )
        #     shorts = 0
        #     for x in value:
        #         if x < 100:
        #             shorts += 1
        #     print( "BLROPS: %s %s" % ( sched_name, shorts ) )
    aggregate_gaps.sort()

    not_recur_gaps = []
    for sched_name, value in s.gaps.items():
        if sched_name == "interrupted" or sched_name == "uninterrupted" or sched_name.startswith( "RECURRING" ):
            continue
        not_recur_gaps.extend( value )
    not_recur_gaps.sort()

    printIles( "AL", s.macro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "IL", s.micro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    printIles( "GA", s.gaps[ "uninterrupted" ], [ 0.01, 0.05,  0.1,  0.5, 0.9 ] )
    printIles( "BC", s.before_counts, [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "CC", s.after_counts,  [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "C2", s.chain2_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "C3", s.chain3_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    printIles( "C4", s.chain4_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )

    print( "SHORTIES 1 millisecond %s" % shorties1m )
    print( "SHORTIES 100 microseconds %s" % shorties100u )

    # print( s.api_kind )

    # fancyPlot( "lengths", [ [ s.recur_lengths ], [ allnr_lengths ] ] )
    short_gaps  = list( filter( lambda g: g < 1000, aggregate_gaps ) )
    short_spans = list( filter( lambda s: s < 1000, s.spans ) )
    fancyPlot( "gap-span", [ [ short_gaps ], [ short_spans ] ] )
    # fancyPlot( "children", [ [ s.children_per_cont ], [ s.pchildren_per_edge ] ] )
    # fancyPlot( "gaps", [ [ s.gaps[ "uninterrupted" ] ], [ s.gaps[ "interrupted" ] ], [ not_recur_gaps ] ] )
    # fancyPlot( "micros", [ [ s.micro_lengths ], [ s.after_counts, False, True ] ] )

    # fancyPlot( "gaps", [ [ s.gaps[ "uninterrupted" ] ], [ s.gaps[ "interrupted" ] ], [ aggregate_gaps ] ] )
    # fancyPlot( "chains", [ [ s.chain2_lengths ], [ s.chain3_lengths ], [ s.chain4_lengths ] ] )

def stacker( trace ):
    all_continuations = []
    most_recent_task = None
    global_continuation = Object()
    global_continuation.tkid = None
    global_continuation.events = []
    continuation_stack = [ global_continuation ]
    max_depth = 1
    call_depths = GrowingList()
    micro_depths = GrowingList()
    macro_lengths = []
    recur_lengths = []
    not_recur_lengths = []
    micro_lengths = []
    before_counts = []
    after_counts = []
    gaps ={ "interrupted":[],
            "uninterrupted":[]
    }
    chain2_lengths = []
    chain3_lengths = []
    chain4_lengths = []
    pc_good = 0
    missing_parent = 0
    missing_end = 0

    parent_of = {}
    parent_of[ "micro" ] = None
    api_kind = {}
    def incrApiKind( kind ):
        try:
            api_kind[ kind ] += 1
        except:
            api_kind[ kind ] = 1

    def printStack():
        print( "STACK ", end=" " )
        for x in continuation_stack:
            print( x[ "task" ], end=" - " )
        print( "" )

    line_count = 0

    for line in trace:
        line_count += 1
        max_depth = max( max_depth, len( continuation_stack ) )

        ev = lineToEvent( line )
        curr_cont = continuation_stack[ -1 ]
        curr_cont.events.append( ev )

        if ev.source == "macro":
            if ev.kind == "scheduled":
                incrApiKind( ev.name )
                call_depths[ len( continuation_stack ) ] += 1
                if curr_cont != global_continuation:
                    parent_of[ ev.tkid ] = ( curr_cont, ev )

            elif ev.kind == "ctor":
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                curr_cont.events.append( next_cont )
                continuation_stack.append( next_cont )
                if ev.tkid in parent_of:
                    ( parent_cont, sched_ev ) = parent_of[ ev.tkid ]
                    parent_cont.children.append( next_cont )
                    try:
                        gap = ev.ts - parent_cont.end.ts
                        if most_recent_task != parent_cont.begin.tkid:
                            gaps[ "interrupted" ].append( gap )
                        else:
                            gaps[ "uninterrupted" ].append( gap )
                        pc_good += 1
                        try:
                            gaps[ sched_ev.name ].append( gap )
                        except:
                            gaps[ sched_ev.name ] = [ gap ]
                        if gap < 100 and ( "RECURRING" not in sched_ev.name ):
                            print( "!!! %30s %3d" % ( sched_ev.name, gap ) )
                    except:
                        missing_end += 1
                else:
                    missing_parent += 1
                if len( continuation_stack ) > 3:
                    pass # printStack()

            elif ev.kind == "dtor":
                most_recent_task = ev.tkid
                blah = continuation_stack.pop()
                # print( "BLAH %s" % vars( blah ) )
                if ev.tkid != curr_cont.begin.tkid:
                    print( "TASK MISMATCH '%s' '%s'" % ( ev.tkid, curr_cont.begin.tkid ) )
                    exit()
                macro_lengths.append( ev.ts - curr_cont.begin.ts )
                curr_cont.end = ev
                if ev.tkid in parent_of:
                    ( parent_cont, sched_ev ) = parent_of[ ev.tkid ]
                    if sched_ev.name.startswith( "RECURRING" ):
                        recur_lengths.append( ev.ts - curr_cont.begin.ts )
                    else:
                        not_recur_lengths.append( ev.ts - curr_cont.begin.ts )
                    curr_cont.chain2 = ev.ts - parent_cont.begin.ts
                    chain2_lengths.append( curr_cont.chain2 )
                    try:
                        diff = ev.ts - parent_cont.end.ts
                        curr_cont.chain3 = parent_cont.chain2 + diff
                        chain3_lengths.append( curr_cont.chain3 )
                        try:
                            curr_cont.chain4 = parent_cont.chain3 + diff
                            chain4_lengths.append( curr_cont.chain4 )
                        except Exception as ex:
                            pass
                    except Exception as ex:
                        pass
                else:
                    not_recur_lengths.append( ev.ts - curr_cont.begin.ts )

                if ev.recurring:
                    ev.name = "RECURRING"
                    try:
                        ( parent_cont, sched_ev ) = parent_of[ ev.tkid ]
                        if sched_ev.name.startswith( "RECURRING" ):
                            ev.name = sched_ev.name
                        else:
                            ev.name = "RECURRING - %s" % sched_ev.name
                    except:
                        pass
                    parent_of[ ev.tkid ] = ( curr_cont, ev )

            elif ev.kind == "canceled" or ev.kind == "all_canceled":
                pass

            else:
                print( "WEIRD MACRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "micro":
            if ev.kind == "enq":
                call_depths[ len( continuation_stack ) ] += 1
                if curr_cont != global_continuation and parent_of[ "micro" ] == None:
                    ev.name = "MICRO"
                    parent_of[ "micro" ] = ( curr_cont, ev )

            elif ev.kind == "before_loop":
                before_counts.append( ev.num_tasks )
                next_cont = Object()
                next_cont.begin    = ev
                next_cont.events   = []
                next_cont.children = []

                all_continuations.append( next_cont )
                curr_cont.events.append( next_cont )
                continuation_stack.append( next_cont )
                if parent_of[ "micro" ] != None:
                    ( parent_cont, sched_ev ) = parent_of[ "micro" ]
                    parent_cont.children.append( next_cont )
                    try:
                        gap = ev.ts - parent_cont.end.ts
                        if most_recent_task != parent_cont.begin.tkid:
                            gaps[ "interrupted" ].append( gap )
                        else:
                            gaps[ "uninterrupted" ].append( gap )
                        pc_good += 1
                        try:
                            gaps[ "micro" ].append( gap )
                        except:
                            gaps[ "micro" ] = [ gap ]
                    except:
                        missing_end += 1
                else:
                    missing_parent += 1
                if len( continuation_stack ) > 3:
                    pass # printStack()

            elif ev.kind.startswith( "start" ):
                if len( continuation_stack ) > 3:
                    pass #printStack()
                micro_depths[ ev.num_tasks ] += 1

            elif ev.kind.startswith( "done" ):
                pass

            elif ev.kind == "after_loop":
                continuation_stack.pop()
                if curr_cont.begin.tkid != "micro":
                    print( "MICROTASK MISMATCH %s\n%s" % ( vars( curr_cont.begin ), vars( ev ) ) )
                    exit()
                curr_cont.end = ev
                micro_lengths.append( ev.ts - curr_cont.begin.ts )
                after_counts.append( ev.count )

                try:
                    ( parent_cont, sched_ev ) = parent_of[ "micro" ]
                    curr_cont.chain2 = ev.ts - parent_cont.begin.ts
                    chain2_lengths.append( curr_cont.chain2 )
                    try:
                        diff = ev.ts - parent.end.ts
                        curr_cont.chain3 = parent_cont.chain2 + diff
                        chain3_lengths.append( curr_cont.chain3 )
                        try:
                            curr_cont.chain4 = parent_cont.chain3 + diff
                            chain4_lengths.append( curr_cont.chain4 )
                        except:
                            pass
                    except:
                        pass
                except:
                    pass

                parent_of[ "micro" ] = None
                most_recent_task = ev.tkid

            else:
                print( "WEIRD MICRO TASK KIND %s" % ev.kind )
                exit()

        elif ev.source == "exec":
            curr_cont.events.append( ev )

        else:
            print( "WEIRD SOURCE %s" %source )
            exit()

    spans = []
    children_per_cont = []
    pchildren_per_edge = []
    for parent in all_continuations:
        n = len( parent.children )
        children_per_cont.append( n )
        for i in range( n ):
            pchildren_per_edge.append( n )
        for child in parent.children:
            try:
                end = child.end.ts
            except:
                print( "MISSING END" );
                continue
            try:
                begin = parent.begin.ts
            except:
                print( "MISSING BEGIN" )
                continue
            span = end - begin
            spans.append( span )

    print( "w00t? %d %d" % ( max_depth, len( global_continuation.events ) ) )
    print( "CALL %s" % call_depths )
    print( "MICRO %s" % micro_depths )

    print( "Good: %d - Missing Parent: %d - Missing End: %d" %
           ( pc_good, missing_parent, missing_end ) )

    print( "." )

    stats = Object()
    stats.children_per_cont = children_per_cont
    stats.pchildren_per_edge = pchildren_per_edge
    stats.macro_lengths = macro_lengths
    stats.recur_lengths = recur_lengths
    stats.not_recur_lengths = not_recur_lengths
    stats.micro_lengths = micro_lengths
    stats.before_counts = before_counts
    stats.after_counts = after_counts
    stats.gaps = gaps
    stats.spans = spans
    stats.chain2_lengths = chain2_lengths
    stats.chain3_lengths = chain3_lengths
    stats.chain4_lengths = chain4_lengths
    stats.api_kind = api_kind

    return stats

def analyze( basepath, ranges, render_processes, stats ):
    print( "======================== analyze %s ======================== %s" % ( basepath, render_processes ) )
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
        print( "File: %s %s" % ( fname, result ) )
        if result is None:
            continue
        id = int( result.group( 2 ) )
        add( id, fname )

    for id in render_processes:
        ts1 = datetime.datetime.now()
        print( "Starting process: %s" % id )
        try:
            paths = id_path_map[ id ]
        except:
            print( "No files for pid %d" % id )
            continue

        merged = merger.mergeTraces( merger.parseTraces( paths ) )
        print( "lines: %d" % ( len( merged ) ) )

        for tid, trace in splitByThread( merged ).items():
            print( "THREAD %s" % tid )
            s = stacker( trace )
            combineStats( stats, s )
            # tracer( trace )
        ts2 = datetime.datetime.now()
        print( "[TS] Done with process %s: %s" % ( id, ( ts2 - ts1 ) ) )


def analyzeDir( dir, stats ):
    for fname in os.listdir( dir ):
        if fname == ".DS_Store":
            continue
        path = os.path.join( dir, fname )
        with open( path ) as f:
            eatTreeFile( f )

        # analyzeDir( path, stats )

def main():
    trees_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "./Trees"
    stats = Object()
    stats.children_per_cont = []
    stats.pchildren_per_edge = []
    stats.macro_lengths = []
    stats.recur_lengths = []
    stats.not_recur_lengths = []
    stats.micro_lengths = []
    stats.before_counts = []
    stats.after_counts = []
    stats.gaps = {}
    stats.spans = []
    stats.chain2_lengths = []
    stats.chain3_lengths = []
    stats.chain4_lengths = []
    stats.api_kind = {}

    for fname in os.listdir( trees_dir ):
        if fname == ".DS_Store":
            continue
        print( "======================== eat %s ========================" % fname )
        path = os.path.join( trees_dir, fname )
        analyzeDir( path, stats )

    # showStats( stats )

if __name__ == "__main__":
    # execute only if run as a script
    main()



# par1 = host.twinx()
# par2 = host.twinx()

# host.set_xlim(0, 2)
# host.set_ylim(0, 2)
# par1.set_ylim(0, 4)
# par2.set_ylim(1, 65)

# host.set_xlabel("Distance")
# host.set_ylabel("Density")
# par1.set_ylabel("Temperature")
# par2.set_ylabel("Velocity")

# # color1 = plt.cm.viridis(0)
# # color2 = plt.cm.viridis(0.5)
# # color3 = plt.cm.viridis(.9)

# p1, = host.plot([0, 1, 2], [0, 1, 2], color=color1,label="Density")
# p2, = par1.plot([0, 1, 2], [0, 3, 2], color=color2, label="Temperature")
# p3, = par2.plot([0, 1, 2], [50, 30, 15], color=color3, label="Velocity")

# lns = [p1, p2, p3]
# host.legend(handles=lns, loc='best')

# # right, left, top, bottom
# par2.spines['right'].set_position(('outward', 60))
# # no x-ticks
# # par2.xaxis.set_ticks([])
# # Sometimes handy, same for xaxis
# #par2.yaxis.set_ticks_position('right')

# host.yaxis.label.set_color(p1.get_color())
# par1.yaxis.label.set_color(p2.get_color())
# par2.yaxis.label.set_color(p3.get_color())

# plt.savefig("pyplot_multiple_y-axis.png", bbox_inches='tight')
