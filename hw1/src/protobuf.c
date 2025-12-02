#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protobuf.h"
#include "zlib_inflate.h"
#include "zlib.h"
#include "debug.h"


/**
 * @brief  Read data from an input stream, interpreting it as a protocol buffer
 * message.
 * @details  This function assumes that the input stream "in" contains at least
 * len bytes of data.  The data is read from the stream, interpreted as a
 * protocol buffer message, and a pointer to the resulting PB_Message object is
 * returned.
 *
 * @param in  The input stream from which to read data.
 * @param len  The number of bytes of data to read from the input stream.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */
int PB_read_message(FILE *in, size_t len, PB_Message *msgp) {
    // Create sentinel
    PB_Field *sentinel = calloc(1, sizeof(PB_Field));
    if (!sentinel) return -1;
    
    sentinel->type = SENTINEL_TYPE;
    sentinel->number = ANY_FIELD;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    
    PB_Field *last = sentinel;
    size_t bytesRead = 0;
    
    while (bytesRead < len) {
        PB_Field *field = calloc(1, sizeof(PB_Field));
        if (!field) return -1;
        
        int n = PB_read_field(in, field);
        if (n <= 0) {
            free(field);
            if (n == 0 && bytesRead == 0) {
                *msgp = sentinel;
                return 0;
            }
            return -1;
        }
        
        // Insert into circular list
        field->next = sentinel;
        field->prev = last;
        last->next = field;
        sentinel->prev = field;
        last = field;
        
        bytesRead += n;
    }
    
    *msgp = sentinel;
    return bytesRead;
}

/**
 * @brief  Read data from a memory buffer, interpreting it as a protocol buffer
 * message.
 * @details  This function assumes that buf points to a memory area containing
 * len bytes of data.  The data is interpreted as a protocol buffer message and
 * a pointer to the resulting PB_Message object is returned.
 *
 * @param buf  The memory buffer containing the compressed data.
 * @param len  The length of the compressed data.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of success, -1 in case any error occurred.
 */

int PB_read_embedded_message(char *buf, size_t len, PB_Message *msgp) {
    if(buf == NULL  || msgp == NULL || len ==0){
        perror("Error reached at PB_read_embedded_message");
        return -1;
    }

    FILE *stream = fmemopen(buf, len, "rb");
    if (stream == NULL) {
        perror("Error: Failed to open memory buffer as file stream");
        return -1;
    }

    int result = PB_read_message(stream, len, msgp);

    fclose(stream);
    if (result > 0) {
        return 0;  // Success
    } else {
        return -1; // Error
    }
}
/**
 * @brief  Read zlib-compressed data from a memory buffer, inflating it
 * and interpreting it as a protocol buffer message.
 * @details  This function assumes that buf points to a memory area containing
 * len bytes of zlib-compressed data.  The data is inflated, then the
 * result is interpreted as a protocol buffer message and a pointer to
 * the resulting PB_Message object is returned.
 *
 * @param buf  The memory buffer containing the compressed data.
 * @param len  The length of the compressed data.
 * @param msgp  Pointer to a caller-provided variable to which to assign the
 * resulting PB_Message.
 * @return 0 in case of success, -1 in case any error occurred.
 */

int PB_inflate_embedded_message(char *buf, size_t len, PB_Message *msgp) {
    if(buf == NULL || msgp == NULL){
        fprintf(stderr, "Error: Invalid parameters in PB_inflate_embedded_message\n");
        return -1;
    }
    
    if ((unsigned char)buf[0] != 0x78) {
        fprintf(stderr, "Error: Invalid zlib header! First byte: %02X\n", (unsigned char)buf[0]);
        return -1;
    }

    FILE *source = fmemopen(buf, len, "rb");
    if (source == NULL) {
        fprintf(stderr, "Error: Failed to open source memory stream\n");
        return -1;
    }

    size_t decompressed_size = 16384;
    char *decompressed_buf = malloc(decompressed_size);
    if (decompressed_buf == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for decompressed buffer\n");
        fclose(source);
        return -1;
    }

    FILE *dest = open_memstream(&decompressed_buf, &decompressed_size);
    if (dest == NULL) {
        fprintf(stderr, "Error: Failed to open destination memory stream\n");
        fclose(source);
        free(decompressed_buf);
        return -1;
    }

    int ret = zlib_inflate(source, dest);
    fclose(source);
    fclose(dest);

    if (ret != Z_OK) {
        fprintf(stderr, "Error: Decompression failed with code %d\n", ret);
        free(decompressed_buf);
        return -1;
    }

    size_t actual_decompressed_size = decompressed_size; 

    int result = PB_read_embedded_message(decompressed_buf, actual_decompressed_size, msgp);
    free(decompressed_buf);  
    
    // Fix: This function should also return 0 for success
    return result;  // PB_read_embedded_message now returns 0 for success
}

