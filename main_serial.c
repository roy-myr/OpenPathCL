#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For DBL_MAX

#include "common.h"  // Include the common header

#define V 6 // Number of vertices in the graph

// Function to find the vertex with the minimum distance value
int minDistance(double dist[], bool visited_map[]) {
    double min = DBL_MAX; // Use DBL_MAX for double precision
    int min_index = -1; // Initialize to -1 for safety

    for (int v = 0; v < V; v++) {
        if (!visited_map[v] && dist[v] < min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// Dijkstra's single-source shortest path algorithm with structs
void dijkstra(double graph[V][V], Node nodes[], int start_id, int destination_id) {
    double dist[V];     // Output array. dist[i] holds the shortest distance from src to i
    bool visited_map[V]; // visited_map[i] is true if vertex i is included in the shortest path tree
    int prev[V];     // prev[i] stores the previous vertex in the path

    // Find the index of the start and destination nodes using their IDs
    int start = -1, destination = -1;
    for (int i = 0; i < V; i++) {
        if (nodes[i].id == start_id) {
            start = i;
        }
        if (nodes[i].id == destination_id) {
            destination = i;
        }
    }

    // If the source or target doesn't exist, exit the function
    if (start == -1 || destination == -1) {
        printf("Invalid source or target ID.\n");
        return;
    }

    // Initialize all distances as INFINITE and visited_map[] as false
    for (int i = 0; i < V; i++) {
        dist[i] = DBL_MAX;
        visited_map[i] = false;
        prev[i] = -1; // Undefined previous vertex
    }

    // Distance of source vertex from itself is always 0
    dist[start] = 0;

    // Find the shortest path for all vertices
    for (int count = 0; count < V - 1; count++) {
        // Pick the minimum distance vertex from the set of vertices not yet processed.
        int u = minDistance(dist, visited_map);

        // If the selected vertex has an infinite distance, no further vertices are reachable
        if (dist[u] == DBL_MAX) {
            break; // No need to process further, as remaining vertices are unreachable
        }

        // Mark the picked vertex as processed
        visited_map[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (int v = 0; v < V; v++) {
            // Update dist[v] only if it's not in visited_map, there is an edge from u to v,
            // and the total weight of the path from src to v through u is smaller than the current value of dist[v]
            if (!visited_map[v] && graph[u][v] && dist[u] != DBL_MAX && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                prev[v] = u; // Update previous vertex
            }
        }

        // Check if the target vertex has been reached
        if (u == destination) {
            break; // Stop the loop when the shortest path to the target is found
        }
    }

    // After the loop, check if the target vertex has been reached
    if (dist[destination] != DBL_MAX) {
        // Retrieve and print the path
        PathNode* nodePath = NULL; // Initialize the linked list for the nodePath
        int current = destination;

        // Trace back the path from destination to source
        while (current != -1) {
            appendToNodePath(&nodePath, nodes[current]); // Add the node to the linked list
            current = prev[current]; // Move to the previous node
        }

        printf("Shortest distance from ID %d to ID %d is %.2fm\n", start_id, destination_id, dist[destination]);
        printf("Path: ");
        printNodePath(nodePath); // Print the nodePath
        displayPathOnMap(nodePath);  // Display the Path on a Map
        freeNodePath(nodePath);   // Free the allocated memory for the nodePath
    } else {
        printf("Target %d cannot be reached from source %d\n", destination_id, start_id);
    }
}

int main() {
    // Example Graph
    double graph[V][V] = {
        {0, 28.22, 0, 0, 0, 0},
        {28.22, 0, 33.42, 0, 0, 0},
        {0, 33.42, 0, 285.02, 104.44, 0},
        {0, 0, 285.02, 0, 0, 84.46},
        {0, 0, 104.44, 0, 0, 298.28},
        {0, 0, 0, 84.46, 298.28, 0}
    };

    // Define the nodes with their ID and index
    Node nodes[V] = {
        {0, 100, 53.347781, 8.466496}, // Node with ID 101 and index 0
        {1, 101, 53.348035, 8.466381},
        {2, 102, 53.348112, 8.466833},
        {3, 103, 53.350525, 8.465404},
        {4, 104, 53.348410, 8.468341},
        {5, 105, 53.350880, 8.466570}
    };

    // Specify the source and target using node IDs
    int source_id = 100;
    int target_id = 105;

    // Run Dijkstra's algorithm with the source and target IDs
    dijkstra(graph, nodes, source_id, target_id);

    return 0;
}
