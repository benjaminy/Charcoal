#!/usr/bin/env python3

import sys
import os
import json
import numpy
import matplotlib.pyplot as plt
from bisect import bisect_left
from functools import reduce

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

subtree_limits = [ 100, 1000, 10000, 100000 ]
stupid0 = 0
stupid1 = 0
stupid2 = 0
stupid3 = 0

def printCont( c ):
    print( "[ %d, %s ]" % ( c.begin.ts, "?" if c.sched_ev is None else c.sched_ev.name ), end="" )

# Begin code for input reading

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

def lineToContinuation( j ):
    cont = Object()
    cont.begin        = dictToEv( j[ 0 ] )
    cont.end          = dictToEv( j[ 1 ] )
    cont.sched_ev     = dictToEv( j[ 2 ] )
    cont.parent_ts    = j[ 3 ]
    cont.children_tss = j[ 4 ]
    if ( not hasattr( cont, "begin" ) ) or cont.begin is None:
        print( "MISSING BEGIN" )
    if ( not hasattr( cont, "end" ) ) or cont.end is None:
        print( "MISSING END" )
    return cont

def eatTreeFile( f ):
    conts = []
    for line in f:
        ( json.loads |o| lineToContinuation |o| conts.append )( line )
    conts.sort( key=lambda c: c.begin.ts )
    keys = [ c.begin.ts for c in conts ]
    def lookup( ts ):
        idx = bisect_left( keys, ts )
        if keys[ idx ] != ts:
            print( "ERROR: Timestamp indexing thing" )
            sys.exit()
        return conts[ idx ]

    # Replace parent and child timestamps with object references
    position = 0
    for cont in conts:
        cont.pos = position
        position += 1
        cont.parent = None if cont.parent_ts is None else lookup( cont.parent_ts )
        del cont.parent_ts
        cont.children = list( map( lookup, cont.children_tss ) )
        del cont.children_tss
    return conts

def analyzeDir( dir, stats ):
    for fname in os.listdir( dir ):
        if fname == ".DS_Store":
            continue
        path = os.path.join( dir, fname )
        with open( path ) as f:
            tree = eatTreeFile( f )
            analyzeTree( tree, stats )

        # analyzeDir( path, stats )

# End code for input reading

# Begin code for Analyzing trees
def isRecur( c ):
    return ( c.sched_ev is not None ) and hasattr( c.sched_ev, "recurring" ) and c.sched_ev.recurring

def analyzeDurations( cont, stats ):
    if ( not hasattr( cont, "begin" ) ) or cont.begin is None:
        return
    if ( not hasattr( cont, "end" ) ) or cont.end is None:
        return


    duration = cont.end.ts - cont.begin.ts
    if isRecur( cont ):
        stats.durs_recur.append( duration )
    else:
        stats.durs_nrecur.append( duration )
        if cont.begin.source == "macro":
            stats.durs_macro.append( duration )
        elif cont.begin.source == "micro":
            stats.durs_micro.append( duration )
        else:
            sys.exit()

def analyzeGaps( cont, stats ):
    if ( cont.begin is None ) or ( cont.parent is None ) or ( cont.parent.end is None ):
        return

    gap = cont.begin.ts - cont.parent.end.ts
    if gap < 0:
        stats.neg_gaps += 1
        return
    stats.pos_gaps += 1

    if cont.sched_ev is None:
        stats.gaps_weird.append( gap )
    elif isRecur( cont ):
        stats.gaps_recur.append( gap )
    else:
        if cont.pos == cont.parent.pos + 1:
            stats.gaps_uninterrupted.append( gap )
        else:
            stats.gaps_interrupted.append( gap )
        name = cont.sched_ev.name
        try:
            stats.gaps[ name ].append( gap )
        except:
            stats.gaps[ name ] = [ gap ]

