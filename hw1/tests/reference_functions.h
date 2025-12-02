#ifndef REFERENCE_FUNCTIONS_H
#define REFERENCE_FUNCTIONS_H

#define MAX_LINE_LENGTH 1024
#define EPSILON 1e-9

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "../include/protobuf.h"

PB_Message generate_message();
PB_Field* PB_next_field_ref(const PB_Field* prev, int fnum, PB_WireType type, PB_Direction dir);

int compare_messages(PB_Message expected, PB_Message generated);
char* hex_str_to_array(const char* hexstr, size_t* out_len);

int PB_read_message_ref(FILE* in, size_t len, PB_Message* msgp);

int compare_numeric_files(const char *filename1, const char *filename2);
#endif //REFERENCE_FUNCTIONS_H
