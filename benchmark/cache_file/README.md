### Benchmark for reading file: from disk vs page cache vs shared memory vs tmpfs

#### Prepare
Need something to see what is hapennig with file cache. For example:
```
curl -L -o pcstat https://github.com/tobert/pcstat/raw/2014-05-02-01/pcstat.x86_64
chmod a+x pcstat
```

Or you can use `pcstat` already stored in this repo (in current folder).

#### Clone repo and generate a big file
This will generate 1Gb file 
```
dd if=/dev/zero of=big.txt bs=1073741824 count=1
```

#### Check of big.txt not in file cache
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
$ ./pcstat --nohdr  big.txt 
|----------+----------------+------------+-----------+---------|
| big.txt  | 1073741824     | 262144     | 0         | 000.000 |
|----------+----------------+------------+-----------+---------|
```
Okay, now it's fine.

PS: also this command allows clean file cache:
```
sysctl -w vm.drop_caches=3
```

#### Compile
```

```


