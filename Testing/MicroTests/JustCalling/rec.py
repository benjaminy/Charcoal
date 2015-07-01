A = [ 4, 8, 3, 5, 6, 7, 2, 1, 0 ]

def f( n, x ):
    if n < 2:
        return A[ x ]
    else:
        return f( ( n + 1 ) / 2, f( n / 2, x ) )

N = 100000000
print( f( N, 1 ) )
