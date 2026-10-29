#include "connection.h"
#include "../channel/channel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TCP CONNECTION INCLUDES
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int quit_signal = -1;

//
// BST TREE IMPLEMENTATIONS
//

typedef struct cgc_handler_node cgc_handler_node;
struct cgc_handler_node {
  cgc_handler_node *left;
  cgc_handler_node *right;

  cgc_endpoint endpoint;
  cgc_handler_func func;
};

typedef struct cgc_handler_tree {
  cgc_handler_node *head;
  int size;
} cgc_handler_tree;

static cgc_handler_node *cgc_handler_tree_get(cgc_handler_tree *tree,
                                              cgc_endpoint endpoint) {
  cgc_handler_node *curr = tree->head;

  while (curr != NULL) {
    int cmp = strcmp(endpoint, curr->endpoint);

    if (cmp < 0) {
      curr = curr->left;
    } else if (cmp > 0) {
      curr = curr->right;
    } else {
      return curr;
    }
  }

  return NULL;
}

static void cgc_handler_tree_put(cgc_handler_tree *tree,
                                 cgc_handler_node *node) {
  cgc_handler_node **curr = &tree->head;

  while (*curr != NULL) {
    int cmp = strcmp(node->endpoint, (*curr)->endpoint);

    if (cmp < 0) {
      curr = &(*curr)->left;
    } else if (cmp > 0) {
      curr = &(*curr)->right;
    } else {
      free(node);
      return;
    }
  }

  node->left = NULL;
  node->right = NULL;
  tree->size++;
  *curr = node;
}

static void cgc_handler_register(cgc_handler_tree *tree, cgc_endpoint data,
                                 cgc_handler_func func) {
  cgc_handler_node *node = malloc(sizeof *node);
  node->endpoint = data;
  node->func = func;
  cgc_handler_tree_put(tree, node);
}

static cgc_handler_tree *new_handler_tree() {
  cgc_handler_tree *ret = malloc(sizeof(*ret));
  ret->head = NULL;
  ret->size = 0;
  return ret;
}

void cgc_handler_free(cgc_handler_tree *tree) {}

//
// WORKER IMPLEMENTATIONS
//

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
  cgc_handler_tree *h_tree;
};

pthread_t worker_threads[WORKER_COUNT];

static cgc_worker *new_worker(channel *client, cgc_handler_tree *tree) {
  cgc_worker *ret = malloc(sizeof(cgc_worker));
  ret->id = worker_id;
  ret->status = READY;
  ret->client = client;
  ret->h_tree = tree;
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

    if (data == NULL) {
      continue;
    }

    if (data == &quit_signal) {
      logger.Info("worker %d is closing, channel return value is %d",
                  worker->id, *data);
      return NULL;
    }

    int fd = *data;
    free(data);

    logger.Info("worker %d accepted fd %d ", worker->id, fd);
    sleep(2);

    logger.Info("worker %d finished fd %d", worker->id, fd);
    close(fd);
  }
  return NULL;
}

typedef struct cgc_worker_pool {
  cgc_worker *workers[WORKER_COUNT];
} cgc_worker_pool;

static cgc_worker_pool new_worker_pool(channel *client,
                                       cgc_handler_tree *tree) {
  cgc_worker_pool ret;
  for (int i = 0; i < WORKER_COUNT; i++) {
    ret.workers[i] = new_worker(client, tree);
    logger.Info("worker %d initialized with %zu byte", i,
                sizeof(*ret.workers[i]));
  }
  return ret;
}

//
//  LISTENER IMPLEMENTATIONS
//

static int create_listener() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) < 0) {
    close(fd);
    return -1;
  }

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_port = htons(PORT),
                             .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};

  if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
    close(fd);
    return -1;
  }

  if (listen(fd, SOMAXCONN) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

//
// SERVER IMPLEMENTATIONS
//

struct cgc_srv {
  channel *client;

  cgc_worker_pool pool;
  cgc_handler_tree *h_tree;
};

void register_endpoint(cgc_srv *srv, cgc_endpoint endpoint,
                       cgc_handler_func func) {
  cgc_handler_register(srv->h_tree, endpoint, func);
}

void start_server(cgc_srv *srv) {
  for (int i = 0; i < WORKER_COUNT; ++i) {
    pthread_create(&worker_threads[i], NULL, start_worker,
                   srv->pool.workers[i]);
  }

  int listener = create_listener();

  while (1) {
    int fd = accept(listener, NULL, NULL);

    if (fd < 0) {
      perror("accept");
      continue;
    }

    int *client_fd = malloc(sizeof *client_fd);
    if (client_fd == NULL) {
      close(fd);
      continue;
    }

    *client_fd = fd;
    push_channel(srv->client, client_fd);
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
  ret->h_tree = new_handler_tree();
  ret->pool = new_worker_pool(ret->client, ret->h_tree);
  logger.Info("worker pool initialized");

  return ret;
}
