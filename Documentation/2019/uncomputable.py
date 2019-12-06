def addA( x1 ):
    return x1 + 4

def addB( x1 ):
    x2 = x1 + 2
    return x2 + 2

def addC( x1 ):
    x2 = x1 + 2
    return x1 + 2

def areFunctionsEqual( function1, function2 ):
    # let's pretend that it's possible to implement this

areFunctionsEqual( addA, addB )
# should be true

areFunctionsEqual( addB, addC )
# should be false



# intuition for why areFunctionsEqual is UNCOMPUTABLE

def someDarnFunction( x ):
    ...

def adversary( x ):
    if areFunctionsEqual( adversary, someDarnFunction ):
        return not someDarnFunction( x )
    else:
        return someDarnFunction( x )
