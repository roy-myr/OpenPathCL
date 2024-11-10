#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For FLT_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <CL/cl.h>

#include "common.h"  // Include the common header

// set OpenCL Version
#define CL_TARGET_OPENCL_VERSION 120

// define opencl platform and device within it
#define OPENCL_CL_PLAT 0
#define OPENCL_DEVICE 1

#define INF FLT_MAX
#define EARTH_RADIUS 6371000  // Earth's radius in meters

#define DELTA 200.0 // The Delta value for bucket ranges, this can be tuned for optimal performance

#define CHECK_ERROR(err, msg) \
    if (err != CL_SUCCESS) { \
    fprintf(stderr, "%s failed with error code %d\n", msg, err); \
    exit(EXIT_FAILURE); \
    }

/*/ define a struct for handling the edges in OpenCL
typedef struct {
    int destination;
    float weight;
} __attribute__((packed, aligned(16))) DeviceEdge;*/

const char* kernel_source =
"__kernel void process_bucket_nodes(                                                \n"
"   __global float* dist,                                                           \n"
"   __global int* prev,                                                             \n"
"   __global const int* edges_start,                                                \n"
"   __global const int* edge_destinations,                                          \n"
"   __global const float* edge_weights,                                             \n"
"   __global const int* bucket_nodes,                                               \n"
"   __global int* nodes_2_bucket,                                                   \n"
"   const float delta,                                                              \n"
"   const int vertices,                                                             \n"
"   const int edge_count,                                                            \n"
"   const int current_bucket                                                        \n"
") {                                                                                \n"
"   int node = bucket_nodes[get_global_id(0)];                                      \n"
"                                                                                   \n"
"   // Check if node is the last index to avoid out-of-bounds access                \n"
"   // Check if the nodes has next nodes                                            \n"
"   if ((node < vertices - 1 && edges_start[node] != edges_start[node + 1]) ||      \n"
"       (node == vertices - 1 && edges_start[node] != edge_count)) {                \n"
"                                                                                   \n"
"       int edge_end = (node == vertices - 1) ? edge_count : edges_start[node + 1]; \n"
"                                                                                   \n"
"       for (int edge = edges_start[node]; edge < edge_end; edge++) {               \n"
"           // calculate the new distance                                           \n"
"           const float new_dist = dist[node] + edge_weights[edge];                 \n"
"                                                                                   \n"
"           // check if the new distance is sorter that the previous one            \n"
"           if (dist[edge_destinations[edge]] > new_dist) {                         \n"
"               // update the dest and prev of the next node                        \n"
"               dist[edge_destinations[edge]] = new_dist;                           \n"
"               prev[edge_destinations[edge]] = node;                               \n"
"                                                                                   \n"
"               // calculate the bucket of the next node                            \n"
"               int next_bucket = (int)(new_dist / delta) + 1;                      \n"
"                                                                                   \n"
"               // check if the bucket is one that isn't done yet                   \n"
"               if (next_bucket <= current_bucket) {                                \n"
"                   // if not set the bucket tto the next one coming                \n"
"                   next_bucket = current_bucket + 1;                               \n"
"               }                                                                   \n"
"                                                                                   \n"
"               // set the bucket of the next node                                  \n"
"               nodes_2_bucket[edge_destinations[edge]] = next_bucket;              \n"
"           }                                                                       \n"
"       }                                                                           \n"
"   }                                                                               \n"
"}";

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

