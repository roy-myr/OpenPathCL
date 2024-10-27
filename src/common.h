#ifndef COMMON_H
#define COMMON_H

// Define the struct for an edge in the adjacency list
typedef struct Edge {
    int destination;  // Index of the destination node
    float weight;    // Weight of the edge
    struct Edge* next;
} Edge;

// Define a struct to store the Nodes
typedef struct Node {
    int64_t id;  // Node ID
    float lat;  // Latitude of the Node
    float lon;  // Longitude of the Node
    Edge* head;  // Head of the linked list of Edges connected to the Node
} Node;

// Define a struct to store the Roads
typedef struct Road {
    int64_t id;  // Way ID
    int nodeCount;
    int64_t *nodes;  // Array of Node IDs
} Road;

// Construct a linked list that resales a path through the graph
typedef struct PathNode {
    Node data;  // Data of the node
    struct PathNode *next; // Pointer to the next node in the list
} PathNode;

// OpenPathCL functions that are the same in both the serial and parallel implementations

// Functions for the path
PathNode* createPathNode(const Node data);
void appendToNodePath(PathNode** head, const Node data);
void printNodePath(const PathNode* head);
void freeNodePath(PathNode* head);

// Functions for the data import
int parseArguments(int argc, char* argv[], float start[2], float dest[2], float** bbox, int* bbox_size);
long long getClosestNode(const float* point);
void getRoadNodes(
    const float* bbox,
    const int bbox_size,
    Node** nodes,
    int* nodeCount,
    Road** roads,
    int* roadCount);

#endif //COMMON_H
