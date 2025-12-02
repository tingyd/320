#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/global.h"
#include "../include/protobuf.h"
#include "../include/osm.h"
#include "reference_functions.h"
#include "test_common.h"

#define PROGRAM_PATH "bin/pbf"

#define TEST_SUITE process_args_suite
/**
 * flags_with_missing_parameters
 * @brief Passes flags to process_args without corresponding parameters/values
 */
#define TEST_NAME flags_with_missing_parameters
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    int argc = 2;
    char* argv[][2] = {
        {PROGRAM_PATH, "-f"},
        {PROGRAM_PATH, "-n"},
        {PROGRAM_PATH, "-w"}
    };

    int ret = process_args(argc, argv[0], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, -1, "process_args should return -1");

    ret = process_args(argc, argv[1], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, -1, "process_args should return -1");

    ret = process_args(argc, argv[2], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, -1, "process_args should return -1");

    argc = 6;
    char* argv6[] = {PROGRAM_PATH, "-w", "44", "345", "3455", "-n"};
    ret = process_args(argc, argv6, NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, -1, "process_args should return -1");
}
#undef TEST_NAME

#define TEST_NAME valid_flags
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    int argc = 7;
    char* argv[][7] = {
        {PROGRAM_PATH, "-s", "-s", "-s", "-s", "-s", "-s"},
        {PROGRAM_PATH, "-b", "-b", "-b", "-b", "-b", "-b"},
        {PROGRAM_PATH, "-n", "23", "-n", "2846", "-n", "8834"},
        {PROGRAM_PATH, "-w", "44", "-w", "95", "-w", "352"},
        {PROGRAM_PATH, "-w", "44", "44", "95", "7847", "352"},
        {PROGRAM_PATH, "-w", "44", "345", "3455", "-n", "23"}
    };

    int ret = process_args(argc, argv[0], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");

    ret = process_args(argc, argv[1], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");

    ret = process_args(argc, argv[2], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");

    ret = process_args(argc, argv[3], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");

    ret = process_args(argc, argv[4], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");

    ret = process_args(argc, argv[5], NULL);
    cr_assert_eq(help_requested, 0, "help_requested value should not change");
    cr_assert_eq(osm_input_file, NULL, "osm_input_file value should not change");
    cr_assert_eq(ret, 0, "process_args should return 0");
}
#undef TEST_NAME

#undef TEST_SUITE
#define TEST_SUITE pb_read_tag_suite

/**
 * pb_read_tag_easy_cases
 * @brief Tests if PB_read_tag passes easy scenarios
 */
#define TEST_NAME pb_read_tag_easy_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    char inputs[][5] = {
        {0x08, 0x00, 0x00, 0x00, 0x00},
        {0x90, 0xC5, 0x06, 0x00, 0x00},

        {0x29, 0x00, 0x00, 0x00, 0x00},

        {0x29, 0xFF, 0x00, 0x00, 0x00},
        {0x82, 0xEF, 0x10, 0x00, 0xFF},

        {0xAB, 0x01, 0xF9, 0x00, 0x00},
        {0x5C, 0xFF, 0x00, 0x00, 0x00},
        {0x3D, 0x00, 0xF2, 0x00, 0x00},
    };
    const int input_sizes[] = {5, 5, 5, 5, 5, 5, 5, 5};

    // Expected Return, Wire_type, field_number
    const int32_t expected_results[][3] = {
        {1, 0, 1},
        {3, 0, 13394},

        {1, 1, 5},

        {1, 1, 5},
        {3, 2, 34544},

        {2, 3, 21},
        {1, 4, 11},
        {1, 5, 7},
    };

    const int num_inputs = sizeof(inputs) / sizeof(inputs[0]);

    for (int i = 0; i < num_inputs; i++) {
        PB_WireType wire_type = 0;
        int32_t field_number = 0;
        FILE* in = fmemopen(inputs[i], input_sizes[i], "r");
        const int ret = PB_read_tag(in, &wire_type, &field_number);
        fclose(in);

        cr_assert(ret == expected_results[i][0], "PB_read_tag case %d should return %d, but got %d", i,
                  expected_results[i][0], ret);
        cr_assert_eq(wire_type, expected_results[i][1],
		     "PB_read_tag case %d should set correct wire type (expected %u, saw %u)",
		     i, expected_results[i][1], wire_type);
        cr_assert_eq(field_number, expected_results[i][2],
		     "PB_read_tag case %d should set correct field number (expected %u, saw %u)",
		     i, expected_results[i][2], (uint32_t)field_number);
    }
}
#undef TEST_NAME

#if 0 // Extremely low pass rate and poor results, for unknown reasons
/**
 * pb_read_tag_hard_cases
 * @brief Tests edge cases for PB_read_tag
 */
#define TEST_NAME pb_read_tag_hard_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    char inputs[][5] = {
        {0xF8, 0xFF, 0xFF, 0xFF, 0x0F}, // 29-bit Max Value
        {0x80, 0x80, 0x80, 0x80, 0x10}, // 29-bit Overflow
        {0x07, 0xC5, 0x00, 0x00, 0x00}, // Invalid Wire Type
        {0x90, 0xC5, 0x00, 0x00, 0x00}, // Premature end of input
        {0x00, 0x00, 0x00, 0x00, 0x00}, // Empty Input file
    };
    const int input_sizes[] = {5, 5, 5, 2, 0};

    // Expected Return, Wire_type, field_number
    const int32_t expected_results[][3] = {
        {5, 0, 536870911},
        {5, 0, 536870912},
        {-1, 0, 0},
        {-1, 0, 0},
        {0, 0, 0}
    };

    const int num_inputs = sizeof(inputs) / sizeof(inputs[0]);

    for (int i = 0; i < num_inputs; i++) {
        PB_WireType wire_type = 0;
        int32_t field_number = 0;
        FILE* in = fmemopen(inputs[i], input_sizes[i], "r");
        const int ret = PB_read_tag(in, &wire_type, &field_number);
        fclose(in);

        cr_assert(ret == expected_results[i][0], "PB_read_tag case %d should return %d, but got %d", i,
                  expected_results[i][0], ret);
        cr_assert_eq(wire_type, expected_results[i][1],
		     "PB_read_tag case %d should set correct wire type (expected %u, saw %u)",
		     i, expected_results[i][1], wire_type);
        cr_assert_eq(field_number, expected_results[i][2],
		     "PB_read_tag case %d should set correct field number (expected %u, saw %u)",
		     i, expected_results[i][2], (uint32_t)field_number);
    }
}
#undef TEST_NAME
#endif

#undef TEST_SUITE
#define TEST_SUITE pb_read_value_suite

/**
 * pb_read_value_easy_cases
 * @brief Check if PB_read_value has correct implementation
 */
