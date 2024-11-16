#include <stdio.h>
#include <stdlib.h>
#include <float.h>  // For FLT_MAX
#include <curl/curl.h>
#include <time.h>
#include <CL/cl.h>

#include "cli_utils.h" // Include parseArguments function
#include "data_loader.h"  // Include OverpassAPI functions
#include "graph_utils.h"  // Include Graph functions
#include "bucket_utils.h"  // Include Bucket functions
#include "parallel_utils.h"  // Include convert_to_device_arrays function

#define INF FLT_MAX

#define DELTA 40.0 // The Delta value for bucket ranges

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