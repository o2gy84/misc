### Benchmark for passing data between threads via: local queue with mutex/condvar, fdevent, mq_queue, semaphore

#### Compile
```
g++ condvar.cpp -pthread -O2 -std=c++11 -lrt
```

1) mutex/condvar:  
```
$ time ./a.out 100000 cond
Test will transmit: 100000 messages through channel
send cycle time: 30ms
receive cycle time: 32ms
sum: 100000

real	0m0.045s
user	0m0.020s
sys	0m0.043s
```

2) semaphore (example without real data transfer, just notify):  
```
Test will transmit: 100000 messages through channel
send cycle time: 30ms
receive cycle time: 36ms
sum: 100000

real	0m0.051s
user	0m0.041s
sys	0m0.025s
```

3) fdevent: 
```
Test will transmit: 100000 messages through channel
send cycle time: 77ms
receive cycle time: 80ms
sum: 100000

real	0m0.088s
user	0m0.034s
sys	0m0.120s
```

4) mq_queue:  
```
Test will transmit: 100000 messages through channel
send cycle time: 292ms
receive cycle time: 293ms
sum: 100000

real	0m0.305s
user	0m0.058s
sys	0m0.222s
```