#define TEST_NAME pb_read_value_easy_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    typedef struct {
        PB_WireType type;
        size_t input_size;
        char* input;
        int expected_ret;
        union value expected_value;
    } pb_value_test_case;

    const pb_value_test_case tests[] = {
        {
            .type = VARINT_TYPE,
            .input_size = 1,
            .input = (char[]){0x08},
            .expected_ret = 1,
            .expected_value = {.i64 = 8}
        },
        {
            .type = VARINT_TYPE,
            .input_size = 3,
            .input = (char[]){0xAC, 0xFF, 0x08},
            .expected_ret = 3,
            .expected_value = {.i64 = 147372}
        },
        {
            .type = I64_TYPE,
            .input_size = 8,
            .input = (char[]){0xEF, 0xCD, 0xAB, 0x89, 0x00, 0x00, 0x00, 0x00},
            .expected_ret = 8,
            .expected_value = {.i64 = 2309737967UL}
        },
        {
            .type = I64_TYPE,
            .input_size = 8,
            .input = (char[]){0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01},
            .expected_ret = 8,
            .expected_value = {.i64 = 81985529216486895UL}
        },
        {
            .type = LEN_TYPE,
            .input_size = 5,
            .input = (char[]){0x04, 't', 'e', 's', 't'},
            .expected_ret = 5,
            .expected_value = {.bytes = {.size = 4, .buf = "test"}}
        },
        {
            .type = LEN_TYPE,
            .input_size = 7,
            .input = (char[]){0x06, 0x0A, 0x04, '\0', 0x7F, 'F', 0x0D},
            .expected_ret = 7,
            .expected_value = {.bytes = {.size = 6, .buf = (char[]){0x0A, 0x04, '\0', 0x7F, 'F', 0x0D}}}
        },
        {
            .type = I32_TYPE,
            .input_size = 4,
            .input = (char[]){0x78, 0x56, 0x34, 0x12},
            .expected_ret = 4,
            .expected_value = {.i32 = 305419896}
        },
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        union value* val = calloc(1, sizeof(union value));
        FILE* in = fmemopen(tests[i].input, tests[i].input_size, "r");
        const int res = PB_read_value(in, tests[i].type, val);

        cr_assert_eq(res, tests[i].expected_ret,
                     "PB_read_value #%d return value should match the expected return value", i);
        if (res == -1) {
            continue;
        }
        switch (tests[i].type) {
        case VARINT_TYPE:
        case I64_TYPE:
            cr_assert_eq(
                val->i64,
                tests[i].expected_value.i64,
                "Test case %d: Expected i64 value %llu, got %llu", i, tests[i].expected_value.i64, val->i64
            );
            break;
        case I32_TYPE:
            cr_assert_eq(
                val->i32,
                tests[i].expected_value.i32,
                "Test case %d: Expected i32 value %u, got %u", i, tests[i].expected_value.i32, val->i32
            );
            break;
        case LEN_TYPE:
            cr_assert_eq(
                val->bytes.size,
                tests[i].expected_value.bytes.size,
                "Test case %d: Expected LEN_TYPE payload size %zu, got %zu", i, tests[i].expected_value.bytes.size,
                val->bytes.size
            );
            cr_assert_arr_eq(
                val->bytes.buf,
                tests[i].expected_value.bytes.buf,
                tests[i].expected_value.bytes.size,
                "Test case %d: LEN_TYPE payload mismatch", i
            );
            free(val->bytes.buf);
            break;
        default: ;
        }
    }
}
#undef TEST_NAME

#define TEST_NAME pb_read_value_empty_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    typedef struct {
        PB_WireType type;
        size_t input_size;
        char* input;
        int expected_ret;
        union value expected_value;
    } pb_value_test_case;

    const pb_value_test_case tests[] = {
        {
            .type = VARINT_TYPE,
            .input_size = 0,
            .input = (char[]){},
            .expected_ret = 0,
            .expected_value = {.i64 = 0}
        },
        {
            .type = I64_TYPE,
            .input_size = 0,
            .input = (char[]){},
            .expected_ret = 0,
            .expected_value = {.i64 = 0}
        },
        {
            .type = LEN_TYPE,
            .input_size = 0,
            .input = (char[]){},
            .expected_ret = 0,
            .expected_value = {.bytes = {.size = 0, .buf = ""}}
        },
        {
            .type = I32_TYPE,
            .input_size = 0,
            .input = (char[]){},
            .expected_ret = 0,
            .expected_value = {.i32 = 0}
        },
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        union value* val = calloc(1, sizeof(union value));
        FILE* in = fmemopen(tests[i].input, tests[i].input_size, "r");
        const int res = PB_read_value(in, tests[i].type, val);

        cr_assert_eq(
            res,
            tests[i].expected_ret,
            "PB_read_value #%d return value %d should match the expected return value %d",
            i, res, tests[i].expected_ret
        );
        if (res == -1) {
            continue;
        }
        switch (tests[i].type) {
        case VARINT_TYPE:
        case I64_TYPE:
            cr_assert_eq(
                val->i64,
                tests[i].expected_value.i64,
                "Test case %d: Expected i64 value %lu, got %lu", i, tests[i].expected_value.i64, val->i64
            );
            break;
        case I32_TYPE:
            cr_assert_eq(
                val->i32,
                tests[i].expected_value.i32,
                "Test case %d: Expected i32 value %u, got %u", i, tests[i].expected_value.i32, val->i32
            );
            break;
        case LEN_TYPE:
            cr_assert_eq(
                val->bytes.size,
                tests[i].expected_value.bytes.size,
                "Test case %d: Expected LEN_TYPE payload size %zu, got %zu", i, tests[i].expected_value.bytes.size,
                val->bytes.size
            );
            cr_assert_arr_eq(
                val->bytes.buf,
                tests[i].expected_value.bytes.buf,
                tests[i].expected_value.bytes.size,
                "Test case %d: LEN_TYPE payload mismatch", i
            );
            free(val->bytes.buf);
            break;
        default: break;
        }
    }
}
#undef TEST_NAME

/**
 * pb_read_value_medium_cases
 * @brief Check if PB_read_value has correct implementation
 */
#define TEST_NAME pb_read_value_medium_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    typedef struct {
        PB_WireType type;
        size_t input_size;
        char* input;
        int expected_ret;
        union value expected_value;
    } pb_value_test_case;

    const pb_value_test_case tests[] = {
        {
            .type = VARINT_TYPE,
            .input_size = 4,
            .input = (char[]){0xAC, 0x02, 0xFF, 0xEE},
            .expected_ret = 2,
            .expected_value = {.i64 = 300}
        },
        {
            .type = I64_TYPE,
            .input_size = 10,
            .input = (char[]){0xEF, 0xCD, 0xAB, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x21, 0x5C},
            .expected_ret = 8,
            .expected_value = {.i64 = 12682136550669798895UL}
        },
        {
            .type = LEN_TYPE,
            .input_size = 1,
            .input = (char[]){0x00},
            .expected_ret = 1,
            .expected_value = {.bytes = {.size = 0, .buf = ""}}
        },
        {
            .type = LEN_TYPE,
            .input_size = 8,
            .input = (char[]){0x04, 'a', 'b', 'c', 'd', 0x11, 0x22, 0x33},
            .expected_ret = 5,
            .expected_value = {.bytes = {.size = 4, .buf = "abcd"}}
        },
        {
            .type = I32_TYPE,
            .input_size = 6,
            .input = (char[]){0xFF, 0xFF, 0xFF, 0xFF, 0xAA, 0xBB},
            .expected_ret = 4,
            .expected_value = {.i32 = 0xFFFFFFFF}
        },
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        union value* val = calloc(1, sizeof(union value));
        FILE* in = fmemopen(tests[i].input, tests[i].input_size, "r");
        const int res = PB_read_value(in, tests[i].type, val);

        cr_assert_eq(
            res,
            tests[i].expected_ret,
            "PB_read_value #%d return value %d should match the expected return value %d",
            i, res, tests[i].expected_ret
        );
        if (res == -1) {
            continue;
        }
        switch (tests[i].type) {
        case VARINT_TYPE:
        case I64_TYPE:
            cr_assert_eq(
                val->i64,
                tests[i].expected_value.i64,
                "Test case %d: Expected i64 value %llu, got %llu", i, tests[i].expected_value.i64, val->i64
            );
            break;
        case I32_TYPE:
            cr_assert_eq(
                val->i32,
                tests[i].expected_value.i32,
                "Test case %d: Expected i32 value %u, got %u", i, tests[i].expected_value.i32, val->i32
            );
            break;
        case LEN_TYPE:
            cr_assert_eq(
                val->bytes.size,
                tests[i].expected_value.bytes.size,
                "Test case %d: Expected LEN_TYPE payload size %zu, got %zu", i, tests[i].expected_value.bytes.size,
                val->bytes.size
            );
            cr_assert_arr_eq(
                val->bytes.buf,
                tests[i].expected_value.bytes.buf,
                tests[i].expected_value.bytes.size,
                "Test case %d: LEN_TYPE payload mismatch", i
            );
            free(val->bytes.buf);
            break;
        default: break;
        }
    }
}
#undef TEST_NAME

