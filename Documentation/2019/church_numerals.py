#!/usr/bin/env python3

from infix_hack import *

def boolean_demo():
    # λt.( λf.( t ) )
    def church_true( representative_of_truth ):
        def helper( representative_of_falsity ):
            return representative_of_truth
        return helper

    # λt.( λf.( f ) )
    def church_false( representative_of_truth ):
        def f( representative_of_falsity ):
            return representative_of_falsity
        return f

    print( "🔅 Testing true and false:" )
    print( church_true( "YES" )( "NO" ) )
    print( church_false( "YES" )( "NO" ) )

    # λb.( λt.( λf.( b( f )( t ) ) ) )
    def church_not( church_bool ):
        def maybe_so( p1 ):
            def maybe_not( p2 ):
                return church_bool( p2 )( p1 )
            return maybe_not
        return maybe_so

    print( "🔅 Testing not:" )
    print( church_not( church_true )( "YES" )( "NO" ) )
    print( church_not( church_false )( "YES" )( "NO" ) )

    # logical AND
    # λb1.( λb2.( b1( b2 )( FALSE ) ) )
    def church_and( church_bool1 ):
        def second_param( church_bool2 ):
            return church_bool1( church_bool2 )( church_false )
        return second_param

    # logical (inclusive) OR
    # λb1.( λb2.( b1( TRUE )( b2 ) ) )
    def church_or( church_bool1 ):
        def second_param( church_bool2 ):
            return church_bool1( church_true )( church_bool2 )
        return second_param

    AND = Infix( lambda p1, p2: church_and( p1 )( p2 ) )
    OR  = Infix( lambda p1, p2: church_or( p1 )( p2 ) )
    NOT = church_not
    T   = church_true
    F   = church_false

    print( "🔅 Testing ( ¬A ∧ B ) ∨ C:" )
    test1 = ( ( NOT( T ) ) |AND| T ) |OR| T
    test2 = ( ( NOT( T ) ) |AND| T ) |OR| F
    test3 = ( ( NOT( T ) ) |AND| F ) |OR| T
    test4 = ( ( NOT( T ) ) |AND| F ) |OR| F
    test5 = ( ( NOT( F ) ) |AND| T ) |OR| T
    test6 = ( ( NOT( F ) ) |AND| T ) |OR| F
    test7 = ( ( NOT( F ) ) |AND| F ) |OR| T
    test8 = ( ( NOT( F ) ) |AND| F ) |OR| F

    print( test1( "YES" )( "NO" ) )
    print( test2( "YES" )( "NO" ) )
    print( test3( "YES" )( "NO" ) )
    print( test4( "YES" )( "NO" ) )
    print( test5( "YES" )( "NO" ) )
    print( test6( "YES" )( "NO" ) )
    print( test7( "YES" )( "NO" ) )
    print( test8( "YES" )( "NO" ) )

def numerals_demo():
    # λf.( λx.( x ) )
    def church_zero( f ):
        def helper( x ):
            return x
        return helper

    # λf.( λx.( f( x ) ) )
    def church_one( f ):
        def helper( x ):
            return f( x )
        return helper

    # λf.( λx.( f( f( x ) ) ) )
    def church_two( f ):
        def helper( x ):
            return f( f( x ) )
        return helper

    # λf.( λx.( f( f( f( x ) ) ) ) )
    def church_three( f ):
        def helper( x ):
            return f( f( f( x ) ) )
        return helper

    def church_numeral_to_normal_number( church_num ):
        def inc( x ):
            return x + 1
        return church_num( inc )( 0 )

    print( "Testing a few small integers" )
    print( church_numeral_to_normal_number( church_zero ) )
    print( church_numeral_to_normal_number( church_one ) )
    print( church_numeral_to_normal_number( church_two ) )
    print( church_numeral_to_normal_number( church_three ) )

    # Addition
    # λn.( λm.( λf.( λx.( n( f )( m( f )( x ) ) ) ) ) )
    def church_add( church_num1 ):
        def more_add( church_num2 ):
            def the_numeral( f ):
                def helper( x ):
                    return church_num1( f )( church_num2( f )( x ) )
                return helper
            return the_numeral
        return more_add

    church_five  = church_add( church_two )( church_three )
    church_eight = church_add( church_five )( church_three )

    print( church_numeral_to_normal_number( church_five ) )
    print( church_numeral_to_normal_number( church_eight ) )

def recursion_demo():
    # λf.( λx.( f( x( x ) ) )( λy.( f( y( y ) ) ) ) )
    def THE_Y_COMBINATOR( f ):
        def helper1( x ):
            return x( x )
        def helper2( y ):
            return f( lambda *args: y( y )( *args ) )
        return helper1( helper2 )

    # Our good old friend the Fibonacci sequence
    def FIB_OUTER( FIB_INNER ):
        def helper( x ):
            if x < 2:
                return 1
            else:
                return FIB_INNER( x - 1 ) + FIB_INNER( x - 2 )
        return helper

    FIB = THE_Y_COMBINATOR( FIB_OUTER )

    print( FIB( 9 ) )

def main():
    # boolean_demo()
    # numerals_demo()
    recursion_demo()

main()