/**
 * @brief  Read a single field of a protocol buffers message and initialize
 * a PB_Field structure.
 * @details  This function reads data from the input stream in and interprets
 * it as a single field of a protocol buffers message.  The information read,
 * consisting of a tag that specifies a wire type and field number,
 * as well as content that depends on the wire type, is used to initialize
 * the caller-supplied PB_Field structure pointed at by the parameter fieldp.
 * @param in  The input stream from which data is to be read.
 * @param fieldp  Pointer to a caller-supplied PB_Field structure that is to
 * be initialized.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_field(FILE *in, PB_Field *fieldp) {
    if (fieldp == NULL) {
        perror("fieldp is NULL");
        return -1;
    }

    // printf("====================Reading field====================\n");
    PB_WireType wiretype;
    int32_t fieldnum;
    int tagByte =0;
    tagByte =PB_read_tag(in, &wiretype, &fieldnum); 
    // printf("NUmber of bytes after reading tag %d\n", tagByte);
    if (tagByte <= 0) {
        perror("Error or EOF while reading tag");
        return -1;
    }
    fieldp->type = wiretype;
    fieldp->number = fieldnum;
    int valueBytes =0;
    valueBytes = PB_read_value(in, wiretype, &fieldp->value);
    // printf("NUmber of bytes after reading value %d\n", valueBytes);

    if (valueBytes < 0 || (valueBytes == 0 && fieldp->type != LEN_TYPE && 
                        fieldp->type != SGROUP_TYPE && fieldp->type != EGROUP_TYPE)) {
        perror("Error or EOF while reading value");
        return -1;
    }
    tagByte += valueBytes;
    return tagByte;
}

/**
 * @brief  Read the tag portion of a protocol buffers field and return the
 * wire type and field number.
 * @details  This function reads a varint-encoded 32-bit tag from the
 * input stream in, separates it into a wire type (from the three low-order bits)
 * and a field number (from the 29 high-order bits), and stores them into
 * caller-supplied variables pointed at by parameters typep and fieldp.
 * If the wire type is not within the legal range [0, 5], an error is reported.
 * @param in  The input stream from which data is to be read.
 * @param typep  Pointer to a caller-supplied variable in which the wire type
 * is to be stored.
 * @param fieldp  Pointer to a caller-supplied variable in which the field
 * number is to be stored.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_tag(FILE *in, PB_WireType *typep, int32_t *fieldp) {
    uint32_t tag = 0;
    int shift = 0;
    int bytesRead = 0;
    
    while (1) {
        int c = fgetc(in);
        if (c == EOF) {
            return (bytesRead == 0) ? 0 : -1;  // Clean EOF vs truncated
        }
        
        bytesRead++;
        uint8_t byte = (uint8_t)c;
        
        tag |= ((uint32_t)(byte & 0x7F) << shift);
        shift += 7;
        
        if ((byte & 0x80) == 0) {
            break;  // No continuation bit
        }
        
        if (shift >= 32) {  // Prevent overflow (32-bit tag)
            return -1;
        }
    }
    
    *typep = (PB_WireType)(tag & 0x07);
    *fieldp = (int32_t)(tag >> 3);
    
    // Validate wire type
    if (*typep > 5) {
        return -1;
    }
    
    return bytesRead;
}

/**
 * @brief  Reads and returns a single value of a specified wire type from a
 * specified input stream.
 * @details  This function reads bytes from the input stream in and interprets
 * them as a single protocol buffers value of the wire type specified by the type
 * parameter.  The number of bytes actually read is variable, and will depend on
 * the wire type and on the particular value read.  The data read is used to
 * initialize the caller-supplied variable pointed at by the valuep parameter.
 * In the case of wire type LEN_TYPE, heap storage will be allocated that is
 * sufficient to hold the number of bytes read and a pointer to this storage
 * will be stored at valuep->bytes.buf.
 * @param in  The input stream from which data is to be read.
 * @param type  The wire type of the value to be read.
 * @param valuep  Pointer to a caller-supplied variable that is to be initialized
 * with the data read.
 * @return 0 in case of an immediate end-of-file on the input stream without
 * any error and no input bytes having been read, -1 if there was an error
 * or unexpected end-of-file after reading a non-zero number of bytes,
 * otherwise the number n > 0 of bytes read if no error occurred.
 */

