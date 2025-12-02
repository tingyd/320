#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "global.h"
#include "osm.h"
#include "debug.h"

/* Variable to be set by process_args if the '-h' flag is seen. */
int help_requested = 0;

/* Variable to be set by process_args to any filename specified with '-f'. */
char *osm_input_file = NULL;

/**
 * @brief  Validate command-line arguments with possible simultaneous execution
 * of queries against a map.
 * @details  This function traverses the command-line arguments specified by
 * argc and argv and verifies that they represent a valid invocation of the
 * program.  In addition, this function determines whether '-h' has been given
 * as the first argument and, if so, sets the global variable help_requested
 * to a nonzero value.  It also checks whether there is an occurrence of
 * '-f filename' and, if so, sets the global variable osm_input_file to the
 * specified filename.
 * @param argc  Argument count, as passed to main.
 * @param argv  Argument vector, as passed to main.
 * @param mp  If non-NULL, this is a pointer to a map to be used for processing
 * the queries specified by the option arguments.  If NULL, then only argument
 * validation is performed and no query processing is done.
 * @return 0  if the arguments are valid and, if mp was non-NULL, then there were
 * no errors in processing they specified.  If the arguments are invalid, or
 * if there were errors in processing the queries, then -1 is returned.
 */

int process_args(int argc, char **argv, OSM_Map *mp) {
    static int f_count = 0;  // Track -f occurrences
    if (argc <= 1) {
        return -1;
    }
        
    // First invocation - validation only
    if (mp == NULL) {
        
        // Check for -h flag
        if (strcmp(argv[1], "-h") == 0) {
            help_requested = 1;
            return 0;
        }
        
        // Validate arguments
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-f") == 0) {
                if (f_count > 0) {
                    return -1;  // Multiple -f not allowed
                }
                if (i + 1 >= argc || argv[i + 1][0] == '-') {
                    return -1;  // Missing filename
                }
                osm_input_file = argv[i + 1];
                f_count++;
                i++;  // Skip filename
            }
            else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-b") == 0) {
                // -s and -b should not have non-option arguments
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    return -1;
                }
            }
            else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-w") == 0) {
                // -n and -w must have an ID argument
                if (i + 1 >= argc || argv[i + 1][0] == '-') {
                    return -1;
                }
                i++;  // Skip the ID
                
                // For -w, skip any additional key arguments
                if (strcmp(argv[i - 1], "-w") == 0) {
                    while (i + 1 < argc && argv[i + 1][0] != '-') {
                        i++;
                    }
                }
            }
            else if (argv[i][0] == '-') {
                // Unknown option
                return -1;
            }
            // Non-option arguments are only valid after -f, -n, or -w
        }
        return 0;
    }
    
    
    // Second invocation - process queries
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            printf("nodes: %d, ways: %d\n", 
                   OSM_Map_get_num_nodes(mp), 
                   OSM_Map_get_num_ways(mp));
        }
        else if (strcmp(argv[i], "-b") == 0) {
            OSM_BBox *bbox = OSM_Map_get_BBox(mp);
            if (bbox) {
                printf("min_lon: %.9f, max_lon: %.9f, max_lat: %.9f, min_lat: %.9f\n",
                    OSM_BBox_get_min_lon(bbox) / 1e9, 
                    OSM_BBox_get_max_lon(bbox) / 1e9,
                    OSM_BBox_get_max_lat(bbox) / 1e9, 
                    OSM_BBox_get_min_lat(bbox) / 1e9);
            }
        }
        else if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                OSM_Id node_id = atoll(argv[i + 1]);
                // Find node with this ID
                for (int j = 0; j < OSM_Map_get_num_nodes(mp); j++) {
                    OSM_Node *node = OSM_Map_get_Node(mp, j);
                    if (node && OSM_Node_get_id(node) == node_id) {
                        printf("%ld\t%.9f\t%.9f\n", 
                               node_id, 
                               OSM_Node_get_lat(node) / 1e9,
                               OSM_Node_get_lon(node) / 1e9);
                        break;
                    }
                }
                i++;  // Skip ID
            }
        }
        else if (strcmp(argv[i], "-w") == 0) {
            if (i + 1 < argc) {
                OSM_Id way_id = atoll(argv[i + 1]);
                i++;  // Skip ID
                
                // Collect any key arguments
                int key_start = i + 1;
                int key_count = 0;
                while (i + 1 < argc && argv[i + 1][0] != '-') {
                    i++;
                    key_count++;
                }
                
                // Find way with this ID
                for (int j = 0; j < OSM_Map_get_num_ways(mp); j++) {
                    OSM_Way *way = OSM_Map_get_Way(mp, j);
                    if (way && OSM_Way_get_id(way) == way_id) {
                        printf("%ld\t", way_id);
                        
                        if (key_count == 0) {
                            // Print node references
                            for (int k = 0; k < OSM_Way_get_num_refs(way); k++) {
                                if (k > 0) printf(" ");
                                printf("%ld", OSM_Way_get_ref(way, k));
                            }
                        } else {
                            // Print values for specified keys
                            int first = 1;
                            for (int k = key_start; k < key_start + key_count; k++) {
                                for (int m = 0; m < OSM_Way_get_num_keys(way); m++) {
                                    if (strcmp(argv[k], OSM_Way_get_key(way, m)) == 0) {
                                        if (!first) printf(" ");
                                        printf("%s", OSM_Way_get_value(way, m));
                                        first = 0;
                                        break;
                                    }
                                }
                            }
                        }
                        printf("\n");
                        break;
                    }
                }
            }
        }
        else if (strcmp(argv[i], "-f") == 0) {
            i++;  // Skip filename
        }
        // Handle -n and -w options...
    }
    return 0;
}