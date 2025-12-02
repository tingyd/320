#include "reference_functions.h"

void append_linked_list(PB_Field* sentinel, PB_WireType type, int fnum, union value val) {
    sentinel->type = SENTINEL_TYPE;

    PB_Field* new_field = calloc(1, sizeof(*new_field));
    new_field->type = type;
    new_field->number = fnum;
    new_field->value = val;

    new_field->next = sentinel;
    new_field->prev = sentinel->prev;
    sentinel->prev->next = new_field;
    sentinel->prev = new_field;
}

PB_Message generate_message() {
    PB_Message sentinel = calloc(1, sizeof(PB_Field));
    sentinel->next = sentinel->prev = sentinel;

    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 1});
    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 2});
    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 3});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 21});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 22});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 23});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test1"}});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test2"}});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test3"}});
    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 4});
    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 5});
    append_linked_list(sentinel, VARINT_TYPE, 1, (union value){.i64 = 6});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test4"}});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test5"}});
    append_linked_list(sentinel, LEN_TYPE, 3, (union value){.bytes = {.size = 5, .buf = "Test6"}});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 41});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 42});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 43});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 24});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 25});
    append_linked_list(sentinel, I64_TYPE, 2, (union value){.i64 = 26});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 44});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 45});
    append_linked_list(sentinel, I32_TYPE, 4, (union value){.i32 = 46});

    return sentinel;
}

PB_Field* PB_next_field_ref(const PB_Field* prev, const int fnum, const PB_WireType type, const PB_Direction dir) {
    if (prev == NULL) return NULL;

    PB_Field* curr_field = (dir == FORWARD_DIR) ? prev->next : prev->prev;
    while (curr_field->type != SENTINEL_TYPE) {
        if (curr_field->number == fnum || fnum == ANY_FIELD) {
            return (type == ANY_TYPE || curr_field->type == type) ? curr_field : NULL;
        }

        curr_field = (dir == FORWARD_DIR) ? curr_field->next : curr_field->prev;
    }

    return NULL;
}

int are_values_same(PB_WireType wire_type, union value expected, union value generated) {
    switch (wire_type) {
    case VARINT_TYPE:
    case I64_TYPE:
        return expected.i64 != generated.i64;
    case LEN_TYPE:
        return (
            (expected.bytes.size != generated.bytes.size) ||
            strncmp(expected.bytes.buf, generated.bytes.buf, expected.bytes.size) != 0
        );
    case I32_TYPE:
        return expected.i32 != generated.i32;
    default:
        return 1;
    }
}

int compare_messages(PB_Message expected, PB_Message generated) {
    if (expected == NULL || generated == NULL) return expected == generated;

    PB_Field *gen_fwd = generated->next, *gen_bwd = generated->prev;
    int forward_match = 1, backward_match = 1;

    for (PB_Field* exp = expected->next; exp != expected; exp = exp->next) {
        if (forward_match) {
            if ((gen_fwd == generated) || (gen_fwd->number != exp->number) || (gen_fwd->type != exp->type) ||
                !are_values_same(expected->type, expected->value, gen_fwd->value))
                forward_match = 0;
            else gen_fwd = gen_fwd->next;
        }

        if (backward_match) {
            if ((gen_bwd == generated) || (gen_bwd->number != exp->number) || (gen_bwd->type != exp->type) ||
                !are_values_same(expected->type, expected->value, gen_bwd->value))
                backward_match = 0;
            else gen_bwd = gen_bwd->prev;
        }

        if (!forward_match && !backward_match) {
            return 0;
        }
    }

    if (forward_match) {
        forward_match = gen_fwd == generated;
    }

    if (backward_match) {
        backward_match = gen_bwd == generated;
    }

    return backward_match || forward_match;
}