int PB_read_value(FILE *in, PB_WireType type, union value *valuep) {
    int shift = 0;
    uint8_t byte;
    uint64_t length = 0;
    int bytesRead = 0;
    
    switch(type) {
        case VARINT_TYPE: {
            uint64_t result = 0;
            while (fread(&byte, 1, 1, in) == 1) {
                bytesRead++;
                result |= (uint64_t)(byte & 0x7F) << shift;
                if ((byte & 0x80) == 0) {
                    valuep->i64 = result;
                    return bytesRead;
                }
                shift += 7;
                if (shift >= 64) return -1; // Prevent overflow
            }
            if (bytesRead == 0) return 0; // Clean EOF
            return -1; // Truncated varint
        }
        
        case I64_TYPE: {
            size_t read = fread(&valuep->i64, 1, sizeof(uint64_t), in);
            if (read == 0) return 0; // Clean EOF
            if (read != sizeof(uint64_t)) {
                fprintf(stderr, "Error: Read %zu bytes, expected %zu\n", read, sizeof(uint64_t));
                return -1;
            }
            return sizeof(uint64_t);
        }
        
        case LEN_TYPE: {
            // Read length varint
            while (fread(&byte, 1, 1, in) == 1) {
                bytesRead++;
                length |= (uint64_t)(byte & 0x7F) << shift;
                shift += 7;
                if ((byte & 0x80) == 0) break;
                if (shift >= 64) return -1;
            }
            
            if (bytesRead == 0) return 0; // Clean EOF
            
            valuep->bytes.size = length;
            if (length == 0) {
                valuep->bytes.buf = NULL;
                return bytesRead;
            }
            
            valuep->bytes.buf = malloc(length);
            if (!valuep->bytes.buf) return -1;
            
            size_t read = fread(valuep->bytes.buf, 1, length, in);
            if (read != length) {
                fprintf(stderr, "Error: Read %zu bytes, expected %lu\n", read, length);
                free(valuep->bytes.buf);
                return -1;
            }
            
            return bytesRead + length;
        }
        
        case I32_TYPE: {
            size_t read = fread(&valuep->i32, 1, 4, in);
            if (read == 0) return 0;
            if (read != 4) {
                fprintf(stderr, "Error: Read %zu bytes, expected 4\n", read);
                return -1;
            }
            return 4;
        }
        
        case SGROUP_TYPE:
        case EGROUP_TYPE:
            return 0; // Deprecated, return 0 bytes
            
        default:
            return -1; // Invalid wire type
    }
}
/**
 * @brief Get the next field with a specified number from a PB_Message object,
 * scanning the fields in a specified direction starting from a specified previous field.
 * @details  This function iterates through the fields of a PB_Message object,
 * until the first field with the specified number has is encountered or the end of
 * the list of fields is reached.  The list of fields is traversed, either in the
 * forward direction starting from the first field after prev if dir is FORWARD_DIR,
 * or the backward direction starting from the first field before prev if dir is BACKWARD_DIR.
 * When the a field with the specified number is encountered (or, if fnum is ANY_FIELD
 * any field is encountered), the wire type of that field is checked to see if it matches
 * the wire type specified by the type parameter.  Unless ANY_TYPE was passed, an error
 * is reported if the wire type of the field is not equal to the wire type specified.
 * If ANY_TYPE was passed, then this check is not performed.  In case of a mismatch,
 * an error is reported and NULL is returned, otherwise the matching field is returned.
 *
 * @param prev  The field immediately before the first field to be examined.
 * If dir is FORWARD_DIR, then this will be the field immediately preceding the first
 * field to be examined, and if dir is BACKWARD_DIR, then this will be the field
 * immediately following the first field to be examined.
 * @param fnum  Field number to look for.  Unless ANY_FIELD is passed, fields that do
 * not have this number are skipped over.  If ANY_FIELD is passed, then no fields are
 * skipped.
 * @type type  Wire type expected for a matching field.  If the first field encountered
 * with the specified number does not match this type, then an error is reported.
 * The special value ANY_TYPE matches any wire type, disabling this error check.
 * @dir  Direction in which to traverse the fields.  If dir is FORWARD_DIR, then traversal
 * is in the forward direction and if dir is BACKWARD_DIR, then traversal is in the
 * backward direction.
 * @return  The first matching field, or NULL if no matching fields are found, or the
 * first field that matches the specified field number does not match the specified
 * wire type.
 */

