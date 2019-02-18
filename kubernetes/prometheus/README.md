### Prometheus
```
kubectl create -f prometheus.operator.manifest.yml 
kubectl create -f prometheus.rbac.rules.yml
kubectl create -f prometheus.yml
```
### Browser: localhost:9090
```
kubectl port-forward prometheus-prometheus-0 9090
```

### After endpoint with /metrics deployed:
```
kubectl create -f prometheus/prometheus.queue.service_monitor.yml
```
