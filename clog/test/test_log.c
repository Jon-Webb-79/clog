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
// eof
