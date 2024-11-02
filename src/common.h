#ifndef COMMON_H
#define COMMON_H

// Define a struct to hold the response data
struct MemoryStruct {
    char* memory;
    size_t size;
};

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

// Define a struct to manage dynamic buckets used in delta stepping
typedef struct {
    int **buckets;  // Array of integer arrays (each array is a bucket)
    int *bucketSizes;  // Array to store the size (number of nodes) in each bucket
    int numBuckets;  // Number of buckets currently allocated
} BucketsArray;

// OpenPathCL functions that are the same in both the serial and parallel implementations

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

// Bucket functions
void initializeBuckets(BucketsArray *bucketsArray);
void resizeBuckets(BucketsArray *bucketsArray, const int newSize);
void addNodeToBucket(BucketsArray *bucketsArray, const int bucketIndex, const int node_id);
void freeBuckets(BucketsArray *bucketsArray);

// Debug functions
void printNodes(const Node* nodes, const int nodeCount);
void printRoads(const Road* roads, const int roadCount);
void printGraph(const Node* nodes, const int nodeCount);
void writeGraphToMermaidFile(const Node* nodes, const int nodeCount);
void printBucket(const int *bucket, const int size);

#endif //COMMON_H
