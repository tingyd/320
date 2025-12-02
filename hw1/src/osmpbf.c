#include <stdio.h>
#include <stdint.h>

#include "global.h"
#include "protobuf.h"
#include "osm.h"
#include "debug.h"
#include "string.h"

struct OSM_Map
{
    OSM_BBox *bbox;
    int num_nodes;
    int num_ways;
    int num_relations;
    OSM_Node **nodes;
    OSM_Way **ways;
    char **string_table;
    int num_strings;
};

struct OSM_BBox
{
    OSM_Lon min_lon;
    OSM_Lon max_lon;
    OSM_Lat max_lat;
    OSM_Lat min_lat;
};

struct OSM_Node
{
    OSM_Id id;
    OSM_Lat lat;
    OSM_Lon lon;
};

struct OSM_Way
{
    OSM_Id id;
    int num_refs;
    int num_keys;
    OSM_Id *refs;
    char **keys;
    char **values;
};
uint32_t decode_varint_uint32(char *buf, size_t *pos, size_t len) {
    uint32_t result = 0;
    int shift = 0;
    
    while (*pos < len && shift < 32) {
        uint8_t byte = buf[*pos];
        (*pos)++;
        result |= (uint32_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

int64_t decode_zigzag_varint(char *buf, size_t *pos, size_t len) {
    uint64_t raw = 0;
    int shift = 0;
    
    while (*pos < len && shift < 64) {
        uint8_t byte = buf[*pos];
        (*pos)++;
        raw |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    
    // Apply zig-zag decoding
    return (raw % 2 == 0) ? (int64_t)(raw / 2) : -((int64_t)(raw + 1) / 2);
}

/**
 * @brief Read map data in OSM PBF format from the specified input stream,
 * construct and return a corresponding OSM_Map object.  Storage required
 * for the map object and any related entities is allocated on the heap.
 * @param in  The input stream to read.
 * @return  If reading was successful, a pointer to the OSM_Map object constructed
 * from the input, otherwise NULL in case of any error.
 */

OSM_Map *OSM_read_Map(FILE *in)
{
    if (in == NULL) {
        return NULL;
    }
    
    OSM_Map *map = malloc(sizeof(OSM_Map));
    if (!map) {
        return NULL;
    }
    
    // Initialize map
    map->bbox = NULL;
    map->num_nodes = 0;
    map->num_ways = 0;
    map->num_relations = 0;
    map->nodes = NULL;
    map->ways = NULL;
    map->string_table = NULL;
    map->num_strings = 0;

    uint32_t length = 0;
    
    while (1) {
        // Read 4-byte length prefix
        size_t bytesRead = fread(&length, 1, sizeof(length), in);
        
        if (bytesRead == 0) {
            if (feof(in)) break;
            if (ferror(in)) return NULL;
            break;
        }
        
        if (bytesRead != sizeof(length)) return NULL;
        
        // Convert from big-endian to host byte order
        length = __builtin_bswap32(length);
        
        // Sanity check the length
        if (length == 0 || length > 1000000) return NULL;
        
        // Read blob header
        PB_Message blobHeader;
        int header_result = PB_read_message(in, length, &blobHeader);
        if (header_result < 0) return NULL;
        
        // Get blob type and size
        PB_Field *type_field = PB_get_field(blobHeader, 1, LEN_TYPE);
        PB_Field *size_field = PB_get_field(blobHeader, 3, VARINT_TYPE);
        if (!type_field || !size_field) continue;
        
        // Extract blob type
        char type_str[100] = {0};
        size_t type_len = type_field->value.bytes.size;
        if (type_len < sizeof(type_str)) {
            memcpy(type_str, type_field->value.bytes.buf, type_len);
            type_str[type_len] = '\0';
        }
        
        uint32_t blob_size = (uint32_t)size_field->value.i64;
        
        // Read blob data
        PB_Message blob;
        int blob_result = PB_read_message(in, blob_size, &blob);
        if (blob_result < 0) return NULL;
        
        // Process blob based on type
        if (strcmp(type_str, "OSMHeader") == 0) {
            // Look for compressed data field
            PB_Field *zlib_field = PB_get_field(blob, 3, LEN_TYPE);
            PB_Field *raw_field = PB_get_field(blob, 1, LEN_TYPE);
            
            PB_Message header_msg;
            int decompress_result = -1;
            
            if (zlib_field) {
                decompress_result = PB_inflate_embedded_message(zlib_field->value.bytes.buf, 
                                                              zlib_field->value.bytes.size, 
                                                              &header_msg);
            } else if (raw_field) {
                decompress_result = PB_read_embedded_message(raw_field->value.bytes.buf,
                                                           raw_field->value.bytes.size,
                                                           &header_msg);
            }
            
            if (decompress_result == 0) {
                // Look for bounding box (field #1)  
                PB_Field *bbox_field = PB_get_field(header_msg, 1, LEN_TYPE);
                if (!bbox_field) {
                    bbox_field = PB_get_field(header_msg, 1, ANY_TYPE);
                }
                
                if (bbox_field) {
                    PB_Message bbox_msg;
                    int bbox_read_result = PB_read_embedded_message(bbox_field->value.bytes.buf,
                                                                   bbox_field->value.bytes.size,
                                                                   &bbox_msg);
                    
                    if (bbox_read_result == 0) {
                        // Get specific bbox fields
                        PB_Field *min_lon_field = PB_get_field(bbox_msg, 1, VARINT_TYPE);
                        PB_Field *max_lon_field = PB_get_field(bbox_msg, 2, VARINT_TYPE);
                        PB_Field *max_lat_field = PB_get_field(bbox_msg, 3, VARINT_TYPE);
                        PB_Field *min_lat_field = PB_get_field(bbox_msg, 4, VARINT_TYPE);
                        
                        if (min_lon_field && max_lon_field && max_lat_field && min_lat_field) {
                            // Get raw values
                            uint64_t min_lon_raw = min_lon_field->value.i64;
                            uint64_t max_lon_raw = max_lon_field->value.i64;
                            uint64_t max_lat_raw = max_lat_field->value.i64;
                            uint64_t min_lat_raw = min_lat_field->value.i64;
                            
                            // Allocate bbox
                            map->bbox = malloc(sizeof(OSM_BBox));
                            if (map->bbox) {
                                map->bbox->min_lon = (min_lon_raw % 2 == 0) ? min_lon_raw / 2 : -((min_lon_raw + 1) / 2);
                                map->bbox->max_lon = (max_lon_raw % 2 == 0) ? max_lon_raw / 2 : -((max_lon_raw + 1) / 2);
                                map->bbox->max_lat = (max_lat_raw % 2 == 0) ? max_lat_raw / 2 : -((max_lat_raw + 1) / 2);
                                map->bbox->min_lat = (min_lat_raw % 2 == 0) ? min_lat_raw / 2 : -((min_lat_raw + 1) / 2);
                            }
                        }
                    }
                }
            }
        } else if (strcmp(type_str, "OSMData") == 0) {
            PB_Field *zlib_field = PB_get_field(blob, 3, LEN_TYPE);
            if (!zlib_field) continue;
            
            PB_Message primitive_block;
            if (PB_inflate_embedded_message(zlib_field->value.bytes.buf, 
                                          zlib_field->value.bytes.size, 
                                          &primitive_block) != 0) {
                continue;
            }
            
            // STEP 1: Parse string table (field #1)
            char **string_table = NULL;
            int num_strings = 0;
            
            PB_Field *string_table_field = PB_get_field(primitive_block, 1, LEN_TYPE);
            if (string_table_field) {
                PB_Message string_table_msg;
                if (PB_read_embedded_message(string_table_field->value.bytes.buf,
                                           string_table_field->value.bytes.size,
                                           &string_table_msg) == 0) {
                    
                    // Count strings
                    PB_Field *str_field = string_table_msg->next;
                    while (str_field != string_table_msg) {
                        if (str_field->number == 1) num_strings++;
                        str_field = str_field->next;
                    }
                    
                    if (num_strings > 0) {
                        string_table = malloc(num_strings * sizeof(char*));
                        int str_idx = 0;
                        
                        // Extract strings
                        str_field = string_table_msg->next;
                        while (str_field != string_table_msg && str_idx < num_strings) {
                            if (str_field->number == 1 && str_field->type == LEN_TYPE) {
                                size_t str_len = str_field->value.bytes.size;
                                string_table[str_idx] = malloc(str_len + 1);
                                memcpy(string_table[str_idx], str_field->value.bytes.buf, str_len);
                                string_table[str_idx][str_len] = '\0';
                                str_idx++;
                            }
                            str_field = str_field->next;
                        }
                    }
                }
            }
            
            // STEP 2: Allocate storage arrays
            if (map->nodes == NULL) {
                map->nodes = malloc(100000 * sizeof(OSM_Node*));
            }
            if (map->ways == NULL) {
                map->ways = malloc(20000 * sizeof(OSM_Way*));
            }
            
            // STEP 3: Process primitive groups (field #2)
            PB_Field *current = primitive_block->next;
            while (current != primitive_block) {
                if (current->number == 2 && current->type == LEN_TYPE) {
                    PB_Message primitive_group;
                    if (PB_read_embedded_message(current->value.bytes.buf, 
                                               current->value.bytes.size, 
                                               &primitive_group) == 0) {
                        
                        // Process entities in this primitive group
                        PB_Field *group_field = primitive_group->next;
                        while (group_field != primitive_group) {
                            
                            if (group_field->number == 2 && group_field->type == LEN_TYPE) {
    // Dense nodes - ACTUALLY PARSE THEM
    PB_Message dense_nodes;
    if (PB_read_embedded_message(group_field->value.bytes.buf,
                               group_field->value.bytes.size,
                               &dense_nodes) == 0) {
        
        PB_Field *id_field = PB_get_field(dense_nodes, 1, LEN_TYPE);
        PB_Field *lat_field = PB_get_field(dense_nodes, 8, LEN_TYPE);  
        PB_Field *lon_field = PB_get_field(dense_nodes, 9, LEN_TYPE);
        
        if (id_field && lat_field && lon_field) {
            // Count how many nodes we have first
            int node_count = 0;
            size_t pos = 0;
            while (pos < id_field->value.bytes.size) {
                decode_zigzag_varint(id_field->value.bytes.buf, &pos, id_field->value.bytes.size);
                node_count++;
            }
            
            if (node_count > 0) {
                // Make sure we have space in the nodes array
                if (map->num_nodes + node_count > 100000) {
                    // Reallocate if needed
                    map->nodes = realloc(map->nodes, (map->num_nodes + node_count) * sizeof(OSM_Node*));
                }
                
                // Parse node IDs (delta-encoded, zig-zag)
                int64_t *node_ids = malloc(node_count * sizeof(int64_t));
                pos = 0;
                int64_t running_id = 0;
                for (int i = 0; i < node_count; i++) {
                    int64_t delta = decode_zigzag_varint(id_field->value.bytes.buf, &pos, id_field->value.bytes.size);
                    running_id += delta;
                    node_ids[i] = running_id;
                }
                
                // Parse latitudes (delta-encoded, zig-zag)
                int64_t *lats = malloc(node_count * sizeof(int64_t));
                pos = 0;
                int64_t running_lat = 0;
                for (int i = 0; i < node_count; i++) {
                    int64_t delta = decode_zigzag_varint(lat_field->value.bytes.buf, &pos, lat_field->value.bytes.size);
                    running_lat += delta;
                    lats[i] = running_lat *100;
                }
                
                // Parse longitudes (delta-encoded, zig-zag)
                int64_t *lons = malloc(node_count * sizeof(int64_t));
                pos = 0;
                int64_t running_lon = 0;
                for (int i = 0; i < node_count; i++) {
                    int64_t delta = decode_zigzag_varint(lon_field->value.bytes.buf, &pos, lon_field->value.bytes.size);
                    running_lon += delta;
                    lons[i] = running_lon*100;
                }
                
                // Create OSM_Node objects and store them
                for (int i = 0; i < node_count; i++) {
                    OSM_Node *node = malloc(sizeof(OSM_Node));
                    if (node) {
                        node->id = node_ids[i];
                        node->lat = lats[i];
                        node->lon = lons[i];
                        
                        // Store in the map's nodes array
                        map->nodes[map->num_nodes] = node;
                        map->num_nodes++;
                    }
                }
                
                // Clean up temporary arrays
                free(node_ids);
                free(lats);
                free(lons);
            }
        }
    }
}
                            else if (group_field->number == 3 && group_field->type == LEN_TYPE) {
                                // Individual Way - COMPLETE PROCESSING
                                if (map->num_ways >= 20000) {
                                    group_field = group_field->next;
                                    continue;
                                }
                                
                                PB_Message way_msg;
                                if (PB_read_embedded_message(group_field->value.bytes.buf,
                                                           group_field->value.bytes.size,
                                                           &way_msg) == 0) {
                                    
                                    // Create new way object
                                    OSM_Way *way = calloc(1, sizeof(OSM_Way));
                                    if (!way) {
                                        group_field = group_field->next;
                                        continue;
                                    }
                                    
                                    // Parse all fields in the way message
                                    PB_Field *way_field = way_msg->next;
                                    while (way_field != way_msg) {
                                        
                                        if (way_field->number == 1 && way_field->type == VARINT_TYPE) {
                                            // Way ID (field #1) - NOT zig-zag encoded
                                            way->id = way_field->value.i64;
                                            
                                        } else if (way_field->number == 2 && way_field->type == LEN_TYPE) {
                                            // Keys (field #2) - packed array of string indices
                                            if (way_field->value.bytes.buf && way_field->value.bytes.size > 0 && string_table) {
                                                // Count keys
                                                int key_count = 0;
                                                size_t pos = 0;
                                                while (pos < way_field->value.bytes.size) {
                                                    decode_varint_uint32(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                    key_count++;
                                                }
                                                
                                                way->num_keys = key_count;
                                                
                                                if (key_count > 0) {
                                                    way->keys = malloc(key_count * sizeof(char*));
                                                    
                                                    // Extract key strings
                                                    pos = 0;
                                                    for (int k = 0; k < key_count; k++) {
                                                        uint32_t str_idx = decode_varint_uint32(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                        if (str_idx < num_strings) {
                                                            way->keys[k] = malloc(strlen(string_table[str_idx]) + 1);
                                                            strcpy(way->keys[k], string_table[str_idx]);
                                                        } else {
                                                            way->keys[k] = malloc(8);
                                                            strcpy(way->keys[k], "unknown");
                                                        }
                                                    }
                                                }
                                            }
                                            
                                        } else if (way_field->number == 3 && way_field->type == LEN_TYPE) {
                                            // Values (field #3) - packed array of string indices
                                            if (way_field->value.bytes.buf && way_field->value.bytes.size > 0 && string_table) {
                                                // Count values
                                                int value_count = 0;
                                                size_t pos = 0;
                                                while (pos < way_field->value.bytes.size) {
                                                    decode_varint_uint32(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                    value_count++;
                                                }
                                                
                                                if (value_count == way->num_keys && value_count > 0) {
                                                    way->values = malloc(value_count * sizeof(char*));
                                                    
                                                    // Extract value strings
                                                    pos = 0;
                                                    for (int v = 0; v < value_count; v++) {
                                                        uint32_t str_idx = decode_varint_uint32(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                        if (str_idx < num_strings) {
                                                            way->values[v] = malloc(strlen(string_table[str_idx]) + 1);
                                                            strcpy(way->values[v], string_table[str_idx]);
                                                        } else {
                                                            way->values[v] = malloc(8);
                                                            strcpy(way->values[v], "unknown");
                                                        }
                                                    }
                                                }
                                            }
                                            
                                        } else if (way_field->number == 8 && way_field->type == LEN_TYPE) {
                                            // Node references (field #8) - packed, delta-encoded, zig-zag
                                            if (way_field->value.bytes.buf && way_field->value.bytes.size > 0) {
                                                // Count refs
                                                int ref_count = 0;
                                                size_t pos = 0;
                                                while (pos < way_field->value.bytes.size) {
                                                    decode_zigzag_varint(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                    ref_count++;
                                                }
                                                
                                                way->num_refs = ref_count;
                                                
                                                if (ref_count > 0) {
                                                    way->refs = malloc(ref_count * sizeof(OSM_Id));
                                                    
                                                    // Decode delta-encoded refs
                                                    pos = 0;
                                                    int64_t running_ref = 0;
                                                    
                                                    for (int r = 0; r < ref_count; r++) {
                                                        int64_t delta = decode_zigzag_varint(way_field->value.bytes.buf, &pos, way_field->value.bytes.size);
                                                        running_ref += delta;
                                                        way->refs[r] = running_ref;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        way_field = way_field->next;
                                    }
                                    
                                    // Store the completed way
                                    map->ways[map->num_ways] = way;
                                    map->num_ways++;
                                }
                            }
                            else if (group_field->number == 4 && group_field->type == LEN_TYPE) {
                                // Relations - just count
                                map->num_relations++;
                            }
                            
                            group_field = group_field->next;
                        }
                    }
                }
                current = current->next;
            }
            
            // Clean up string table
            if (string_table) {
                for (int i = 0; i < num_strings; i++) {
                    free(string_table[i]);
                }
                free(string_table);
            }
        }
    }
    
    return map;
}

/**
 * @brief  Get the number of nodes in an OSM_Map object.
 *
 * @param  mp  The map object to query.
 * @return  The number of nodes.
 */

int OSM_Map_get_num_nodes(OSM_Map *mp)
{
    return mp->num_nodes;
}

/**
 * @brief  Get the number of ways in an OSM_Map object.
 *
 * @param  mp  The map object to query.
 * @return  The number of ways.
 */

int OSM_Map_get_num_ways(OSM_Map *mp)
{
    return mp->num_ways;
}

/**
 * @brief  Get the node at the specified index from an OSM_Map object.
 *
 * @param  mp  The map to be queried.
 * @param  index  The index of the node to be retrieved.
 * @param  return  The node at the specifed index, if the index was in
 * the valid range [0, num_nodes), otherwise NULL.
 */

OSM_Node *OSM_Map_get_Node(OSM_Map *mp, int index) {
    if (mp == NULL || index < 0 || index >= mp->num_nodes) {
        return NULL;
    }
    return mp->nodes[index];
}

/**
 * @brief  Get the way at the specified index from an OSM_Map object.
 *
 * @param  mp  The map to be queried.
 * @param  index  The index of the way to be retrieved.
 * @param  return  The way at the specifed index, if the index was in
 * the valid range [0, num_ways), otherwise NULL.
 */

OSM_Way *OSM_Map_get_Way(OSM_Map *mp, int index) {
    if (mp == NULL || index < 0 || index >= mp->num_ways) {
        return NULL;
    }
    return mp->ways[index];
}

/**
 * @brief  Get the bounding box, if any, of the specified OSM_Map object.
 *
 * @param  mp  The map object to be queried.
 * @return  The bounding box of the map object, if it has one, otherwise NULL.
 */

OSM_BBox *OSM_Map_get_BBox(OSM_Map *mp)
{
    if (mp == NULL)
    {
        return NULL;
    }
    return mp->bbox;
}

/**
 * @brief  Get the id of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The id of the node.
 */

int64_t OSM_Node_get_id(OSM_Node *np)
{
    return np->id;
}

/**
 * @brief  Get the latitude of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The latitude of the node, in nanodegrees.
 */

int64_t OSM_Node_get_lat(OSM_Node *np)
{
    return np->lat;
}

/**
 * @brief  Get the longitude of an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The latitude of the node, in nanodegrees.
 */

int64_t OSM_Node_get_lon(OSM_Node *np)
{
    return np->lon;
}

/**
 * @brief  Get the number of keys in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @return  The number of keys (or key/value pairs) in the node.
 */

int OSM_Node_get_num_keys(OSM_Node *np)
{
    // To be implemented
    abort();
}

/**
 * @brief  Get the key at a specified index in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @param index  The index of the key.
 * @return  The key at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The key is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Node_get_key(OSM_Node *np, int index)
{
    // To be implemented
    abort();
}

/**
 * @brief  Get the value at a specified index in an OSM_Node object.
 *
 * @param np  The node object to be queried.
 * @param index  The index of the value.
 * @return  The value at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The value is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Node_get_value(OSM_Node *np, int index)
{
    // To be implemented
    abort();
}

/**
 * @brief  Get the id of an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @return  The id of the way.
 */

int64_t OSM_Way_get_id(OSM_Way *wp){
    return wp->id;
}

/**
 * @brief  Get the number of node references in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @return  The number of node references contained in the way.
 */

int OSM_Way_get_num_refs(OSM_Way *wp){
    return wp->num_refs;
}

/**
 * @brief  Get the node reference at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the node reference.
 * @return  The id of the node referred to at the specified index,
 * if the index is in the valid range [0, num_refs), otherwise NULL.
 */

OSM_Id OSM_Way_get_ref(OSM_Way *wp, int index) {
    if (wp == NULL || index < 0 || index >= wp->num_refs) {
        return -1;  // Invalid reference
    }
    return wp->refs[index];
}

/**
 * @brief  Get the number of keys in an OSM_Way object.
 *
 * @param np  The node object to be queried.
 * @return  The number of keys (or key/value pairs) in the way.
 */

int OSM_Way_get_num_keys(OSM_Way *wp){
    return wp->num_keys;
}

/**
 * @brief  Get the key at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the key.
 * @return  The key at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The key is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Way_get_key(OSM_Way *wp, int index) {
    if (wp == NULL || index < 0 || index >= wp->num_keys) {
        return NULL;
    }
    return wp->keys[index];
}

/**
 * @brief  Get the value at a specified index in an OSM_Way object.
 *
 * @param wp  The way object to be queried.
 * @param index  The index of the value.
 * @return  The value at the specified index, if the index is in the valid range
 * [0, num_keys), otherwise NULL.  The value is returned as a pointer to a
 * null-terminated string.
 */

char *OSM_Way_get_value(OSM_Way *wp, int index) {
    if (wp == NULL || index < 0 || index >= wp->num_keys) {
        return NULL;
    }
    return wp->values[index];
}

/**
 * @brief  Get the minimum longitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the minimum longitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_min_lon(OSM_BBox *bbp){
    return bbp->min_lon;
}

/**
 * @brief  Get the maximum longitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the maximum longitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_max_lon(OSM_BBox *bbp){
    return bbp->max_lon;
}

/**
 * @brief  Get the maximum latitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the maximum latitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_max_lat(OSM_BBox *bbp){
    return bbp->max_lat;
}

/**
 * @brief  Get the minimum latitude coordinate of an OSM_BBox object.
 *
 * @param bbp the bounding box to be queried.
 * @return  the minimum latitude coordinate of the bounding box, in nanodegrees.
 */

int64_t OSM_BBox_get_min_lat(OSM_BBox *bbp){
    return bbp->min_lat;
}
