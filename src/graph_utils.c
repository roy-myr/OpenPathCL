#include "graph_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "haversine.h"

// Function to fill the graph with the data from the roads
void createGraph(Node* nodes, const int nodeCount, const Road* roads, const int roadCount) {
    // Iterate through each road
    for (int i = 0; i < roadCount; i++) {

        // Go through each pair of consecutive nodes in the road
        for (int j = 0; j < roads[i].nodeCount - 1; j++) {
            const int64_t nodeId1 = roads[i].nodes[j];
            const int64_t nodeId2 = roads[i].nodes[j + 1];

            // Find the indexes of nodeId1 and nodeId2 in the nodes array
            int index1 = -1, index2 = -1;
            for (int k = 0; k < nodeCount; k++) {
                if (nodes[k].id == nodeId1) index1 = k;
                if (nodes[k].id == nodeId2) index2 = k;
                if (index1 != -1 && index2 != -1) break;
            }

            // If both nodes are found, calculate the distance between them
            if (index1 != -1 && index2 != -1) {
                const float distance = haversine(nodes[index1].lat, nodes[index1].lon,
                                                  nodes[index2].lat, nodes[index2].lon);

                // Add edge from index1 to index2
                Edge* newEdge1 = malloc(sizeof(Edge));
                if (newEdge1 != NULL) {
                    newEdge1->destination = index2;
                    newEdge1->weight = distance;
                    newEdge1->next = nodes[index1].head;
                    nodes[index1].head = newEdge1;
                } else {
                    fprintf(stderr, "Failed to allocate memory for edge from %ld to %ld\n", nodeId1, nodeId2);
                }

                // Add edge from index2 to index1 (for undirected graph)
                Edge* newEdge2 = malloc(sizeof(Edge));
                if (newEdge2 != NULL) {
                    newEdge2->destination = index1;
                    newEdge2->weight = distance;
                    newEdge2->next = nodes[index2].head;
                    nodes[index2].head = newEdge2;
                } else {
                    fprintf(stderr, "Failed to allocate memory for edge from %ld to %ld\n", nodeId2, nodeId1);
                }
            } else {
                fprintf(stderr, "Failed to find nodes with IDs %ld and/or %ld in nodes array\n", nodeId1, nodeId2);
            }
        }
    }
}


// Function to free the nodes memory
void freeNodes(Node* nodes, const int nodeCount) {
    for (int i = 0; i < nodeCount; i++) {
        Edge* current = nodes[i].head;
        while (current != NULL) {
            Edge* temp = current;
            current = current->next;
            free(temp); // Free each edge in the linked list
        }
        nodes[i].head = NULL; // Set head to NULL after freeing
    }
    free(nodes); // Finally, free the array of nodes itself
}

// Debug Print to retrieve Nodes
void printNodes(const Node* nodes, const int nodeCount) {
    printf("Nodes:\n");
    for (int i = 0; i < nodeCount; i++) {
        printf("Node %d:\n", i + 1);
        printf("  ID: %ld\n", nodes[i].id);
        printf("  Latitude: %f\n", nodes[i].lat);
        printf("  Longitude: %f\n", nodes[i].lon);
        // Print more details if needed, such as `head` if it’s used
    }
    printf("Total Nodes: %d\n\n", nodeCount);
}

// Debug Print to retrieve Roads
void printRoads(const Road* roads, const int roadCount) {
    printf("Roads:\n");
    for (int i = 0; i < roadCount; i++) {
        printf("Road %d:\n", i + 1);
        printf("  ID: %ld\n", roads[i].id);
        printf("  Node Count: %d\n", roads[i].nodeCount);
        printf("  Node IDs: ");
        for (int j = 0; j < roads[i].nodeCount; j++) {
            printf("%ld ", roads[i].nodes[j]);
        }
        printf("\n");
    }
    printf("Total Roads: %d\n\n", roadCount);
}

void printGraph(const Node* nodes, const int nodeCount) {
    printf("Graph:\n");
    for (int i = 0; i < nodeCount; i++) {
        printf("Node ID: %ld (Index: %d)\n", nodes[i].id, i);
        printf("  Location: (Lat: %f, Lon: %f)\n", nodes[i].lat, nodes[i].lon);

        // Print all edges connected to this node
        Edge* edge = nodes[i].head;
        if (edge == NULL) {
            printf("  No edges connected to this node.\n");
        } else {
            printf("  Edges:\n");
            while (edge != NULL) {
                printf("    -> Destination Node Index: %d, Weight: %.2f\n", edge->destination, edge->weight);
                edge = edge->next;
            }
        }
        printf("\n");
    }
}

void writeGraphToMermaidFile(const Node* nodes, const int nodeCount) {
    FILE* file = fopen("graph.md", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open graph.md for writing.\n");
        return;
    }

    // Write the Mermaid header for a graph
    fprintf(file, "```mermaid\ngraph TD\n");

    // Write each node and its edges
    for (int i = 0; i < nodeCount; i++) {
        fprintf(file, "    %ld[\"Node %ld (%d)<br/>(%.6f, %.6f)\"]\n",
                nodes[i].id, nodes[i].id, i, nodes[i].lat, nodes[i].lon);

        // Process each edge connected to the node
        Edge* edge = nodes[i].head;
        while (edge != NULL) {
            // Write the edge with weight as a label
            fprintf(file, "    %ld -->|%.2fm| %ld\n", nodes[i].id, edge->weight, nodes[edge->destination].id);
            edge = edge->next;
        }
    }

    // Close the Mermaid code block
    fprintf(file, "```\n");
    fclose(file);
}