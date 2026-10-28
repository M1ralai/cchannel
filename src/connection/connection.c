#include "connection.h"
#include "../channel/channel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int quit_signal = -1;
typedef char *cgc_body;
typedef char *cgc_endpoint;

typedef struct {
  char key[32];
  char value[256];
} cgc_header;

typedef struct {
  cgc_header headers[MAX_HEADER];
  cgc_body body;
} cgc_http_data;

typedef struct {
  cgc_http_data resp;
  cgc_http_data req;
} cgc_data;
typedef void (*cgc_handler_func)(cgc_http_data *req, cgc_http_data *resp);

typedef struct cgc_handler_node cgc_handler_node;

struct cgc_handler_node {
  cgc_handler_node *left;
  cgc_handler_node *right;
  int height;

  cgc_endpoint endpoint;
  cgc_handler_func func;
};

typedef struct {
  cgc_handler_node *head;
} cgc_handler_pool;

enum cgc_worker_status {
  READY = 0,
  BUSY = 1,
};

static int worker_id;

struct cgc_worker {
  int id;
  enum cgc_worker_status status;
  channel *client;

  char buffer[WORKER_BUFFER_SIZE];
  cgc_data data;
  cgc_handler_pool *h_pool;
};

pthread_t worker_threads[WORKER_COUNT];

static cgc_worker *new_worker(channel *client) {
  cgc_worker *ret = malloc(sizeof(cgc_worker));
  ret->id = worker_id;
  ret->status = READY;
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
              worker->status == READY ? "READY" : "BUSY");
  while (1) {
    int *data = (int *)pop_channel(worker->client);
    if (*data == -1) {
      logger.Info("worker %d is closing, channel return value is %d",
                  worker->id, *data);
      return NULL;
    }
  }
  return NULL;
}

typedef struct cgc_worker_pool {
  cgc_worker *workers[WORKER_COUNT];
} cgc_worker_pool;

static cgc_worker_pool new_worker_pool(channel *client) {
  cgc_worker_pool ret;
  for (int i = 0; i < WORKER_COUNT; i++) {
    ret.workers[i] = new_worker(client);
    logger.Info("worker %d initialized with %zu byte", i,
                sizeof(*ret.workers[i]));
  }
  return ret;
}

struct cgc_srv {
  channel *client;

  cgc_worker_pool pool;
};

void start_server(cgc_srv *srv) {
  for (int i = 0; i < WORKER_COUNT; ++i) {
    pthread_create(&worker_threads[i], NULL, start_worker,
                   srv->pool.workers[i]);
  }
  while (1) {
    char ch;
    scanf("%c", &ch);
    if (ch == 'q') {
      logger.Info("system is quitting, waiting for threads");
      for (int i = 0; i < WORKER_COUNT; ++i) {
        push_channel(srv->client, &quit_signal);
      }
      break;
    }
  }

  for (int i = 0; i < WORKER_COUNT; i++) {
    pthread_join(worker_threads[i], NULL);
    logger.Info("worker %d joined", i);
  }

  logger.Info("all workers terminated");
}

cgc_srv *new_server() {
  cgc_srv *ret = malloc(sizeof(cgc_srv));
  if (ret == NULL) {
    logger.Error("malloc malfunctioned");
    return ret;
  }
  ret->client = make_channel((int)QUEUE_SIZE);
  ret->pool = new_worker_pool(ret->client);
  logger.Info("worker pool initialized");

  return ret;
}
