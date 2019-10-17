def addA( x ):
    return x + 4

def addB( y ):
    z = y + 2
    return z + 2

def addC( w ):
    u = w + 2
    return w + 2

def areFunctionsEqual( functionA, functionB ):
    ???

areFunctionsEqual( addA, addB )
# should be true

areFunctionsEqual( addA, addC )
# should be false
