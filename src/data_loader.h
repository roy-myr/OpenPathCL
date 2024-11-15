#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "graph_utils.h"  // For Node and Road struct

void parseAndStoreJSON(const char* jsonResponse, Node** nodes, int* nodeCount, Road** roads, int* roadCount);
long long getClosestNode(const float* point);
void getRoadNodes(
    const float* bbox,
    const int bbox_size,
    Node** nodes,
    int* nodeCount,
    Road** roads,
    int* roadCount);

#endif //DATA_LOADER_H