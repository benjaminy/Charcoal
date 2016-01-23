package main

import "fmt"
import "os"
import "strconv"

var A = [...]int64 { 4, 8, 3, 5, 6, 7, 2, 1, 0 }

func f( n int64, x int64 ) int64 {
	if( n <  2 ) {
		return ( A[ x ] )
	} else {
		var half = n / 2
		return f( n - half, f( half, x ) )
	}
}

func main() {
	/* XXX ignoring possible errors */
	var mstr = os.Args[1]
	var m, _ = strconv.Atoi( mstr )
	var e = int64(1) << ( uint32(m) - 1 )
	var rv = f( e, 1 )
	fmt.Printf( "Hello, world. %d %d %s answer: %d\n", 42, m, mstr, rv )
}
