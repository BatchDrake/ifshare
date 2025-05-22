#ifndef _LOG_H
#define _LOG_H

#define LOG_DEFAULT_FP stderr

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

#define Log(level, fmt, arg...) \
  logprintf(level, __FUNCTION__, __FILE__, __LINE__, fmt, ##arg)

#define Err(fmt, arg...)   Log(LogError,   fmt, ##arg)
#define Warn(fmt, arg...)  Log(LogWarning, fmt, ##arg)
#define Info(fmt, arg...)  Log(LogInfo,    fmt, ##arg)
#define Debug(fmt, arg...) Log(LogDebug,   fmt, ##arg)

#endif /* _LOG_H */
