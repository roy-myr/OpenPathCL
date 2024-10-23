#ifndef COMMON_H
#define COMMON_H

// Define a struct to store the Nodes
typedef struct {
    int index;  // Index in the graph matrix
    long long id;  // Node ID
    double lat;  // Latitude of the Node
    double lon;  // Longitude of the Node
} Node;

// Define a struct to store the Roads
typedef struct {
    long long id;
    int nodeCount;
    long long *nodes;
} Road;

// Construct a linked list that resales a path through the graph
typedef struct PathNode {
    Node data;  // Data of the node
    struct PathNode *next; // Pointer to the next node in the list
} PathNode;

// OpenPathCL functions that are the same in both the serial and parallel implementations
void get_data(float latitude, float longitude);
void display_results();

// Functions for the path
PathNode* createPathNode(const Node data);
void appendToNodePath(PathNode** head, const Node data);
void printNodePath(PathNode* head);
void freeNodePath(PathNode* head);
void displayPathOnMap(PathNode* nodePath);

// Functions for the data import
int parseArguments(int argc, char* argv[], double start[2], double dest[2], double** bbox, int* bbox_size);
long long getClosestNode(const double* point);
void getRoadNodes(
    const double* bbox,
    const int bbox_size,
    Node** nodes,
    int* nodeCount,
    Road** roads,
    int* roadCount);

#endif //COMMON_H
