#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For boolean data types
#include <float.h>  // For DBL_MAX
#include <math.h>  // for Pi, sin, cos, atan and sqrt
#include <curl/curl.h>
#include <time.h>
#include <CL/cl.h>

#include "common.h"  // Include the common header

#define INF DBL_MAX
#define EARTH_RADIUS 6371000  // Earth's radius in meters

// Delta value (adjust as needed)
#define DELTA 10.0

// OpenCL kernel source code as a string
const char* deltaSteppingKernelSource =
"struct Node {"
"    long id;"
"    float lat;"
"    float lon;"
"};"
"struct Road {"
"    long id;"
"    int nodeCount;"
"    long nodes[16];"     // Fixed-size array for neighbors
"    float weights[16];"  // Fixed-size array for weights
"};"
"__kernel void deltaSteppingKernel(__global const struct Node *nodes,"
"                                  __global const struct Road *roads,"
"                                  __global float *distances,"
"                                  const int nodeCount,"
"                                  const int roadCount,"
"                                  const int sourceIndex,"
"                                  const float delta) {"
"    int id = get_global_id(0);"
"    if (id >= nodeCount) return;"
"    if (id == sourceIndex) {"
"        distances[id] = 0.0f;"
"    } else if (distances[id] == 0.0f) {"
"        distances[id] = INFINITY;"
"    }"
"    for (int i = 0; i < roadCount; i++) {"
"        struct Road road = roads[i];"
"        for (int j = 0; j < road.nodeCount; j++) {"
"            if (road.nodes[j] == nodes[id].id) {"
"                for (int k = 0; k < road.nodeCount; k++) {"
"                    if (k == j) continue;"
"                    long neighborNodeId = road.nodes[k];"
"                    float weight = road.weights[k];"
"                    float newDist = distances[id] + weight;"
"                    if (newDist < distances[neighborNodeId]) {"
"                        distances[neighborNodeId] = newDist;"
"                        int bucketIndex = (int)(newDist / delta);"
"                    }"
"                }"
"            }"
"        }"
"    }"
"}";



// OpenCL variables
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel deltaSteppingKernel;

// Error handling function
void checkError(cl_int error, const char* message) {
    if (error != CL_SUCCESS) {
        fprintf(stderr, "%s failed with error %d\n", message, error);
        exit(1);
    }
}

// Initialize OpenCL
void initializeOpenCL() {
    cl_platform_id platform;
    cl_device_id device;
    cl_int error;

    // Get platform and device
    error = clGetPlatformIDs(1, &platform, NULL);
    checkError(error, "Getting platform ID");
    error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    checkError(error, "Getting device ID");

    // Create context and queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &error);
    checkError(error, "Creating context");
    queue = clCreateCommandQueue(context, device, 0, &error);
    checkError(error, "Creating command queue");

    // Create and build the program from the embedded kernel source
    program = clCreateProgramWithSource(context, 1, &deltaSteppingKernelSource, NULL, &error);
    checkError(error, "Creating program");
    error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (error != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build error:\n%s\n", log);
        free(log);
        exit(1);
    }

    // Create kernel
    deltaSteppingKernel = clCreateKernel(program, "deltaSteppingKernel", &error);
    checkError(error, "Creating kernel");
}

