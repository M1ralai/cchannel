#ifndef CGC_LOGGER_H
#define CGC_LOGGER_H

typedef struct cgc_logger {
  void (*Info)(const char *fmt, ...);
  void (*Error)(const char *fmt, ...);
  void (*Warn)(const char *fmt, ...);
} cgc_logger;

void cgc_logger_new(void);
extern cgc_logger logger;

#ifdef CGC_LOGGER_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>

cgc_logger logger;

static void cgc_log(const char *level, const char *fmt, va_list args) {
  flockfile(stderr);

  fprintf(stderr, "[%s] ", level);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);

  funlockfile(stderr);
}

static void cgc_info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  cgc_log("INFO", fmt, args);

  va_end(args);
}

static void cgc_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  cgc_log("ERROR", fmt, args);

  va_end(args);
}

static void cgc_warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  cgc_log("WARN", fmt, args);

  va_end(args);
}

void cgc_logger_new(void) {
  logger.Info = cgc_info;
  logger.Error = cgc_error;
  logger.Warn = cgc_warn;
}

#endif
#endif
