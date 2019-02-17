## Kubernetes scheduler tests

### Build images
```
cd queue && sudo docker build -t my_queue:v1 . && cd ..
cd worker/ && sudo docker build -t my_worker:v1 . && cd ..
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
sizes:  {u'queue1': 0, u'queue2': 0}
sizes:  {u'queue1': 0, u'queue2': 0}
...
```

### Try to set some tasks from another console
```
$ curl localhost:8000/put?queue1=100
{"queue1": 100, "queue2": 0}
```
then you should see that worker received 'tasks' and 'process' that tasks:
```
sizes:  {u'queue1': 99, u'queue2': 0}
sizes:  {u'queue1': 98, u'queue2': 0}
sizes:  {u'queue1': 97, u'queue2': 0}
sizes:  {u'queue1': 96, u'queue2': 0}
...
```

