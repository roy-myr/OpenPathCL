#ifndef PARALLEL_UTILS_H
#define PARALLEL_UTILS_H

void convert_to_device_arrays(
    const Node* nodes,
    const int nodeCount,
    int* edges_start,
    int** edge_destinations,
    float** edge_weights,
    int* edge_count);

#endif //PARALLEL_UTILS_H