def analyzeChains( cont, stats ):
    if cont.end is None:
        return
    ancestor = cont
    end = cont.end.ts
    for depth in range( 2, 5 ):
        if ( ancestor.parent is None ) or ( ancestor.sched_ev is None ):
            return
        if isRecur( ancestor ):
            return
        duration = end - ancestor.parent.begin.ts
        if ( depth == 2 ) and ( duration <= 100 ):
            print( "CHAIN %d %d" % ( ancestor.parent.begin.ts, ancestor.begin.ts ) )
        stats.chains[ depth ].append( duration )
        ancestor = ancestor.parent

def analyzeBranching( cont, stats ):
    global stupid0
    global stupid1
    global stupid2
    global stupid3
    stupid0 += 1
    stats.branching_A.append( len( cont.children ) )
    rc = 0
    nc = 0
    for c in cont.children:
        if isRecur( c ):
            rc += 1
        else:
            nc += 1
    if rc == 0:
        if nc == 0:
            if isRecur( cont ):
                stats.branching_R.append( 0 )
            else:
                stats.branching_N.append( 0 )
        else:
            stats.branching_N.append( nc )
            if isRecur( cont ):
                stupid1 += 1
    else:
        if nc == 0:
            stats.branching_R.append( rc )
            if not isRecur( cont ):
                stupid2 += 1
        else:
            stupid3 += 1
            if isRecur( cont ):
                stats.branching_R.append( nc + rc )
            else:
                stats.branching_N.append( nc + rc )

def analyzeConcurrency( tree, stats ):
    liveConts = set()
    liveContsNR = set()
    for cont in tree:
        if cont.begin.ts in liveConts:
            liveConts.remove( cont.begin.ts )
        if cont.begin.ts in liveContsNR:
            liveContsNR.remove( cont.begin.ts )
        stats.concurrency.append( len( liveConts ) )
        if not isRecur( cont ):
            stats.concurrency_NA.append( len( liveConts ) )
            stats.concurrency_NN.append( len( liveContsNR ) )
        for c in cont.children:
            liveConts.add( c.begin.ts )
            if not isRecur( c ):
                liveContsNR.add( c.begin.ts )
    if len( liveConts ) != 0:
        print( "!!!!! analyzeConcurrency %d" % ( len( liveConts ) ) )
    if len( liveContsNR ) != 0:
        print( "!!!!! analyzeConcurrencyNR %d" % ( len( liveContsNR ) ) )

