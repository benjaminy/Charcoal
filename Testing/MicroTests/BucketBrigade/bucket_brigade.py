import threading

def f(me, next, m):
    i = 0
    while i < m:
        i += 1
        me.acquire()
        next.release()
    print i

def main():
    n = 10
    m = 100000
    sems = []
    for i in range(n):
        sems.append(threading.Semaphore(value=0))
    for i in range(n):
        t = threading.Thread(target=f, args=(sems[i], sems[(i+1)%n], m))
        t.start()
    sems[0].release()
    print "Hooray"

main()
