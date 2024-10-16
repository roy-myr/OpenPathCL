#ifndef COMMON_H
#define COMMON_H

// Define the Node struct
typedef struct {
    int index;  // Index in the graph matrix
    long long id;  // Node ID
    double lat;  // Latitude of the Node
    double lon;  // Longitude of the Node
} Node;

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
void getRoadNodes(const double lat1, const double lon1, const double lat2, const double lon2, const double buffer, Node** nodes, int* nodeCount);

#endif //COMMON_H
