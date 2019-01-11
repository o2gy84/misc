### benchmark for resizing ind cropping image
## SPOILER:
There is (almost) no difference between C++ and Python, because of both just uses OpenCV,  
so this is just libopencv.so tesing, but no languages.  
But.. Instead of using multi-threading, you have to use processes in python (because of GIL).  

## C++
compile:
```
g++ -O2 -msse -msse2 -msse3 -mfpmath=sse -mavx resize.cpp -std=c++14 -lopencv_imgproc -lopencv_highgui -lopencv_core -fopenmp
```
run 10000 iterations:
```
$ ./a.out ~/download/photo/test.jpg 10000
iterations: 10000
image: w=401, h=600
total: 1317 ms
counter: 10000
```
  
## Python
run 10000 iterations:
```
$ python resize.py --path ~/download/photo/test.jpg --num 10000
iterations:  10000
shape:  (600, 401, 3)
total: 1373.06904793 ms
counter:  10000
```