// Release OpenCL resources
void cleanupOpenCL() {
    clReleaseKernel(deltaSteppingKernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

// OpenCL parallelized delta-stepping function
void deltaStepping(int nodeCount, Node *nodes, Road *roads, int roadCount, int sourceIndex, double **distances) {
    cl_int error;

    // Allocate and initialize buffers
    cl_mem nodeBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Node) * nodeCount, nodes, &error);
    checkError(error, "Creating node buffer");

    cl_mem roadBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Road) * roadCount, roads, &error);
    checkError(error, "Creating road buffer");

    *distances = (double *)malloc(nodeCount * sizeof(double));
    for (int i = 0; i < nodeCount; i++) (*distances)[i] = INFINITY;
    (*distances)[sourceIndex] = 0;

    cl_mem distanceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double) * nodeCount, *distances, &error);
    checkError(error, "Creating distance buffer");

    // Set kernel arguments
    const double delta = DELTA;
    error = clSetKernelArg(deltaSteppingKernel, 0, sizeof(cl_mem), &nodeBuffer);
    error |= clSetKernelArg(deltaSteppingKernel, 1, sizeof(cl_mem), &roadBuffer);
    error |= clSetKernelArg(deltaSteppingKernel, 2, sizeof(cl_mem), &distanceBuffer);
    error |= clSetKernelArg(deltaSteppingKernel, 3, sizeof(int), &nodeCount);
    error |= clSetKernelArg(deltaSteppingKernel, 4, sizeof(int), &roadCount);
    error |= clSetKernelArg(deltaSteppingKernel, 5, sizeof(int), &sourceIndex);
    error |= clSetKernelArg(deltaSteppingKernel, 6, sizeof(double), &delta);
    checkError(error, "Setting kernel arguments");

    // Define global and local work sizes
    size_t globalSize = nodeCount;
    size_t localSize = 64;

    // Execute kernel
    error = clEnqueueNDRangeKernel(queue, deltaSteppingKernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
    checkError(error, "Enqueuing kernel");

    // Read back the results
    error = clEnqueueReadBuffer(queue, distanceBuffer, CL_TRUE, 0, sizeof(double) * nodeCount, *distances, 0, NULL, NULL);
    checkError(error, "Reading buffer");

    // Cleanup
    clReleaseMemObject(nodeBuffer);
    clReleaseMemObject(roadBuffer);
    clReleaseMemObject(distanceBuffer);
}

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

int find_node_with_id(Node nodes[], const int vertices, const long long node_id) {
    for (int i = 0; i < vertices; i++) {
        if (nodes[i].id == node_id) {
            return  i;
        }
    }
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
        fprintf(stderr, "Couldn't find closest Node to the start coordinates (%f, %f)\n", start[0], start[1]);
        return 1;
    }
    printf("\t\"startNode\": %lld,\n", start_id);

    long long destination_id = getClosestNode(dest);
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

    // Find the index of the start and dest node
    const int start_index = find_node_with_id(nodes, nodeCount, start_id);
    const int dest_index = find_node_with_id(nodes, nodeCount, destination_id);

    // If the source or target doesn't exist, exit the function
    if (start_index == -1 || dest_index == -1) {
        fprintf(stderr, "Invalid source or target ID\n");
        return -1;
    }

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
    const double graph_time  = ((double) (graph_time_end - graph_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"graphTime\": %.f,\n", graph_time);

    // Run delta stepping algorithm with the source and target IDs
    clock_t routing_time_start = clock();  // Start the routing time
    initializeOpenCL();
    double *distances = NULL;
    deltaStepping(nodeCount, nodes, roads, roadCount, start_index, &distances);

    // Print or process the results in distances array
    for (int i = 0; i < nodeCount; i++) {
        printf("Distance to node %d: %f\n", i, distances[i]);
    }

    free(distances);
    cleanupOpenCL();

    // end the routing time and print its result
    const clock_t routing_time_end = clock();
    const double routing_time_ms = (double)(routing_time_end - routing_time_start) * 1000 / CLOCKS_PER_SEC;

    printf("\t\"routingTime\": %.f,\n", routing_time_ms);

    // free the graph
    for (int i = 0; i < nodeCount; i++) {
        free(graph[i]);
    }
    free(graph);

    // get the total time and print its result
    const clock_t total_time_end = clock();
    const double total_time = ((double) (total_time_end - total_time_start)) * 1000 / CLOCKS_PER_SEC;
    printf("\t\"totalTime\": %.f,\n", total_time);

    // End the Response JSON
    printf("\t\"success\": true\n}\n");
    return 0;
}