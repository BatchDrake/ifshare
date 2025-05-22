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

static unsigned g_log_level      = LogInfo;
bool            g_log_line_start = true;
FILE           *g_logfp          = NULL;
bool            g_use_colors         = false;

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

  va_end(ap);
}
