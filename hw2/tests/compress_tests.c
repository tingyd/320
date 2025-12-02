#include <criterion/criterion.h>
#include <criterion/logging.h>
#include "const.h"
#include "sequitur.h"
#include "debug.h"

#define TEST_TIMEOUT 15

#define STUDENT_OUTPUT "student_output"
#define TEST_INPUT "tests/inputs" 

#define COMPARE_OUTPUT(output, reference, exp_ret)				\
    run_with_system("cmp "STUDENT_OUTPUT"/"output" "TEST_INPUT"/"reference, exp_ret);

void run_with_system(char *cmd, int exp_status);

void init_output();

int convertToUTF(int value, int bytesize, FILE *out);

/**
 * ================================
 * PART I: Low-level unit tests for compression helper functions
 * convertToUTF(int value, int bytesize, FILE *out) unit-tests
 * ================================
 *
 */

// Test(compress_suite, convertToUTF1Byte, .init=init_output, .timeout=TEST_TIMEOUT) {
//     int exp_ret = 1;
//     int value = 36;
//     int bytesize = 1;

//     FILE *in = fopen(TEST_INPUT"/utf1.in.txt","w");
//     FILE *out = fopen(STUDENT_OUTPUT"/utf1.out.txt","w");
    
//     int ret = convertToUTF(value, bytesize, out);

//     if (in == NULL || out == NULL) {
//         cr_log_error("%s: FAILED TO OPEN FILE", __func__);
//     }

//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);

//     COMPARE_OUTPUT("utf1.out.txt", "utf1.in.txt", 0);
// }

// Test(compress_suite, convertToUTF2Byte, .init=init_output, .timeout=TEST_TIMEOUT) {
//     int exp_ret = 1;
//     int value = 255;
//     int bytesize = 2;

//     FILE *in = fopen(TEST_INPUT"/utf2.in.txt","w");
//     FILE *out = fopen(STUDENT_OUTPUT"/utf2.out.txt","w");
    
//     int ret = convertToUTF(value, bytesize, out);

//     if (in == NULL || out == NULL) {
//         cr_log_error("%s: FAILED TO OPEN FILE", __func__);
//     }

//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);

//     COMPARE_OUTPUT("utf2.out.txt", "utf2.in.txt", 0);
// }

// Test(compress_suite, convertToUTF3Byte, .init=init_output, .timeout=TEST_TIMEOUT) {
//     int exp_ret = 1;
//     int value = 8364;
//     int bytesize = 3;

//     FILE *in = fopen(TEST_INPUT"/utf3.in.txt","w");
//     FILE *out = fopen(STUDENT_OUTPUT"/utf3.out.txt","w");
    
//     int ret = convertToUTF(value, bytesize, out);

//     if (in == NULL || out == NULL) {
//         cr_log_error("%s: FAILED TO OPEN FILE", __func__);
//     }

//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);

//     COMPARE_OUTPUT("utf3.out.txt", "utf3.in.txt", 0);
// }

// Test(compress_suite, convertToUTF4Byte, .init=init_output, .timeout=TEST_TIMEOUT) {
//     int exp_ret = 1;
//     int value = 128512;
//     int bytesize = 4;
    
//     FILE *in = fopen(TEST_INPUT"/utf4.in.txt","w");
//     FILE *out = fopen(STUDENT_OUTPUT"/utf4.out.txt","w");
    
//     int ret = convertToUTF(value, bytesize, out);

//     if (in == NULL || out == NULL) {
//         cr_log_error("%s: FAILED TO OPEN FILE", __func__);
//     }

//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);

//     COMPARE_OUTPUT("utf4.out.txt", "utf4.in.txt", 0);
// }

/**
 * ================================
 * PART II: Compression
 * compress(FILE *in, FILE *out, int bsize) unit-tests
 * ================================
 *
 */

/**
 * compress_txt_small
 * @brief test compress on a small text file with blocksize=1
 * in: TEST_INPUT/sheet.txt.txt
 * out: STUDENT_OUTPUT/sheet.txt.seq
 */
Test(compress_suite, compress_txt_small, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    int bsize = 1;
    FILE *in = fopen(TEST_INPUT"/sheet.txt","r");
    FILE *out = fopen(STUDENT_OUTPUT"/sheet.txt.seq","w");

    ret = compress(in, out, bsize);
    exp_ret = 302;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
         ret, exp_ret);
}

/**
 * compress_txt_large
 * @brief test compress on a large text file with blocksize=1
 * in: TEST_INPUT/gettysburg.txt
 * out: STUDENT_OUTPUT/gettysburg.txt.seq
 */
Test(compress_suite, compress_txt_large, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    int bsize = 1;
    FILE *in = fopen(TEST_INPUT"/gettysburg.txt","r");
    FILE *out = fopen(STUDENT_OUTPUT"/gettysburg.txt.seq","w");

    ret = compress(in, out, bsize);
    exp_ret = 7367;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
         ret, exp_ret);
}

/**
 * compress_emoji
 * @brief test compress on a file containing only emojis with blocksize=1
 * in: TEST_INPUT/emoji.in
 * out: STUDENT_OUTPUT/emoji.in.seq
 */
