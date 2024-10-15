#include <stdio.h>
#include <limits.h>  // For INT_MAX
#include <stdbool.h> // For boolean data types
#include <stdlib.h>

#include "common.h"  // Include the common header

#define V 5 // Number of vertices in the graph

// A utility function to find the vertex with the minimum distance value
int minDistance(int dist[], bool visited_map[]) {
    // Initialize minimum value
    int min = INT_MAX, min_index;

    for (int v = 0; v < V; v++) {
        if (!visited_map[v] && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// Dijkstra's single-source shortest path algorithm with structs
void dijkstra(int graph[V][V], Node nodes[], int start_id, int destination_id) {
    int dist[V];     // Output array. dist[i] holds the shortest distance from src to i
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
        dist[i] = INT_MAX;
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
        if (dist[u] == INT_MAX) {
            break; // No need to process further, as remaining vertices are unreachable
        }

        // Mark the picked vertex as processed
        visited_map[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (int v = 0; v < V; v++) {
            // Update dist[v] only if it's not in visited_map, there is an edge from u to v,
            // and the total weight of the path from src to v through u is smaller than the current value of dist[v]
            if (!visited_map[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v]) {
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
    if (dist[destination] != INT_MAX) {
        // Retrieve and print the path
        PathNode* nodePath = NULL; // Initialize the linked list for the nodePath
        int current = destination;

        // Trace back the path from destination to source
        while (current != -1) {
            appendToNodePath(&nodePath, nodes[current]); // Add the node to the linked list
            current = prev[current]; // Move to the previous node
        }

        printf("Path: ");
        printNodePath(nodePath); // Print the nodePath
        freeNodePath(nodePath);   // Free the allocated memory for the nodePath
    } else {
        printf("Target %d cannot be reached from source %d\n", destination_id, start_id);
    }
}

int main() {
    // Example Graph
    int graph[V][V] = {
        {0, 4, 2, 0, 0},
        {4, 0, 1, 3, 2},
        {2, 1, 0, 0, 0},
        {0, 3, 0, 0, 5},
        {0, 2, 0, 5, 0}
    };

    // Define the nodes with their ID and index
    Node nodes[V] = {
        {0, 101, 56.98496, -83.27986}, // Node with ID 101 and index 0
        {1, 102, 57.98496, -93.27986}, // Node with ID 102 and index 1
        {2, 103, 58.98496, -103.27986}, // Node with ID 103 and index 2
        {3, 104, 59.98496, -113.27986}, // Node with ID 104 and index 3
        {4, 105, 60.98496, -123.27986}  // Node with ID 105 and index 4
    };

    // Specify the source and target using node IDs
    int source_id = 101; // Node ID 101
    int target_id = 102; // Node ID 104

    // Run Dijkstra's algorithm with the source and target IDs
    dijkstra(graph, nodes, source_id, target_id);

    return 0;
}
