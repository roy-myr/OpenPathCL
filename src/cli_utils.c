#include <stdio.h>
#include <stdlib.h>

// Function to parse command-line arguments
int parseArguments(const int argc, char* argv[], float start[2], float dest[2], float** bbox, int* bbox_size) {
    // Check for the required number of arguments
    if (argc < 10 || (argc - 6) % 2 != 1) {
        fprintf(stderr, "Invalid Arguments\n "
                        "Usage: start_lat start_lon dest_lat dest_lon bbox_lat1 bbox_lon1 bbox_lat2 bbox_lon2 ...\n");
        return -1; // Indicate an error
    }

    // Parse start point
    start[0] = strtof(argv[1], NULL);  // start latitude
    start[1] = strtof(argv[2], NULL);  // start longitude

    // Parse destination point
    dest[0] = strtof(argv[3], NULL);   // destination latitude
    dest[1] = strtof(argv[4], NULL);   // destination longitude

    // Calculate number of bounding box coordinates
    *bbox_size = argc - 6;  // Remaining arguments are bbox points
    *bbox = (float*)malloc(*bbox_size * sizeof(float)); // Dynamically allocate memory for bbox

    // Check for successful memory allocation
    if (*bbox == NULL) {
        fprintf(stderr, "Memory allocation failed for bounding box.\n");
        return -1; // Indicate an error
    }

    // Parse bounding box coordinates
    for (int i = 0; i < *bbox_size + 1; ++i) {
        (*bbox)[i] = strtof(argv[5 + i], NULL);
    }

    return 0; // Successful parsing
}