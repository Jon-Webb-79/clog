// ================================================================================
// ================================================================================
// - File:    logger.h
// - Purpose: Describe the file purpose here
//
// Source Metadata
// - Author:  Jonathan A. Webb
// - Date:    September 03, 2025
// - Version: 1.0
// - Copyright: Copyright 2025, Jon Webb Inc.
// ================================================================================
// ================================================================================
// Include modules here

#ifndef logger_H
#define logger_H
// ================================================================================ 
// ================================================================================ 

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)
  /* C11 threads */
  #include <threads.h>
  typedef mtx_t logger_mutex_t;
  #define LOGGER_MUTEX_INIT_OK(m)    (mtx_init(&(m), mtx_plain) == thrd_success)
  #define LOGGER_MUTEX_LOCK(m)       mtx_lock(&(m))
  #define LOGGER_MUTEX_UNLOCK(m)     mtx_unlock(&(m))
  #define LOGGER_MUTEX_DESTROY(m)    mtx_destroy(&(m))

#elif defined(_WIN32)
  /* Win32 */
  #include <windows.h>
  typedef CRITICAL_SECTION logger_mutex_t;
  static inline bool logger_mutex_init_ok(logger_mutex_t* m) { InitializeCriticalSection(m); return true; }
  #define LOGGER_MUTEX_INIT_OK(m)    logger_mutex_init_ok(&(m))
  #define LOGGER_MUTEX_LOCK(m)       EnterCriticalSection(&(m))
  #define LOGGER_MUTEX_UNLOCK(m)     LeaveCriticalSection(&(m))
  #define LOGGER_MUTEX_DESTROY(m)    DeleteCriticalSection(&(m))

#else
  /* POSIX pthreads */
  #include <pthread.h>
  typedef pthread_mutex_t logger_mutex_t;
  #define LOGGER_MUTEX_INIT_OK(m)    (pthread_mutex_init(&(m), NULL) == 0)
  #define LOGGER_MUTEX_LOCK(m)       pthread_mutex_lock(&(m))
  #define LOGGER_MUTEX_UNLOCK(m)     pthread_mutex_unlock(&(m))
  #define LOGGER_MUTEX_DESTROY(m)    pthread_mutex_destroy(&(m))
#endif
// ================================================================================ 
// ================================================================================ 

