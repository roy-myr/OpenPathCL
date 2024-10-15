#include <stdio.h>
#include <stdlib.h>
#include "common.h"

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