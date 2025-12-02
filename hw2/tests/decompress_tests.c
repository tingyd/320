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

// Include function prototypes
int isMarkerValue(int markerRep, int bytes);
int isValidMarker(int byte);
int isMarker(int byte);

int isNonterminalStart(int byte);
int getNextNonterminalByte(FILE *in, FILE *out);
int getNextTerminalByte(FILE *in, FILE *out);
int makeNonterminalNext(int span, int prevbyte, FILE *in, FILE *out);
int isNonterminalStart(int byte);

int readRuleData(FILE *in, FILE *out);
int decompress(FILE *in, FILE *out);

/**
 * ================================
 * PART I: Low-level unit tests for de-compression helper functions
 * convertToUTF(int value, int bytesize, FILE *out) unit-tests
 * ================================
 *
 */

// Test(decompress_suite, readRuleData, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret;
//     FILE *file = fopen("rsrc/sheet.txt.seq","r");

//     fgetc(file);
//     fgetc(file);
//     exp_ret = 0x85;
//     ret = readRuleData(file, file);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);

//     file = fopen("rsrc/shortutf.txt.seq","r");
//     ret = readRuleData(file, file);
//     exp_ret = 0;
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
//          ret, exp_ret);
// }

// Test(decompress_suite, makeNonterminalNext_isNonterminalStart, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret;
//     FILE *file = fopen("rsrc/sheet.txt.seq","r");

//     fgetc(file); // sot
//     fgetc(file); // sob
//     fgetc(file);
//     fgetc(file);
//     int byte = fgetc(file);
//     int span = isNonterminalStart(byte);
//     ret = makeNonterminalNext(span, byte, file, file);
//     exp_ret = 265; // 265
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
//     cr_assert_eq(byte, 0b11000100, "Invalid return.  Got: %x | Expected: %x",
//          byte, 0b10000100);
//     cr_assert_eq(span, 1, "Invalid return.  Got: %d | Expected: %d",
//          span, 1);
// }

// Test(decompress_suite, getNextNonterminalByte, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret;
//     FILE *file = fopen("rsrc/sheet.txt.seq","r");

//     exp_ret = 0x81;
//     ret = getNextNonterminalByte(file, file);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(decompress_suite, getNextTerminalByte, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret;
//     FILE *file = fopen("rsrc/sheet.txt.seq","r");

//     exp_ret = 0x81;
//     ret = getNextTerminalByte(file, file);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(decompress_suite, isMarker, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret;
//     int b;

//     b = 0b10111111;
//     exp_ret = 1;
//     ret = isMarker(b);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11111111;
//     exp_ret = 0;
//     ret = isMarker(b);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(decompress_suite, valid_markers, .timeout=TEST_TIMEOUT) {
//     int ret;
//     int exp_ret = 0;
//     int b = 0x86;

//     ret = isValidMarker(b);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     exp_ret = 1;
//     b = 0x85;
//     ret = isValidMarker(b);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(decompress_suite, markerVals_and_tags, .timeout=TEST_TIMEOUT) {
//     int ret = 0;
//     int exp_ret = 0;
//     int b;

//     exp_ret = 1;
//     b = 0x81;
//     ret = isMarkerValue(0x81, b);
//     cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }


// Test(decompress_suite, isNonterminalStart, .timeout=TEST_TIMEOUT) {
//     int b = 0b11000000;
//     int ret = isNonterminalStart(b);
//     int exp_ret  = 0;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11010000;
//     ret = isNonterminalStart(b);
//     exp_ret  = 1;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11100000;
//     ret = isNonterminalStart(b);
//     exp_ret  = 2;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11110111;
//     ret = isNonterminalStart(b);
//     exp_ret  = 3;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11000011;
//     ret = isNonterminalStart(b);
//     exp_ret  = 0;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     b = 0b11000100;
//     ret = isNonterminalStart(b);
//     exp_ret  = 1;
//     cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
//          ret, exp_ret);

// }

/**
 * ================================
 * PART II: De-compression
 * decompress(FILE *in, FILE *out) unit-tests
 * ================================
 *
 */

/**
 * decompress_txt_small
 * @brief test decompress on a small file
 * in: TEST_INPUT/sheet.txt.seq
 * out: STUDENT_OUTPUT/emoji_1024.in.seq
 */
Test(decompress_suite, decompress_txt_small, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    FILE *in = fopen(TEST_INPUT"/sheet.txt.seq","r");
    FILE *out = fopen(STUDENT_OUTPUT"/sheet.txt","w");

    ret = decompress(in, out);
    exp_ret = 60;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
         ret, exp_ret);
}

/**
 * decompress_txt_large
 * @brief test decompress on a small file
 * in: TEST_INPUT/gettysburg.txt.seq
 * out: STUDENT_OUTPUT/gettysburg.txt
 */
Test(decompress_suite, decompress_txt_large, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    FILE *in = fopen(TEST_INPUT"/gettysburg.txt.seq","r");
    FILE *out = fopen(STUDENT_OUTPUT"/gettysburg.txt","w");

    ret = decompress(in, out);
    exp_ret = 1473;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
         ret, exp_ret);
}

/**
 * decompress_txt_emoji
 * @brief test decompress on a file containing only emojis
 * in: TEST_INPUT/emoji.in.seq
 * out: STUDENT_OUTPUT/emoji.in
 */
Test(decompress_suite, decompress_txt_emoji, .init=init_output, .timeout=TEST_TIMEOUT) {
    int ret;
    int exp_ret;
    FILE *in = fopen(TEST_INPUT"/emoji.in.seq","r");
    FILE *out = fopen(STUDENT_OUTPUT"/emoji.in","w");

    ret = decompress(in, out);
    exp_ret = 1745;
    cr_assert_eq(ret, exp_ret, "Invalid return.  Got: %x | Expected: %x",
         ret, exp_ret);
}