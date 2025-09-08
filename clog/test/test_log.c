// ================================================================================
// ================================================================================
// - File:    test_log.c
// - Purpose: Describe the file purpose here
//
// Source Metadata
// - Author:  Jonathan A. Webb
// - Date:    August 31, 2022
// - Version: 1.0
// - Copyright: Copyright 2022, Jon Webb Inc.
// ================================================================================
// ================================================================================
// Include modules here

#include "logger.h"
#include "test_log.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/select.h>
  #if defined(__APPLE__)
    #include <util.h>   /* openpty on macOS/BSD */
  #else
    #include <pty.h>    /* openpty on Linux */
  #endif
#endif
// ================================================================================ 
// ================================================================================ 

/* Create a path guaranteed to fail to open (parent dir does not exist). */
static const char* bad_log_path(void) {
    return "this/path/definitely/does/not/exist/app.log";
}
// -------------------------------------------------------------------------------- 

static void set_errno_sentinel(void) {
    errno = 0x7F; /* unlikely real errno value */
}
// -------------------------------------------------------------------------------- 

int test_suite_setup(void **state) {
    (void)state;
    return 0;
}
// -------------------------------------------------------------------------------- 
int test_suite_teardown(void **state) {
    (void)state;
    return 0;
}
// -------------------------------------------------------------------------------- 

static bool has_iso8601_prefix(const char* s) {
    if (!s) return false;
    for (int i = 0; i < 4; ++i) if (!isdigit((unsigned char)s[i])) return false; /* YYYY */
    if (s[4] != '-') return false;
    if (!isdigit((unsigned char)s[5]) || !isdigit((unsigned char)s[6])) return false; /* MM */
    if (s[7] != '-') return false;
    if (!isdigit((unsigned char)s[8]) || !isdigit((unsigned char)s[9])) return false; /* DD */
    if (s[10] != 'T') return false;
    if (!isdigit((unsigned char)s[11]) || !isdigit((unsigned char)s[12])) return false; /* hh */
    if (s[13] != ':') return false;
    if (!isdigit((unsigned char)s[14]) || !isdigit((unsigned char)s[15])) return false; /* mm */
    if (s[16] != ':') return false;
    if (!isdigit((unsigned char)s[17]) || !isdigit((unsigned char)s[18])) return false; /* ss */
    if (s[19] != 'Z') return false;
    return true;
}
// ================================================================================ 
// ================================================================================ 

