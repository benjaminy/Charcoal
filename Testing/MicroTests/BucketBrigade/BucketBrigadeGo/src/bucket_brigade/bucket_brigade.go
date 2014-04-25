package main

import "fmt"
import "sync"

const ( N = 10 )

func main() {
    var mtx sync.Mutex
    var cnd *sync.Cond
    var cnds[N] *sync.Cond
    var mtxs[N] sync.Mutex
    cnd = sync.NewCond(&mtx)
    for i := 0; i < N; i++ {
	cnds[i] = sync.NewCond(&mtxs[i])
    }
    for i := 0; i < N; i++ {
        go func(me int, m *sync.Mutex, c1 *sync.Cond, c2 *sync.Cond) {
	    fmt.Printf("Hello, world. %d\n", me)
            if me == 0 {
                cnd.Signal()
            }
            for j := 0; j < 10000000; j++ {
                m.Lock()
                c1.Wait()
                m.Unlock()
                c2.Signal()
            }
            if me == N-1 {
                cnd.Signal()
            }
        }(i, &mtxs[i], cnds[i], cnds[(i+1)%N])
    }
    mtx.Lock()
    cnd.Wait()
    mtx.Unlock()
    cnds[0].Signal()
    mtx.Lock()
    cnd.Wait()
    mtx.Unlock()
}