/**
 * pb_read_value_hard_cases
 * @brief Check if PB_read_value has correct implementation
 */
#define TEST_NAME pb_read_value_hard_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    typedef struct {
        PB_WireType type;
        size_t input_size;
        char* input;
        int expected_ret;
        union value expected_value;
    } pb_value_test_case;

    const pb_value_test_case tests[] = {
        {
            .type = VARINT_TYPE,
            .input_size = 1,
            .input = (char[]){0xAC},
            .expected_ret = -1,
            .expected_value = {.i64 = 0}
        },
        {
            .type = VARINT_TYPE,
            .input_size = 10,
            .input = (char[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01},
            .expected_ret = 10,
            .expected_value = {.i64 = 0xFFFFFFFFFFFFFFFFUL}
        },
        {
            .type = I64_TYPE,
            .input_size = 7,
            .input = (char[]){0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23},
            .expected_ret = -1,
            .expected_value = {.i64 = 0}
        },
        {
            .type = I64_TYPE,
            .input_size = 8,
            .input = (char[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            .expected_ret = 8,
            .expected_value = {.i64 = 0xFFFFFFFFFFFFFFFFULL}
        },
        {
            .type = LEN_TYPE,
            .input_size = 1,
            .input = (char[]){0x84}, // Size has continuation bit set.
            .expected_ret = -1,
            .expected_value = {.bytes = {.size = 0, .buf = NULL}}
        },
        {
            .type = LEN_TYPE,
            .input_size = 4,
            .input = (char[]){0x04, 't', 'e', 's'}, // Truncated len
            .expected_ret = -1,
            .expected_value = {.bytes = {.size = 0, .buf = NULL}}
        },
        {
            .type = I32_TYPE,
            .input_size = 3,
            .input = (char[]){0x78, 0x56, 0x34},
            .expected_ret = -1,
            .expected_value = {.i32 = 0}
        },
        {
            .type = 15, // Invalid WireType
            .input_size = 2,
            .input = (char[]){0xAC, 0x02},
            .expected_ret = -1,
        }
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        union value* val = calloc(1, sizeof(union value));
        FILE* in = fmemopen(tests[i].input, tests[i].input_size, "r");
        const int res = PB_read_value(in, tests[i].type, val);

        cr_assert_eq(res, tests[i].expected_ret,
                     "PB_read_value #%d return value should match the expected return value", i);
        if (res == -1) {
            continue;
        }
        switch (tests[i].type) {
        case VARINT_TYPE:
        case I64_TYPE:
            cr_assert_eq(
                val->i64,
                tests[i].expected_value.i64,
                "Test case %d: Expected i64 value %llu, got %llu", i, tests[i].expected_value.i64, val->i64
            );
            break;
        case I32_TYPE:
            cr_assert_eq(
                val->i32,
                tests[i].expected_value.i32,
                "Test case %d: Expected i32 value %u, got %u", i, tests[i].expected_value.i32, val->i32
            );
            break;
        case LEN_TYPE:
            cr_assert_eq(
                val->bytes.size,
                tests[i].expected_value.bytes.size,
                "Test case %d: Expected LEN_TYPE payload size %zu, got %zu", i, tests[i].expected_value.bytes.size,
                val->bytes.size
            );
            cr_assert_arr_eq(
                val->bytes.buf,
                tests[i].expected_value.bytes.buf,
                tests[i].expected_value.bytes.size,
                "Test case %d: LEN_TYPE payload mismatch", i
            );
            free(val->bytes.buf);
            break;
        default: ;
        }
    }
}
#undef TEST_NAME

#undef TEST_SUITE
#define TEST_SUITE pb_next_field_suite

#define TEST_NAME pb_next_field_easy_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message sentinel = generate_message();

    // Node index (prev), fnum, type, direction
    const int tests[][4] = {
        {2, 1, VARINT_TYPE, BACKWARD_DIR},
        {3, 1, VARINT_TYPE, FORWARD_DIR},
        {3, 1, VARINT_TYPE, BACKWARD_DIR},
        {5, 2, I64_TYPE, BACKWARD_DIR},
        {6, 2, I64_TYPE, FORWARD_DIR},
        {6, 2, I64_TYPE, BACKWARD_DIR},
        {8, 3, LEN_TYPE, BACKWARD_DIR},
        {9, 3, LEN_TYPE, FORWARD_DIR},
        {9, 3, LEN_TYPE, BACKWARD_DIR},
        {17, 4, I32_TYPE, BACKWARD_DIR},
        {18, 4, I32_TYPE, BACKWARD_DIR},
        {18, 4, I32_TYPE, FORWARD_DIR},
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    int proper_count = 0, reverse_count = 0;
    int last_proper_fail_at = -1, last_reverse_fail_at = -1;

    for (int i = 0; i < num_tests; i++) {
        PB_Field* prev = sentinel;
        int moved = 0;
        while (moved < tests[i][0]) {
            prev = prev->next;
            moved++;
        }

        const PB_Field* expected = PB_next_field_ref(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_proper = PB_next_field(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_reverse = PB_next_field(prev, tests[i][1], tests[i][2], !tests[i][3]);

        if (actual_proper == expected) { proper_count++; } else { last_proper_fail_at = i; }
        if (actual_reverse == expected) { reverse_count++; } else { last_reverse_fail_at = i; }
    }

    cr_assert((proper_count == num_tests) || (reverse_count == num_tests),
              "At least one of Proper (%d) or Reverse (%d) count should match (%d). Last proper fail: %d | reverse fail: %d",
              proper_count, reverse_count, num_tests, last_proper_fail_at, last_reverse_fail_at);
}
#undef TEST_NAME

#define TEST_NAME pb_next_field_medium_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message sentinel = generate_message();

    // Node index (prev), fnum, type, direction
    const int tests[][4] = {
        {2, 1, ANY_TYPE, FORWARD_DIR},
        {2, 1, ANY_TYPE, BACKWARD_DIR},
        {5, 1, ANY_TYPE, FORWARD_DIR},
        {5, 1, ANY_TYPE, BACKWARD_DIR},

        {14, ANY_FIELD, ANY_TYPE, FORWARD_DIR},
        {14, ANY_FIELD, ANY_TYPE, BACKWARD_DIR},

        {11, ANY_FIELD, VARINT_TYPE, BACKWARD_DIR},
        {11, ANY_FIELD, VARINT_TYPE, FORWARD_DIR},
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    int proper_count = 0, reverse_count = 0;
    int last_proper_fail_at = -1, last_reverse_fail_at = -1;

    for (int i = 0; i < num_tests; i++) {
        PB_Field* prev = sentinel;
        int moved = 0;
        while (moved < tests[i][0]) {
            prev = prev->next;
            moved++;
        }

        const PB_Field* expected = PB_next_field_ref(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_proper = PB_next_field(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_reverse = PB_next_field(prev, tests[i][1], tests[i][2], !tests[i][3]);

        if (actual_proper == expected) { proper_count++; } else { last_proper_fail_at = i; }
        if (actual_reverse == expected) { reverse_count++; } else { last_reverse_fail_at = i; }
    }

    cr_assert((proper_count == num_tests) || (reverse_count == num_tests),
              "At least one of Proper (%d) or Reverse (%d) count should match (%d). Last proper fail: %d | reverse fail: %d",
              proper_count, reverse_count, num_tests, last_proper_fail_at, last_reverse_fail_at);
}
#undef TEST_NAME

#define TEST_NAME pb_next_field_hard_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message sentinel = generate_message();

    // Node index (prev), fnum, type, direction
    const int tests[][4] = {
        {0, 2, I64_TYPE, FORWARD_DIR},
        {0, 2, I64_TYPE, BACKWARD_DIR},
        {0, 2, ANY_TYPE, FORWARD_DIR},
        {0, 2, ANY_TYPE, BACKWARD_DIR},

        {0, 2, LEN_TYPE, FORWARD_DIR},
        {0, 2, I32_TYPE, BACKWARD_DIR},

        {0, ANY_FIELD, VARINT_TYPE, FORWARD_DIR},
        {0, ANY_FIELD, VARINT_TYPE, BACKWARD_DIR},
        {0, ANY_FIELD, I32_TYPE, FORWARD_DIR},
        {0, ANY_FIELD, I32_TYPE, BACKWARD_DIR},

        {3, 5, ANY_TYPE, FORWARD_DIR},
        {10, 5, ANY_TYPE, BACKWARD_DIR},

        {20, 1, VARINT_TYPE, FORWARD_DIR},
        {20, 1, VARINT_TYPE, BACKWARD_DIR},
        {5, 1, LEN_TYPE, FORWARD_DIR},
        {5, 1, LEN_TYPE, BACKWARD_DIR},

        {24, ANY_FIELD, ANY_TYPE, FORWARD_DIR},
        {24, ANY_FIELD, ANY_TYPE, BACKWARD_DIR},

        {7, 3, ANY_TYPE, FORWARD_DIR},
        {7, 3, ANY_TYPE, BACKWARD_DIR},

        {23, 1, ANY_TYPE, FORWARD_DIR},
        {23, 1, ANY_TYPE, BACKWARD_DIR},
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);

    int proper_count = 0, reverse_count = 0;
    int last_proper_fail_at = -1, last_reverse_fail_at = -1;

    for (int i = 0; i < num_tests; i++) {
        PB_Field* prev = sentinel;
        int moved = 0;
        while (moved < tests[i][0]) {
            prev = prev->next;
            moved++;
        }

        const PB_Field* expected = PB_next_field_ref(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_proper = PB_next_field(prev, tests[i][1], tests[i][2], tests[i][3]);
        const PB_Field* actual_reverse = PB_next_field(prev, tests[i][1], tests[i][2], !tests[i][3]);

        if (actual_proper == expected) { proper_count++; } else { last_proper_fail_at = i; }
        if (actual_reverse == expected) { reverse_count++; } else { last_reverse_fail_at = i; }
    }

    cr_assert((proper_count == num_tests) || (reverse_count == num_tests),
              "At least one of Proper (%d) or Reverse (%d) count should match (%d). Last proper fail: %d | reverse fail: %d",
              proper_count, reverse_count, num_tests, last_proper_fail_at, last_reverse_fail_at);
}
#undef TEST_NAME

#undef TEST_SUITE
#define TEST_SUITE pb_read_message_suite

#define TEST_NAME pb_read_message_easy_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    const int num_tests = 10;
    int i = 0;
    size_t sizes[num_tests];
    char* tests[] = {
        hex_str_to_array("080010001800210000000000000000", &sizes[i++]),
        hex_str_to_array("0896d2bf9adbffffffff01100018002190f7aa9509bf0540", &sizes[i++]),
        hex_str_to_array("08e3b1a987d8a1011085ffffffffffffffff011801218dd13aaa9aa0f6bf", &sizes[i++]),
        hex_str_to_array("080010ffffffff071800216c26df6c73e3f93f", &sizes[i++]),
        hex_str_to_array("08808080808080808080011080808080f8ffffffff01180121f168e388b5f8e43e", &sizes[i++]),
        hex_str_to_array("0a057bc8039506120568656c6c6f20532a05416c69636535c80000003968c5ffffffffffff", &sizes[i++]),
        hex_str_to_array("0a06cd8306cad1021205776f726c642080102a03426f62359401000039a861000000000000", &sizes[i++]),
        hex_str_to_array("0a076fde01cd02bc03120870726f746f627566208bc8782a07436861726c696535f4010000396079feffffffffff", &sizes[i++]),
        hex_str_to_array("0a05ff93ebdc031206666f6f62617220002a054461766964352e0100003915cd5b0700000000", &sizes[i++]),
        hex_str_to_array("1205656d70747920542a034576653564000000394f9721c5ffffffff", &sizes[i++]),
    };

    for (int idx = 0; idx < num_tests; idx++) {
        FILE* in1 = fmemopen(tests[idx], sizes[idx], "r");
        FILE* in2 = fmemopen(tests[idx], sizes[idx], "r");

        PB_Message expected, generated;
        const int expected_return = PB_read_message_ref(in1, sizes[idx], &expected);
        const int generated_return = PB_read_message(in2, sizes[idx], &generated);

        cr_assert_eq(expected_return, generated_return,
                     "Case #%d: Expected return (%d) should match generated return (%d)", idx, expected_return,
                     generated_return);
        cr_assert(compare_messages(expected, generated), "Case #%d: Expected and Generated messages do not match", idx);

        fclose(in1);
        fclose(in2);
    }
}
#undef TEST_NAME

#define TEST_NAME pb_read_message_medium_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    const int num_tests = 10;
    int i = 0;
    size_t sizes[num_tests];
    char* tests[] = {
        hex_str_to_array("0a04746167310a04746167320a0474616733120e546869732069732067726561742112114e6565647320696d70726f76656d656e74120957656c6c20646f6e65", &sizes[i++]),
        hex_str_to_array("0a05616c7068610a0462657461120d466972737420636f6d6d656e74120e5365636f6e6420636f6d6d656e74", &sizes[i++]),
        hex_str_to_array("0a046e6577730a067570646174650a08627265616b696e6712124c6174657374206e65777320757064617465120b537461792074756e656421", &sizes[i++]),
        hex_str_to_array("120c4e6f2074616773206865726512124a75737420736f6d6520636f6d6d656e7473", &sizes[i++]),
        hex_str_to_array("0a0a73696e676c652d746167", &sizes[i++]),
        hex_str_to_array("0a0c080112084974656d206f6e650a0c080212084974656d2074776f120f0863120b4d61696e2064657461696c", &sizes[i++]),
        hex_str_to_array("0a0908651205416c7068610a0908ca011204426574610a0a08af02120547616d6d61121208ab04120d44657461696c656420696e666f", &sizes[i++]),
        hex_str_to_array("1219082a12154f6e6c792064657461696c2c206e6f206974656d73", &sizes[i++]),
        hex_str_to_array("0a1808ffffffffffffffffff01120b4e6567617469766520494412120800120e5a65726f2049442064657461696c", &sizes[i++]),
        hex_str_to_array("0a09080712054c75636b790a0c080d1208556e6c75636b793f1213088906120e5370656369616c206e756d626572", &sizes[i++]),
    };

    for (int idx = 0; idx < num_tests; idx++) {
        FILE* in1 = fmemopen(tests[idx], sizes[idx], "r");
        FILE* in2 = fmemopen(tests[idx], sizes[idx], "r");

        PB_Message expected, generated;
        const int expected_return = PB_read_message_ref(in1, sizes[idx], &expected);
        const int generated_return = PB_read_message(in2, sizes[idx], &generated);

        cr_assert_eq(expected_return, generated_return,
                     "Case #%d: Expected return (%d) should match generated return (%d)", idx, expected_return,
                     generated_return);
        cr_assert(compare_messages(expected, generated), "Case #%d: Expected and Generated messages do not match", idx);

        fclose(in1);
        fclose(in2);
    }
}
#undef TEST_NAME

#define TEST_NAME pb_read_message_hard_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    const int num_tests = 10;
    int i = 0;
    size_t sizes[num_tests];
    char* tests[] = {
        hex_str_to_array("0802120d4669727374204d65737361676518012100000000000012402d0a000000320548656c6c6f3a0564c801ac024206757267656e74420d686967682d7072696f726974794a1e0865120c496e697469616c20746573741a0ccdcc8c3fcdcc0c40333353405219080112095375624974656d20411a04746167311a04746167325a120a0863617465676f7279120673797374656d5a100a067374617475731206616374697665", &sizes[i++]),
        hex_str_to_array("085312104e6567617469766520494420546573741800216666666666660e402d05000000320e5468697320697320612074657374420c6578706572696d656e74616c4a0e08940312094e6f7420666f756e645a0d0a056572726f72120474727565", &sizes[i++]),
        hex_str_to_array("08fe887a120e4d61782056616c7565205465737418012100000000000014402de7030000320a6c617267652066696c653a06c0c407f1f7274208746f702d746965724208637269746963616c4a1a08c8011207537563636573731a0c0ad71f417b140e41d7a3f8405217080b12064974656d20581a05616c7068611a04626574615211080c12064974656d20591a0567616d6d615a100a04747970651208686967686c6f61645a110a06726567696f6e120775732d65617374", &sizes[i++]),
        hex_str_to_array("0800120c456d707479204669656c647318002100000000000000002d0000000032004a0408001200", &sizes[i++]),
        hex_str_to_array("08920c120c4c75636b79204e756d626572180121cdcccccccccc1e402d4d00000032056c75636b793a04074d890642056c75636b7942077370656369616c4a1608890612074a61636b706f741a08d7a3f8403d8a9b42521a088906120a4c75636b79204974656d1a0377696e1a04676f6c645a0d0a05636861726d120474727565", &sizes[i++]),
        hex_str_to_array("08cd0f12104e65676174697665204d657472696373180021000000000000f83f2d0300000032046e6f70653a1effffffffffffffffff01feffffffffffffffff01fdffffffffffffffff01420c6c6f772d7072696f726974794a11080d1207556e6c75636b791a04b81e053e520808421204546573745a0c0a047269736b120468696768", &sizes[i++]),
        hex_str_to_array("08f2c001120c416c6c205375624974656d73180121cdcccccccccc23402d5a000000320e616c6c7920737562206974656d733a04e707f806420462756c6b420866756c6c2d7365744a2208c102121345766572797468696e6720696e636c756465641a08a4704d40a470cd405212080112064974656d20411a0261311a0261325212080212064974656d20421a0262311a026232520e080312064974656d20431a0263315a100a046d6f64651208616476616e6365645a0c0a04746965721204676f6c64", &sizes[i++]),
        hex_str_to_array("08ee08120c5061796c6f6164204f6e6c791800219a999999999901402d16000000320c6f6e6c79207061796c6f61644a10080c120c4d696e696d616c20696e666f5a170a046e6f7465120f7370656369616c207061796c6f6164", &sizes[i++]),
        hex_str_to_array("08deac261210506920417070726f78696d6174696f6e1801216e861bf0f92109402d1f00000032067069206865783a04030e9f0142046d6174684208636f6e7374616e744a2108ba02120e50692063616c63756c6174696f6e1a0cc3f548401f85cb3f9a9929405a110a07666f726d756c611206636972636c65", &sizes[i++]),
        hex_str_to_array("08d01f120b4675747572652059656172180121cdcccccccccc12402d14000000320d616476616e63656420746563683a06e80fe90fee0f42066675747572654204746563684a1908ca01120a496e6e6f766174696f6e1a08ae470140ae47814052180808120c414920417373697374616e741a0261691a026d6c5a160a0863617465676f7279120a746563686e6f6c6f67795a0f0a057472656e641206726973696e67", &sizes[i++]),
    };

    for (int idx = 0; idx < num_tests; idx++) {
        FILE* in1 = fmemopen(tests[idx], sizes[idx], "r");
        FILE* in2 = fmemopen(tests[idx], sizes[idx], "r");

        PB_Message expected, generated;
        const int expected_return = PB_read_message_ref(in1, sizes[idx], &expected);
        const int generated_return = PB_read_message(in2, sizes[idx], &generated);

        cr_assert_eq(expected_return, generated_return,
                     "Case #%d: Expected return (%d) should match generated return (%d)", idx, expected_return,
                     generated_return);
        cr_assert(compare_messages(expected, generated), "Case #%d: Expected and Generated messages do not match", idx);

        fclose(in1);
        fclose(in2);
    }
}
#undef TEST_NAME

#define TEST_NAME pb_read_message_failure_cases
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    const int num_tests = 10;
    int i = 0;
    size_t sizes[num_tests];
    char* tests[] = {
        hex_str_to_array("083f0943", &sizes[i++]),
        hex_str_to_array("0a057bc803950612056865", &sizes[i++]),
        hex_str_to_array("0a057bc8039506120568656c6c6f20532a0541", &sizes[i++]),
        hex_str_to_array("08cd0f12104e65676174697665204d6574726963", &sizes[i++]),
        hex_str_to_array("080d1208556e6c75636b793f1213088906120e5370", &sizes[i++]),
        hex_str_to_array("1a0261691a026d6c5a160a0863617465676f7279120a746563", &sizes[i++]),
        hex_str_to_array("0d616476616e63656420746563683a06e80fe90fee0f4206667574757265", &sizes[i++]),
        hex_str_to_array("0a06920417070726f78696d6174696f6e1801216e861bf0f92109402d1f00000032067", &sizes[i++]),
        hex_str_to_array("e6f70653a1effffffffffffffffff01feffffffffffffffff01fdffffffffffffffff01420", &sizes[i++]),
        hex_str_to_array("0802120d4669727374204d65737361676518012100000000000012402d0a000000320548656c6c6f3a0564", &sizes[i++]),
    };

    for (int idx = 0; idx < num_tests; idx++) {
        FILE* in1 = fmemopen(tests[idx], sizes[idx], "r");
        FILE* in2 = fmemopen(tests[idx], sizes[idx], "r");

        PB_Message expected, generated;
        const int expected_return = PB_read_message_ref(in1, sizes[idx], &expected);
        const int generated_return = PB_read_message(in2, sizes[idx], &generated);

        cr_assert_eq(expected_return, generated_return,
                     "Case #%d: Expected return (%d) should match generated return (%d)", idx, expected_return,
                     generated_return);

        fclose(in1);
        fclose(in2);
    }
}
#undef TEST_NAME

#undef TEST_SUITE
#define TEST_SUITE pb_expand_tests

// Helper function to create a sentinel message
static PB_Message create_message() {
    PB_Message msg = calloc(1, sizeof(PB_Field));  // Sentinel node
    msg->type = SENTINEL_TYPE;
    msg->next = msg;
    msg->prev = msg;
    return msg;
}

// Helper function to create a packed field (LEN_TYPE)
static PB_Field *create_packed_field(int fnum, PB_WireType type, const char *data, size_t size) {
    PB_Field *field = calloc(1, sizeof(PB_Field));
    field->type = LEN_TYPE;  // Must be LEN_TYPE for packed encoding
    field->number = fnum;
    field->value.bytes.size = size;
    field->value.bytes.buf = malloc(size);
    memcpy(field->value.bytes.buf, data, size);
    return field;
}

// Tests basic expansion: packed {10, 20, 30} → individual VARINT fields.
#define TEST_NAME expand_basic
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    // Create packed field containing integers {10, 20, 30} as varints
    char packed_data[] = {10, 20, 30};  // Small values fit in one byte each
    PB_Field *packed_field = create_packed_field(1, VARINT_TYPE, packed_data, sizeof(packed_data));

    // Link into message
    packed_field->next = msg;
    packed_field->prev = msg;
    msg->next = packed_field;
    msg->prev = packed_field;

    // Expand packed fields
    int ret = PB_expand_packed_fields(msg, 1, VARINT_TYPE);
    cr_assert_eq(ret, 0, "PB_expand_packed_fields failed");

    // Check expansion results
    PB_Field *cur = msg->next;
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        cr_assert(cur->number == 1, "Expanded field has incorrect number");
        cr_assert(cur->type == VARINT_TYPE, "Expanded field has incorrect type");
        cr_assert_eq(cur->value.i32, values[i], "Incorrect value in expanded field");
        cur = cur->next;
    }

    // Ensure it links back to the sentinel
    cr_assert(cur == msg, "Expanded fields do not correctly terminate");
}
#undef TEST_NAME

