#ifndef CGC_CONNECTION
#define CGC_CONNECTION
#include "../logger/logger.h"

#define WORKER_BUFFER_SIZE 2048
#define WORKER_COUNT 4
#define QUEUE_SIZE 8
#define MAX_HEADER 32

typedef struct cgc_worker cgc_worker;
typedef struct cgc_srv cgc_srv;

cgc_srv *new_server();
void start_server(cgc_srv *srv);

#endif