void init_stream_null_logger(void **state) {
    (void)state;
    FILE *stream = stderr;
    set_errno_sentinel();
    bool ok = logger_init_stream(NULL, stream, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_stream_null_stream(void **state) {
    (void)state;
    Logger lg; /* stack allocation; content unused on failure */
    set_errno_sentinel();
    bool ok = logger_init_stream(&lg, NULL, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_file_null_logger(void **state) {
    (void)state;
    set_errno_sentinel();
    bool ok = logger_init_file(NULL, "app.log", LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_file_null_path(void **state) {
    (void)state;
    Logger lg;
    set_errno_sentinel();
    bool ok = logger_init_file(&lg, NULL, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_file_open_fail_bad_parent(void **state) {
    (void)state;
    Logger lg;
    errno = 0;
    bool ok = logger_init_file(&lg, bad_log_path(), LOG_INFO);
    assert_false(ok);
    /* fopen should have set errno to something non-zero (likely ENOENT). */
    assert_true(errno != 0);
}
// -------------------------------------------------------------------------------- 

void init_dual_null_logger(void **state) {
    (void)state;
    set_errno_sentinel();
    bool ok = logger_init_dual(NULL, "app.log", stderr, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_dual_null_path(void **state) {
    (void)state;
    Logger lg;
    set_errno_sentinel();
    bool ok = logger_init_dual(&lg, NULL, stderr, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void init_dual_null_stream(void **state) {
    (void)state;
    Logger lg;
    set_errno_sentinel();
    bool ok = logger_init_dual(&lg, "app.log", NULL, LOG_INFO);
    assert_false(ok);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void close_null_preserves_errno(void **state) {
    (void)state;
    set_errno_sentinel();
    logger_close(NULL);
    assert_int_equal(errno, 0x7F); /* unchanged */
}
// -------------------------------------------------------------------------------- 

void close_idempotent(void **state) {
    (void)state;
    Logger lg;
    assert_true(logger_init_stream(&lg, stderr, LOG_INFO));

    logger_close(&lg);
    assert_null(lg.file);
    assert_null(lg.stream);

    /* Second close should not crash and should leave sinks NULL. */
    logger_close(&lg);
    assert_null(lg.file);
    assert_null(lg.stream);
}
// ================================================================================ 
// ================================================================================ 
// TEST FILTERING 

static FILE* make_temp_stream(void) {
    FILE* f = tmpfile();            /* Portable temp stream */
    assert_non_null(f);
    return f;
}
// -------------------------------------------------------------------------------- 

static char* slurp_stream(FILE* f, size_t* out_len) {
    assert_non_null(f);
    fflush(f);
    long pos = ftell(f);
    if (pos < 0) pos = 0;
    fseek(f, 0, SEEK_END);
    long end = ftell(f);
    assert_true(end >= 0);
    size_t len = (size_t)end;
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(len + 1);
    assert_non_null(buf);
    size_t got = fread(buf, 1, len, f);
    assert_int_equal(got, len);
    buf[len] = '\0';
    if (out_len) *out_len = len;
    return buf;
}

static size_t count_newlines(const char* s) {
    size_t n = 0;
    for (; *s; ++s) if (*s == '\n') ++n;
    return n;
}
// -------------------------------------------------------------------------------- 

void level_filter_suppresses(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;

    /* Only ERROR and higher should pass */
    assert_true(logger_init_stream(&lg, sink, LOG_ERROR));

    LOG_INFO(&lg,  "this should NOT appear");
    LOG_DEBUG(&lg, "this should NOT appear either");

    /* Read back stream: expect zero bytes */
    size_t len = 0;
    char*  buf = slurp_stream(sink, &len);

    assert_int_equal(len, 0);      /* nothing written */
    assert_string_equal(buf, "");  /* empty */

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// -------------------------------------------------------------------------------- 

void level_filter_emits(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;

    /* INFO or higher should pass (INFO, WARNING, ERROR, CRITICAL) */
    assert_true(logger_init_stream(&lg, sink, LOG_INFO));

    LOG_INFO(&lg,     "info line");
    LOG_WARNING(&lg,  "warn line");
    LOG_ERROR(&lg,    "error line");
    LOG_CRITICAL(&lg, "crit line");

    size_t len = 0;
    char*  buf = slurp_stream(sink, &len);

    /* Expect some content */
    assert_true(len > 0);

    /* Expect exactly 4 lines */
    assert_int_equal(count_newlines(buf), 4);

    /* Spot-check the presence of level names.
       (Format is %-8s so names might be padded; strstr on "INFO"/"WARNING"... is fine.) */
    assert_non_null(strstr(buf, "INFO"));
    assert_non_null(strstr(buf, "WARNING"));
    assert_non_null(strstr(buf, "ERROR"));
    assert_non_null(strstr(buf, "CRITICAL"));

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// ================================================================================ 
// ================================================================================ 
// TEST OUTPUT FORMAT 

void format_contains_fields(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;
    assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
    logger_enable_timestamps(&lg, true);
    logger_set_name(&lg, NULL); /* keep format stable for this test */

    const char* msg = "hello-world-msg";
    /* Ensure we know the exact source line of the LOG call */
    const int expected_line = __LINE__ + 1;
    LOG_WARNING(&lg, "%s", msg);

    size_t len = 0;
    char* buf = slurp_stream(sink, &len);
    assert_true(len > 0);
    /* one line only */
    assert_int_equal(count_newlines(buf), 1);

    /* Check LEVEL token is present */
    assert_non_null(strstr(buf, "WARNING"));

    /* Check the function name appears (this test function name) */
    assert_non_null(strstr(buf, "format_contains_fields"));

    /* Check :LINE: pattern exists */
    char needle[64];
    snprintf(needle, sizeof(needle), ":%d:", expected_line);
    assert_non_null(strstr(buf, needle));

    /* Check message text present */
    assert_non_null(strstr(buf, msg));

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// -------------------------------------------------------------------------------- 

void timestamp_toggle(void **state) {
    (void)state;

    /* --- timestamps ON --- */
    {
        FILE* sink = make_temp_stream();
        Logger lg;
        assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
        logger_enable_timestamps(&lg, true);

        LOG_INFO(&lg, "ts-on");
        size_t len = 0;
        char* buf = slurp_stream(sink, &len);
        assert_true(len > 0);
        /* Check prefix */
        assert_true(has_iso8601_prefix(buf));

        free(buf);
        logger_close(&lg);
        fclose(sink);
    }

    /* --- timestamps OFF --- */
    {
        FILE* sink = make_temp_stream();
        Logger lg;
        assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
        logger_enable_timestamps(&lg, false);

        LOG_INFO(&lg, "ts-off");
        size_t len = 0;
        char* buf = slurp_stream(sink, &len);
        assert_true(len > 0);
        /* Should NOT have ISO8601 prefix at start */
        assert_false(has_iso8601_prefix(buf));

        free(buf);
        logger_close(&lg);
        fclose(sink);
    }
}
// -------------------------------------------------------------------------------- 

void name_toggle(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;
    assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
    logger_enable_timestamps(&lg, false); /* simplify prefix checks */

    logger_set_name(&lg, "demo-name");
    LOG_INFO(&lg, "first");

    logger_set_name(&lg, NULL); /* clear name */
    LOG_INFO(&lg, "second");

    size_t len = 0;
    char* buf = slurp_stream(sink, &len);
    assert_true(len > 0);
    /* two lines */
    assert_int_equal(count_newlines(buf), 2);

    /* Expect exactly one "[demo-name]" occurrence */
    const char* tag = "[demo-name]";
    const char* p = buf;
    int count = 0;
    while ((p = strstr(p, tag)) != NULL) { ++count; ++p; }
    assert_int_equal(count, 2);

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// ================================================================================ 
// ================================================================================ 

void macro_location(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;
    assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
    logger_enable_timestamps(&lg, false); /* simplify prefix */
    logger_set_name(&lg, NULL);

    const char* msg = "macro-location-probe";
    const int expected_line = __LINE__ + 1;
    LOG_INFO(&lg, "%s", msg);

    size_t len = 0;
    char* buf = slurp_stream(sink, &len);
    assert_true(len > 0);

    /* Exactly one line */
    size_t lines = 0;
    for (const char* p = buf; *p; ++p) if (*p == '\n') ++lines;
    assert_int_equal(lines, 1);

    /* Function name present */
    assert_non_null(strstr(buf, "macro_location"));

    /* Exact :LINE: present */
    char needle[64];
    snprintf(needle, sizeof(needle), ":%d:", expected_line);
    assert_non_null(strstr(buf, needle));

    /* Message present */
    assert_non_null(strstr(buf, msg));

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// -------------------------------------------------------------------------------- 

void no_color_for_file(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;
    assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));

    logger_enable_timestamps(&lg, false);
    logger_enable_colors(&lg, true);

    LOG_CRITICAL(&lg, "color check file sink");

    size_t len = 0;
    char* buf = slurp_stream(sink, &len);
    assert_true(len > 0);

    /* No ANSI CSI introducer in file output */
    assert_null(strstr(buf, "\x1b["));  /* ESC [ */

    free(buf);
    logger_close(&lg);
    fclose(sink);
}
// ================================================================================ 
// ================================================================================ 

void setters_null_lg(void **state) {
    (void)state;

    errno = 0;
    logger_set_level(NULL, LOG_DEBUG);
    assert_int_equal(errno, EINVAL);

    errno = 0;
    logger_set_name(NULL, "name");
    assert_int_equal(errno, EINVAL);

    errno = 0;
    logger_enable_timestamps(NULL, true);
    assert_int_equal(errno, EINVAL);

    errno = 0;
    logger_enable_colors(NULL, true);
    assert_int_equal(errno, EINVAL);

    errno = 0;
    logger_enable_locking(NULL, true);
    assert_int_equal(errno, EINVAL);
}
// -------------------------------------------------------------------------------- 

void setters_success_leave_errno(void **state) {
    (void)state;

    FILE* sink = make_temp_stream();
    Logger lg;
    assert_true(logger_init_stream(&lg, sink, LOG_INFO));

    errno = EAGAIN;
    logger_set_level(&lg, LOG_DEBUG);
    assert_int_equal(lg.level, LOG_DEBUG);
    assert_int_equal(errno, EAGAIN);

    errno = EAGAIN;
    logger_set_name(&lg, "unit");
    assert_non_null(lg.name);
    assert_string_equal(lg.name, "unit");
    assert_int_equal(errno, EAGAIN);

    errno = EAGAIN;
    logger_enable_timestamps(&lg, false);
    assert_false(lg.timestamps);
    assert_int_equal(errno, EAGAIN);

    errno = EAGAIN;
    logger_enable_colors(&lg, false);
    assert_false(lg.colors);
    assert_int_equal(errno, EAGAIN);

    errno = EAGAIN;
    logger_enable_locking(&lg, false);
    assert_false(lg.locking);
    assert_int_equal(errno, EAGAIN);

    logger_close(&lg);
    fclose(sink);
}
// --------------------------------------------------------------------------------

void log_impl_null_args(void **state) {
    (void)state;

    /* (a) NULL logger */
    {
        errno = 0;
        /* Use harmless file/line/func values; they won't be used */
        logger_log_impl(NULL, LOG_INFO, __FILE__, __LINE__, __func__, "msg");
        assert_int_equal(errno, EINVAL);
    }

    /* (b) NULL fmt on a valid logger → no output, errno=EINVAL */
    // {
    //     FILE* sink = make_temp_stream();
    //     Logger lg;
    //     assert_true(logger_init_stream(&lg, sink, LOG_DEBUG));
    //
    //     errno = 0;
    //     logger_log_impl(&lg, LOG_INFO, __FILE__, __LINE__, __func__, NULL);
    //     assert_int_equal(errno, EINVAL);
    //
    //     size_t len = 0;
    //     char* buf = slurp_stream(sink, &len);
    //     assert_int_equal(len, 0);
    //     assert_string_equal(buf, "");
    //
    //     free(buf);
    //     logger_close(&lg);
    //     fclose(sink);
    // }
}
// ================================================================================
// ================================================================================
// eof