// Tests that an empty message remains unchanged.
#define TEST_NAME expand_empty_message
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    int ret = PB_expand_packed_fields(msg, 1, VARINT_TYPE);
    cr_assert_eq(ret, 0, "PB_expand_packed_fields should return 0 even if there are no matching fields");
}
#undef TEST_NAME

// Tests function does nothing if no packed fields exist.
#define TEST_NAME expand_no_packed_fields
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    // Add a non-packed field
    PB_Field *field = calloc(1, sizeof(PB_Field));
    field->type = VARINT_TYPE;
    field->number = 2;
    field->value.i32 = 42;
    field->next = msg;
    field->prev = msg;
    msg->next = field;
    msg->prev = field;

    int ret = PB_expand_packed_fields(msg, 1, VARINT_TYPE);
    cr_assert_eq(ret, 0, "PB_expand_packed_fields should not modify message if no packed fields exist");

    // Ensure the existing field is still present
    cr_assert(msg->next->number == 2, "Non-packed field should not be modified");
    cr_assert(msg->next->value.i32 == 42, "Non-packed field value should remain unchanged");
}
#undef TEST_NAME

// Expands packed {1, 2, 3, 4, 5} → individual VARINT fields.
#define TEST_NAME expand_packed_varints
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    // Add a packed field containing {1, 2, 3, 4, 5} as varints
    char packed_data[] = {1, 2, 3, 4, 5}; // Simple varint encoding
    PB_Field *packed_field = create_packed_field(1, VARINT_TYPE, packed_data, sizeof(packed_data));

    // Insert the packed field into the message
    packed_field->next = msg;
    packed_field->prev = msg;
    msg->next = packed_field;
    msg->prev = packed_field;

    // Call function to expand packed fields
    int ret = PB_expand_packed_fields(msg, 1, VARINT_TYPE);
    cr_assert_eq(ret, 0, "PB_expand_packed_fields failed");

    // Check that the packed field has been replaced by expanded individual fields
    PB_Field *cur = msg->next;
    int values[] = {1, 2, 3, 4, 5};
    int count = 0;
    
    while (cur != msg) {
        cr_assert(cur->number == 1, "Expanded field has incorrect number");
        cr_assert(cur->type == VARINT_TYPE, "Expanded field has incorrect type");
        cr_assert_eq(cur->value.i32, values[count], "Incorrect value in expanded field, expected %d but got %d", values[count], cur->value.i32);
        cur = cur->next;
        count++;
    }

    cr_assert_eq(count, 5, "Incorrect number of expanded fields, expected 5 but got %d", count);
}
#undef TEST_NAME

