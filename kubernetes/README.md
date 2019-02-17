## Kubernetes scheduler tests

### Build images
```
cd queue && sudo docker build -t my_queue . && cd ..
cd worker/ && sudo docker build -t my_worker . && cd ..
```

### Run queue
```
sudo docker run -it -p 8000:8000 --rm --name queue_cont --network=host my_queue python /root/queue.py
```

### Check (from another console)
```
$ curl localhost:8000/size
{"queue1": 0, "queue2": 0}
```

### Run worker
```
sudo docker run -it --rm --name worker_cont --network=host my_worker python /root/worker.py queue1 127.0.0.1:8000
```
