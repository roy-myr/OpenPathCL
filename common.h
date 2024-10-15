#ifndef COMMON_H
#define COMMON_H

// Define the Node struct
typedef struct {
    int index; // Index in the graph matrix
    int id;    // Node ID
    double lat; // Latitude of the Node
    double lon; // Longitude of the Node
} Node;

// OpenPathCL functions that are the same in both the serial and parallel implementations
void get_data(float latitude, float longitude);
void display_results();

// Function for printing used path
void printPath(int prev[], int target_index, Node nodes[]);

#endif //COMMON_H