// Tests function does nothing if no packed field matches `fnum`.
#define TEST_NAME no_packed_field
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    // Add an unrelated field (not packed)
    PB_Field *field = calloc(1, sizeof(PB_Field));
    field->type = VARINT_TYPE;
    field->number = 4;
    field->value.i32 = 42;
    field->next = msg;
    field->prev = msg;
    msg->next = field;
    msg->prev = field;

    int ret = PB_expand_packed_fields(msg, 1, VARINT_TYPE);
    cr_assert_eq(ret, 0, "PB_expand_packed_fields should do nothing when no packed field exists");

    // Ensure the existing field remains unchanged
    cr_assert(msg->next->number == 4, "Unrelated field should not be modified");
    cr_assert(msg->next->value.i32 == 42, "Unrelated field value should remain unchanged");
}
#undef TEST_NAME

#if 0 // Pass rate is zero -- unclear if this test makes sense
// Tests expansion of an empty packed field (should remove it).
#define TEST_NAME expand_empty_packed_field
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    PB_Message msg = create_message();

    // Add an empty packed field
    PB_Field *packed_field = create_packed_field(5, VARINT_TYPE, NULL, 0);

    packed_field->next = msg;
    packed_field->prev = msg;
    msg->next = packed_field;
    msg->prev = packed_field;

    // int ret = PB_expand_packed_fields(msg, 5, VARINT_TYPE);
    // cr_assert_eq(ret, 0, "PB_expand_packed_fields should return 0 even if the packed field is empty");

    // Ensure the message is still empty after expansion
    cr_assert(msg->next == msg, "Empty packed field should be removed");
}
#undef TEST_NAME
#endif
#undef TEST_SUITE

