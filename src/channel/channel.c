#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queue queue;

struct queue {
  queue *head;
};

typedef struct queue_node queue_node;
struct queue_node {
  queue_node *next;
};

typedef struct {
  void **data;
  int cap;
  int len;
  int size;

  queue *recv;
  queue *send;

  pthread_mutex_t mut;

  pthread_cond_t received;
  pthread_cond_t consumed;

} channel;

channel *make_channel(int cap, int size) {
  channel *ch;

  if (cap <= 0) {
    return NULL;
  }

  ch = malloc(sizeof *ch + (size_t)cap * sizeof(void *));
  if (ch == NULL) {
    return NULL;
  }

  memset(ch, 0, sizeof *ch);

  ch->data = (void *)(ch + 1);
  ch->cap = cap;
  ch->size = size;
  ch->len = 0;

  ch->send->head = NULL;
  ch->recv->head = NULL;

  pthread_mutex_init(&ch->mut, NULL);
  pthread_cond_init(&ch->received, NULL);
  pthread_cond_init(&ch->consumed, NULL);

  return ch;
}

static void *pop_from_channel(channel *ch) {
  pthread_mutex_lock(&ch->mut);
  while (!ch->len) {
    printf("data tried to get but channel is empty \n");
    pthread_cond_wait(&ch->received, &ch->mut);
  }
  printf("data taken from channel\n");

  --ch->len;
  void *ret = ch->data[0];
  for (int i = 1; i < ch->cap; i++) {
    ch->data[i - 1] = ch->data[i];
  }
  pthread_cond_signal(&ch->consumed);
  pthread_mutex_unlock(&ch->mut);
  return ret;
}

void *pop_channel(channel *ch) { return pop_from_channel(ch); }

static void push_from_channel(channel *ch, void *data) {
  pthread_mutex_lock(&ch->mut);
  while (ch->cap == ch->len) {
    printf("data tried to sent but channel is full \n");
    printf("%d data in channel \n", ch->len);
    pthread_cond_wait(&ch->consumed, &ch->mut);
  }

  printf("data sent to a channel \n");
  ch->data[ch->len] = data;
  ++ch->len;
  pthread_cond_signal(&ch->received);

  pthread_mutex_unlock(&ch->mut);
}

void push_channel(channel *ch, void *data) {
  return push_from_channel(ch, data);
}

void free_channel(channel *ch) {
  pthread_cond_destroy(&ch->consumed);
  pthread_cond_destroy(&ch->received);
  pthread_mutex_destroy(&ch->mut);
  free(ch);
}
