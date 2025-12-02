#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "osm.h"
#include "debug.h"

int main(int argc, char **argv) {
    // First pass: validate arguments
    if (process_args(argc, argv, NULL) != 0) {
        USAGE(argv[0], EXIT_FAILURE);
    }
    
    // Check if help was requested
    if (help_requested) {
        USAGE(argv[0], EXIT_SUCCESS);
    }
    
    // Open input file if specified
    FILE *osm = NULL;
    if (osm_input_file != NULL) {
        osm = fopen(osm_input_file, "rb");  // IMPORTANT: Binary mode!
        if (osm == NULL) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", osm_input_file);
            return EXIT_FAILURE;
        }
    } else {
        // No file specified, read from stdin
        osm = stdin;
    }
    
    // Read the map
    OSM_Map *map = OSM_read_Map(osm);
    
    // Close file if we opened it (don't close stdin)
    if (osm != stdin && osm != NULL) {
        fclose(osm);
    }
    
    if (map == NULL) {
        fprintf(stderr, "Error: Failed to read map\n");
        return EXIT_FAILURE;
    }
    
    // Second pass: process queries
    if (process_args(argc, argv, map) != 0) {
        fprintf(stderr, "Error: Failed to process queries\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