Test(compress_suite, compress_emoji, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    int bsize = 1;
    FILE *in = fopen(TEST_INPUT"/emoji.in","r");
    FILE *out = fopen(STUDENT_OUTPUT"/emoji.in.seq","w");

    ret = compress(in, out, bsize);
    exp_ret = 10471;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
         ret, exp_ret);
}

/**
 * compress_txt_large_1024
 * @brief test compress on a large text file with blocksize=1024
 * in: TEST_INPUT/gettysburg.txt
 * out: STUDENT_OUTPUT/gettysburg_1024.txt.seq
 */
Test(compress_suite, compress_txt_large_1024, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    int bsize = 1024;
    FILE *in = fopen(TEST_INPUT"/gettysburg.txt","r");
    FILE *out = fopen(STUDENT_OUTPUT"/gettysburg_1024.txt.seq","w");

    ret = compress(in, out, bsize);
    exp_ret = 2156;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
         ret, exp_ret);
}

/**
 * compress_emoji_1024
 * @brief test compress on a file containing only emojis with blocksize=1024
 * in: TEST_INPUT/emoji.in
 * out: STUDENT_OUTPUT/emoji_1024.in.seq
 */
Test(compress_suite, compress_emoji_1024, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    int bsize = 1024;
    FILE *in = fopen(TEST_INPUT"/emoji.in","r");
    FILE *out = fopen(STUDENT_OUTPUT"/emoji_1024.in.seq","w");

    ret = compress(in, out, bsize);
    exp_ret = 1172;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
         ret, exp_ret);
}

/**
 * compress_txt_small_inverse
 * @brief Checks if student compress/decompress are inverses of each other
 * in: TEST_INPUT/sheet.txt
 * out: STUDENT_OUTPUT/sheet_inverse.txt.seq
 */
Test(compress_suite, compress_txt_small_inverse, .init=init_output, .timeout=TEST_TIMEOUT) {
    FILE *in = fopen(TEST_INPUT"/sheet.txt", "r");
    FILE *reversed = fopen(STUDENT_OUTPUT"/sheet_inverse.txt", "w");
    FILE *out = fopen(STUDENT_OUTPUT"/sheet_inverse.txt.seq", "w");

    if (in == NULL || out == NULL || reversed == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int cret = compress(in, out, 1024);

    if (fclose(out) == EOF){
      cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    }

    out = fopen(STUDENT_OUTPUT"/sheet_inverse.txt.seq", "r");

    if (out == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int dret = decompress(out, reversed);

    if (fclose(in) == EOF || fclose(out) == EOF || fclose(reversed) == EOF)
        cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    COMPARE_OUTPUT("sheet_inverse.txt", "sheet.txt", 0);
}

/**
 * compress_txt_large_inverse
 * @brief Checks if student compress/decompress are inverses of each other
 * in: TEST_INPUT/gettysburg.txt
 * out: STUDENT_OUTPUT/sheet_inverse.txt.seq
 */
Test(compress_suite, compress_txt_large_inverse, .init=init_output, .timeout=TEST_TIMEOUT) {
    FILE *in = fopen(TEST_INPUT"/gettysburg.txt", "r");
    FILE *reversed = fopen(STUDENT_OUTPUT"/gettysburg_inverse.txt", "w");
    FILE *out = fopen(STUDENT_OUTPUT"/gettysburg_inverse.txt.seq", "w");

    if (in == NULL || out == NULL || reversed == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int cret = compress(in, out, 1024);

    if (fclose(out) == EOF){
      cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    }

    out = fopen(STUDENT_OUTPUT"/gettysburg_inverse.txt.seq", "r");

    if (out == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int dret = decompress(out, reversed);

    if (fclose(in) == EOF || fclose(out) == EOF || fclose(reversed) == EOF)
        cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    COMPARE_OUTPUT("gettysburg_inverse.txt", "gettysburg.txt", 0);
}

/**
 * compress_emoji_inverse
 * @brief Checks if student compress/decompress are inverses of each other
 * in: TEST_INPUT/emoji.in
 * out: STUDENT_OUTPUT/emoji_inverse.in.seq
 */
Test(compress_suite, compress_emoji_inverse, .init=init_output, .timeout=TEST_TIMEOUT) {
    FILE *in = fopen(TEST_INPUT"/emoji.in", "r");
    FILE *reversed = fopen(STUDENT_OUTPUT"/emoji_inverse.in", "w");
    FILE *out = fopen(STUDENT_OUTPUT"/emoji_inverse.in.seq", "w");

    if (in == NULL || out == NULL || reversed == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int cret = compress(in, out, 1024);

    if (fclose(out) == EOF){
      cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    }

    out = fopen(STUDENT_OUTPUT"/emoji_inverse.in.seq", "r");

    if (out == NULL) {
        cr_log_error("%s: FAILED TO OPEN FILE", __func__);
    }

    int dret = decompress(out, reversed);

    if (fclose(in) == EOF || fclose(out) == EOF || fclose(reversed) == EOF)
        cr_log_warn("%s: FAILED TO CLOSE FD", __func__);
    COMPARE_OUTPUT("emoji_inverse.in", "emoji.in", 0);
}