#include <criterion/criterion.h>
#include <criterion/logging.h>
#include "const.h"
#include "sequitur.h"
#include "debug.h"

#define TEST_TIMEOUT 15

Test(validargs_suite, validargs_valid_1, .timeout=TEST_TIMEOUT) {
    int argc = 5;
    char *argv[] = {"bin/sequitur", "-h", "random", "stuff", "here", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x1;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set. Got: %x", opt);
}

Test(validargs_suite, validargs_valid_2, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "1", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x00010002;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set. Got: %x", opt);
}

Test(validargs_suite, validargs_valid_3, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "000000001", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x00010002;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_1, .timeout=TEST_TIMEOUT) {
    int argc = 3;
    char *argv[] = {"bin/sequitur", "-d", "-c", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_2, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "1025", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_3, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "b0000001", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_4, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "Hi Professor Stark.", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_5, .timeout=TEST_TIMEOUT) {
    int argc = 5;
    char *argv[] = {"bin/sequitur", "-c", "-d", "-b", "1024", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_6, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-b", "1024", "c", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

Test(validargs_suite, validargs_invalid_7, .timeout=TEST_TIMEOUT) {
    int argc = 4;
    char *argv[] = {"bin/sequitur", "-c", "-b", "1k", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int flag = 0x0;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit not set for. Got: %x", opt);
}

// Test(validargs_suite, modifyGlobalOptions, .timeout=TEST_TIMEOUT) {
//     // Include declaration
//     int modifyGlobalOptions(int blocksize, char *flag);

//     // Declaring variables
//     int blocksize;
//     char *flag;
//     int exp_go;

//     // Test 1
//     blocksize = 100;
//     flag = "-h";
//     exp_go = 0x00000001; // 0b 0000 0000 0000 0000 0000 0000 0000 0001
//     modifyGlobalOptions(blocksize, flag);
//     cr_assert_eq(global_options, exp_go, "Invalid return for modifyGlobalOptions.  Got: %X | Expected: %X",
//          global_options, exp_go);

//     // Test 2
//     blocksize = 100;
//     flag = "-d";
//     exp_go = 0x00000004; // 0b 0000 0000 0000 0000 0000 0000 0000 0100
//     modifyGlobalOptions(blocksize, flag);
//     cr_assert_eq(global_options, exp_go, "Invalid return for modifyGlobalOptions.  Got: %X | Expected: %X",
//          global_options, exp_go);

//     // Test 3
//     blocksize = 100;
//     flag = "-c";
//     exp_go = 0x00640002; // 0b 0000 0000 0110 0100 0000 0000 0000 0010
//     modifyGlobalOptions(blocksize, flag);
//     cr_assert_eq(global_options, exp_go, "Invalid return for modifyGlobalOptions.  Got: %X | Expected: %X",
//          global_options, exp_go);

//     // Test 3
//     blocksize = 1024;
//     flag = "-c";
//     exp_go = 0x04000002; // 0b 0000 0100 0000 0000 0000 0000 0000 0010
//     modifyGlobalOptions(blocksize, flag);
//     cr_assert_eq(global_options, exp_go, "Invalid return for modifyGlobalOptions.  Got: %X | Expected: %X",
//          global_options, exp_go);

// }

// Test(validargs_suite, parseBlocksizeInvalid, .timeout=TEST_TIMEOUT) {
//     // Include declaration
//     int parseBlocksize(char *string);

//     // Declaring variables
//     char *string;
//     int ret;
//     int exp_ret;

//     // Testing Fail Cases
//     exp_ret = -1;

//     // Test Fail 1
//     string = "invalid block size because these are not numbers!";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 2
//     string = "0b0000000000000000001024";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 3
//     string = "";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 4
//     string = "1z24";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 5
//     string = "1025";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 6
//     string = "0";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 7
//     string = "-1";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 8
//     string = "!";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 9
//     string = "  24";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Fail 10
//     string = "0000000";
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(validargs_suite, parseBlocksizeValid, .timeout=TEST_TIMEOUT) {
//     // Include declaration
//     int parseBlocksize(char *string);

//     // Declaring variables
//     char *string;
//     int ret;
//     int exp_ret;

//     // Test Pass 1
//     string = "1";
//     exp_ret = 1;
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Pass 2
//     string = "1024";
//     exp_ret = 1024;
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Pass 3
//     string = "00000001024";
//     exp_ret = 1024;
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Pass 3
//     string = "00000004";
//     exp_ret = 4;
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test Pass 4
//     string = "100";
//     exp_ret = 100;
//     ret = parseBlocksize(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for parseBlocksize.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }

// Test(validargs_suite, stringLength, .timeout=TEST_TIMEOUT) {
//     // Include declaration
//     int stringLength(char *string);

//     // Declaring variables
//     char *string;
//     int ret;
//     int exp_ret;

//     // Test 1
//     string = "length of 42 not counting null terminator.";
//     exp_ret = 42;
//     ret = stringLength(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringLength.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test 2
//     string = "";
//     exp_ret = 0;
//     ret = stringLength(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringLength.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test 3
//     string = " ";
//     exp_ret = 1;
//     ret = stringLength(string);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringLength.  Got: %d | Expected: %d",
//          ret, exp_ret);

// }


// Test(validargs_suite, stringCompare, .timeout=TEST_TIMEOUT) {
//     // Include declaration
//     int stringCompare(char *string1, char *string2);

//     // Declaring variables
//     char *string1;
//     char *string2;
//     int ret;
//     int exp_ret;

//     // Note:
//     // 1 : true
//     // 0 : false

//     // ------------------------------------------- //

//     // Set exp_ret to 1, true.
//     exp_ret = 1;

//     // Test True 1
//     string1 = "$abcde!";
//     string2 = "$abcde!";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test True 2
//     string1 = "";
//     string2 = "";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test True 3
//     string1 = "-h";
//     string2 = "-h";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test True 4
//     string1 = "  \t";
//     string2 = "  \t";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test True 5
//     string1 = "hi";
//     ret = stringCompare(string1, "hi");
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // ------------------------------------------- //

//     // Set exp_ret to 0, false.
//     exp_ret = 0;

//     // Test False 1
//     string1 = "$ABCDE!";
//     string2 = "$abcde!";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test False 2
//     string1 = "";
//     string2 = " ";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test False 2
//     string1 = "-h";
//     string2 = "-c";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);

//     // Test False 2
//     string1 = "   ";
//     string2 = " ";
//     ret = stringCompare(string1, string2);
//     cr_assert_eq(ret, exp_ret, "Invalid return for stringCompare.  Got: %d | Expected: %d",
//          ret, exp_ret);
// }