def analyzeSubtrees( cont, stats ):
    def printSubtree( tree, depth ):
        print( " " * depth, end="" )
        if tree is None:
            print( "_", end="" )
            return
        printCont( tree.orig )
        if len( tree.children ) < 1:
            print( "[]" )
            return
        print( "[" )
        for c in tree.children:
            printSubtree( c, depth + 4 )
        print( ( " " * depth ) + "]" )

    for limit in subtree_limits:
        def helper( parent ):
            if isRecur( parent ):
                return False
            if ( parent.end is None ) or ( ( parent.end.ts - cont.begin.ts ) > limit ):
                return None
            node = Object()
            node.orig = parent
            node.children = []
            latest = parent.end.ts
            for child in parent.children:
                thing = helper( child )
                if thing == False:
                    continue
                if thing == None:
                    node.children.append( None )
                    continue
                ( child_latest, child_tree ) = thing
                node.children.append( child_tree )
                latest = max( latest, child_latest )
            return ( latest, node )

        thing = helper( cont )
        if thing == False or thing == None:
            continue
        ( latest, tree ) = thing
        if not any( c is not None for c in tree.children ):
            continue
        if ( cont.parent is not None ) and ( latest - cont.parent.begin.ts ) <= limit:
            continue

        # finally, we have a subtree to analyze!
        def isLinear( node ):
            if node == None:
                return True
            if len( node.orig.children ) > 1:
                return False
            return all( isLinear( c ) for c in node.children )

        def binaryNone( f ):
            def g( a, b ):
                if a is None:
                    return b
                if b is None:
                    return a
                return f( a, b )
            return g

        minNone = binaryNone( min )
        maxNone = binaryNone( max )

        def earliestDeadEnd( node ):
            if node is None:
                return None
            if len( node.orig.children ) < 1:
                return node.orig.end.ts
            return reduce( minNone, map( earliestDeadEnd, node.children ), None )

        def latestWithOutsideChild( node ):
            if ( node is None ) or len( node.children ) < 1:
                return None
            latest = None
            if any( c is None for c in node.children ):
                latest = node.orig.begin.ts
            return reduce( maxNone, map( latestWithOutsideChild, node.children ), latest )

        def escapeCountStrict( node ):
            if node is None:
                return 0
            escapersHere = 1 if any( c is None for c in node.children ) else 0
            return reduce( lambda x, y: x + y, map( escapeCountStrict, node.children ), escapersHere )

        def escapeCountLoose( node ):
            if node is None:
                return 1
            return reduce( lambda x, y: x + y, map( escapeCountLoose, node.children ), 0 )

        def anyBranching( node ):
            if node is None:
                return False
            if 1 < len( node.children ):
                return True
            return any( anyBranching( c ) for c in node.children )

        def interrupted( node ):
            poses = []
            def collect( n ):
                if n is None:
                    return
                poses.append( n.orig.pos )
                for c in n.children:
                    collect( c )
            collect( node )
            poses.sort()
            for i in range( len( poses ) - 1 ):
                if ( poses[ i ] + 1 ) != poses[ i + 1 ]:
                    if limit == 1000:
                        print( "BLAH %s" % poses )
                    return True
            return False

        if isLinear( tree ):
            stats.trees[ limit ][ "linear" ] += 1
        else:
            stats.trees[ limit ][ "nonlinear" ] += 1
        earliestDE = earliestDeadEnd( tree )
        latestWOCh = latestWithOutsideChild( tree )
        if ( earliestDE is not None ) and ( latestWOCh is not None ):
            stats.trees[ limit ][ "internalBranching" ] += 1
            if earliestDE < latestWOCh:
                stats.trees[ limit ][ "maybeWaiting" ] += 1
                if limit == 10:
                    printSubtree( tree, 4 )
        if anyBranching( tree ):
            stats.trees[ limit ][ "anyBranching" ] += 1
        if escapeCountStrict( tree ) > 1:
            stats.trees[ limit ][ "multipleEscapes" ] += 1
        if interrupted( tree ):
            stats.trees[ limit ][ "interrupted" ] += 1
        # print( "%s" % stats.trees[ limit ] )

# def anayzeMicros( tree ):
#                 elif ev.kind.startswith( "start" ):
#                 if len( continuation_stack ) > 3:
#                     pass #printStack()
#                 micro_depths[ ev.num_tasks ] += 1

#             elif ev.kind.startswith( "done" ):
#                 pass

#             elif ev.kind == "after_loop":
#                 continuation_stack.pop()
#                 if curr_cont.begin.tkid != "micro":
#                     print( "MICROTASK MISMATCH %s\n%s" % ( vars( curr_cont.begin ), vars( ev ) ) )
#                     exit()
#                 curr_cont.end = ev
#                 micro_lengths.append( ev.ts - curr_cont.begin.ts )
#                 after_counts.append( ev.count )

#                 try:
#                     ( parent_cont, sched_ev ) = parent_of[ "micro" ]
#                     curr_cont.chain2 = ev.ts - parent_cont.begin.ts
#                     chain2_lengths.append( curr_cont.chain2 )
#                     try:
#                         diff = ev.ts - parent.end.ts
#                         curr_cont.chain3 = parent_cont.chain2 + diff
#                         chain3_lengths.append( curr_cont.chain3 )
#                         try:
#                             curr_cont.chain4 = parent_cont.chain3 + diff
#                             chain4_lengths.append( curr_cont.chain4 )
#                         except:
#                             pass
#                     except:
#                         pass
#                 except:
#                     pass