PB_Field *PB_next_field(PB_Field *prev, int fnum, PB_WireType type, PB_Direction dir) {
    if (prev == NULL) {
        return NULL;
    }
    
    PB_Field *current;
    
    // Determine starting field based on direction
    if (dir == 0) {  // Assume 0 = FORWARD
        current = prev->next;
    } else {  // Assume non-zero = BACKWARD  
        current = prev->prev;
    }
    
    // Traverse until we come back to prev
    while (current != prev) {
        // Only examine non-sentinel fields
        if (current != NULL && current->type != SENTINEL_TYPE) {
            // Check field number match
            if (fnum == ANY_FIELD || current->number == fnum) {
                // Check wire type match
                if (type == ANY_TYPE || current->type == type) {
                    return current; // Found a match
                }
            }
        }
        
        // Move to next field
        if (dir == 0) {  // FORWARD
            current = current->next;
        } else {  // BACKWARD
            current = current->prev;
        }
        
        // Safety check
        if (current == NULL) {
            break;
        }
    }
    
    return NULL; // No match found
}
/**
 * @brief Get a single field with a specified number from a PB_Message object.
 * @details  This is a convenience function for use when it is desired to get just
 * a single field with a specified field number from a PB_Message, rather than
 * iterating through a sequence of fields.  If there is more than one field having
 * the specified number, then the last such field is returned, as required by
 * the protocol buffers specification.
 *
 * @param msg  The PB_Message object from which to get the field.
 * @param fnum  The field number to get.
 * @param type  The wire type expected for the field, or ANY_TYPE if no particular
 * wire type is expected.
 * @return  A pointer to the field, if a field with the specified number exists
 * in the message, and (unless ANY_TYPE was passed) that the type of the field
 * matches the specified wire type.  If there is no field with the specified number,
 * or the last field in the message with the specified field number does not match
 * the specified wire type, then NULL is returned.
 */

PB_Field *PB_get_field(PB_Message msg, int fnum, PB_WireType type) {
    PB_Field *field = NULL;
    if(msg == NULL ){
        return NULL;
    }
    
    for (PB_Field *current = msg->next; current != msg; current = current->next) {        
        if (current-> number == fnum){
            if (type == ANY_TYPE || current ->type == type){
                field = current;
            }
        }
    }
    return field;
}

/**
 * @brief  Output a human-readable representation of a message field
 * to a specified output stream.
 * @details  This function, which is intended only for debugging purposes,
 * outputs a human-readable representation of the message field object
 * pointed to by fp, to the output stream out.  The output may be in any
 * format deemed useful.
 */


/**
 * @brief  Replace packed fields in a message by their expansions.
 * @detail  This function traverses the fields in a message, looking for fields
 * with a specified field number.  For each such field that is encountered,
 * the content of the field is treated as a "packed" sequence of primitive values.
 * The original field must have wire type LEN_TYPE, otherwise an error is reported.
 * The content is unpacked to produce a list of normal (unpacked) fields,
 * each of which has the specified wire type, which must be a primitive type
 * (i.e. not LEN_TYPE) and the specified field number.
 * The message is then modified by splicing in the expanded list in place of
 * the original packed field.
 *
 * @param msg  The message whose fields are to be expanded.
 * @param fnum  The field number of the fields to be expanded.
 * @param type  The wire type expected for the expanded fields.
 * @return 0 in case of success, -1 in case of an error.
 * @modifies  the original message in case any fields are expanded.
 */

int PB_expand_packed_fields(PB_Message msg, int fnum, PB_WireType type) {
    if (msg == NULL || type == LEN_TYPE) {
        return -1;
    }
    
    PB_Field *current = msg->next;
    
    while (current != msg) {
        PB_Field *next = current->next; // Save next before potential modification
        
        // Look for packed fields with matching field number and LEN_TYPE
        if (current->number == fnum && current->type == LEN_TYPE) {
            // This is a packed field we need to expand
            char *data = current->value.bytes.buf;
            size_t size = current->value.bytes.size;
            
            if (data == NULL || size == 0) {
                // Remove empty packed field
                current->prev->next = current->next;
                current->next->prev = current->prev;
                free(current);
                current = next;
                continue;
            }
            
            // Create memory stream to read packed data
            FILE *packed_stream = fmemopen(data, size, "rb");
            if (packed_stream == NULL) {
                return -1;
            }
            
            // Read individual values and create new fields
            PB_Field *insert_after = current->prev;
            size_t bytes_read = 0;
            
            while (bytes_read < size) {
                PB_Field *new_field = calloc(1, sizeof(PB_Field));
                if (new_field == NULL) {
                    fclose(packed_stream);
                    return -1;
                }
                
                new_field->number = fnum;
                new_field->type = type;
                
                // Read the value based on type
                int value_bytes = PB_read_value(packed_stream, type, &new_field->value);
                if (value_bytes <= 0) {
                    free(new_field);
                    break;
                }
                
                bytes_read += value_bytes;
                
                // Insert new field into the list
                new_field->next = insert_after->next;
                new_field->prev = insert_after;
                insert_after->next->prev = new_field;
                insert_after->next = new_field;
                
                insert_after = new_field;
            }
            
            fclose(packed_stream);
            
            // Remove the original packed field
            current->prev->next = current->next;
            current->next->prev = current->prev;
            free(current->value.bytes.buf);
            free(current);
        }
        
        current = next;
    }
    
    return 0;
}

