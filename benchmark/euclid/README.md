### benchmark for calculating euclidian distance and dot product on 128-dim float32 vectors

## C++
compile:
```
g++ -O2 -msse -msse2 -msse3 -mfpmath=sse -mavx emb.cpp -std=c++14
```
run 1000 iterations:
```
$ ./a.out 1000
iterations: 1000
total: 1 ms
sum: -6.39999e+15
```
run 10000 iterations:
```
$ ./a.out 10000
iterations: 10000
total: 12 ms
sum: -6.39958e+20
```
run 100000 iterations:
```
$ ./a.out 100000
iterations: 100000
total: 58 ms
sum: -6.39984e+25
```
  
## Python
run 1000 iterations:
```
$ python emb.py --num 1000
iterations:  1000
total: 4.50205802917 ms
sum:  -6.39998556161e+15
```
run 10000 iterations:
```
$ python emb.py --num 10000
iterations:  10000
total: 45.7859039307 ms
sum:  -6.39957537498e+20
```
run 100000 iterations:
```
$ python emb.py --num 100000
iterations:  100000
total: 476.865053177 ms
sum:  -6.39988249576e+25
```