int deltaStepping(
        const int vertices,
        Node nodes[],
        const int start_index,
        const int dest_index) {

    // define the distance array. dist[i] holds the shortest distance form src to i
    float dist[vertices];

    // define the previous array. prev[i] stores the previous node in the path to i
    int prev[vertices];

    // define an array that allows us to handle adding nodes to buckets form inside OpenCl
    // nodes_2_buckets[i] contains the bucket index where the node i belongs to
    int nodes_2_buckets[vertices];

    // Initialize all distances as INFINITE, previous as -1 and nodes_2_buckets as -1
    for (int i = 0; i < vertices; i++) {
        dist[i] = INF;  // Infinite distance to node
        prev[i] = -1; // Undefined previous node
        nodes_2_buckets[i] = -1;  // no bucket where the node belongs to
    }

    // Distance of source vertex from itself is always 0
    dist[start_index] = 0;

    // Create the buckets array
    BucketsArray bucketsArray;
    initializeBuckets(&bucketsArray);

    // Add the start node to the first bucket
    addNodeToBucket(&bucketsArray, 0, start_index);

    // Create Arrays for handling the data inside OpenCL
    int edges_start[vertices];  // Array that holds the starting index inside the edges array for each node
    int *edge_destinations = NULL; // Array to hold the destination of each edge
    float *edge_weights = NULL;    // Array to hold the weight of each edge

    // Convert nodes and edges to OpenCL-compatible flattened arrays
    int edge_count = 0;
    convert_to_device_arrays(nodes, vertices, edges_start, &edge_destinations, &edge_weights, &edge_count);

    // ---- Initialize OpenCL ----
    // Variable to check the output of the opencl API calls
    cl_int cl_status;

    // get the number of platforms
    cl_uint numPlatforms = 0;
    cl_status = clGetPlatformIDs(0, NULL, &numPlatforms);
    CHECK_ERROR(cl_status, "clGetPlatformIDs")

    // Allocate space for each platform
    cl_platform_id *platforms = NULL;
    platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));

    // Fill the platforms
    cl_status = clGetPlatformIDs(numPlatforms, platforms, NULL);
    CHECK_ERROR(cl_status, "clGetPlatformIDs")

    // Retrieve the number of devices for the selected platform
    cl_uint numDevices = 0;
    cl_status = clGetDeviceIDs(platforms[OPENCL_CL_PLAT], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    CHECK_ERROR(cl_status, "clGetDeviceIDs")

    // Allocate space for each device
    cl_device_id *devices;
    devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));

    // Fill in the devices of the selected platform
    cl_status = clGetDeviceIDs(platforms[OPENCL_CL_PLAT], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    CHECK_ERROR(cl_status, "clGetDeviceIDs")

    // create the OpenCL context
    cl_context context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateContext")

    // create the command queue
    cl_command_queue queue = clCreateCommandQueue(context, devices[OPENCL_DEVICE], 0, &cl_status);
    CHECK_ERROR(cl_status, "clCreateCommandQueue")

    // create and build the program
    cl_program program = clCreateProgramWithSource(context, 1, (const char **) &kernel_source, NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateProgramWithSource")
    cl_status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
    if (cl_status != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Kernel build log:\n%s\n", log);
        free(log);
        exit(EXIT_FAILURE);
    }

    // create the kernel
    cl_kernel kernel = clCreateKernel(program, "process_bucket_nodes", &cl_status);
    CHECK_ERROR(cl_status, "clCreateKernel")

    // create buffers for device data
    cl_mem dist_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, vertices * sizeof(float), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for dist_buffer")
    cl_mem prev_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, vertices * sizeof(float), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for prev_buffer")
    cl_mem edges_start_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, vertices * sizeof(int), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for edges_start_buffer")
    cl_mem edge_destinations_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, edge_count * sizeof(int), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for edge_destinations_buffer")
    cl_mem edge_weights_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, edge_count * sizeof(float), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for edge_weights_buffer")
    cl_mem bucket_nodes_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, vertices * sizeof(int), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for bucket_nodes_buffer")
    cl_mem nodes_2_bucket_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, vertices * sizeof(int), NULL, &cl_status);
    CHECK_ERROR(cl_status, "clCreateBuffer for nodes_2_bucket_buffer")

    // copy the data to the buffers
    cl_status = clEnqueueWriteBuffer(queue, dist_buffer, CL_TRUE, 0, vertices * sizeof(float), dist, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for dist_buffer")
    cl_status = clEnqueueWriteBuffer(queue, prev_buffer, CL_TRUE, 0, vertices * sizeof(float), prev, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for prev_buffer")
    cl_status = clEnqueueWriteBuffer(queue, edges_start_buffer, CL_TRUE, 0, vertices * sizeof(int), edges_start, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for edges_start_buffer")
    cl_status = clEnqueueWriteBuffer(queue, edge_destinations_buffer, CL_TRUE, 0, edge_count * sizeof(int), edge_destinations, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for edge_destinations_buffer")
    cl_status = clEnqueueWriteBuffer(queue, edge_weights_buffer, CL_TRUE, 0, edge_count * sizeof(float), edge_weights, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for edge_weights_buffer")

    // set the kernel arguments
    cl_status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &dist_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for dist_buffer")
    cl_status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &prev_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for prev_buffer")
    cl_status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &edges_start_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for edges_start_buffer")
    cl_status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &edge_destinations_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for edge_destinations_buffer")
    cl_status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &edge_weights_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for edge_weights_buffer")
    cl_status = clSetKernelArg(kernel, 5, sizeof(cl_mem), &bucket_nodes_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for bucket_nodes_buffer")
    cl_status = clSetKernelArg(kernel, 6, sizeof(cl_mem), &nodes_2_bucket_buffer);
    CHECK_ERROR(cl_status, "clSetKernelArg for nodes_2_bucket_buffer")
    const float delta = DELTA;
    cl_status = clSetKernelArg(kernel, 7, sizeof(float), &delta);
    CHECK_ERROR(cl_status, "clSetKernelArg for delta")
    cl_status = clSetKernelArg(kernel, 8, sizeof(int), &vertices);
    CHECK_ERROR(cl_status, "clSetKernelArg for vertices")
    cl_status = clSetKernelArg(kernel, 9, sizeof(int), &edge_count);
    CHECK_ERROR(cl_status, "clSetKernelArg for edge_count")

    // run a loop over every bucket
    int bucket_id = 0;
    while (bucket_id < bucketsArray.numBuckets) {
        // set work size
        size_t globalWorkSize[1] = {bucketsArray.bucketSizes[bucket_id]};

        // check if there is stuff to do
        if (globalWorkSize[0] == 0) continue;

        // set the new bucket_nodes and reset the nodes_2_bucket in OpenCL
        cl_status = clEnqueueWriteBuffer(queue, bucket_nodes_buffer, CL_TRUE, 0, bucketsArray.bucketSizes[bucket_id] * sizeof(int), bucketsArray.buckets[bucket_id], 0, NULL, NULL);
        CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for bucket_nodes_buffer")
        cl_status = clEnqueueWriteBuffer(queue, nodes_2_bucket_buffer, CL_TRUE, 0, vertices * sizeof(int), nodes_2_buckets, 0, NULL, NULL);
        CHECK_ERROR(cl_status, "clEnqueueWriteBuffer for nodes_2_bucket_buffer")

        // set the current_bucket argument
        cl_status = clSetKernelArg(kernel, 10, sizeof(int), &bucket_id);
        CHECK_ERROR(cl_status, "clSetKernelArg for current_bucket")

        // execute kernels on the GPU
        cl_status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        CHECK_ERROR(cl_status, "clEnqueueNDRangeKernel")

        // wait for the results
        cl_status = clFinish(queue);
        CHECK_ERROR(cl_status, "clFinish")

        // get back the calculated nodes_2_buckets
        cl_status = clEnqueueReadBuffer(queue, nodes_2_bucket_buffer, CL_TRUE, 0, vertices * sizeof(float), nodes_2_buckets, 0, NULL, NULL);
        CHECK_ERROR(cl_status, "clEnqueueReadBuffer for nodes_2_buckets_buffer")

        // Add the nodes to their buckets
        for (int node_index = 0; node_index < vertices; node_index++) {
            if (nodes_2_buckets[node_index] != -1) {
                //printf("%d\n", node_index);
                addNodeToBucket(&bucketsArray, nodes_2_buckets[node_index], node_index);
                nodes_2_buckets[node_index] = -1;  // Reset the value
            }
        }
        bucket_id++;
    }

    // get the calculated distance and previous arrays
    cl_status = clEnqueueReadBuffer(queue, dist_buffer, CL_TRUE, 0, vertices * sizeof(float), dist, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueReadBuffer for dist_buffer")
    cl_status = clEnqueueReadBuffer(queue, prev_buffer, CL_TRUE, 0, vertices * sizeof(float), prev, 0, NULL, NULL);
    CHECK_ERROR(cl_status, "clEnqueueReadBuffer for prev_buffer")

    // release the OpenCL objects
    clReleaseMemObject(dist_buffer);
    clReleaseMemObject(prev_buffer);
    clRetainMemObject(edges_start_buffer);
    clReleaseMemObject(edge_destinations_buffer);
    clReleaseMemObject(edge_weights_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);


    // Free each bucket's allocated memory
    freeBuckets(&bucketsArray);


    // Free edge memory
    free(edge_destinations);
    free(edge_weights);

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