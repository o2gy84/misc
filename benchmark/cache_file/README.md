### Benchmark for reading file: from disk vs page cache vs shared memory vs tmpfs

#### Prepare
Need something to see what is hapennig with file cache. For example:
```
curl -L -o pcstat https://github.com/tobert/pcstat/raw/2014-05-02-01/pcstat.x86_64
chmod a+x pcstat
```

Or you can use `pcstat` already stored in this repo (in current folder).

#### Clone repo and generate a big file
This will generate 1Gb text file
```
base64 /dev/urandom | head -c 1073741824 > big.txt
```

#### Lets check if big.txt is present in file cache or not
```
$ ./pcstat big.txt 
|----------+----------------+------------+-----------+---------|
| Name     | Size           | Pages      | Cached    | Percent |
|----------+----------------+------------+-----------+---------|
| big.txt  | 1073741824     | 262144     | 262144    | 100.000 |
|----------+----------------+------------+-----------+---------|
```
Ooops, file is in cache already. Lets drop it from cache:
```
$ echo 3 | sudo tee /proc/sys/vm/drop_caches
```
And recheck:
```
$ ./pcstat --nohdr big.txt 
|----------+----------------+------------+-----------+---------|
| big.txt  | 1073741824     | 262144     | 0         | 000.000 |
|----------+----------------+------------+-----------+---------|
```
Okay, now it's fine.

PS: also this command allows clean file cache:
```
sysctl -w vm.drop_caches=3
```

#### Compile locker and tester
```
$ g++ -O2 lock_file_in_ram.cpp -o lock
$ g++ -O2 test_locked_file.cpp -o test
```

#### Lets test for reading big file directly from disk
```
$ ./test big.txt cache
read from disk at: 2021 msec
size: 1073741824 bytes
```

#### Again, check if big.txt is present in file cache and
```
$ ./pcstat --nohdr big.txt 
|----------+----------------+------------+-----------+---------|
| big.txt  | 1073741824     | 262144     | 262144    | 100.000 |
|----------+----------------+------------+-----------+---------|
```

#### Lets test for reading big file from file cache
```
$ ./test big.txt cache
read from disk at: 195 msec
size: 1073741824 bytes
```

So, it's clear that reading from cache is much faster than reading from disk.  
Lets try something else.

#### Check for reading big file from shared memory
First, we need to store file into shared mem
```
$ ./lock big.txt shmem
can't lock shared memory: Cannot allocate memory           # it's okay, explanation below
file: big.txt is stored into shared memory
size: 1073741824 bytes
crc: 42
```

After that, programm is waiting for user press 'q' button, or type 'exit' ot 'quit'.  
After user press 'q', programm will terminated and shared memory segment will destroyed.  
  
By default, programm tries to lock shared memory into RAM, to prevent swapping.  
But each user has limitation about locked memory: see `ulimit -l`.  
This does not affect the test, but you can increase that limit to avoid warning either:  
 - use sudo and just type `ulimit -l unlimited` as root, then run `./lock` as root
 - or modify `/etc/security/limits.conf` or `/etc/limits.conf` as normal user  
   
Okay, lets try to read file from shared memory:
```
$ ./test big.txt shmem
read from shared memory at: 281 msec
```
  
So, reading from shared memory still much faster than reading from disk, but a bit slower than reading from cache.  
  
#### Finally, lets test tmpfs
Create tmpfs and put big file into it:
```
sudo mount -t tmpfs -o size=2000M none /mnt/test_dir
cp big.txt /mnt/test_dir/
```

Run test:
```
$ ./test /mnt/test_dir/big.txt cache
read from disk at: 191 msec
```

So, tmpfs is almost same with file cache.  
Unmount:
```
sudo umount /mnt/test_dir
```









