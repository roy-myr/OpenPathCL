#include <stdio.h>
#include <stdlib.h>

#include "graph_utils.h"

#include "parallel_utils.h"  // For Node struct

void convert_to_device_arrays(
    const Node* nodes,
    const int nodeCount,
    int* edges_start,
    int** edge_destinations,
    float** edge_weights,
    int* edge_count) {

    *edge_count = 0; // Start with zero edges

    // Initial allocation for edges_destination and edges_weight
    int edge_capacity = nodeCount * 2; // Initial estimated capacity, adjust as needed
    *edge_destinations = malloc(edge_capacity * sizeof(int));
    *edge_weights = malloc(edge_capacity * sizeof(float));

    if (*edge_destinations == NULL || *edge_weights == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for edges.\n");
        exit(EXIT_FAILURE);
    }

    // Populate edges and update edge count
    for (int i = 0; i < nodeCount; i++) {
        edges_start[i] = *edge_count; // Starting index for edges of node `i`

        const Edge* edge = nodes[i].head; // Access the linked list of edges for this node
        while (edge) {
            // Expand capacity if necessary
            if (*edge_count >= edge_capacity) {
                edge_capacity *= 2;
                *edge_destinations = realloc(*edge_destinations, edge_capacity * sizeof(int));
                *edge_weights = realloc(*edge_weights, edge_capacity * sizeof(float));

                if (*edge_destinations == NULL || *edge_weights == NULL) {
                    fprintf(stderr, "Error: Unable to reallocate memory for edges.\n");
                    exit(EXIT_FAILURE);
                }
            }

            // Add destination and weight to separate arrays
            (*edge_destinations)[*edge_count] = edge->destination;
            (*edge_weights)[*edge_count] = edge->weight;
            (*edge_count)++;

            edge = edge->next; // Move to the next edge in the linked list
        }
    }

    // Shrink arrays to fit exact edge count
    *edge_destinations = realloc(*edge_destinations, *edge_count * sizeof(int));
    *edge_weights = realloc(*edge_weights, *edge_count * sizeof(float));

    if ((*edge_destinations == NULL || *edge_weights == NULL) && *edge_count > 0) {
        fprintf(stderr, "Error: Unable to reallocate memory to fit exact edge count.\n");
        exit(EXIT_FAILURE);
    }
}