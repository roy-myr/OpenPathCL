#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For FLT_MAX
#include <curl/curl.h>
#include <time.h>

#include "cli_utils.h" // Include parseArguments function
#include "data_loader.h"  // Include OverpassAPI functions
#include "graph_utils.h"  // Include Graph functions

#define INF FLT_MAX

// Function to find the vertex with the minimum distance value
int minDistance(const int vertices, const float dist[], const bool visited_map[]) {
    double min = INF; // Use INF for initialization
    int min_index = -1; // Initialize to -1 for safety

    for (int v = 0; v < vertices; v++) {
        if (!visited_map[v] && dist[v] < min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// Dijkstra's single-source shortest path algorithm
int dijkstra(
        const int vertices,
        Node nodes[],
        const int start_index,
        const int dest_index) {

    float dist[vertices];     // Output array. dist[i] holds the shortest distance from src to i
    bool visited_map[vertices]; // visited_map[i] is true if vertex i is included in the shortest path tree
    int prev[vertices];     // prev[i] stores the previous vertex in the path

    // Initialize all distances as INFINITE, visited_map[] as false and previous as -1
    for (int i = 0; i < vertices; i++) {
        dist[i] = INF;
        visited_map[i] = false;
        prev[i] = -1; // Undefined previous vertex
    }

    // Distance of source vertex from itself is always 0
    dist[start_index] = 0;

    // Find the shortest path for all vertices
    for (int count = 0; count < vertices - 1; count++) {
        // Pick the minimum distance vertex from the set of vertices not yet processed.
        const int u = minDistance(vertices, dist, visited_map);

        // check if a vertices was found
        if (u == -1) {
            // All remaining vertices are inaccessible from source
            break;
        }

        // If the selected vertex has an infinite distance, no further vertices are reachable
        if (dist[u] == INF) {
            break; // No need to process further, as remaining vertices are unreachable
        }

        // Mark the picked vertex as processed
        visited_map[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (Edge* edge = nodes[u].head; edge != NULL; edge = edge->next) {
            const int v = edge->destination;
            const float weight = edge->weight;
            // Update dist[v] only if it's not in visited_map, there is an edge from u to v,
            // and the total weight of the path from src to v through u is smaller than the current value of dist[v]
            if (!visited_map[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                prev[v] = u; // Update previous vertex
            }
        }
        // Check if the target vertex has been reached
        if (u == dest_index) {
            break; // Stop the loop when the shortest path to the target is found
        }
    }

    // After the loop, check if the target vertex has been reached
    if (dist[dest_index] != INF) {
        // Retrieve and print the path
        int current = dest_index;

        printf("\t\"route\": [");
        while (current != -1) {
            printf("[%f, %f]", nodes[current].lat, nodes[current].lon);
            current = prev[current]; // Move to the previous node
            if (current != -1) {
                printf(", ");
            }
        }
        printf("],\n");

        printf("\t\"routeLength\": \"%.2fm\",\n", dist[dest_index]);
        return 0;
    }
    fprintf(stderr, "Target cannot be reached from source\n");
    return 1;
}


int main(const int argc, char *argv[]) {
    // get the timestamp of the execution start
    const clock_t total_time_start = clock();

    // Start the Response JSON
    printf("{\n");

    // define arrays for start and destination
    float start[2];   // Array for starting coordinates
    float dest[2];    // Array for destination coordinates
    float* bbox;      // Pointer for bounding box coordinates
    int bbox_size;     // Size of the bounding box

    // Parse the command-line arguments
    if (parseArguments(argc, argv, start, dest, &bbox, &bbox_size) != 0) {
        free(bbox);
        return 1; // Exit if parsing failed
    }

    // initialise curl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // get the nodes closest to the given address
    const int64_t start_id = getClosestNode(start);
    if (start_id == -1) {
        fprintf(stderr, "Couldn't find closest Node to the start coordinates (%f, %f)\n", start[0], start[1]);
        return 1;
    }
    printf("\t\"startNode\": %lld,\n", start_id);

    const int64_t destination_id = getClosestNode(dest);
    if (destination_id == -1) {
        fprintf(stderr, "Couldn't find closest node to the destination coordinates (%f, %f)\n", dest[0], dest[1]);
        return 1;
    }
    printf("\t\"destNode\": %lld,\n", destination_id);

    // Initialise nodes Array and nodeCount
    Node* nodes = NULL;
    int nodeCount = 0;

    // Initialise roads Array and roadCount
    Road* roads = NULL;
    int roadCount = 0;

    // Data import
    getRoadNodes(
        bbox,
        bbox_size,
        &nodes,
        &nodeCount,
        &roads,
        &roadCount);

    // end curl
    curl_global_cleanup();

    // free the not needed data
    free(bbox);

    // Find the index of the start and dest node
    int start_index = -1;
    int dest_index = -1;
    for (int i = 0; i < nodeCount; i++) {
        if (nodes[i].id == start_id) start_index = i;
        if (nodes[i].id == destination_id) dest_index = i;
        if (start_index != -1 && dest_index != -1) break;
    }

    // If the source or target doesn't exist, exit the function
    if (start_index == -1 || dest_index == -1) {
        fprintf(stderr, "Invalid source or target ID\n");
        free(nodes);
        return -1;
    }

    // Define the Graph
    const clock_t graph_time_start = clock();  // start the graph time measurement

    // Fill the Graph using the Roads Data
    createGraph(nodes, nodeCount, roads, roadCount);

    // free the not needed data
    free(roads);

    // end the graph time and prints its result
    const clock_t graph_time_end = clock();
    const double graph_time  = ((double) (graph_time_end - graph_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"graphTime\": %.f,\n", graph_time);

    // Run Dijkstra's algorithm with the source and target IDs
    const clock_t routing_time_start = clock();  // Start the routing time

    if (dijkstra(nodeCount, nodes, start_index, dest_index) != 0) {
        freeNodes(nodes, nodeCount);
        return 1;
    }

    freeNodes(nodes, nodeCount);

    // end the routing time and print its result
    const clock_t routing_time_end = clock();
    const double routing_time_ms = (double)(routing_time_end - routing_time_start) * 1000 / CLOCKS_PER_SEC;

    printf("\t\"routingTime\": %.f,\n", routing_time_ms);

    // get the total time and print its result
    const clock_t total_time_end = clock();
    const double total_time = ((double) (total_time_end - total_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"totalTime\": %.f,\n", total_time);

    // End the Response JSON
    printf("\t\"success\": true\n}\n");
    return 0;
}