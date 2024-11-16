#ifndef GRAPH_UTILS_H
#define GRAPH_UTILS_H
#include <stdint.h>

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

// Graph functions
void createGraph(Node* nodes, const int nodeCount, const Road* roads, const int roadCount);
void freeNodes(Node* nodes, const int nodeCount);

// Debug functions
void printNodes(const Node* nodes, const int nodeCount);
void printRoads(const Road* roads, const int roadCount);
void printGraph(const Node* nodes, const int nodeCount);
void writeGraphToMermaidFile(const Node* nodes, const int nodeCount);

#endif //GRAPH_UTILS_H