#define TEST_SUITE pb_embedded_tests

/* Helper function to generate large protobuf messages */
static void generate_large_protobuf(char *buffer, size_t *size, size_t num_fields) {
    size_t i;
    for (i = 0; i < num_fields; i++) {
        buffer[i * 3] = 0x08;  // Field 1 (varint)
        buffer[i * 3 + 1] = 0x96;
        buffer[i * 3 + 2] = 0x01;  // Value 150
    }
    *size = num_fields * 3;
}

/* Helper function to generate a valid compressed protobuf message */
static void compress_valid_protobuf(char *compressed_data, size_t *compressed_size, size_t num_fields) {
    char raw_data[4096];
    size_t raw_size;
    generate_large_protobuf(raw_data, &raw_size, num_fields);
    
    uLong dest_size = *compressed_size;
    int ret = compress((Bytef *)compressed_data, &dest_size, (const Bytef *)raw_data, raw_size);
    if (ret != Z_OK) {
        fprintf(stderr, "Compression failed\n");
        *compressed_size = 0;
        return;
    }
    *compressed_size = dest_size;
}

/* Tests PB_read_embedded_message with a simple valid protobuf message */
#define TEST_NAME read_embedded_basic
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    char data[10];
    size_t size = 3;
    data[0] = 0x08;  // Field 1 (varint)
    data[1] = 0x96;
    data[2] = 0x01;  // Value 150
    PB_Message msg = NULL;
    int result = PB_read_embedded_message(data, size, &msg);

    cr_assert_geq(result, 0, "PB_read_embedded_message failed. Expect non-negative, get %d", result);
    cr_assert_not_null(msg, "PB_read_embedded_message returned NULL message. Expect non-null, get NULL");
}
#undef TEST_NAME

