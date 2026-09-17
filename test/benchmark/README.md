# IM Server Benchmark

## 1. TCP Benchmark

Run:

```
./tcp_benchmark
```

Default workload:

- clients: 100
- messages/client: 100
- total messages: 10000

Metrics:

- successful connections
- failed connections
- total messages
- elapsed time
- QPS

## 2. IM Protocol Benchmark

Current implementation measures transport layer performance.

Future extension:

```
login request
    |
friend request
    |
send message
    |
message routing
    |
offline storage
```

Can be extended with protobuf generated message packets.
