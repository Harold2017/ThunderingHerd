#

## Thundering herd

[wiki](https://en.wikipedia.org/wiki/Thundering_herd_problem)

main problem: un-necessary context switch, cache miss

usual way: mutex

### Linux: (Accept) / Select / Poll / Epoll

#### (Accept):

thundering herd has been solved after kernel 2.6: (flag: `WQ_FLAG_EXCLUSIVE`)

`wait->flags |= WQ_FLAG_EXCLUSIVE;`

as the following shows: only one child process waked up

```bash
➜  curl -v http://localhost:6000

Worker 1 (pid:24185) starts to accept connections
Worker 0 (pid:24184) starts to accept connections
Worker 2 (pid:24186) starts to accept connections
Worker 3 (pid:24187) starts to accept connections
Worker 5 (pid:24189) starts to accept connections
Worker 6 (pid:24190) starts to accept connections
Worker 7 (pid:24191) starts to accept connections
Worker 4 (pid:24188) starts to accept connections
Worker 1 successfully accepts connection from 127.0.0.1:63717
Worker 1 (pid:24185) starts to accept connections
```

#### Epoll:

- epoll_create before fork
  - all child processes share the same epoll events, all will be waken up
    - as the following shows: 8 workers, 4 were waken up, but only one successfully accepted connection, others got `EAGAIN`
  - but this can be solved as `accept` by adding flag `EPOLLEXCLUSIVE`
    - notice: `EPOLLEXCLUSIVE` only guaranteed one or more of the epoll file descriptors will receive an event but NOT exactly one
- epoll_create after fork
  - every child process keeps its internal event loop, and only registers its own events, so if they listen to the same fd, thundering herd will happen -> need mutex
  - lock on `mmap` or file lock
  - one fd <-> one worker
  - process first get lock then accept, others who not get lock just remove fd from its events
  - `Nginx`

```bash
# epoll_create before fork
Create worker 0 (pid:29406)
Create worker 1 (pid:29407)
Create worker 2 (pid:29408)
Create worker 3 (pid:29409)
Create worker 5 (pid:29411)
Create worker 6 (pid:29412)
Create worker 4 (pid:29410)
Create worker 7 (pid:29413)

➜  curl -v http://localhost:6000

Worker 7 (pid:29413) starts to accept connections
Worker 4 (pid:29410) starts to accept connections
Worker 5 (pid:29411) starts to accept connections
Worker 4 failed to accept connection
Worker 6 (pid:29412) starts to accept connections
Worker 7 successfully accepts connection from 127.0.0.1:39143
Worker 5 failed to accept connection
Worker 6 failed to accept connection

# strace
[pid 30310] accept(3, 0x7ffc73eb3ec0, [16]) = -1 EAGAIN (Resource temporarily unavailable)

# after add flag: `EPOLLEXCLUSIVE`, two processes received waken-up events
Worker 7 (pid:30716) starts to accept connections
Worker 6 (pid:30715) starts to accept connections
Worker 7 successfully accepts connection from 127.0.0.1:19688
Worker 6 failed to accept connection
```
