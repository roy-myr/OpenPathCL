#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For DBL_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <curl/curl.h>
#include <time.h>

#include "common.h"  // Include the common header

#define INF DBL_MAX
#define EARTH_RADIUS 6371000  // Earth's radius in meters


// Function for calculating the distance between two coordinated on earth
double haversine(double lat1, double lon1, double lat2, double lon2) {
    double lat_distance = (lat2 - lat1) * (M_PI / 180.0);
    double lon_distance = (lon2 - lon1) * (M_PI / 180.0);

    lat1 = lat1 * (M_PI / 180.0);
    lat2 = lat2 * (M_PI / 180.0);

    double a = sin(lat_distance / 2) * sin(lat_distance / 2) +
               sin(lon_distance / 2) * sin(lon_distance / 2) * cos(lat1) * cos(lat2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS * c;
}

// Function to fill the graph with the data from the roads
void fillGraph(double **graph, int nodeCount, Node* nodes, Road* roads, int roadCount) {
    // Initialize the graph with 0 or INF for non-edges
    for (int i = 0; i < nodeCount; i++) {
        for (int j = 0; j < nodeCount; j++) {
            graph[i][j] = 0;  // Set all values to 0
        }
    }

    // Iterate through each road
    for (int i = 0; i < roadCount; i++) {
        const Road road = roads[i];

        // Go through each pair of consecutive nodes in the road
        for (int j = 0; j < road.nodeCount - 1; j++) {
            const long long nodeId1 = road.nodes[j];
            const long long nodeId2 = road.nodes[j + 1];

            // Find the indexes of nodeId1 and nodeId2 in the nodes array
            int index1 = -1, index2 = -1;
            for (int k = 0; k < nodeCount; k++) {
                if (nodes[k].id == nodeId1) index1 = k;
                if (nodes[k].id == nodeId2) index2 = k;
            }

            // If both nodes are found, calculate the distance between them
            if (index1 != -1 && index2 != -1) {
                const double distance = haversine(nodes[index1].lat, nodes[index1].lon,
                                                  nodes[index2].lat, nodes[index2].lon);

                // Update the graph for both directions
                graph[index1][index2] = distance;
                graph[index2][index1] = distance;
            }
        }
    }
}

// Function to find the vertex with the minimum distance value
int minDistance(const int vertices, double dist[], bool visited_map[]) {
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

// Dijkstra's single-source shortest path algorithm with structs
int dijkstra(
        const int vertices,
        double **graph,
        Node nodes[],
        const long long start_id,
        const long long destination_id) {

    double dist[vertices];     // Output array. dist[i] holds the shortest distance from src to i
    bool visited_map[vertices]; // visited_map[i] is true if vertex i is included in the shortest path tree
    int prev[vertices];     // prev[i] stores the previous vertex in the path

    // Find the index of the start and destination nodes using their IDs
    int start = -1, destination = -1;
    for (int i = 0; i < vertices; i++) {
        if (nodes[i].id == start_id) {
            start = i;
        }
        if (nodes[i].id == destination_id) {
            destination = i;
        }
    }

    // If the source or target doesn't exist, exit the function
    if (start == -1 || destination == -1) {
        printf("\t\"success\": false,\n"
               "\t\"error\": \"Invalid source or target ID\"\n"
               "}");
        return -1;
    }

    // Initialize all distances as INFINITE and visited_map[] as false
    for (int i = 0; i < vertices; i++) {
        dist[i] = INF;
        visited_map[i] = false;
        prev[i] = -1; // Undefined previous vertex
    }

    // Distance of source vertex from itself is always 0
    dist[start] = 0;

    // Find the shortest path for all vertices
    for (int count = 0; count < vertices - 1; count++) {
        // Pick the minimum distance vertex from the set of vertices not yet processed.
        int u = minDistance(vertices, dist, visited_map);

        // check if a vertices was found
        if (u == -1) {
            // All remaining vertices are inaccessible from source
            break;
        }

        //printf("Picked vertices %d, distance: %f\n", u, dist[u]);

        // If the selected vertex has an infinite distance, no further vertices are reachable
        if (dist[u] == INF) {
            break; // No need to process further, as remaining vertices are unreachable
        }

        // Mark the picked vertex as processed
        visited_map[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (int v = 0; v < vertices; v++) {
            // Update dist[v] only if it's not in visited_map, there is an edge from u to v,
            // and the total weight of the path from src to v through u is smaller than the current value of dist[v]
            if (!visited_map[v] && graph[u][v] && dist[u] != DBL_MAX && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                prev[v] = u; // Update previous vertex
                // printf("Updated verticie %d with distance %f, and previous %d\n", v, dist[v], prev[v]);
            }
        }

        // Check if the target vertex has been reached
        if (u == destination) {
            break; // Stop the loop when the shortest path to the target is found
        }
    }

    // print the results for testing
    // printDijkstraState(vertices, visited_map, dist, prev);

    // After the loop, check if the target vertex has been reached
    if (dist[destination] != INF) {
        // Retrieve and print the path
        PathNode* nodePath = NULL; // Initialize the linked list for the nodePath
        int current = destination;

        // Trace back the path from destination to source
        while (current != -1) {
            appendToNodePath(&nodePath, nodes[current]); // Add the node to the linked list
            current = prev[current]; // Move to the previous node
        }

        printf("\t\"routeLength\": \"%.2fm\",\n", dist[destination]);
        printNodePath(nodePath); // Print the nodePath
        freeNodePath(nodePath);   // Free the allocated memory for the nodePath
        return 0;
    }
    printf("\t\"success\": false,\n"
           "\t\"error\": \"Target %lld cannot be reached from source %lld\n\""
           "}", destination_id, start_id);
    return -1;
}

int main(int argc, char *argv[]) {
    // get the timestamp of the execution start
    clock_t total_time_start = clock();

    // Start the Response JSON
    printf("{\n");

    // define arrays for start and destination
    double start[2];   // Array for starting coordinates
    double dest[2];    // Array for destination coordinates
    double* bbox;      // Pointer for bounding box coordinates
    int bbox_size;     // Size of the bounding box

    // Parse the command-line arguments
    if (parseArguments(argc, argv, start, dest, &bbox, &bbox_size) != 0) {
        return 1; // Exit if parsing failed
    }

    // initialise curl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // get the nodes closest to the given address
    long long start_id = getClosestNode(start);
    if (start_id == -1) {
        printf("\t\"success\": false,\n"
               "\t\"error\": \"Couldn't find closest Node to the start coordinates (%f, %f).\"\n"
               "}\n", start[0], start[1]);
        return 1;
    }
    printf("\t\"startNode\": %lld,\n", start_id);

    long long destination_id = getClosestNode(dest);
    if (destination_id == -1) {
        printf("\t\"success\": false,\n"
               "\t\"error\": \"Couldn't find closest Node to the destination coordinates (%f, %f).\"\n"
               "}\n", dest[0], dest[1]);
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

    // Define the Graph
    clock_t graph_time_start = clock();  // start the graph time measurement
    double **graph = malloc(nodeCount * sizeof(double *));
    for (int i = 0; i < nodeCount; i++) {
        graph[i] = malloc(nodeCount * sizeof(double));
    }


    // Fill the Graph using the Roads Data
    fillGraph(graph, nodeCount, nodes, roads, roadCount);

    // end the graph time and prints its result
    const clock_t graph_time_end = clock();
    const double graph_time  = ((double) (graph_time_end - graph_time_start));
    printf("\t\"graphTime\": %f,\n", graph_time);

    // ToDo: look if the start and dest are actually inside the node

    // Run Dijkstra's algorithm with the source and target IDs
    clock_t routing_time_start = clock();  // Start the routing time
    if (dijkstra(nodeCount, graph, nodes, start_id, destination_id) != 0) {
        return 1;
    }

    // end the routing time and print its result
    const clock_t routing_time_end = clock();
    const double routing_time  = ((double) (routing_time_end - routing_time_start));
    printf("\t\"routingTime\": %f,\n", routing_time);

    // free the graph
    for (int i = 0; i < nodeCount; i++) {
        free(graph[i]);
    }
    free(graph);

    // get the total time and print its result
    const clock_t total_time_end = clock();
    const double total_time = ((double) (total_time_end - total_time_start));
    printf("\t\"totalTime\": %f,\n", total_time);

    // End the Response JSON
    printf("\t\"success\": true\n}\n");
    return 0;
}