char* hex_str_to_array(const char* hexstr, size_t* out_len) {
    if (hexstr == NULL) {
        return NULL;
    }

    size_t hex_len = strlen(hexstr);
    if (hex_len % 2 != 0) {
        return NULL;
    }

    size_t array_len = hex_len / 2;
    char* array = malloc(array_len);
    if (array == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < array_len; i++) {
        char pair[3] = {hexstr[2 * i], hexstr[2 * i + 1], '\0'};
        array[i] = (char)strtol(pair, NULL, 16);
    }

    if (out_len) {
        *out_len = array_len;
    }

    return array;
}

int PB_read_value_varint_ref(FILE* in, uint64_t* varintp) {
    int total_bytes_read = 0;

    *varintp = 0;

    int position = 0, ch = fgetc(in);
    while (ch != EOF) {
        total_bytes_read += 1;

        *varintp |= ((uint64_t)(ch & 0b1111111) << (position * 7));

        if ((ch & 0b10000000) == 0) {
            break;
        }

        position += 1;
        ch = fgetc(in);
        if (ch == EOF) return -1;
    }

    return total_bytes_read;
}

int PB_read_value_i64_ref(FILE* in, uint64_t* i64p) {
    int total_bytes_read = 0;

    *i64p = 0;

    for (int i = 0; i < 8; i++) {
        const int ch = fgetc(in);
        if (ch == EOF) return (i == 0) ? 0 : -1;
        total_bytes_read += 1;

        *i64p |= ((uint64_t)ch << (i * 8));
    }

    return total_bytes_read;
}

int PB_read_value_len_ref(FILE* in, struct bytes* lenp) {
    int total_bytes_read = PB_read_value_varint_ref(in, &(lenp->size));
    if (total_bytes_read <= 0) return total_bytes_read;

    lenp->buf = (char*)calloc(1, lenp->size);
    if (lenp->buf == NULL) return -1;

    for (int i = 0; i < lenp->size; i++) {
        const int ch = fgetc(in);
        if (ch == EOF) return -1;

        lenp->buf[i] = (char)ch;

        total_bytes_read += 1;
    }

    return total_bytes_read;
}

int PB_read_value_i32_ref(FILE* in, uint32_t* i32p) {
    int total_bytes_read = 0;

    *i32p = 0;

    for (int i = 0; i < 4; i++) {
        const int ch = fgetc(in);
        if (ch == EOF) return (i == 0) ? 0 : -1;
        total_bytes_read += 1;

        *i32p |= ((uint32_t)ch << (i * 8));
    }

    return total_bytes_read;
}

int PB_read_tag_ref(FILE* in, PB_WireType* typep, int32_t* fieldp) {
    uint64_t tag;
    const int bytes_read = PB_read_value_varint_ref(in, &tag);

    if (bytes_read > 0) {
        *typep = tag & 0x7;
        if (*typep < 0 || *typep > 5) return -1;

        *fieldp = (int32_t)(tag >> 3);
    }

    return bytes_read;
}

int PB_read_value_ref(FILE* in, const PB_WireType type, union value* valuep) {
    if (type == VARINT_TYPE) {
        return PB_read_value_varint_ref(in, &(valuep->i64));
    }

    if (type == I64_TYPE) {
        return PB_read_value_i64_ref(in, &(valuep->i64));
    }

    if (type == LEN_TYPE) {
        return PB_read_value_len_ref(in, &(valuep->bytes));
    }

    if (type == SGROUP_TYPE || type == EGROUP_TYPE) {
        valuep->bytes.size = 0;
        valuep->bytes.buf = NULL;
        return 0;
    }

    if (type == I32_TYPE) {
        return PB_read_value_i32_ref(in, &(valuep->i32));
    }

    return -1;
}

int PB_read_field_ref(FILE* in, PB_Field* fieldp) {
    if (fieldp == NULL) return -1;
    int total_bytes_read = 0;

    int bytes_read = PB_read_tag_ref(in, &(fieldp->type), &(fieldp->number));
    if (bytes_read <= 0) return bytes_read;
    total_bytes_read += bytes_read;

    bytes_read = PB_read_value_ref(in, fieldp->type, &(fieldp->value));
    if (bytes_read <= 0) return -1;
    total_bytes_read += bytes_read;

    return total_bytes_read;
}

