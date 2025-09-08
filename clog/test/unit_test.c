// ================================================================================
// ================================================================================
// - File:    unit_test.c
// - Purpose: This file contains an implementation of the cmocka unit test frameword 
//            for the c_string library
//
// Source Metadata
// - Author:  Jonathan A. Webb
// - Date:    December 30, 2024
// - Version: 0.1
// - Copyright: Copyright 2024, Jon Webb Inc.
// ================================================================================
// ================================================================================
// Include modules here

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_log.h"
// ================================================================================ 
// ================================================================================
// Begin code

const struct CMUnitTest test_init[] = {
    /* init_stream_* */
    cmocka_unit_test(init_stream_null_logger),
    cmocka_unit_test(init_stream_null_stream),
    /* init_file_* */
    cmocka_unit_test(init_file_null_logger),
    cmocka_unit_test(init_file_null_path),
    cmocka_unit_test(init_file_open_fail_bad_parent),
    /* init_dual_* */
    cmocka_unit_test(init_dual_null_logger),
    cmocka_unit_test(init_dual_null_path),
    cmocka_unit_test(init_dual_null_stream),
    /* close_* */
    cmocka_unit_test(close_null_preserves_errno),
    cmocka_unit_test(close_idempotent),
};
// -------------------------------------------------------------------------------- 

const struct CMUnitTest test_filter[] = {
    /* init_stream_* */
    cmocka_unit_test(level_filter_suppresses),
    cmocka_unit_test(level_filter_emits),
};
// -------------------------------------------------------------------------------- 

const struct CMUnitTest output_tests[] = {
    cmocka_unit_test(level_filter_suppresses),
    cmocka_unit_test(level_filter_emits),
    cmocka_unit_test(format_contains_fields),
    cmocka_unit_test(timestamp_toggle),
    cmocka_unit_test(name_toggle),
};
// -------------------------------------------------------------------------------- 

const struct CMUnitTest test_output[] = {
    cmocka_unit_test(macro_location),
    cmocka_unit_test(no_color_for_file),
};
// -------------------------------------------------------------------------------- 

const struct CMUnitTest test_errno[] = {
    cmocka_unit_test(setters_null_lg),
    cmocka_unit_test(setters_success_leave_errno),
    cmocka_unit_test(log_impl_null_args),
};

// ================================================================================ 
// ================================================================================ 

int main(int argc, const char * argv[]) {
    int status;
    status = cmocka_run_group_tests(test_init, NULL, NULL);
    if (status != 0)
        return status;
    status = cmocka_run_group_tests(test_filter, NULL, NULL);
    if (status != 0)
        return status;
    status = cmocka_run_group_tests(output_tests, NULL, NULL);
    if (status != 0)
        return status;
    status = cmocka_run_group_tests(test_output, NULL, NULL);
    if (status != 0)
        return status;
    status = cmocka_run_group_tests(test_errno, NULL, NULL);
    return status;
}
// ================================================================================
// ================================================================================
// eof

