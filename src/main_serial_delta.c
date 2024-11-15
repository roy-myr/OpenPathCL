#include <stdio.h>
#include <stdlib.h>
#include <float.h>  // For FLT_MAX
#include <curl/curl.h>
#include <time.h>

#include "cli_utils.h" // Include parseArguments function
#include "data_loader.h"  // Include OverpassAPI functions
#include "graph_utils.h"  // Include Graph functions
#include "bucket_utils.h"  // Include Bucket functions

#define INF FLT_MAX

#define DELTA 40.0 // The Delta value for bucket ranges, this can be tuned for optimal performance

// Delta-Stepping algorithm
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
    BucketsArray bucketsArray;
    initializeBuckets(&bucketsArray);

    // add the start node to the first bucket
    addNodeToBucket(&bucketsArray, 0, start_index);

    // go through each Bucket
    int bucket_id = 0;
    while (bucket_id < bucketsArray.numBuckets) {
        // go through each node inside the bucket
        for (int i = 0; i < bucketsArray.bucketSizes[bucket_id]; i++) {
            const int node = bucketsArray.buckets[bucket_id][i];

            // get the edge of the node
            const Edge* edge = nodes[node].head;

            // process light edges (weight <= DELTA)
            while (edge) {
                if (edge->weight <= DELTA) {
                    // calculate the new distance
                    const float new_distance = dist[node] + edge->weight;
                    // check if the distance is less than the current one
                    if (new_distance < dist[edge->destination]) {
                        // set the new distance for the next node
                        dist[edge->destination] = new_distance;
                        // set the previous of the next node
                        prev[edge->destination] = node;
                        // calculate the bucket where the next node belongs to
                        const int new_bucket_index = (int) (new_distance / DELTA);
                        // add the next node to the correct bucket
                        addNodeToBucket(&bucketsArray, new_bucket_index, edge->destination);
                    }
                }
                // go to next edge
                edge = edge->next;
            }

            // reset the edges
            edge = nodes[node].head;

            // process heavy edges (weight > DELTA)
            while (edge) {
                if (edge->weight > DELTA) {
                    // calculate the new distance
                    const float new_distance = dist[node] + edge->weight;
                    // check if the distance is less than the current one
                    if (new_distance < dist[edge->destination]) {
                        // set the new distance for the next node
                        dist[edge->destination] = new_distance;
                        //set the previous of the next node
                        prev[edge->destination] = node;
                        // calculate the bucket where the next node belongs to
                        const int new_bucket_index = (int) (new_distance / DELTA);
                        // add the next node to the correct bucket
                        addNodeToBucket(&bucketsArray, new_bucket_index, edge->destination);
                    }
                }
                // go to next edge
                edge = edge->next;
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

    // end the graph time and prints its result
    const clock_t graph_time_end = clock();
    const double graph_time  = ((double) (graph_time_end - graph_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"graphTime\": %.f,\n", graph_time);

    // Run Delta stepping algorithm with the source and target IDs
    const clock_t routing_time_start = clock();  // Start the routing time

    if (deltaStepping(nodeCount, nodes, start_index, dest_index) != 0) {
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