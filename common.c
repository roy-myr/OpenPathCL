#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// Function to create a PathNode
PathNode* createPathNode(const Node data) {
    PathNode* newNode = (PathNode*)malloc(sizeof(PathNode));
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
        current = current->next; // Traverse to the last node
    }
    current->next = newNode; // Add the new node at the end
}

// Function to print the path
void printNodePath(PathNode* head) {
    const PathNode* current = head;
    while (current != NULL) {
        printf("%d (%f, %f)", current->data.id, current->data.lat, current->data.lon);
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


// Function to extract the Nodes from the JSOn Response
void parseAndStoreNoes(const char* jsonResponse, Node** nodes, int* nodeCount) {
    const char* pos = jsonResponse;  // Pointer to traverse the response
    *nodeCount = 0;
    int capacity = 10;

    // Allocate memory for nodes
    *nodes = (Node*)malloc(capacity * sizeof(Node));

    // Find each node in the response
    while ((pos = strstr(pos, "\"type\": \"node\"")) != NULL) {
        Node node;

        // Find the node ID
        if (sscanf(strstr(pos, "\"id\":") + 5, "%lld", &node.id) != 1) break;

        // Find the latitude
        if (sscanf(strstr(pos, "\"lat\":") + 6, "%lf", &node.lat) != 1) break;

        // Find the longitude
        if (sscanf(strstr(pos, "\"lon\":") + 6, "%lf", &node.lon) != 1) break;

        // Set the index to the current nodeCount
        node.index = *nodeCount;

        // Add the node to the array
        (*nodes)[*nodeCount] = node;
        (*nodeCount)++;

        // Resize the array if necessary
        if (*nodeCount >= capacity) {
            capacity *= 2;
            *nodes = (Node*)realloc(*nodes, capacity * sizeof(Node));
        }

        pos++;  // Move the pointer forward for the next search
    }
}


// Function to get road nodes between two coordinates with a buffer
void getRoadNodes(const double lat1, const double lon1, const double lat2, const double lon2, const double buffer, Node** nodes, int* nodeCount) {
    // Define the Overpass API host and endpoint
    LPCWSTR host = L"overpass-api.de";
    LPCWSTR endpoint = L"/api/interpreter";

    // Initialize WinHTTP session
    const HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
                                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                           WINHTTP_NO_PROXY_NAME,
                                           WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        // Connect to the Overpass API
        const HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTP_PORT, 0);

        if (hConnect) {
            // Create a request for the POST method
            const HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint,
                                                          NULL, WINHTTP_NO_REFERER,
                                                          WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

            if (hRequest) {
                // Prepare the Overpass API query
                char postData[1024];
                snprintf(postData, sizeof(postData),
                    "[out:json];"
                    "way[\"highway\"](around:%f,%f,%f,%f,%f);"
                    "out body;>;out skel qt;",
                    buffer, lat1, lon1, lat2, lon2);

                // Set up headers
                const wchar_t *headers = L"Content-Type: application/x-www-form-urlencoded";
                DWORD postDataLength = (DWORD)strlen(postData);

                // Send the request
                BOOL bResults = WinHttpSendRequest(hRequest, headers, -1L, (LPVOID)postData, postDataLength, postDataLength, 0);

                // Receive a response
                if (bResults) {
                    bResults = WinHttpReceiveResponse(hRequest, NULL);
                }

                if (bResults) {
                    DWORD dwSize = 0;
                    DWORD dwDownloaded = 0;
                    LPSTR pszOutBuffer;
                    BOOL  bContinue = TRUE;

                    // Initial allocation for the response buffer
                    size_t bufferSize = 1024; // Start with 1 KB
                    LPSTR responseBuffer = (LPSTR)malloc(bufferSize);
                    if (!responseBuffer) {
                        printf("Memory allocation failed\n");
                        return;
                    }
                    responseBuffer[0] = '\0'; // Initialize the buffer

                    // Keep checking for data until there's none left
                    do {
                        dwSize = 0;
                        if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                            // Resize the buffer if necessary
                            if (dwSize + strlen(responseBuffer) + 1 > bufferSize) {
                                // Increase the buffer size to fit the new data
                                bufferSize = (dwSize + strlen(responseBuffer) + 1) * 2; // double the size
                                responseBuffer = (LPSTR)realloc(responseBuffer, bufferSize);
                                if (!responseBuffer) {
                                    printf("Memory reallocation failed\n");
                                    return;
                                }
                            }

                            // Read the data
                            if (WinHttpReadData(hRequest, (LPVOID)(responseBuffer + strlen(responseBuffer)), dwSize, &dwDownloaded)) {
                                responseBuffer[strlen(responseBuffer) + dwDownloaded] = '\0'; // Null-terminate the string
                            }
                        }
                    } while (dwSize > 0);
                    // Print the response before parsing
                    printf("Full JSON Response: %s\n", responseBuffer);

                    // Parse the response
                    parseAndStoreNoes(responseBuffer, nodes, nodeCount);

                    // Clean up
                    free(responseBuffer); // Free the buffer after use
                }

                // Clean up
                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }

        WinHttpCloseHandle(hSession);
    }
}