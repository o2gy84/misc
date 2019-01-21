### benchmark for calculating condition_variable.notify_one() time

compile:
```
g++ condvar_mut_perf.cpp -pthread -O2 -std=c++11
```
run:
```
./a.out 100000 0
```
100000 - approximate num of cycles  
0      - force unlock mutex before notify_one() is disabled  
```
$ ./a.out 100000 0
Sum threshold: 100000, force_mutex_unlock: false
cycle time: 141ms
sum: 100000
Channel::get()->mutex_lock: 55ms
Channel::put()->mutex_lock: 33ms
Channel::put()->notify_one: 26ms
```

ltrace:
```
$ ltrace -c ./a.out 1000 0
% time     seconds  usecs/call     calls      function
------ ----------- ----------- --------- --------------------
 43.62    1.493498         371      4024 _ZNSt6chrono3_V212steady_clock3nowEv
 22.61    0.774102         767      1009 pthread_mutex_unlock
 17.38    0.594947         589      1009 pthread_mutex_lock
 15.72    0.538072         535      1005 _ZNSt18condition_variable10notify_oneEv
```
