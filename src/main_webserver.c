#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cjson/cJSON.h>

#include "input_map.h"
#include "images.h"

#define PORT 5050
#define BUFFER_SIZE 4096

void serve_image(int client_fd, const char *image_name, unsigned char *image_data, unsigned int image_len, const char *content_type) {
    char response[BUFFER_SIZE];
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\n\r\n", content_type, image_len);
    send(client_fd, response, strlen(response), 0);
    send(client_fd, image_data, image_len, 0);
}

void serve_html(int client_fd) {
    char response[BUFFER_SIZE];

    // Send HTTP headers
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    send(client_fd, response, strlen(response), 0);

    // Send the embedded HTML content
    send(client_fd, input_map_html, input_map_html_len, 0);
}

void handle_form_submission(int client_fd, char *request_body) {

    char response[BUFFER_SIZE];

    // Check if the request body is valid
    if (request_body == NULL) {
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Parse JSON data
    cJSON *json = cJSON_Parse(request_body);
    if (json == NULL) {
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Extract data from JSON
    const cJSON *algorithm = cJSON_GetObjectItemCaseSensitive(json, "algorithm");
    const cJSON *bbox = cJSON_GetObjectItemCaseSensitive(json, "bbox");
    const cJSON *start = cJSON_GetObjectItemCaseSensitive(json, "start");
    const cJSON *dest = cJSON_GetObjectItemCaseSensitive(json, "dest");

    // Check if all fields are present
    if (!cJSON_IsString(algorithm) || !cJSON_IsArray(bbox) ||
        !cJSON_IsArray(start) || !cJSON_IsArray(dest)) {
        cJSON_Delete(json);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Log extracted data
    printf("Algorithm: %s\n", algorithm->valuestring);

    printf("BBox: ");
    for (int i = 0; i < cJSON_GetArraySize(bbox); i++) {
        printf("%f ", cJSON_GetArrayItem(bbox, i)->valuedouble);
    }
    printf("\n");

    printf("Start: ");
    for (int i = 0; i < cJSON_GetArraySize(start); i++) {
        printf("%f ", cJSON_GetArrayItem(start, i)->valuedouble);
    }
    printf("\n");

    printf("Dest: ");
    for (int i = 0; i < cJSON_GetArraySize(dest); i++) {
        printf("%f ", cJSON_GetArrayItem(dest, i)->valuedouble);
    }
    printf("\n");

    // Prepare a response
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
    send(client_fd, response, strlen(response), 0);

    // Prepare JSON response
    cJSON *json_response = cJSON_CreateObject();
    cJSON_AddStringToObject(json_response, "status", "success");
    char *json_string = cJSON_Print(json_response);

    // Send the JSON response
    send(client_fd, json_string, strlen(json_string), 0);

    // Cleanup
    free(json_string);
    cJSON_Delete(json_response);
    cJSON_Delete(json);

    fflush(stdout);
}


void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_fd, buffer, sizeof(buffer));

    // Parse the request to determine which image to serve
    if (strstr(buffer, "GET / ")) {
        serve_html(client_fd);
    } else if (strstr(buffer, "GET /images/marker.svg ")) {
        serve_image(client_fd, "marker", marker, marker_len, "image/svg+xml"); // Update content type as needed
    } else if (strstr(buffer, "GET /images/polygon.svg ")) {
        serve_image(client_fd, "polygon", polygon, polygon_len, "image/svg+xml"); // Update content type as needed
    } else if (strstr(buffer, "GET /images/rectangle.svg ")) {
        serve_image(client_fd, "rectangle", rectangle, rectangle_len, "image/svg+xml");
    } else if (strstr(buffer, "POST /submit ")) {
        // Extract the request body (form data)
        char *request_body = strstr(buffer, "\r\n\r\n") + 4;
        handle_form_submission(client_fd, request_body);  // Handle form submission
    } else {
        // Serve a 404 Not Found if the endpoint is not recognized
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_fd, not_found_response, strlen(not_found_response), 0);
    }

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Main loop to accept and process requests
    while (1) {
        // Accept incoming connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        handle_client(client_fd);
    }

    return 0;
}
