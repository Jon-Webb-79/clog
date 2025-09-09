// ================================================================================
// ================================================================================
// - File:    logger.c
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

#define _POSIX_C_SOURCE 200809L
#include "logger.h"

#include <string.h>
#include <errno.h>

#if defined(_WIN32)
  #include <io.h>
  #define LOGGER_ISATTY(h)   _isatty(_fileno(h))
#else
  #include <unistd.h>
  #define LOGGER_ISATTY(h)   (isatty(fileno(h)))
#endif
// ================================================================================ 
// ================================================================================ 

static const char* level_name(LogLevel lv) {
    switch (lv) {
        case LOG_DEBUG:    return "DEBUG";
        case LOG_INFO:     return "INFO";
        case LOG_WARNING:  return "WARNING";
        case LOG_ERROR:    return "ERROR";
        case LOG_CRITICAL: return "CRITICAL";
        default:           return "LVL?";
    }
}

// -------------------------------------------------------------------------------- 

static const char* level_color(LogLevel lv) {
    /* ANSI colors */
    switch (lv) {
        case LOG_DEBUG:    return "\033[2m";   // dim
        case LOG_INFO:     return "\033[0m";   // reset
        case LOG_WARNING:  return "\033[33m";  // yellow
        case LOG_ERROR:    return "\033[31m";  // red
        case LOG_CRITICAL: return "\033[1;41m";// bold + red bg
        default:           return "\033[0m";
    }
}

// -------------------------------------------------------------------------------- 

static bool is_tty(FILE* s) {
    return (s != NULL) && LOGGER_ISATTY(s);
}

// -------------------------------------------------------------------------------- 

static void now_iso8601(char* buf, size_t n) {
    /* Example: 2025-09-03T21:07:15Z (UTC) */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    time_t secs = ts.tv_sec;
#else
    time_t secs = time(NULL);
#endif
    struct tm tmv;
#if defined(_WIN32)
    gmtime_s(&tmv, &secs);
#else
    gmtime_r(&secs, &tmv);
#endif
    strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &tmv);
}

// -------------------------------------------------------------------------------- 

static bool init_common(Logger* lg, LogLevel level) {
    if (!lg) {
        errno = EINVAL;
        return false;
    }
    memset(lg, 0, sizeof(*lg));
    lg->level = level;
    lg->timestamps = true;
    lg->colors = true;
    lg->locking = true;
    if (!LOGGER_MUTEX_INIT_OK(lg->lock)) return false; 
    lg->initialized = true;
    return true;
}

// ================================================================================ 
// ================================================================================ 

bool logger_init_stream(Logger* lg, FILE* stream, LogLevel level) {
    if (!lg || !stream) {
        errno = EINVAL;
        return false;
    }
    if (!init_common(lg, level)) return false;
    lg->stream = stream;
    /* If this is a TTY, line-buffer for fewer syscalls but prompt output.
       Call setvbuf() before any I/O on the stream. */
    if (LOGGER_ISATTY(stream)) {
        setvbuf(stream, NULL, _IOLBF, 0);
    }
    return true;
}

// -------------------------------------------------------------------------------- 

bool logger_init_file(Logger* lg, const char* path, LogLevel level) {
    if (!lg || !path) {
        errno = EINVAL;
        return false;
    }
    if (!init_common(lg, level)) return false;
    FILE* fp = fopen(path, "a");
    if (!fp) {
        LOGGER_MUTEX_DESTROY(lg->lock);
        return false;
    }
    lg->file = fp;
    lg->owns_file = true;
    /* Files are block-buffered; make the buffer larger to cut write calls. */
    setvbuf(lg->file, NULL, _IOFBF, 1<<20);  // 1 MiB
    return true;
}

// -------------------------------------------------------------------------------- 

bool logger_init_dual(Logger* lg, const char* path, FILE* stream, LogLevel level) {
    if (!lg || !path || !stream) {
        errno = EINVAL;
        return false;
    }
    if (!logger_init_file(lg, path, level)) return false;
    lg->stream = stream;
    /* Line-buffer terminals for prompt visibility; leave files full-buffered. */
    if (LOGGER_ISATTY(stream)) setvbuf(stream, NULL, _IOLBF, 0);
    return true;
}

// -------------------------------------------------------------------------------- 

void logger_close(Logger* lg) {
    if (!lg) return;
    if (lg->file) fflush(lg->file);
    if (lg->stream) fflush(lg->stream);
    if (lg->owns_file && lg->file) fclose(lg->file);
    lg->file = NULL;
    lg->stream = NULL;
    LOGGER_MUTEX_DESTROY(lg->lock); 
    lg->initialized = false;
}

