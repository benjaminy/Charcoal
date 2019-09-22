#!/usr/bin/env python3

import math
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

def main():
    years_cites = [ ( 1993, 7 ), ( 1994, 6 ), ( 1995, 11 ), ( 1996, 9 ), ( 1997, 12 ), ( 1998, 10 ), ( 1999, 8 ), ( 2000, 4 ), ( 2001, 8 ), ( 2002, 12 ), ( 2003, 17 ), ( 2004, 25 ), ( 2005, 58 ), ( 2006, 105 ), ( 2007, 158 ), ( 2008, 202 ), ( 2009, 253 ), ( 2010, 269 ), ( 2011, 258 ), ( 2012, 255 ), ( 2013, 220 ), ( 2014, 209 ), ( 2015, 206 ), ( 2016, 185 ), ( 2017, 163 ), ( 2018, 138 ), ( 2019, 59 ) ]
    years = list( map( lambda yc : yc[ 0 ], years_cites ) )
    cites = list( map( lambda yc : yc[ 1 ], years_cites ) )

    eraser = [ ( 1997, 1 ), ( 1998, 13 ), ( 1999, 14 ), ( 2000, 25 ), ( 2001, 23 ), ( 2002, 45 ), ( 2003, 62 ), ( 2004, 64 ), ( 2005, 80 ), ( 2006, 78 ), ( 2007, 100 ), ( 2008, 101 ), ( 2009, 102 ), ( 2010, 127 ), ( 2011, 110 ), ( 2012, 127 ), ( 2013, 121 ), ( 2014, 88 ), ( 2015, 109 ), ( 2016, 96 ), ( 2017, 95 ), ( 2018, 73 ), ( 2019, 40 ) ]
    ey = list( map( lambda yc : yc[ 0 ], eraser ) )
    ec = list( map( lambda yc : yc[ 1 ], eraser ) )

    fig, ax = plt.subplots()
    ax.plot( years, cites )
    ax.plot( ey, ec )
    # ax.plot( xs, ysinv )

    # ax.set(xlabel='time (s)', ylabel='voltage (mV)',
    #        title='About as simple as it gets, folks')
    # ax.set( xlabel='Number line (log base 2)', ylabel='Number density/distance (log base 2)' )
    # ax.grid()

    # fig.savefig("test.png")
    plt.show()

main()
