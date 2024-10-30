#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For FLT_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <curl/curl.h>
#include <time.h>

#include "common.h"  // Include the common header

#define INF FLT_MAX
#define EARTH_RADIUS 6371000  // Earth's radius in meters

#define DELTA 10.0 // The Delta value for bucket ranges, this can be tuned for optimal performance

// Define an initial capacity for each bucket
#define INITIAL_BUCKET_CAPACITY 10

// Function for calculating the distance between two coordinated on earth
float haversine(float lat1, const float lon1, float lat2, const float lon2) {
    const float lat_distance = (float) ((lat2 - lat1) * (M_PI / 180.0));
    const float lon_distance = (float) ((lon2 - lon1) * (M_PI / 180.0));

    lat1 = (float) (lat1 * (M_PI / 180.0));
    lat2 = (float) (lat2 * (M_PI / 180.0));

    const float a = (float) (sin(lat_distance / 2) * sin(lat_distance / 2) +
                    sin(lon_distance / 2) * sin(lon_distance / 2) * cos(lat1) * cos(lat2));

    const float c = (float) (2 * atan2(sqrt(a), sqrt(1 - a)));

    return EARTH_RADIUS * c;
}

// Function to fill the graph with the data from the roads
void createGraph(Node* nodes, const int nodeCount, const Road* roads, const int roadCount) {
    // Iterate through each road
    for (int i = 0; i < roadCount; i++) {
        const Road road = roads[i];

        // Go through each pair of consecutive nodes in the road
        for (int j = 0; j < road.nodeCount - 1; j++) {
            const int64_t nodeId1 = road.nodes[j];
            const int64_t nodeId2 = road.nodes[j + 1];

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


void add_to_bucket(int **buckets, int *bucket_count, int *bucket_capacity, const int index, const int node) {
    // Check if the bucket needs to be initialized or resized
    if (buckets[index] == NULL) {
        // Allocate initial memory for the bucket
        bucket_capacity[index] = INITIAL_BUCKET_CAPACITY;
        buckets[index] = malloc(bucket_capacity[index] * sizeof(int));
        if (buckets[index] == NULL) {
            fprintf(stderr, "Failed to allocate memory for bucket %d\n", index);
            exit(EXIT_FAILURE);
        }
    } else if (bucket_count[index] >= bucket_capacity[index]) {
        // Resize the bucket if the count exceeds the current capacity
        bucket_capacity[index] *= 2;  // Double the capacity
        buckets[index] = realloc(buckets[index], bucket_capacity[index] * sizeof(int));
        if (buckets[index] == NULL) {
            fprintf(stderr, "Failed to reallocate memory for bucket %d\n", index);
            exit(EXIT_FAILURE);
        }
    }

    // Add the node to the bucket and increase the bucket count
    buckets[index][bucket_count[index]++] = node;
}


int deltaStepping(
        const int vertices,
        Node nodes[],
        const int start_index,
        const int dest_index) {

    float dist[vertices];     // Output array. dist[i] holds the shortest distance from src to i
    int prev[vertices];     // prev[i] stores the previous vertex in the path

    // Initialize all distances as INFINITE and previous as -1
    for (int i = 0; i < vertices; i++) {
        dist[i] = INF;  // Infinite distance to node
        prev[i] = -1; // Undefined previous vertex
    }

    // Distance of source vertex from itself is always 0
    dist[start_index] = 0;

    // Define the buckets array with dynamic resizing capability
    int **buckets = malloc(vertices * sizeof(int *));
    int bucket_count[vertices];
    int bucket_capacity[vertices];

    // Initialize the buckets with an initial capacity and count
    for (int i = 0; i < vertices; i++) {
        buckets[i] = malloc(INITIAL_BUCKET_CAPACITY * sizeof(int));
        bucket_count[i] = 0;
        bucket_capacity[i] = INITIAL_BUCKET_CAPACITY;
    }

    // add the start node to the first bucket
    add_to_bucket(buckets, bucket_count, bucket_capacity, 0, start_index);

    // go through each Node
    for (int i = 0; i < vertices; i++) {
        while (bucket_count[i] > 0) {
            // get the last node in the bucket and update the bucket count
            int u = buckets[i][--bucket_count[i]];

            // get the edge of the node
            const Edge* edge = nodes[u].head;

            // process light edges (weight <= DELTA)
            while (edge) {
                if (edge->weight <= DELTA) {
                    // calculate the new distance
                    const float new_distance = dist[u] + edge->weight;
                    // check if the distance is less than the current one
                    if (new_distance < dist[edge->destination]) {
                        // set the new distance for the next node
                        dist[edge->destination] = new_distance;
                        // set the previous of the next node
                        prev[edge->destination] = u;
                        // calculate the bucket where the next node belongs to
                        const int new_bucket_index = (int) (new_distance / DELTA);
                        // add the next node to the correct bucket
                        add_to_bucket(buckets, bucket_count, bucket_capacity, new_bucket_index, edge->destination);
                    }
                }
                // go to next edge
                edge = edge->next;
            }

            // reset the edges
            edge = nodes[u].head;

            // process heavy edges (weight > DELTA)
            while (edge) {
                if (edge->weight > DELTA) {
                    // calculate the new distance
                    const float new_distance = dist[u] + edge->weight;
                    // check if the distance is less than the current one
                    if (new_distance < dist[edge->destination]) {
                        // set the new distance for the next node
                        dist[edge->destination] = new_distance;
                        //set the previous of the next node
                        prev[edge->destination] = u;
                        // calculate the bucket where the next node belongs to
                        const int new_bucket_index = (int) (new_distance / DELTA);
                        // add the next node to the correct bucket
                        add_to_bucket(buckets, bucket_count, bucket_capacity, new_bucket_index, edge->destination);
                    }
                }
                // go to next edge
                edge = edge->next;
            }
        }
    }

    // free the bucket memory
    for (int i = 0; i < vertices; i++) {
        free(buckets[i]);
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
    /*
    if (dijkstra(nodeCount, nodes, start_index, dest_index) != 0) {
        freeNodes(nodes, nodeCount);
        return 1;
    }
    //*/
    //*
    if (deltaStepping(nodeCount, nodes, start_index, dest_index) != 0) {
        freeNodes(nodes, nodeCount);
        return 1;
    }
    //*/

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