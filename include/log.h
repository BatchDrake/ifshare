/*
  log.h: Logging infrastructure
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

#ifndef _LOG_H
#define _LOG_H

#define LOG_DEFAULT_FP stderr
#include <stddef.h>

enum loglevel {
  LogDebug,
  LogInfo,
  LogWarning,
  LogError
};

void logprintf(
  enum loglevel,
  const char *function,
  const char *file,
  int line,
  const char *fmt,
  ...);

void log_hexdump(enum loglevel, const void *data, size_t size);

#define Log(level, fmt, arg...) \
  logprintf(level, __FUNCTION__, __FILE__, __LINE__, fmt, ##arg)

#define Err(fmt, arg...)   Log(LogError,   fmt, ##arg)
#define Warn(fmt, arg...)  Log(LogWarning, fmt, ##arg)
#define Info(fmt, arg...)  Log(LogInfo,    fmt, ##arg)
#define Debug(fmt, arg...) Log(LogDebug,   fmt, ##arg)

#endif /* _LOG_H */