void PB_show_field(PB_Field *fp, FILE *out) {
   
}

/**
 * @brief  Output a human-readable representation of a message object
 * to a specified output stream.
 * @details  This function, which is intended only for debugging purposes,
 * outputs a human-readable representation of the message object msg to
 * the output stream out.  The output may be in any format deemed useful.
 */

void PB_show_message(PB_Message msg, FILE *out) {
   if (msg == NULL) {
        perror("NULL Message");
        return;
    }
    PB_Field *current = msg->next;
    // fprintf(out, "Message {\n");

    while (current != msg){
        char * buffer = current->value.bytes.buf; 
        if (current->number != 0) {
            fprintf(out, "PB_Field #%d ", current->number);
            fflush(out);
        } 
       
        switch (current->type){
            case(VARINT_TYPE):
            {
                // fprintf(out, "[type: varint, ");
                // fprintf(out, "value: %zu]", current->value.bytes.size);
                fflush(out);
                break;
            }
            case (LEN_TYPE):
            {
                fprintf(out, "[type: len, ");
                fprintf(out, "size: %zu, ", current->value.bytes.size); 
                fprintf(out, "content: "); 
                fflush(out);
                if (current->value.bytes.buf != NULL) {
                    if (current->number != 1){
                        for (int i = 0;i<10;i++){
                            fprintf(out, "%02x", (unsigned char)buffer[i]);

                        }
                        break;

                    }
                for (size_t i = 0; i <current->value.bytes.size; i++) {
                        // if (i ==9){
                        //     fprintf(out, "... ");
                        //     break;
                        // }
                        if(current->number==1){
                        fprintf(out, "%c", (unsigned char)buffer[i]);

                        }
                    }
                    fprintf(out, "]"); 
                    fflush(out);
                // if (current->number == 1){
                //     for (size_t i = 0; i < current->value.bytes.size; i++) {
                //         fprintf(out, "%c", (unsigned char)buffer[i]);
                //     }
                //     fprintf(out, "]"); 
                //     fflush(out);
                // }else{
                //     for (size_t i = 0; i < current->value.bytes.size; i++) {
                //         if (i ==9){
                //             fprintf(out, "... ");
                //             break;
                //         }
                //         fprintf(out, "%02x ", (unsigned char)buffer[i]);
                //     }
                //     fprintf(out, "]"); 
                //     fflush(out);
                //     }
                // }
                }
            }
                break;
            case(I32_TYPE):
            {
                fprintf(out, "[type: i32 ");
                fprintf(out, "Size: %zu,", current->value.bytes.size); 
                fprintf(out, "Content:"); 

                for (size_t i = 0; i < sizeof(uint32_t); i++) {
                    fprintf(out, "%c", buffer[i]);  
                }
                break;
            }
            case(I64_TYPE):
            {
                fprintf(out, "type: i64");
                fprintf(out, "Size: %zu,", current->value.bytes.size); 
                fprintf(out, "Content:"); 
                uint64_t value = current->value.i64;  
                uint8_t *bytes = (uint8_t *)&value;  

                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    fprintf(out, "%c ", (char)bytes[i]);  
                }
                break;
            }
            default:
            {
                fprintf(out, "[type: %d",current->type);
                fprintf(out, "size: %zu, ", current->value.bytes.size); 
                fprintf(out, "content: "); 
                for (size_t i = 0; i < current->value.bytes.size; i++) {
                    // printf("%02x ", (unsigned char)buffer[i]); 
                    fprintf(out, "%c", (char)buffer[i]);
                }
                break;
            }
        }
        current = current->next;

        fprintf(out, "\n");
    }
        fprintf(out, "}\n");
}