/* Tests PB_read_embedded_message with more valid protobuf message */
#define TEST_NAME read_embedded_varying_sizes
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    char data[4096];
    size_t size;
    for (size_t num_fields = 50; num_fields <= 500; num_fields += 50) {
        generate_large_protobuf(data, &size, num_fields);
        PB_Message msg = NULL;

        // Call the function
        int result = PB_read_embedded_message(data, size, &msg);

        // Ensure msg is non-null
        cr_assert_not_null(msg, 
            "PB_read_embedded_message returned NULL message for %zu fields. Expected non-null.", num_fields);

        // Ensure that result is non-negative (avoid failure case)
        cr_assert_geq(result, 0, 
            "PB_read_embedded_message failed for %zu fields. Expected non-negative return, got %d", num_fields, result);

        // Verify structure: msg should be a circular linked list
        cr_assert_eq(msg->type, SENTINEL_TYPE, 
            "Message sentinel field has wrong type. Expected SENTINEL_TYPE, got %d", msg->type);
        
        // Check that fields match expected values
        PB_Field *field = msg->next;
        size_t count = 0;
        while (field != msg) {  // Traverse the circular linked list
            cr_assert_eq(field->type, VARINT_TYPE, 
                "Field %zu has wrong type. Expected VARINT_TYPE, got %d", count, field->type);
            cr_assert_eq(field->number, 1, 
                "Field %zu has wrong number. Expected 1, got %d", count, field->number);
            cr_assert_eq(field->value.i32, 150, 
                "Field %zu has wrong value. Expected 150, got %d", count, field->value.i32);
            field = field->next;
            count++;
        }

        // Ensure the correct number of fields are in the message
        cr_assert_eq(count, num_fields, 
            "PB_read_embedded_message returned incorrect number of fields. Expected %zu, got %zu", num_fields, count);
    }
}
#undef TEST_NAME

/* Additional positive test for PB_inflate_embedded_message */
#define TEST_NAME inflate_embedded_valid
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT) {
    char compressed_data[1024];
    size_t size = sizeof(compressed_data);
    compress_valid_protobuf(compressed_data, &size, 10);
    PB_Message msg = NULL;
    int result = PB_inflate_embedded_message(compressed_data, size, &msg);

    cr_assert_geq(result, 0, "PB_inflate_embedded_message failed on valid compressed input. Expect non-negative, get %d", result);
    cr_assert_not_null(msg, "PB_inflate_embedded_message returned NULL message for valid compressed input. Expect non-null, get NULL");
}
#undef TEST_NAME
#undef TEST_SUITE

#define TEST_SUITE monaco_test_suite
#define TEST_NAME summary_monaco_map
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);
    assert_files_match(ref_outfile, test_outfile, NULL);
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

#define TEST_NAME query_monaco_map_way
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-w 24874398"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    assert_files_match(ref_outfile, test_outfile, NULL);
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

#define TEST_NAME query_monaco_map_way_key
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-w 360063228 surface"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    assert_files_match(ref_outfile, test_outfile, NULL);
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

#define TEST_NAME query_monaco_map_node
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-n 1567015838 "); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    int ret = compare_numeric_files(ref_outfile, test_outfile);
    cr_assert_eq(ret, 0, "The output was not what was expected (diff exited with "
                     "status %d).\n",
                     WEXITSTATUS(ret));
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

#define TEST_NAME query_monaco_map_bbox
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-b"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    int ret = compare_numeric_files(ref_outfile, test_outfile);
    cr_assert_eq(ret, 0, "The output was not what was expected (diff exited with "
                     "status %d).\n",
                     WEXITSTATUS(ret));
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME


#if 0 // Test has been 'exploded' above, and this version gives poor results
#define TEST_NAME query_monaco_map
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-w 360063228 surface -n 1567015838 ");
    fprintf(f, "-w 1081736480 building -n 8488947332 ");
    fprintf(f, "-w 24874398 -b"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    int ret = compare_numeric_files(ref_outfile, test_outfile);
    cr_assert_eq(ret, 0, "The output was not what was expected (diff exited with "
                     "status %d).\n",
                     WEXITSTATUS(ret));
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME
#endif
#undef TEST_SUITE

#define TEST_SUITE truncated_input_suite
#define TEST_NAME trunc_blobheader 
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

#define TEST_NAME trunc_blobproper
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME
#undef TEST_SUITE

#define TEST_SUITE tiny_map_suite
#define TEST_NAME summary_tiny_map
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);
    assert_files_match(ref_outfile, test_outfile, NULL);
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

#define TEST_NAME query_tiny_map
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s -b -n 1 -n 2 -n 3 -n 4 -w 2"); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);

    int ret = compare_numeric_files(ref_outfile, test_outfile);
    cr_assert_eq(ret, 0, "The output was not what was expected (diff exited with "
                     "status %d).\n",
                     WEXITSTATUS(ret));
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME
#undef TEST_SUITE

#define TEST_SUITE osm_test_suite
#define TEST_NAME read_osm_map
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");
}
#undef TEST_NAME

#define TEST_NAME read_osm_map_invalid_1
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/trunc_blobheader/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp == NULL, "A NULL OSM_Map pointer was expected\n");
}
#undef TEST_NAME

#if 0  // Mostly redundant with previous test and results are not great
#define TEST_NAME read_osm_map_invalid_2
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/trunc_blobproper/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp == NULL, "A NULL OSM_Map pointer was expected\n");
}
#undef TEST_NAME
#endif

#define TEST_NAME read_osm_bbox
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");
    OSM_BBox *bbox = OSM_Map_get_BBox(mp);
    cr_assert(bbox != NULL, "A non-NULL OSM_BBox pointer was expected\n");
}
#undef TEST_NAME

#define TEST_NAME read_osm_stats
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");
	int node_count = OSM_Map_get_num_nodes(mp);
	int way_count = OSM_Map_get_num_ways(mp);
	cr_assert_eq(node_count, 10, "Node count is incorrect. Expected 10, Got %d\n", node_count);
	cr_assert_eq(node_count, 10, "Way count is incorrect. Expected 10, Got %d\n", way_count);
}
#undef TEST_NAME

