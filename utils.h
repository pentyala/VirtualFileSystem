#ifndef __UTILS_H__
#define __UTILS_H__

typedef enum LogLevel { LOG_DBG = 0, LOG_INFO = 1, LOG_ERR = 2 } LogLevel;

#define LOG(level, fmt, ...) _log(level, __func__, fmt "\n", ##__VA_ARGS__)

#define VFS_BOOL char
#define VFS_TRUE 1
#define VFS_FALSE 0

void set_log_level(LogLevel level);

void reset_log_level();

void _log(LogLevel level, const char *, char *msg, ...);


void initialize_logger();
void cleanup_logger();


#endif
