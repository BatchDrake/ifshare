/*
  log.c: Logging subsystem
  Copyright (C) 2025 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#define _POSIX_SOURCE

#include <log.h>
#include <defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <util.h>
#include <pthread.h>
#include <stdint.h>
#include <ctype.h>

static unsigned g_log_level      = LogDebug;
bool            g_log_line_start = true;
FILE           *g_logfp          = NULL;
bool            g_use_colors     = false;
pthread_mutex_t g_log_mutex      = PTHREAD_MUTEX_INITIALIZER;

static const char *g_months[] = {
  "jan", "feb", "mar",
  "apr", "may", "jun",
  "jul", "aug", "sep",
  "oct", "nov", "dec"
};

static const char *g_level_colors[] = {
  "32", "0", "1;33", "1;31"
};

static const char *g_level_abbrev[] = {
  "debug", "info", "warning", "error"
};

void
logprintf(
  enum loglevel level,
  const char *function,
  const char *file,
  int line,
  const char *fmt,
  ...)
{
  va_list ap;
  char *msg = NULL;

  va_start(ap, fmt);

  pthread_mutex_lock(&g_log_mutex);

  if (level < g_log_level)
    goto done;

  if (g_logfp == NULL) {
    g_logfp          = LOG_DEFAULT_FP;
    g_log_line_start = true;
    g_use_colors     = true;
  }
  
  if (g_log_line_start) {
    struct timeval tv;
    struct tm      tm;

    g_log_line_start = false;

    gettimeofday(&tv, NULL);
    gmtime_r(&tv.tv_sec, &tm);

    if (g_use_colors)
      fprintf(g_logfp, "\e[%sm", g_level_colors[level]);

    fprintf(
      g_logfp,
      "%04d/%s/%02d %02d:%02d:%02d [%-7s] ",
      tm.tm_year + 1900,
      g_months[tm.tm_mon],
      tm.tm_mday,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec,
      g_level_abbrev[level]);
  }

  TRY(msg = vstrbuild(fmt, ap));

done:
  if (msg != NULL) {

    if (g_logfp != NULL) {
      size_t len = strlen(msg);

      if (len > 0) {
        fputs(msg, g_logfp);

        g_log_line_start = msg[len - 1] == '\r' || msg[len - 1] == '\n';
        if (g_log_line_start && g_use_colors && g_logfp != NULL)
          fprintf(g_logfp, "\e[0m");
      }
    }
    
    free(msg);
  }

  pthread_mutex_unlock(&g_log_mutex);

  va_end(ap);
}

void
log_hexdump(enum loglevel level, const void *data, size_t size)
{
  const uint8_t *bytes = (const uint8_t *) data;
  int i, j;

  for (i = 0; i < size; ++i) {
    if ((i & 0xf) == 0)
      Log(level, "%08x  ", i);

    Log(level, "%s%02x ", (i & 0xf) == 8 ? " " : "", bytes[i]);

    if ((i & 0xf) == 0xf) {
      Log(level, " | ");

      for (j = i - 15; j <= i; ++j)
        Log(level, "%c", isprint(bytes[j]) ? bytes[j] : '.');

      Log(level, "\n");
    }
  }

  if ((i & 0xf) != 0) {
    for (j = i; j < ((size + 0xf) >> 4) << 4; ++j)
      Log(level, "   %s", (j & 0xf) == 8 ? " " : "");
    Log(level, " | ");

    for (j = i & ~0xf; j < size; ++j)
      Log(level, "%c", isprint (bytes[j]) ? bytes[j] : '.');

    Log(level, "\n");
  }

  Log(level, "%08x  \n", i);
}
