#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

// Function to create a PathNode
PathNode* createPathNode(const Node data) {
    PathNode* newNode = malloc(sizeof(PathNode));
    // check if the allocation was successful
    if (!newNode) {
        // Not successful. Return an error.
        perror("Failed to allocate memory for new path node");
        exit(EXIT_FAILURE); // Handle memory allocation failure
    }
    // Successful. Fill the PathNode
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to add a Node to the End of the PathNode linked list
void appendToNodePath(PathNode** head, const Node data) {
    PathNode* newNode = createPathNode(data);
    if (*head == NULL) {
        // List is empty. Set the New Node as the head
        *head = newNode;
        return;
    }
    PathNode* current = *head;
    while (current->next != NULL) {
        if (current->next == NULL) break;
        current = current->next; // Traverse to the last node
    }
    current->next = newNode; // Add the new node at the end
}

// Function to print the path
void printNodePath(PathNode* head) {
    const PathNode* current = head;
    while (current != NULL) {
        printf("%lld (%f, %f)", current->data.id, current->data.lat, current->data.lon);
        if (current->next != NULL) {
            printf(" <- ");
        }
        current = current->next;
    }
    printf("\n");
}

// Function to free the linked list
void freeNodePath(PathNode* head) {
    PathNode* current = head;
    PathNode* nextNode;
    while (current != NULL) {
        nextNode = current->next; // Store the next node
        free(current);            // Free the current node
        current = nextNode;       // Move to the next node
    }
}

// Function to display the path on OpenStreetMap using Leaflet
void displayPathOnMap(PathNode* nodePath) {
    // Create an HTML file to display the map
    FILE *file = fopen("map.html", "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Write the HTML content
    fprintf(file, "<!DOCTYPE html>\n");
    fprintf(file, "<html>\n");
    fprintf(file, "<head>\n");
    fprintf(file, "    <title>Path on OpenStreetMap</title>\n");
    fprintf(file, "    <meta charset='utf-8' />\n");
    fprintf(file, "    <meta name='viewport' content='width=device-width, initial-scale=1.0' />\n");
    fprintf(file, "    <link rel='stylesheet' href='https://unpkg.com/leaflet/dist/leaflet.css' />\n");
    fprintf(file, "    <style>\n");
    fprintf(file, "        #map {\n");
    fprintf(file, "            position: absolute;\n");
    fprintf(file, "            top: 0;\n");
    fprintf(file, "            bottom: 0;\n");
    fprintf(file, "            left: 0;\n");
    fprintf(file, "            right: 0;\n");
    fprintf(file, "            overflow: hidden;\n");
    fprintf(file, "        }\n");
    fprintf(file, "    </style>\n");
    fprintf(file, "</head>\n");
    fprintf(file, "<body>\n");
    fprintf(file, "    <div id='map'></div>\n");
    fprintf(file, "    <script src='https://unpkg.com/leaflet/dist/leaflet.js'></script>\n");
    fprintf(file, "    <script>\n");

    // Initialize the map
    fprintf(file, "    var map = L.map('map').setView([%f, %f], 13);\n",
            nodePath->data.lat, nodePath->data.lon);

    // Add OpenStreetMap tile layer
    fprintf(file, "    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {\n");
    fprintf(file, "        maxZoom: 19,\n");
    fprintf(file, "        attribution: '&copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors'\n");
    fprintf(file, "    }).addTo(map);\n");

    // Create a polyline for the path
    fprintf(file, "    var latlngs = [\n");

    // Write the latitude and longitude to the polyline
    while (nodePath != NULL) {
        fprintf(file, "        [%f, %f],\n", nodePath->data.lat, nodePath->data.lon);
        nodePath = nodePath->next;  // Move to the next node
    }

    fprintf(file, "    ];\n");
    fprintf(file, "    var polyline = L.polyline(latlngs, {color: 'red'}).addTo(map);\n");

    // Fit the map to the polyline
    fprintf(file, "    map.fitBounds(polyline.getBounds());\n");
    fprintf(file, "</script>\n");
    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");

    fclose(file); // Close the file

    // Open the generated HTML file in the default browser
    printf("Opening the map in the browser...\n");
    char command[256];

    // Use the appropriate command based on your operating system
#ifdef _WIN32
    sprintf(command, "start map.html"); // For Windows
#elif __APPLE__
    sprintf(command, "open map.html"); // For macOS
#else
    sprintf(command, "xdg-open map.html"); // For Linux
#endif

    system(command); // Execute the command
}


// Function to extract the Nodes from the JSON Response
void parseAndStoreJSON(const char* jsonResponse, Node** nodes, int* nodeCount, Road** roads, int* roadCount) {
    // Parse the JSON response
    cJSON *root = cJSON_Parse(jsonResponse);
    if (root == NULL) {
        printf("Error parsing JSON\n");
        return;
    }

    *nodeCount = 0;
    int nodeCapacity = 10;
    *nodes = (Node*)malloc(nodeCapacity * sizeof(Node));

    *roadCount = 0;
    int roadCapacity = 10;
    *roads = (Road*)malloc(roadCapacity * sizeof(Road));

    // Iterate through the JSON array of elements
    const cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "elements");
    cJSON *element = NULL;
    cJSON_ArrayForEach(element, elements) {
        const cJSON *type = cJSON_GetObjectItemCaseSensitive(element, "type");

        // Handle "node" elements
        if (cJSON_IsString(type) && (strcmp(type->valuestring, "node") == 0)) {
            Node node;
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
            const cJSON *lat = cJSON_GetObjectItemCaseSensitive(element, "lat");
            const cJSON *lon = cJSON_GetObjectItemCaseSensitive(element, "lon");

            if (cJSON_IsNumber(id) && cJSON_IsNumber(lat) && cJSON_IsNumber(lon)) {
                node.id = id->valuedouble;
                node.lat = lat->valuedouble;
                node.lon = lon->valuedouble;
                node.index = *nodeCount;

                (*nodes)[*nodeCount] = node;
                (*nodeCount)++;

                // Resize the nodes array if necessary
                if (*nodeCount >= nodeCapacity) {
                    nodeCapacity *= 2;  // Double the size
                    *nodes = (Node*)realloc(*nodes, nodeCapacity * sizeof(Node));
                    if (*nodes == NULL) {
                        perror("Memory reallocation failed for nodes");
                        exit(EXIT_FAILURE);  // Handle failure
                    }
                }
            }
        }
        // Handle "way" elements (Roads)
        else if (cJSON_IsString(type) && (strcmp(type->valuestring, "way") == 0)) {
            Road road;
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
            const cJSON *nodesArray = cJSON_GetObjectItemCaseSensitive(element, "nodes");

            if (cJSON_IsNumber(id) && cJSON_IsArray(nodesArray)) {
                road.id = id->valuedouble;
                road.nodeCount = cJSON_GetArraySize(nodesArray);

                // Allocate memory for the node IDs in this road
                road.nodes = (long long*)malloc(road.nodeCount * sizeof(long long));

                int nodeIndex = 0;
                cJSON *nodeId = NULL;
                cJSON_ArrayForEach(nodeId, nodesArray) {
                    if (cJSON_IsNumber(nodeId)) {
                        road.nodes[nodeIndex] = nodeId->valuedouble;
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
                        exit(EXIT_FAILURE);  // Handle failure
                    }               }
            }
        }
    }

    // Clean up
    cJSON_Delete(root);
}


// Structure to hold response data
struct MemoryStruct {
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(const void* contents, const size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    struct MemoryStruct* mem = userp;

    char* ptr = realloc(mem->memory, mem->size + realSize + 1);
    if (ptr == NULL) {
        // out of memory
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realSize);
    mem->size += realSize;
    mem->memory[mem->size] = 0;

    return realSize;
}

// Function to get the closest node to given coordinates
long long getClosestNode(const double* point) {
    CURL* curl;
    CURLcode res;
    struct MemoryStruct response;

    response.memory = malloc(1);  // will be grown as needed by realloc
    response.size = 0;            // no data at this point

    curl = curl_easy_init();

    if (curl) {
        char postData[512];
        snprintf(postData, sizeof(postData),
            "[out:json];"
            "way(around:50,%f,%f)[\"highway\"];"
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
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse the response and find the closest node
            cJSON *root = cJSON_Parse(response.memory);
            if (root != NULL) {
                const cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "elements");
                cJSON *element = NULL;

                long long closestNodeId = -1;
                double closestDistance = -1;

                cJSON_ArrayForEach(element, elements) {
                    const cJSON *type = cJSON_GetObjectItemCaseSensitive(element, "type");
                    if (cJSON_IsString(type) && (strcmp(type->valuestring, "node") == 0)) {
                        const cJSON *id = cJSON_GetObjectItemCaseSensitive(element, "id");
                        const cJSON *latNode = cJSON_GetObjectItemCaseSensitive(element, "lat");
                        const cJSON *lonNode = cJSON_GetObjectItemCaseSensitive(element, "lon");

                        if (cJSON_IsNumber(id) && cJSON_IsNumber(latNode) && cJSON_IsNumber(lonNode)) {
                            double nodeLat = latNode->valuedouble;
                            double nodeLon = lonNode->valuedouble;

                            // Calculate the distance (simple Euclidean distance)
                            double distance = (point[0] - nodeLat) * (point[0] - nodeLat) + (point[1] - nodeLon) * (point[1] - nodeLon);
                            if (closestDistance < 0 || distance < closestDistance) {
                                closestDistance = distance;
                                closestNodeId = id->valuedouble;
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
        const double* bbox,
        const int bbox_size,
        Node** nodes,
        int* nodeCount,
        Road** roads,
        int* roadCount) {

    CURL* curl;
    CURLcode res;
    struct MemoryStruct response;

    response.memory = malloc(1);  // will be grown as needed by realloc
    response.size = 0;            // no data at this point

    curl = curl_easy_init();

    if (curl) {
        char postData[2048];  // Assuming 2048 bytes is sufficient, adjust as needed
        char polyBuffer[1024] = {0}; // To hold the polygon (bbox) coordinates

        // Start constructing the Overpass QL query
        strcpy(postData, "[out:json];way[\"highway\"](poly:\"");

        // Add bbox polygon coordinates to the polyBuffer
        for (int i = 0; i < bbox_size; i += 2) {
            char coord[64];  // Buffer for one lat-lon pair
            snprintf(coord, sizeof(coord), "%f %f ", bbox[i], bbox[i+1]);
            strncat(polyBuffer, coord, sizeof(polyBuffer) - strlen(polyBuffer) - 1);  // Concatenate each lat-lon pair to the polygon
        }

        // Close the polygon by appending the first point again (optional)
        char firstPoint[64];
        snprintf(firstPoint, sizeof(firstPoint), "%f %f", bbox[0], bbox[1]);
        strncat(polyBuffer, firstPoint, sizeof(polyBuffer) - strlen(polyBuffer) - 1);

        // Complete the Overpass query string
        strncat(postData, polyBuffer, sizeof(postData) - strlen(postData) - 1);    // Add the polygon to postData string
        strncat(postData, "\");out body;>;out skel qt;", sizeof(postData) - strlen(postData) - 1);

        // Debugging print to see the constructed query
        printf("Constructed Overpass API query:\n%s\n", postData);

        // Set the API endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://overpass-api.de/api/interpreter");

        // Set POST request data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

        // Set the callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);

        // Perform the request
        res = curl_easy_perform(curl);

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