int PB_read_message_ref(FILE* in, const size_t len, PB_Message* msgp) {
    if ((*msgp = calloc(1, sizeof(PB_Field))) == NULL) {
        return -1;
    }

    PB_Field* sentinel_field = *msgp;
    sentinel_field->type = SENTINEL_TYPE;
    sentinel_field->next = sentinel_field->prev = sentinel_field;

    PB_Field* tail = sentinel_field;

    int total_bytes_read = 0;
    while (total_bytes_read < len) {
        PB_Field* field = calloc(1, sizeof(PB_Field));
        const int bytes_read = PB_read_field_ref(in, field);
        if (bytes_read <= 0) {
            free(field);
            if (bytes_read < 0) return -1;
            break;
        }

        field->next = tail->next;
        field->prev = tail;
        tail->next = field;
        field->next->prev = field;
        tail = field;

        total_bytes_read += bytes_read;
    }

    return total_bytes_read;
}


// Checks if given string is a numeric value
bool is_numeric(const char *str) {
	if (str == NULL || *str == '\0') return 0;

	int i = 0;
	if (str[i] == '-') i++;

	bool has_digits = false, has_dot = false;

	for (; str[i] != '\0'; i++) {
		if (isdigit(str[i])) {
			has_digits = true;
		}
		else if (str[i] == '.' && !has_dot) {
			has_dot = true;
		}
		else {
			return 0;
		}
	}

	return has_digits;
}

bool compare_tokens(const char *token1, const char *token2) {
	bool both_numeric = is_numeric(token1) && is_numeric(token2);
	if (both_numeric) {
		double val1 = atof(token1);
		double val2 = atof(token2);

		if (fabs(val1 - val2) < EPSILON) {
			return true;
		}
		return false;
	}
	else {
		return (strcmp(token1, token2) == 0);
	}
}

int compare_numeric_files(const char *filename1, const char *filename2) {
    FILE *fp1 = fopen(filename1, "r");
    FILE *fp2 = fopen(filename2, "r");
    if (!fp1 || !fp2) {
        // fprintf(stderr, "Error: Could not open one or both files.\n");
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return -1;
    }

    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    int line_number = -1;

    while (true) {
        char *res1 = fgets(line1, MAX_LINE_LENGTH, fp1);
        char *res2 = fgets(line2, MAX_LINE_LENGTH, fp2);

		// EOF on both
        if (!res1 && !res2) {
            break;
        }
        if (!res1 || !res2) {
            // fprintf(stderr, "Files differ in length (line %d).\n", line_number + 1);
            fclose(fp1);
            fclose(fp2);
            return -1;
        }

        line_number++;

        // Tokenize both lines
        char *token1 = strtok_r(res1, " ,\t\n\r", &res1);
        char *token2 = strtok_r(res2, " ,\t\n\r", &res2);

        while (token1 && token2) {
            if (!compare_tokens(token1, token2)) {
				/*
                fprintf(stderr, "Mismatch on line %d:\n", line_number);
                fprintf(stderr, "  File1 token: '%s'\n", token1);
                fprintf(stderr, "  File2 token: '%s'\n", token2);
				*/
                fclose(fp1);
                fclose(fp2);
                return -1;
            }
            token1 = strtok_r(res1, " ,\t\n\r", &res1);
            token2 = strtok_r(res2, " ,\t\n\r", &res2);
        }

        if (token1 || token2) {
            // fprintf(stderr, "Mismatch on line %d: different number of tokens.\n", line_number);
            fclose(fp1);
            fclose(fp2);
            return -1;
        }
    }

    fclose(fp1);
    fclose(fp2);
    return 0;
}
