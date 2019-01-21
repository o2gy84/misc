### benchmark for passing data between threads via: local queue with mutex/condvar, fdevent and mq_queue

1) mutex/condvar:
compile:
```
g++ condvar.cpp -pthread -O2 -std=c++11

```
run:
```
$ time ./a.out 100000
Test will transmit: 100000 messages through channel
cycle time: 32ms
sum: 100000

real	0m0.043s
user	0m0.018s
sys	0m0.028s
```

2) semaphore: TODO
compile:
```
g++ sema.cpp -pthread -O2 -std=c++11
```
run:
```

```

3) fdevent:
compile:
```
g++ fdevent.cpp -pthread -O2 -std=c++11
```
run:
```
$ time ./a.out 100000
Test will transmit: 100000 messages through channel
cycle time: 81ms
sum: 100000

real	0m0.093s
user	0m0.037s
sys	0m0.111s
```

4) mq_queue:
compile:
```
g++ mq_queue.cpp -pthread -lrt -O2 -std=c++11
```
run:
```
$ time ./a.out 100000
Test will transmit: 100000 messages through channel
cycle time: 333ms
sum: 100000

real	0m0.344s
user	0m0.030s
sys	0m0.270s
```