// -------------------------------------------------------------------------------- 

void logger_set_level(Logger* lg, LogLevel level) {
    if (!lg) {
        errno = EINVAL;
        return;
    }
    lg->level = level; 
}

// -------------------------------------------------------------------------------- 

void logger_set_name(Logger* lg, const char* name) {
    if (!lg) {
        errno  = EINVAL;
        return;
    }
    lg->name = name;   
}

// -------------------------------------------------------------------------------- 

void logger_enable_timestamps(Logger* lg, bool on) { 
    if (!lg) {
        errno = EINVAL;
        return;
    }
    lg->timestamps = on; 
}

// -------------------------------------------------------------------------------- 

void logger_enable_colors(Logger* lg, bool on) { 
    if (!lg) {
        errno = EINVAL;
        return;
    }
    lg->colors = on; 
}

// -------------------------------------------------------------------------------- 

void logger_enable_locking(Logger* lg, bool on) { 
    if (!lg) {
        errno = EINVAL;
        return;
    }
    lg->locking = on; 
}

// -------------------------------------------------------------------------------- 

static void emit_one(FILE* out, bool colorize, const char* color,
                     const char* timestamp, const char* name,
                     LogLevel level, const char* file, int line,
                     const char* func, const char* msg)
{
    if (!out) return;

    if (colorize) fputs(color, out);
    if (timestamp && *timestamp) fprintf(out, "%s ", timestamp);
    if (name) fprintf(out, "[%s] ", name);
    fprintf(out, "%-8s ", level_name(level));
    fprintf(out, "%s:%d:%s: ", file, line, func);
    fputs(msg, out);
    fputc('\n', out);
    if (colorize) fputs("\033[0m", out);
    fflush(out);
}

// -------------------------------------------------------------------------------- 

void logger_vlog_impl(Logger* lg,
                      LogLevel level,
                      const char* file,
                      int line,
                      const char* func,
                      const char* fmt,
                      va_list args) {
    /* Validate inputs for errno discipline */
    if (!lg || !fmt) {
        errno = EINVAL;
        return;
    }

    /* Not an error: filtered-out messages must not modify errno */
    if (level < lg->level) return;

    if (lg->locking) LOGGER_MUTEX_LOCK(lg->lock);

    char ts[32] = {0};
    if (lg->timestamps) now_iso8601(ts, sizeof(ts));

    char buf[2048];
    vsnprintf(buf, sizeof(buf), fmt, args);

    const char* color = level_color(level);

    bool stream_color = lg->colors && lg->stream && is_tty(lg->stream);
    bool file_color   = false;

    if (lg->stream) emit_one(lg->stream, stream_color, color, ts, lg->name, level, file, line, func, buf);
    if (lg->file)   emit_one(lg->file,   file_color,   "",    ts, lg->name, level, file, line, func, buf);

    if (lg->locking) LOGGER_MUTEX_UNLOCK(lg->lock);
}
// -------------------------------------------------------------------------------- 

void logger_log_impl(Logger* lg,
                     LogLevel level,
                     const char* file,
                     int line,
                     const char* func,
                     const char* fmt, ...) {
    /* Validate inputs for errno discipline */
    if (!lg || !fmt) {
        errno = EINVAL;
        return;
    }

    va_list args;
    va_start(args, fmt);
    logger_vlog_impl(lg, level, file, line, func, fmt, args);
    va_end(args);
}
// -------------------------------------------------------------------------------- 

void logger_write(Logger* lg,
                  LogLevel level,
                  const char* file,
                  int line,
                  const char* func,
                  const char* msg)
{
    if (!lg || !msg) { errno = EINVAL; return; }
    /* Level filtering and locking identical to logger_vlog_impl */
    if (level < lg->level) return;

    if (lg->locking) LOGGER_MUTEX_LOCK(lg->lock);

    char ts[32] = {0};
    if (lg->timestamps) now_iso8601(ts, sizeof(ts));

    const char* color = level_color(level);
    bool stream_color = lg->colors && lg->stream && is_tty(lg->stream);
    bool file_color   = false;

    if (lg->stream) emit_one(lg->stream, stream_color, color, ts, lg->name, level, file, line, func, msg);
    if (lg->file)   emit_one(lg->file,   file_color,   "",    ts, lg->name, level, file, line, func, msg);

    if (lg->locking) LOGGER_MUTEX_UNLOCK(lg->lock);
}
// ================================================================================
// ================================================================================
// eof
