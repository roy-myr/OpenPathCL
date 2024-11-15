#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "graph_utils.h"  // for Node and Road Struct

// Define a struct to hold the response data
struct MemoryStruct {
    char* memory;
    size_t size;
};

// Function to extract the Nodes from the JSON Response
void parseAndStoreJSON(const char* jsonResponse, Node** nodes, int* nodeCount, Road** roads, int* roadCount) {
    // Parse the JSON response
    cJSON *root = cJSON_Parse(jsonResponse);
    if (root == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return;
    }

    *nodeCount = 0;
    int nodeCapacity = 10;
    *nodes = (Node*)malloc(nodeCapacity * sizeof(Node));
    if (*nodes == NULL) {
        perror("Initial memory allocation failed for nodes");
        cJSON_Delete(root);
        exit(EXIT_FAILURE);
    }

    *roadCount = 0;
    int roadCapacity = 10;
    *roads = (Road*)malloc(roadCapacity * sizeof(Road));
    if (*roads == NULL) {
        perror("Initial memory allocation failed for roads");
        free(*nodes);
        cJSON_Delete(root);
        exit(EXIT_FAILURE);
    }

    // Retrieve and iterate through the JSON array of elements
    const cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "elements");
    if (!cJSON_IsArray(elements)) {
        fprintf(stderr, "\"elements\" is missing or not an array\n");
        free(*nodes);
        free(*roads);
        cJSON_Delete(root);
        return;
    }

    cJSON *element = NULL;
    cJSON_ArrayForEach(element, elements) {
        const cJSON *type = cJSON_GetObjectItemCaseSensitive(element, "type");

        // Handle "node" elements
        if (cJSON_IsString(type) && (strcmp(type->valuestring, "node") == 0)) {
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
            const cJSON *lat = cJSON_GetObjectItemCaseSensitive(element, "lat");
            const cJSON *lon = cJSON_GetObjectItemCaseSensitive(element, "lon");

            if (cJSON_IsNumber(id) && cJSON_IsNumber(lat) && cJSON_IsNumber(lon)) {
                Node node;
                node.id = (int64_t) id->valuedouble;
                node.lat = (float) lat->valuedouble;
                node.lon = (float) lon->valuedouble;
                node.head = NULL;

                (*nodes)[*nodeCount] = node;
                (*nodeCount)++;

                // Resize the nodes array if necessary
                if (*nodeCount >= nodeCapacity) {
                    nodeCapacity *= 2;  // Double the size
                    *nodes = (Node*)realloc(*nodes, nodeCapacity * sizeof(Node));
                    if (*nodes == NULL) {
                        perror("Memory reallocation failed for nodes");
                        free(*roads);
                        cJSON_Delete(root);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        // Handle "way" elements (Roads)
        else if (cJSON_IsString(type) && (strcmp(type->valuestring, "way") == 0)) {
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
            const cJSON *nodesArray = cJSON_GetObjectItemCaseSensitive(element, "nodes");

            if (cJSON_IsNumber(id) && cJSON_IsArray(nodesArray)) {
                Road road;
                road.id = (int64_t) id->valuedouble;
                road.nodeCount = cJSON_GetArraySize(nodesArray);

                // Allocate memory for the node IDs in this road
                road.nodes = (int64_t*)malloc(road.nodeCount * sizeof(int64_t));
                if (road.nodes == NULL) {
                    perror("Memory allocation failed for road nodes");
                    free(*nodes);
                    free(*roads);
                    cJSON_Delete(root);
                    exit(EXIT_FAILURE);
                }

                int nodeIndex = 0;
                const cJSON *nodeId = NULL;
                cJSON_ArrayForEach(nodeId, nodesArray) {
                    if (cJSON_IsNumber(nodeId)) {
                        road.nodes[nodeIndex] = (int64_t) nodeId->valuedouble;
                        nodeIndex++;
                    }
                }

                // Add the road to the roads array
                (*roads)[*roadCount] = road;
                (*roadCount)++;

                // Resize the roads array if necessary
                if (*roadCount >= roadCapacity) {
                    roadCapacity *= 2;  // Double the size
                    *roads = (Road*)realloc(*roads, roadCapacity * sizeof(Road));
                    if (*roads == NULL) {
                        perror("Memory reallocation failed for roads");
                        free(*nodes);
                        cJSON_Delete(root);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    // Clean up
    cJSON_Delete(root);

    // Print results
    printf("\t\"nodesInBoundingBox\": %d,\n", *nodeCount);
    printf("\t\"roadsInBoundingBox\": %d,\n", *roadCount);
}


static size_t WriteMemoryCallback(const void* contents, const size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    struct MemoryStruct* mem = userp;

    char* ptr = realloc(mem->memory, mem->size + realSize + 1);
    if (ptr == NULL) {
        // out of memory
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realSize);
    mem->size += realSize;
    mem->memory[mem->size] = 0;

    return realSize;
}

// Function to get the closest node to given coordinates
long long getClosestNode(const float* point) {
    struct MemoryStruct response;

    response.memory = malloc(1);  // will be grown as needed by realloc
    response.size = 0;            // no data at this point

    CURL *curl = curl_easy_init();

    if (curl) {
        char postData[512];
        snprintf(postData, sizeof(postData),
            "[out:json];"
            "way(around:50,%f,%f)['highway'];"
            "node(w)->.nodes;"
            "(._;>;);"
            "out body;",
            point[0], point[1]);

        // Set the API endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://overpass-api.de/api/interpreter");

        // Set POST request data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

        // Set the callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);

        // Perform the request
        const CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse the response and find the closest node
            cJSON *root = cJSON_Parse(response.memory);
            if (root != NULL) {
                const cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "elements");
                const cJSON *element = NULL;

                long long closestNodeId = -1;
                double closestDistance = -1;

                cJSON_ArrayForEach(element, elements) {
                    const cJSON *type = cJSON_GetObjectItemCaseSensitive(element, "type");
                    if (cJSON_IsString(type) && (strcmp(type->valuestring, "node") == 0)) {
                        const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
                        const cJSON *latNode = cJSON_GetObjectItemCaseSensitive(element, "lat");
                        const cJSON *lonNode = cJSON_GetObjectItemCaseSensitive(element, "lon");

                        if (cJSON_IsNumber(id) && cJSON_IsNumber(latNode) && cJSON_IsNumber(lonNode)) {
                            const float nodeLat = (float) latNode->valuedouble;
                            const float nodeLon = (float) lonNode->valuedouble;

                            // Calculate the distance (simple Euclidean distance)
                            const float distance = (point[0] - nodeLat) * (point[0] - nodeLat) + (point[1] - nodeLon) * (point[1] - nodeLon);
                            if (closestDistance < 0 || distance < closestDistance) {
                                closestDistance = distance;
                                closestNodeId = (int64_t) id->valuedouble;
                            }
                        }
                    }
                }

                cJSON_Delete(root);
                free(response.memory);
                curl_easy_cleanup(curl);
                return closestNodeId;
            }
            free(response.memory);
        }

        // Cleanup
        curl_easy_cleanup(curl);
        free(response.memory);
    }
    return -1;
}

// Function to get road nodes between two coordinates with a buffer
void getRoadNodes(
        const float* bbox,
        const int bbox_size,
        Node** nodes,
        int* nodeCount,
        Road** roads,
        int* roadCount) {
    struct MemoryStruct response;

    response.memory = malloc(1);  // will be grown as needed by realloc
    response.size = 0;            // no data at this point

    CURL *curl = curl_easy_init();

    if (curl) {
        char postData[2048];  // holds the post data
        char polyBuffer[1024] = {0}; // To hold the polygon (bbox) coordinates

        // Start constructing the Overpass QL query
        strcpy(postData, "[out:json];way['highway'](poly:'");

        // Add bbox polygon coordinates to the polyBuffer
        for (int i = 0; i < bbox_size; i += 2) {
            char coord[64];  // Buffer for one lat-lon pair
            snprintf(coord, sizeof(coord), " %f %f", bbox[i], bbox[i+1]);
            strncat(polyBuffer, coord, sizeof(polyBuffer) - strlen(polyBuffer) - 1);  // Concatenate each lat-lon pair to the polygon
        }

        // Complete the Overpass query string
        strncat(postData, polyBuffer, sizeof(postData) - strlen(postData) - 1);    // Add the polygon to postData string
        strncat(postData, "');out body;>;out skel qt;", sizeof(postData) - strlen(postData) - 1);

        // Debugging print to see the constructed query
        printf("\t\"nodesRequest\": \"https://overpass-turbo.eu/?Q=%s\",\n", postData);

        // Set the API endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://overpass-api.de/api/interpreter");

        // Set POST request data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

        // Set the callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);

        // Perform the request
        const CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse the response
            parseAndStoreJSON(response.memory, nodes, nodeCount, roads, roadCount);
        }

        // Cleanup
        curl_easy_cleanup(curl);
        free(response.memory);
    }
}