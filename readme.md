# mini-http-with-workers

A small, evolving systems programming project written in **C**. The goal is to build an HTTP/1.1 server from the ground up while exploring POSIX threads, synchronization, memory management, and networking.

> **Status: early prototype.** The channel and worker-pool scaffolding exists, but this is **not an HTTP server yet**. The current code still has correctness issues and is being developed incrementally.

## Why this project?

Rather than treating concurrency and networking as black boxes, this project is an excuse to implement and understand their building blocks. It started with a Go-inspired channel experiment in C and is gradually growing into a small worker-based server.

The focus is on learning by building, debugging, measuring, and revisiting design decisions—not on making a production-ready web server.

## Current implementation

- **Bounded pointer channel** (`src/channel/`): a circular-buffer design using `pthread_mutex_t` and two `pthread_cond_t` condition variables to coordinate producers and consumers.
- **Fixed worker pool** (`src/connection/`): four `pthread` workers share one channel and wait for work.
- **Shutdown signaling**: the main thread sends one shutdown token per worker, then calls `pthread_join()` to wait for them to exit.
- **Simple logger** (`src/logger/`): a small header-based logging utility.
- **Sanitizer-enabled builds**: the Makefile uses Clang's AddressSanitizer and UndefinedBehaviorSanitizer.

**Important:** The current channel dequeue logic has a length-accounting bug, and the worker loop does not yet process real client connections. Treat this as an implementation in progress.

## Architecture (current prototype)

```text
                  +-----------------------+
                  |      Main thread      |
                  | create / start / stop |
                  +-----------+-----------+
                              |
                              v
                  +-----------------------+
                  | Shared bounded channel|
                  |  mutex + conditions   |
                  +-----------+-----------+
                              |
              +---------------+---------------+
              |               |               |
              v               v               v
          +--------+      +--------+      +--------+    ...
          |Worker 0|      |Worker 1|      |Worker 2|  Worker 3
          +--------+      +--------+      +--------+

  'q' on stdin -> enqueue shutdown tokens -> workers return
                 -> pthread_join() -> main continues
```

The intended next step is to put accepted socket connections into the queue and let workers handle them.

## Build and run

Requirements: a C compiler with pthread support, Make, and Clang sanitizer support for the current Makefile.

```sh
git clone https://github.com/M1ralai/mini-http-with-workers.git
cd mini-http-with-workers
mkdir -p bin
make
```

`make` currently **builds and runs** the program. Type `q` and press Enter to request worker shutdown.

The default build uses:

```text
-g -O0 -fsanitize=address,undefined -pthread
```

## Repository layout

```text
src/
  main.c                  Entry point
  channel/
    channel.h             Channel API
    channel.c             Bounded queue and synchronization
  connection/
    connection.h          Server / worker configuration
    connection.c          Worker pool and shutdown prototype
  logger/
    logger.h              Logging utility
makefile                  Build and run
```

## Roadmap

The plan is deliberately incremental. Each stage should leave something that can be tested before moving to the next one.

### 0. Make the concurrency foundation reliable

- [ ] Validate allocation and `pthread_*` error paths.
- [ ] Add cleanup for workers, the server, and the channel.
- [ ] Test full/empty queue behavior and multiple producers/consumers.
- [ ] Run sanitizer-backed stress tests and investigate deadlocks.

### 1. Connect the worker pool to sockets

- [ ] Create a listening TCP socket (`socket`, `bind`, `listen`, `accept`).
- [ ] Define an owned connection/job representation.
- [ ] Enqueue accepted connections for workers.
- [ ] Handle reads, writes, partial I/O, and connection closure.
- [ ] Define shutdown behavior for active and queued connections.

### 2. Build a minimal HTTP/1.1 layer

- [ ] Parse request lines and headers with explicit size limits.
- [ ] Define a valid header representation and request/response lifecycle.
- [ ] Generate correct status lines, headers, and response bodies.
- [ ] Support a minimal route/handler interface.
- [ ] Add request-body handling and basic HTTP error responses.
- [ ] Explore persistent connections after basic correctness is established.

### 3. Test, measure, and refine

- [ ] Add unit tests and integration tests.
- [ ] Benchmark throughput and latency under different worker/queue sizes.
- [ ] Document memory usage and the impact of design choices.
- [ ] Compare blocking I/O with event-driven approaches (optional).
- [ ] Revisit channel storage, allocation strategies, and backpressure.

## Scope

This is a **learning project**, not a production-ready HTTP server. Security hardening, complete HTTP compliance, and high performance are not current claims.

The roadmap is a direction, not a promise: the implementation will change as the project—and my understanding of C systems programming—develops.
