#ifndef CGC_CONNECTION
#define CGC_CONNECTION
#include "../logger/logger.h"

#define PORT 8080
#define WORKER_BUFFER_SIZE 2048
#define WORKER_COUNT 4
#define QUEUE_SIZE 8
#define MAX_HEADER 32

typedef struct cgc_worker cgc_worker;
typedef struct cgc_srv cgc_srv;

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

cgc_srv *new_server();
void register_endpoint(cgc_srv *srv, cgc_endpoint endpoint,
                       cgc_handler_func func);
void start_server(cgc_srv *srv);

#endif
