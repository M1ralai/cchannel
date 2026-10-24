#define CGC_LOGGER_IMPLEMENTATION
#include "connection/connection.h"
#include "logger/logger.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void logger_silencer(const char *fmt, ...) {};

int main() {
  cgc_logger_new();
  logger.Info("Main function started");
  cgc_srv *srv = new_server();
  start_server(srv);
}