#define TEST_NAME query_osm_bbox
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");
    OSM_BBox *bbox = OSM_Map_get_BBox(mp);
    cr_assert(bbox != NULL, "A non-NULL OSM_BBox pointer was expected\n");
	OSM_Lon min_lon = OSM_BBox_get_min_lon(bbox);
	OSM_Lon max_lon = OSM_BBox_get_max_lon(bbox);
	OSM_Lat min_lat = OSM_BBox_get_min_lat(bbox);
	OSM_Lat max_lat = OSM_BBox_get_max_lat(bbox);
	OSM_Lon exp_min_lon = -122084000000;
	OSM_Lon exp_max_lon = -122083000000;
	OSM_Lat exp_min_lat = 37421900000;
	OSM_Lat exp_max_lat = 37422900000;
	cr_assert_eq(min_lon, exp_min_lon, "Minimum longitude is incorrect. Expected %ld, Got %ld\n", 
			exp_min_lon, min_lon);
	cr_assert_eq(max_lon, exp_max_lon, "Maximum longitude is incorrect. Expected %ld, Got %ld\n", 
			exp_max_lon, max_lon);
	cr_assert_eq(min_lat, exp_min_lat, "Minimum latitude is incorrect. Expected %ld, Got %ld\n", 
			exp_min_lat, min_lat);
	cr_assert_eq(max_lat, exp_max_lat, "Maximum latitude is incorrect. Expected %ld, Got %ld\n", 
			exp_max_lat, max_lat);
}
#undef TEST_NAME

// TODO: Is there a better way to test this?
#define TEST_NAME read_osm_node
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nn = OSM_Map_get_num_nodes(mp);
	for(int i = 0; i < nn; i++) {
		OSM_Node *np = OSM_Map_get_Node(mp, i);
		cr_assert(np != NULL, "A non-NULL OSM_Node pointer was expected\n");
		OSM_Id id = OSM_Node_get_id(np);
		cr_assert(id > 0, "Invalid node detected!\n");
	}
}
#undef TEST_NAME

#define TEST_NAME read_osm_node_coords
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");
	
	OSM_Lon min_lon = -122084000000;
	OSM_Lon max_lon = -122083000000;
	OSM_Lat min_lat = 37421900000;
	OSM_Lat max_lat = 37422900000;

	int nn = OSM_Map_get_num_nodes(mp);
	for(int i = 0; i < nn; i++) {
		OSM_Node *np = OSM_Map_get_Node(mp, i);
		cr_assert(np != NULL, "A non-NULL OSM_Node pointer was expected\n");
		OSM_Id id = OSM_Node_get_id(np);
		OSM_Lat lat = OSM_Node_get_lat(np);
		OSM_Lon lon = OSM_Node_get_lon(np);
		cr_assert(id > 0, "Invalid node detected!\n");
		cr_assert(lat >= min_lat && lat <= max_lat, "Invalid node detected!\n");
		cr_assert(lon >= min_lon && lon <= max_lon, "Invalid node detected!\n");
	}
}
#undef TEST_NAME

#define TEST_NAME read_osm_node_invalid
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
	char *filename = "tests/rsrc/osm_small_map/ref.in";
	FILE *in = fopen(filename, "r");
	cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
	OSM_Map *mp = OSM_read_Map(in);
	cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nn = OSM_Map_get_num_nodes(mp);
	OSM_Node *np = OSM_Map_get_Node(mp, nn+1);
	cr_assert(np == NULL, "A NULL OSM_Node pointer was expected\n");
}
#undef TEST_NAME

#define TEST_NAME read_osm_way
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	for (int i = 0; i < nw; i++) {
		OSM_Way *wp = OSM_Map_get_Way(mp, i);
		cr_assert(wp != NULL, "A non-NULL OSM_Way pointer was expected\n");
		OSM_Id id = OSM_Way_get_id(wp);
		cr_assert(id > 0, "Invalid way detected!\n");
	}
}
#undef TEST_NAME

#define TEST_NAME read_osm_way_refs
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	for (int i = 0; i < nw; i++) {
		OSM_Way *wp = OSM_Map_get_Way(mp, i);
		cr_assert(wp != NULL, "A non-NULL OSM_Way pointer was expected\n");
		OSM_Id id = OSM_Way_get_id(wp);
		cr_assert(id > 0, "Invalid way detected!\n");

		int nr = OSM_Way_get_num_refs(wp);

		for (int j = 0; j < nr; j++) {
			OSM_Id ref_id = OSM_Way_get_ref(wp, j);
			cr_assert(ref_id > 0, "Invalid way reference detected!\n");
		}
	}
}
#undef TEST_NAME

#define TEST_NAME read_osm_way_keys
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	for (int i = 0; i < nw; i++) {
		OSM_Way *wp = OSM_Map_get_Way(mp, i);
		cr_assert(wp != NULL, "A non-NULL OSM_Way pointer was expected\n");
		OSM_Id id = OSM_Way_get_id(wp);
		cr_assert(id > 0, "Invalid way detected!\n");

		int nk = OSM_Way_get_num_keys(wp);

		for (int j = 0; j < nk; j++) {
			char* key = OSM_Way_get_key(wp, j);
			char* val = OSM_Way_get_value(wp, j);
			cr_assert(key != NULL, "Invalid way key detected!\n");
			cr_assert(val != NULL, "Invalid way value detected!\n");
		}
	}
}

#undef TEST_NAME

#define TEST_NAME read_osm_way_invalid
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
	char *filename = "tests/rsrc/osm_small_map/ref.in";
	FILE *in = fopen(filename, "r");
	cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
	OSM_Map *mp = OSM_read_Map(in);
	cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	OSM_Way *wp = OSM_Map_get_Way(mp, nw+1);

	cr_assert(wp == NULL, "A NULL OSM_Way pointer was expected\n");
}
#undef TEST_NAME

#define TEST_NAME read_osm_way_ref_invalid
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	for (int i = 0; i < nw; i++) {
		OSM_Way *wp = OSM_Map_get_Way(mp, i);
		cr_assert(wp != NULL, "A non-NULL OSM_Way pointer was expected\n");
		OSM_Id id = OSM_Way_get_id(wp);
		cr_assert(id > 0, "Invalid way detected!\n");

		int nr = OSM_Way_get_num_refs(wp);

		OSM_Id ref_id = OSM_Way_get_ref(wp, nr+1);
		cr_assert(ref_id <= 0, "A NULL (-1) way reference was expected!\n");
	}
}
#undef TEST_NAME

#define TEST_NAME read_osm_way_key_invalid
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *filename = "tests/rsrc/osm_small_map/ref.in";
    FILE *in = fopen(filename, "r");
    cr_assert(in != NULL, "The file '%s' could not be opened\n", filename);
    OSM_Map *mp = OSM_read_Map(in);
    cr_assert(mp != NULL, "A non-NULL OSM_Map pointer was expected\n");

	int nw = OSM_Map_get_num_ways(mp);
	for (int i = 0; i < nw; i++) {
		OSM_Way *wp = OSM_Map_get_Way(mp, i);
		cr_assert(wp != NULL, "A non-NULL OSM_Way pointer was expected\n");
		OSM_Id id = OSM_Way_get_id(wp);
		cr_assert(id > 0, "Invalid way detected!\n");

		int nk = OSM_Way_get_num_keys(wp);

		char* key = OSM_Way_get_key(wp, nk+1);
		char* val = OSM_Way_get_value(wp, nk+1);
		cr_assert(key == NULL, "A NULL way key was expected!\n");
		cr_assert(val == NULL, "A NULL way value was expected!\n");
	}
}

#undef TEST_NAME

#undef TEST_SUITE

#undef TEST_TIMEOUT