def analyzeTree( tree, stats ):
    analyzeConcurrency( tree, stats )
    for cont in tree:
        analyzeDurations( cont, stats )
        analyzeGaps( cont, stats )
        analyzeChains( cont, stats )
        analyzeBranching( cont, stats )
        analyzeSubtrees( cont, stats )

# End code for Analyzing trees

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

    if name is "durations":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 1000000, y1, y2 ) )

    if name is "gaps":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 400000, 0, y2 ) )

    if name is "gapsZ":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 400000, 0, 50000 ) )

    if name is "chains":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( x1, 1000000, y1, y2 ) )

    if name is "gaps_1_5":
        x1, x2, y1, y2 = plt.axis()
        plt.axis( ( 8, 24000, -100, 6000 ) )


    if False:
        plt.show()
    else:
        plt.savefig( "%s.pdf" % os.path.join( "Graphs", name ), bbox_inches='tight' )

def printIles( name, items, iles ):
    L = len( items )
    print( "%s %10d " % ( name, L ), end="" )
    if L < 1:
        print( "EMPTY" )
        return
    for p in iles:
        print( "%8.0f " % items[ int( L * p ) ], end="" )
    print( "" )

def showStats( s ):
    s.durs_macro.sort()
    s.durs_recur.sort()
    s.durs_nrecur.sort()
    s.durs_micro.sort()

    s.gaps_weird.sort()
    s.gaps_recur.sort()
    s.gaps_interrupted.sort()
    s.gaps_uninterrupted.sort()

    print( "neg gaps: %d -:- pos gaps: %d -:- weird gaps: %d" %
           ( s.neg_gaps, s.pos_gaps, len( s.gaps_weird ) ) )
    print( "I%d U%d %d %d" % ( len( s.gaps_interrupted ), len( s.gaps_uninterrupted ),
                               s.gaps_interrupted[0], s.gaps_interrupted[1] ) )
    for limit, tree_stats in s.trees.items():
        # print( "SDFSDF %d %d %d" % ( limit, tree_stats[ "linear"], tree_stats[ "nonlinear"] ) )
        print( "SDFSDF %d %s" % ( limit, tree_stats ) )

    s.concurrency.sort()
    s.concurrency_NA.sort()
    s.concurrency_NN.sort()
    s.branching_A.sort()
    s.branching_N.sort()
    s.branching_R.sort()
    # print( s.branching )
    # print( s.branchingNR )
    s.chains[ 2 ].sort()
    s.chains[ 3 ].sort()
    s.chains[ 4 ].sort()

    # s.before_counts.sort()
    # s.after_counts.sort()
    # s.spans.sort()
    # shorties1m = {}
    # shorties100u = {}
    # def incrShorty1m( sched_name ):
    #     try:
    #         shorties1m[ sched_name ] += 1
    #     except:
    #         shorties1m[ sched_name ] = 1
    # def incrShorty100u( sched_name ):
    #     try:
    #         shorties100u[ sched_name ] += 1
    #     except:
    #         shorties100u[ sched_name ] = 1

    sgaps = []
    for name, gaps in s.gaps.items():
        gaps.sort()
        short_gaps = 1
        for gap in gaps:
            if gap < 1000:
                short_gaps += 1
            else:
                break
        sgaps.append( ( 1.0 / short_gaps, name, gaps ) )
    sgaps.sort()

    i = 0
    for l, name, gaps in sgaps:
        i += 1
        fancyPlot( "gaps_%d_%s_%d" % ( i, name, len( gaps ) ), [ [ gaps ] ] )
    fancyPlot( "gaps_1_5", [ [ sgaps[ 0 ][ 2 ] ], [ sgaps[ 1 ][ 2 ] ], [ sgaps[ 2 ][ 2 ] ], [ sgaps[ 3 ][ 2 ] ], [ sgaps[ 4 ][ 2 ] ] ] )

    # printIles( "AL", s.macro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    # printIles( "IL", s.micro_lengths, [ 0.1,  0.5,   0.9, 0.95, 0.99 ] )
    # printIles( "GA", s.gaps[ "uninterrupted" ], [ 0.01, 0.05,  0.1,  0.5, 0.9 ] )
    # printIles( "BC", s.before_counts, [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    # printIles( "CC", s.after_counts,  [ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    # printIles( "C2", s.chain2_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    # printIles( "C3", s.chain3_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )
    # printIles( "C4", s.chain4_lengths,[ 0.5,  0.9,   0.95, 0.99, 0.999 ] )

    # print( "SHORTIES 1 millisecond %s" % shorties1m )
    # print( "SHORTIES 100 microseconds %s" % shorties100u )

    # print( s.api_kind )

    fancyPlot( "durations", [ [ s.durs_nrecur ], [ s.durs_recur ] ] )
    fancyPlot( "branching", [ [ s.branching_N ], [ s.branching_R ] ] )
    # fancyPlot( "concurrency", [ [ s.concurrency ], [ s.branching_A, False, True ] ] )
    fancyPlot( "concurrency", [ [ s.concurrency_NN ], [ s.concurrency_NA ] ] )
    fancyPlot( "chains", [ [ s.chains[ 2 ] ], [ s.chains[ 3 ] ], [ s.chains[ 4 ] ] ] )
    fancyPlot( "chainsZ", [ [ s.chains[ 2 ] ], [ s.chains[ 3 ] ], [ s.chains[ 4 ] ] ] )
    fancyPlot( "gaps", [ [ s.gaps_recur ], [ s.gaps_interrupted ], [ s.gaps_uninterrupted ] ] )
    fancyPlot( "gapsZ", [ [ s.gaps_recur ], [ s.gaps_interrupted ], [ s.gaps_uninterrupted ] ] )
    # short_gaps  = list( filter( lambda g: g < 1000, aggregate_gaps ) )
    # short_spans = list( filter( lambda s: s < 1000, s.spans ) )
    # fancyPlot( "gap-span", [ [ short_gaps ], [ short_spans ] ] )
    # # fancyPlot( "children", [ [ s.children_per_cont ], [ s.pchildren_per_edge ] ] )
    # # fancyPlot( "gaps", [ [ s.gaps[ "uninterrupted" ] ], [ s.gaps[ "interrupted" ] ], [ not_recur_gaps ] ] )

    # # fancyPlot( "gaps", [ [ s.gaps[ "uninterrupted" ] ], [ s.gaps[ "interrupted" ] ], [ aggregate_gaps ] ] )

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

    return stats

