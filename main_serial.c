#include <stdio.h>
#include <limits.h>  // For INT_MAX
#include <stdbool.h> // For boolean data types

#define V 5 // Number of vertices in the graph

// A utility function to find the vertex with the minimum distance value
int minDistance(int dist[], bool sptSet[]) {
    // Initialize minimum value
    int min = INT_MAX, min_index;

    for (int v = 0; v < V; v++) {
        if (sptSet[v] == false && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// A utility function to print the distance array
void printSolution(int dist[], int prev[]) {
    printf("Vertex\tDistance from Source\tPrevious Vertex\n");
    for (int i = 0; i < V; i++) {
        printf("%d \t\t %d\t\t\t%d\n", i, dist[i], prev[i]);
    }
}

// Function that implements Dijkstra's single source shortest path algorithm
void dijkstra(int graph[V][V], int src) {
    int dist[V];  // The output array. dist[i] will hold the shortest distance from src to i
    bool visited_map[V]; // visited_map[i] will be true if vertex i is included in the shortest path tree
    int prev[V];    // prev[i] will store the previous vertex in the path

    // Initialize all distances as INFINITE and sptSet[] as false
    for (int i = 0; i < V; i++) {
        dist[i] = INT_MAX;
        visited_map[i] = false;
        prev[i] = -1; // Undefined previous vertex
    }

    // Distance of source vertex from itself is always 0
    dist[src] = 0;

    // Find the shortest path for all vertices
    for (int count = 0; count < V - 1; count++) {
        // Pick the minimum distance vertex from the set of vertices not yet processed.
        int u = minDistance(dist, visited_map);

        // Mark the picked vertex as processed
        visited_map[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (int v = 0; v < V; v++) {
            // Update dist[v] only if is not in visited_map, there is an edge from u to v,
            // and the total weight of the path from src to v through u is smaller than the current value of dist[v]
            if (!visited_map[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                prev[v] = u; // Update previous vertex
            }
        }
    }

    // Print the constructed distance array
    printSolution(dist, prev);
}

int main() {
    /* Let us create the example graph discussed above */
    int graph[V][V] = {
        {0, 4, 2, 0, 0},
        {4, 0, 1, 3, 2},
        {2, 1, 0, 0, 0},
        {0, 3, 0, 0, 5},
        {0, 2, 0, 5, 0}
    };

    dijkstra(graph, 0);

    return 0;
}
