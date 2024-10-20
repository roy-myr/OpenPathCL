#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For DBL_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <curl/curl.h>

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

void printDijkstraState(int vertices, const bool visited_map[], const double dist[], const int prev[]) {
    printf("| Vertex | Visited | Distance | Previous |\n");
    printf("|--------|---------|----------|----------|\n");

    for (int i = 0; i < vertices; i++) {
        printf("| %6d | %7s | %8.2f | %8d |\n",
            i,
            visited_map[i] ? "True" : "False",
            dist[i] == INF ? -1 : dist[i], // Print -1 if distance is INF
            prev[i]);
    }

    printf("\n"); // Extra line for separation
}

// Dijkstra's single-source shortest path algorithm with structs
void dijkstra(
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

    printf("Start and dest: %lld (%d), %lld (%d)\n", start_id, start, destination_id, destination);

    // If the source or target doesn't exist, exit the function
    if (start == -1 || destination == -1) {
        printf("Invalid source or target ID.\n");
        return;
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
            printf("Couldn't find a vertices with minimal distance. Terminate the program.\n");
            break;
        }

        printf("Picked vertices %d, distance: %f\n", u, dist[u]);

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

        printf("Shortest distance from ID %lld to ID %lld is %.2fm\n", start_id, destination_id, dist[destination]);
        printf("Path: ");
        printNodePath(nodePath); // Print the nodePath
        displayPathOnMap(nodePath);  // Display the Path on a Map
        freeNodePath(nodePath);   // Free the allocated memory for the nodePath
    } else {
        printf("Target %lld cannot be reached from source %lld\n", destination_id, start_id);
    }
}

int main() {
    // set the coordinates
    const double start_lat = 53.753829290390485;
    const double start_lon = 9.671952540458692;
    const double destination_lat = 53.54434910428687;
    const double destination_lon =   9.936003265725867;

    // initialise curl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // get the nodes closest to the given address
    long long start_id = getClosestNode(start_lat, start_lon);
    if (start_id == -1) {
        printf("Couldn't find closest Node to the start coordinates (%f, %f).\n", start_lat, start_lon);
        return 1;
    }
    printf("Start_id: %lld\n", start_id);

    long long destination_id = getClosestNode(destination_lat, destination_lon);
    if (destination_id == -1) {
        printf("Couldn't find closest Node to the destination coordinates (%f, %f).\n", destination_lat, destination_lon);
        return 1;
    }
    printf("Destination_id: %lld\n", destination_id);

    // Initialise nodes Array and nodeCount
    Node* nodes = NULL;
    int nodeCount = 0;

    // Initialise roads Array and roadCount
    Road* roads = NULL;
    int roadCount = 0;

    // Data import
    getRoadNodes(
        start_lat,
        start_lon,
        destination_lat,
        destination_lon,
        3000,
        &nodes,
        &nodeCount,
        &roads,
        &roadCount);

    // end curl
    curl_global_cleanup();

    /*
    printf("Nodes: %d\n", nodeCount);
    for (int i = 0; i < nodeCount; i++) {
        printf("Index: %d, ID: %lld, Latitude: %.7f, Longitude: %.7f\n",
                nodes[i].index, nodes[i].id, nodes[i].lat, nodes[i].lon);
    }
    printf("Roads: %d\n", roadCount);
    for (int i = 0; i < roadCount; i++) {
        printf("Index: %lld, Nodes: [\n", roads[i].id);
        for (int j = 0; j < roads[i].nodeCount; j++) {
            printf("    Node%d: %lld\n", j, roads[i].nodes[j]);
        }
        printf("]\n");
    }
    //*/

    // Define the Graph
    double **graph = malloc(nodeCount * sizeof(double *));
    for (int i = 0; i < nodeCount; i++) {
        graph[i] = malloc(nodeCount * sizeof(double));
    }


    // Fill the Graph using the Roads Data
    fillGraph(graph, nodeCount, nodes, roads, roadCount);

    /*/ Print the Graph for testing
    printf("{");
    for (int i = 0; i < nodeCount; i++) {
        printf("%d: [", i);
        for (int j = 0; j < nodeCount; j++) {
            printf("%.f, ",
                graph[i][j] == INF ? -1 : graph[i][j]);
        }
        printf("]\n");
    }
    printf("}\n");//*/

    // ToDo: look if the start and dest are actually inside the node

    // Run Dijkstra's algorithm with the source and target IDs
    dijkstra(nodeCount, graph, nodes, start_id, destination_id);

    // free the graph
    for (int i = 0; i < nodeCount; i++) {
        free(graph[i]);
    }
    free(graph);

    return 0;
}
