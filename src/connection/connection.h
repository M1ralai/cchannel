#ifndef CGC_CONNECTION
#define CGC_CONNECTION
#include "../logger/logger.h"

#define PORT 8080
#define WORKER_BUFFER_SIZE 2048
#define WORKER_COUNT 4
#define QUEUE_SIZE 8
#define MAX_HEADER 32

#include <stddef.h>

typedef struct cgc_worker cgc_worker;
typedef struct cgc_srv cgc_srv;

typedef char *cgc_body;
typedef char *cgc_endpoint;

typedef enum {
  CGC_GET,
  CGC_POST,
  CGC_PUT,
  CGC_DELETE,
  CGC_PATCH,
  CGC_HEAD,
  CGC_OPTIONS,
  CGC_UNKNOWN_METHOD
} cgc_method;

typedef enum {
  CGC_HTTP_1_0,
  CGC_HTTP_1_1,
  CGC_UNKNOWN_VERSION
} cgc_http_version;

typedef struct {
  char key[32];
  char value[256];
} cgc_header;

typedef struct {
  cgc_header headers[MAX_HEADER];
  size_t header_count;

  char *body;
  size_t body_len;
} cgc_http_data;

typedef struct {
  cgc_http_data http;

  cgc_method method;
  cgc_endpoint endpoint;
  cgc_http_version version;
} cgc_request;

typedef struct {
  cgc_http_data http;

  int status_code;
  cgc_http_version version;
} cgc_response;

typedef struct {
  cgc_request req;
  cgc_response resp;
} cgc_data;

typedef void (*cgc_handler_func)(const cgc_request *req, cgc_response *resp);

cgc_srv *new_server();
void register_endpoint(cgc_srv *srv, cgc_endpoint endpoint,
                       cgc_handler_func func);
void start_server(cgc_srv *srv);

#endif
