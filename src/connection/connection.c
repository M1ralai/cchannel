#include "connection.h"
#include "../channel/channel.c"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char *cgc_body;

typedef struct {
  char key[32];
  char value[];
} cgc_header;

typedef struct {
  cgc_header headers[MAX_HEADER];
  cgc_body body;
} cgc_http_data;
typedef void (*handle_func)(cgc_http_data *req, cgc_http_data *resp);

typedef struct {
  cgc_http_data resp;
  cgc_http_data req;
} cgc_data;

enum cgc_worker_status {
  READY = 0,
  BUSY = 1,
};

static int worker_id;

struct cgc_worker {
  int id;
  enum cgc_worker_status status;
  channel *quit;
  channel *client;

  char buffer[WORKER_BUFFER_SIZE];
  cgc_data data;
};

pthread_t worker_threads[WORKER_COUNT];

static cgc_worker *new_worker(channel *quit, channel *client) {
  cgc_worker *ret = malloc(sizeof(cgc_worker));
  ret->id = worker_id;
  ret->status = READY;
  ret->quit = quit;
  ret->client = client;
  worker_id++;

  return ret;
}

static void *quit_worker(void *argc) {
  channel *chan = (channel *)argc;
  return pop_channel(chan);
}

static void *start_worker(void *arg) {
  cgc_worker *worker = (cgc_worker *)arg;
  logger.Info("worker is started running, id: %d status %s", worker->id,
              worker->status ? "READY" : "BUSY");
  pthread_t t_quit, t_client;
  return NULL;
}

typedef struct cgc_worker_pool {
  cgc_worker *workers[WORKER_COUNT];
} cgc_worker_pool;

static cgc_worker_pool new_worker_pool(channel *quit, channel *client) {
  cgc_worker_pool ret;
  for (int i = 0; i < WORKER_COUNT; i++) {
    ret.workers[i] = new_worker(quit, client);
    logger.Info("worker %d initialized with %zu byte", i,
                sizeof(*ret.workers[i]));
  }
  return ret;
}

struct cgc_srv {
  channel *quit;
  channel *client;

  cgc_worker_pool pool;
};

void start_server(cgc_srv *srv) {
  for (int i = 0; i < WORKER_COUNT; ++i) {
    pthread_create(&worker_threads[i], NULL, start_worker,
                   &srv->pool.workers[i]);
  }
  while (1) {
    char ch;
    scanf("%c", &ch);
    if (ch == 'q') {
      logger.Error("system is quitting, waiting for threads");
      push_channel(srv->quit, (int *)0);
    }
  }
  for (int i = 0; i < WORKER_COUNT; ++i) {
    pthread_join(worker_threads[i], NULL);
  }
}

cgc_srv *new_server() {
  cgc_srv *ret = malloc(sizeof(cgc_srv));
  if (ret == NULL) {
    logger.Error("malloc malfunctioned");
    return ret;
  }
  ret->quit = make_channel((int)WORKER_COUNT);
  ret->client = make_channel((int)QUEUE_SIZE);
  ret->pool = new_worker_pool(ret->quit, ret->client);
  logger.Info("worker pool initialized");

  return ret;
}