#ifdef __cplusplus
extern "C" {
#endif
// ================================================================================ 
// ================================================================================ 

/**
 * @enum LogLevel
 * @brief Severity levels for log messages.
 *
 * Used to classify log output by importance. The logger will filter out
 * messages below its configured threshold.
 */
typedef enum {
    LOG_DEBUG    = 10,
    LOG_INFO     = 20,
    LOG_WARNING  = 30,
    LOG_ERROR    = 40,
    LOG_CRITICAL = 50
} LogLevel;
// -------------------------------------------------------------------------------- 

/**
 * @struct Logger
 * @brief Configurable logging object for emitting messages to file and/or stream.
 *
 * Encapsulates state needed for thread-safe logging. A Logger instance can be
 * passed into functions that need to emit logs, avoiding reliance on global
 * variables.
 */
typedef struct Logger {
    FILE*       file;       /* Primary sink (may be NULL) */
    FILE*       stream;     /* Secondary sink (e.g., stderr). NULL to disable. */
    LogLevel    level;      /* Minimum level to emit */
    const char* name;       /* Optional logger name (not owned) */
    bool        timestamps; /* Prepend ISO-8601 timestamp */
    bool        colors;     /* ANSI colors for TTY streams */
    bool        owns_file;  /* Close 'file' on logger_close if true */
    bool        locking;    /* Enable/disable locking (for single-thread apps) */
    logger_mutex_t lock;    /* Portable mutex */
    bool initialized;       /* initialized flag */
} Logger;
// ================================================================================ 
// ================================================================================ 

/**
 * @brief Initialize a logger that writes only to an existing stream.
 *
 * Sets up the Logger object to emit messages to the specified @p stream
 * (e.g., stderr or stdout). The caller retains ownership of the stream.
 *
 * @param[in,out] lg     Pointer to a Logger object to initialize.
 * @param[in]     stream Output stream to use (must be valid and open).
 * @param[in]     level  Minimum log level to emit (messages below are suppressed).
 *
 * @retval true  Initialization succeeded.
 * @retval false Initialization failed (e.g., null @p lg).
 */
bool logger_init_stream(Logger* lg, FILE* stream, LogLevel level);

// -------------------------------------------------------------------------------- 

/**
 * @brief Initialize a logger that writes only to a file.
 *
 * Opens the file at @p path in append mode and configures the Logger to write
 * to it. The logger takes ownership of the file handle and will close it when
 * logger_close() is called.
 *
 * @param[in,out] lg    Pointer to a Logger object to initialize.
 * @param[in]     path  Path to the log file (opened in append mode).
 * @param[in]     level Minimum log level to emit.
 *
 * @retval true  Initialization succeeded and file opened.
 * @retval false Initialization failed (e.g., could not open file).
 */
bool logger_init_file(Logger* lg, const char* path, LogLevel level);

// -------------------------------------------------------------------------------- 

/**
 * @brief Initialize a logger that writes to both a file and a stream.
 *
 * Configures the Logger to write to a newly opened file at @p path as well
 * as an existing @p stream (e.g., stderr). The logger owns the file and will
 * close it in logger_close(), but does not own the stream.
 *
 * @param[in,out] lg     Pointer to a Logger object to initialize.
 * @param[in]     path   Path to the log file (opened in append mode).
 * @param[in]     stream Output stream to also log to (not owned).
 * @param[in]     level  Minimum log level to emit.
 *
 * @retval true  Initialization succeeded.
 * @retval false Initialization failed (e.g., could not open file).
 */
bool logger_init_dual(Logger* lg, const char* path, FILE* stream, LogLevel level);

// -------------------------------------------------------------------------------- 

/**
 * @brief Shut down a logger and release owned resources.
 *
 * Flushes and closes the log file if it was opened by the logger,
 * releases the internal mutex, and resets fields to a safe state.
 * Safe to call multiple times, though calling twice has no additional effect.
 *
 * @param[in,out] lg Pointer to a Logger object to close.
 */
void logger_close(Logger* lg);

// ================================================================================ 
// ================================================================================ 

/**
 * @brief Set the minimum severity level for the logger.
 *
 * Messages with a level lower than @p level will be suppressed.
 *
 * @param[in,out] lg    Pointer to the Logger to configure.
 * @param[in]     level New minimum log level to apply.
 */
void logger_set_level(Logger* lg, LogLevel level);

// -------------------------------------------------------------------------------- 

/**
 * @brief Assign a short name to the logger for identification.
 *
 * The @p name pointer is not copied; the caller must ensure that
 * the string remains valid for the lifetime of the Logger.
 *
 * @param[in,out] lg   Pointer to the Logger to configure.
 * @param[in]     name Null-terminated string identifying the logger
 *                     (e.g., module name). May be NULL to clear.
 */
void logger_set_name(Logger* lg, const char* name); 

// -------------------------------------------------------------------------------- 

/**
 * @brief Enable or disable timestamps in log output.
 *
 * When enabled, each log message is prefixed with an ISO-8601 UTC
 * timestamp (e.g., "2025-09-03T21:07:15Z").
 *
 * @param[in,out] lg Pointer to the Logger to configure.
 * @param[in]     on True to enable timestamps, false to disable.
 */
void logger_enable_timestamps(Logger* lg, bool on);

// -------------------------------------------------------------------------------- 

/**
 * @brief Enable or disable ANSI color codes for terminal output.
 *
 * Colors are applied only when writing to a TTY stream. File sinks
 * are never colorized, regardless of this setting.
 *
 * @param[in,out] lg Pointer to the Logger to configure.
 * @param[in]     on True to enable colors, false to disable.
 */
void logger_enable_colors(Logger* lg, bool on);

// -------------------------------------------------------------------------------- 

/**
 * @brief Enable or disable internal locking for thread safety.
 *
 * When enabled, a mutex guards concurrent log operations. In
 * single-threaded applications, this can be disabled for slightly
 * lower overhead.
 *
 * @param[in,out] lg Pointer to the Logger to configure.
 * @param[in]     on True to enable locking, false to disable.
 */
void logger_enable_locking(Logger* lg, bool on);

// ================================================================================ 
// ================================================================================ 

/**
 * @brief Emit a formatted log message with printf-style arguments.
 *
 * This is the core logging function used by the LOG_* macros. It applies
 * filtering based on the logger's current log level, formats the message,
 * and writes to the configured sinks (file and/or stream).
 *
 * Normally, you do not call this directly; instead, use the LOG_DEBUG,
 * LOG_INFO, LOG_WARNING, LOG_ERROR, or LOG_CRITICAL macros, which capture
 * @p file, @p line, and @p func automatically.
 *
 * @param[in,out] lg   Pointer to the Logger to use.
 * @param[in]     level Severity level of the message.
 * @param[in]     file  Source filename where the log was emitted.
 * @param[in]     line  Source line number where the log was emitted.
 * @param[in]     func  Function name where the log was emitted.
 * @param[in]     fmt   printf-style format string for the log message.
 * @param[in]     ...   Variadic arguments matching @p fmt.
 */
void logger_log_impl(Logger* lg,
                     LogLevel level,
                     const char* file,
                     int line,
                     const char* func,
                     const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 6, 7)))