def main():
    trees_dir = sys.argv[ 1 ] if len( sys.argv ) > 1 else "./Trees"
    stats = Object()
    stats.neg_gaps = 0
    stats.pos_gaps = 0
    stats.concurrency = []
    stats.concurrency_NA = []
    stats.concurrency_NN = []
    stats.durs_recur = []
    stats.durs_nrecur = []
    stats.durs_macro = []
    stats.durs_micro = []
    stats.gaps = {}
    stats.gaps_weird = []
    stats.gaps_recur = []
    stats.gaps_interrupted = []
    stats.gaps_uninterrupted = []
    stats.branching_A = []
    stats.branching_N = []
    stats.branching_R = []
    stats.chains = [ [], [], [], [], [], [] ]
    stats.trees = {}
    for limit in subtree_limits:
        stats.trees[ limit ] = { "linear":0, "nonlinear":0, "internalBranching":0, "maybeWaiting":0, "anyBranching":0, "multipleEscapes":0, "interrupted":0 }

    # stats.children_per_cont = []
    # stats.pchildren_per_edge = []
    # stats.before_counts = []
    # stats.after_counts = []
    # stats.spans = []
    # stats.api_kind = {}

    for fname in os.listdir( trees_dir ):
        if fname == ".DS_Store":
            continue
        print( "======================== eat %s ========================" % fname )
        path = os.path.join( trees_dir, fname )
        analyzeDir( path, stats )

    print( "stupids %d %d %d %d" % ( stupid0, stupid1, stupid2, stupid3 ) )
    showStats( stats )

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
