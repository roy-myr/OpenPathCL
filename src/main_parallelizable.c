#include <stdio.h>
#include <stdlib.h>
#include <float.h>  // For FLT_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <CL/cl.h>

#include "common.h"  // Include the common header

#define INF FLT_MAX
#define EARTH_RADIUS 6371000  // Earth's radius in meters

#define DELTA 40.0 // The Delta value for bucket ranges

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

int parallelizableDeltaStepping(
        const int vertices,
        const int edge_count,
        Node nodes[],
        const int* edges_start,
        const int* edge_destinations,
        const float* edge_weights,
        const int start_index,
        const int dest_index) {

    // define the distance array. dist[i] holds the shortest distance form src to i
    float dist[vertices];

    // define the previous array. prev[i] stores the previous node in the path to i
    int prev[vertices];

    // define an array that allows us to handle adding nodes to buckets form inside OpenCl
    // nodes_2_buckets[i] contains the bucket index where the node i belongs to
    int nodes_2_bucket[vertices];

    // Initialize all distances as INFINITE, previous as -1 and nodes_2_buckets as -1
    for (int i = 0; i < vertices; i++) {
        dist[i] = INF;  // Infinite distance to node
        prev[i] = -1; // Undefined previous node
        nodes_2_bucket[i] = -1;  // no bucket where the node belongs to
    }

    // Distance of source vertex from itself is always 0
    dist[start_index] = 0;

    // Create the buckets array
    BucketsArray bucketsArray;
    initializeBuckets(&bucketsArray);

    // Add the start node to the first bucket
    addNodeToBucket(&bucketsArray, 0, start_index);

    // run a loop over every bucket
    int bucket_id = 0;
    while (bucket_id < bucketsArray.numBuckets) {
        // set work size
        size_t globalWorkSize[1] = {bucketsArray.bucketSizes[bucket_id]};

        // check if there is stuff to do
        if (globalWorkSize[0] == 0) {
            bucket_id++;
            continue;
        }

        for (int i = 0; i < bucketsArray.bucketSizes[bucket_id]; i++) {
            const int node = bucketsArray.buckets[bucket_id][i];

            // Check if node is the last index to avoid out-of-bounds access
            // Check if the nodes has next nodes
            if ((node < vertices - 1 && edges_start[node] != edges_start[node + 1]) ||
                (node == vertices - 1 && edges_start[node] != edge_count)) {

                int edge_end = (node == vertices - 1) ? edge_count : edges_start[node + 1];

                for (int edge = edges_start[node]; edge < edge_end; edge++) {
                    // calculate the new distance
                    const float new_dist = dist[node] + edge_weights[edge];

                    // check if the new distance is sorter that the previous one
                    if (dist[edge_destinations[edge]] > new_dist) {
                        // update the dest and prev of the next node
                        dist[edge_destinations[edge]] = new_dist;
                        prev[edge_destinations[edge]] = node;

                        // calculate the bucket of the next node
                        int next_bucket = (int)(new_dist / DELTA) + 1;

                        // check if the bucket is one that isn't done yet
                        if (next_bucket <= bucket_id) {
                            // if not set the bucket tto the next one coming
                            next_bucket = bucket_id + 1;
                        }

                        // set the bucket of the next node
                        nodes_2_bucket[edge_destinations[edge]] = next_bucket;
                    }
                }
            }
        }

        // Add the nodes to their buckets
        for (int node_index = 0; node_index < vertices; node_index++) {
            if (nodes_2_bucket[node_index] != -1) {
                addNodeToBucket(&bucketsArray, nodes_2_bucket[node_index], node_index);
                nodes_2_bucket[node_index] = -1;  // Reset the value
            }
        }
        bucket_id++;
    }

    // Free each bucket's allocated memory
    freeBuckets(&bucketsArray);

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

    // Create Arrays for handling the data inside OpenCL
    int edges_start[nodeCount];  // Array that holds the starting index inside the edges array for each node
    int *edge_destinations = NULL; // Array to hold the destination of each edge
    float *edge_weights = NULL;    // Array to hold the weight of each edge

    // Convert nodes and edges to OpenCL-compatible flattened arrays
    int edge_count = 0;
    convert_to_device_arrays(nodes, nodeCount, edges_start, &edge_destinations, &edge_weights, &edge_count);

    // end the graph time and prints its result
    const clock_t graph_time_end = clock();
    const double graph_time  = ((double) (graph_time_end - graph_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"graphTime\": %.f,\n", graph_time);

    // Run Dijkstra's algorithm with the source and target IDs
    const clock_t routing_time_start = clock();  // Start the routing time
    if (parallelizableDeltaStepping(
        nodeCount,
        edge_count,
        nodes,
        edges_start,
        edge_destinations,
        edge_weights,
        start_index,
        dest_index) != 0) {
        freeNodes(nodes, nodeCount);
        return 1;
    }

    freeNodes(nodes, nodeCount);
    free(edge_destinations);
    free(edge_weights);

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