#endif
;

// -------------------------------------------------------------------------------- 

/**
 * @brief Emit a formatted log message using an existing va_list.
 *
 * This variant of logger_log_impl() accepts a @c va_list rather than
 * variadic arguments. It is intended for cases where you already have a
 * va_list (e.g., inside a wrapper function). Otherwise, prefer the
 * LOG_* macros or logger_log_impl().
 *
 * @param[in,out] lg   Pointer to the Logger to use.
 * @param[in]     level Severity level of the message.
 * @param[in]     file  Source filename where the log was emitted.
 * @param[in]     line  Source line number where the log was emitted.
 * @param[in]     func  Function name where the log was emitted.
 * @param[in]     fmt   printf-style format string for the log message.
 * @param[in]     args  va_list containing arguments matching @p fmt.
 */
void logger_vlog_impl(Logger* lg,
                      LogLevel level,
                      const char* file,
                      int line,
                      const char* func,
                      const char* fmt,
                      va_list args);
// -------------------------------------------------------------------------------- 

/* Non-variadic message writer for MISRA-friendly use.
   Caller preformats into 'msg' (NUL-terminated). */
void logger_write(Logger* lg,
                  LogLevel level,
                  const char* file,
                  int line,
                  const char* func,
                  const char* msg);
// -------------------------------------------------------------------------------- 

/**
 * @def LOG_DEBUG
 * @brief Emit a debug-level log message.
 *
 * Convenience macro wrapping logger_log_impl() that automatically captures
 * the current source file, line, and function. Used for verbose diagnostic
 * messages intended primarily for developers.
 *
 * @param[in] lg   Pointer to the Logger to use.
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Optional arguments corresponding to @p fmt.
 */
#define LOG_DEBUG(lg, fmt, ...)    logger_log_impl((lg), LOG_DEBUG,    __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// -------------------------------------------------------------------------------- 

/**
 * @def LOG_INFO
 * @brief Emit an informational log message.
 *
 * Convenience macro wrapping logger_log_impl() that automatically captures
 * the current source file, line, and function. Use for general runtime events
 * or status updates.
 *
 * @param[in] lg   Pointer to the Logger to use.
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Optional arguments corresponding to @p fmt.
 */
#define LOG_INFO(lg, fmt, ...)     logger_log_impl((lg), LOG_INFO,     __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// -------------------------------------------------------------------------------- 

/**
 * @def LOG_WARNING
 * @brief Emit a warning-level log message.
 *
 * Convenience macro wrapping logger_log_impl() that automatically captures
 * the current source file, line, and function. Use to report unexpected
 * events or conditions that may require attention but are not fatal.
 *
 * @param[in] lg   Pointer to the Logger to use.
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Optional arguments corresponding to @p fmt.
 */
#define LOG_WARNING(lg, fmt, ...)  logger_log_impl((lg), LOG_WARNING,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// -------------------------------------------------------------------------------- 

/**
 * @def LOG_ERROR
 * @brief Emit an error-level log message.
 *
 * Convenience macro wrapping logger_log_impl() that automatically captures
 * the current source file, line, and function. Use to report serious errors
 * that prevent part of the program from functioning correctly.
 *
 * @param[in] lg   Pointer to the Logger to use.
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Optional arguments corresponding to @p fmt.
 */
#define LOG_ERROR(lg, fmt, ...)    logger_log_impl((lg), LOG_ERROR,    __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// -------------------------------------------------------------------------------- 

/**
 * @def LOG_CRITICAL
 * @brief Emit a critical-level log message.
 *
 * Convenience macro wrapping logger_log_impl() that automatically captures
 * the current source file, line, and function. Use for critical conditions
 * requiring immediate attention, often preceding program termination.
 *
 * @param[in] lg   Pointer to the Logger to use.
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Optional arguments corresponding to @p fmt.
 */
#define LOG_CRITICAL(lg, fmt, ...) logger_log_impl((lg), LOG_CRITICAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// ================================================================================ 
// ================================================================================ 
#ifdef __cplusplus
}
#endif /* cplusplus */
#endif /* logger_H */
// ================================================================================
// ================================================================================
// eof
