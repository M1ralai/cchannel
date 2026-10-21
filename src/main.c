#include "channel/channel.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  char msg[256];
  int status_code;
} err;

void *thread1(void *ch) {
  int index = 0;
  while (1) {
    ++index;
    err data = {
        .msg = "Hello World",
        .status_code = index,
    };
    push_channel((channel *)ch, (void *)&data);
    sleep(2);
  }
}

void *thread2(void *ch) {
  while (1) {
    err *error = (err *)pop_channel((channel *)ch);
    printf("%s error message came with %d status code\n", error->msg,
           error->status_code);
    sleep(1);
  }
}

int main() {
  pthread_t t1, t2;
  channel *ch = make_channel(10, sizeof(err));
  if (ch == NULL) {
    printf("ch is NULL");
    return -1;
  }
  pthread_create(&t1, NULL, thread1, ch);
  pthread_create(&t2, NULL, thread2, ch);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
