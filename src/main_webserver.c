#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cjson/cJSON.h>

#include "input_map.h"
#include "output_map.h"
#include "images.h"

#define PORT 5050
#define BUFFER_SIZE 4096

void serve_image(int client_fd, const char *image_name, unsigned char *image_data, unsigned int image_len, const char *content_type) {
    char response[BUFFER_SIZE];
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\n\r\n", content_type, image_len);
    send(client_fd, response, strlen(response), 0);
    send(client_fd, image_data, image_len, 0);
}

void serve_html(int client_fd, const char *html_content, size_t content_length) {
    char response[BUFFER_SIZE];

    // Send HTTP headers
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", content_length);
    send(client_fd, response, strlen(response), 0);

    // Send the embedded HTML content
    send(client_fd, html_content, content_length, 0);
}

void handle_form_submission(int client_fd, char *request_body) {
    char response[BUFFER_SIZE];

    // Log the incoming request body
    printf("Received request body: %s\n", request_body);
    fflush(stdout);

    // Check if the request body is valid
    if (request_body == NULL) {
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Parse JSON data
    cJSON *json = cJSON_Parse(request_body);
    if (json == NULL) {
        printf("Failed to parse JSON.\n");
        fflush(stdout);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Extract data from JSON
    const cJSON *algorithm = cJSON_GetObjectItemCaseSensitive(json, "algorithm");
    const cJSON *bbox = cJSON_GetObjectItemCaseSensitive(json, "bbox");
    const cJSON *start = cJSON_GetObjectItemCaseSensitive(json, "start");
    const cJSON *dest = cJSON_GetObjectItemCaseSensitive(json, "dest");

    // Log the extracted fields
    printf("Extracted JSON fields: algorithm=%s, bbox size=%d, start size=%d, dest size=%d\n",
           algorithm ? algorithm->valuestring : "NULL",
           cJSON_GetArraySize(bbox),
           cJSON_GetArraySize(start),
           cJSON_GetArraySize(dest));
    fflush(stdout);

    // Check if all fields are present
    if (!cJSON_IsString(algorithm) || !cJSON_IsArray(bbox) ||
        !cJSON_IsArray(start) || !cJSON_IsArray(dest)) {
        printf("Missing or invalid fields in JSON.\n");
        fflush(stdout);
        cJSON_Delete(json);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Serve output_map.html with the received data
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    send(client_fd, response, strlen(response), 0);

    // Send the output_map.html content
    // Note: Make sure you have defined output_map_html and output_map_html_len
    send(client_fd, output_map_html, output_map_html_len, 0);

    fflush(stdout);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(client_fd, buffer, sizeof(buffer));

    // Check if it's a GET request for serving images or HTML
    if (strstr(buffer, "GET / ")) {
        serve_html(client_fd, input_map_html, input_map_html_len);
    } else if (strstr(buffer, "GET /images/marker.svg ")) {
        serve_image(client_fd, "marker", marker, marker_len, "image/svg+xml");
    } else if (strstr(buffer, "GET /images/polygon.svg ")) {
        serve_image(client_fd, "polygon", polygon, polygon_len, "image/svg+xml");
    } else if (strstr(buffer, "GET /images/rectangle.svg ")) {
        serve_image(client_fd, "rectangle", rectangle, rectangle_len, "image/svg+xml");
    } else if (strstr(buffer, "GET /submit ")) {
        serve_html(client_fd, output_map_html, output_map_html_len);
    } else if (strstr(buffer, "GET /run ")) {
        // Handle POST request
        // Extract the "Content-Length" header to determine the size of the body
        char *content_length_str = strstr(buffer, "Content-Length: ");
        int content_length = 0;
        if (content_length_str) {
            content_length_str += 16;  // Move the pointer to the start of the number
            content_length = atoi(content_length_str);  // Convert Content-Length to an integer
        }

        // Find the start of the body (after the headers)
        char *request_body = strstr(buffer, "\r\n\r\n");
        if (request_body) {
            request_body += 4;  // Skip the "\r\n\r\n" to get to the body
            int body_length = bytes_read - (request_body - buffer);

            // If the body isn't fully read, read the remaining part of the body
            if (body_length < content_length) {
                int remaining_body_len = content_length - body_length;
                char body_buffer[remaining_body_len + 1];
                int extra_bytes_read = read(client_fd, body_buffer, remaining_body_len);
                body_buffer[extra_bytes_read] = '\0';  // Null-terminate the body
                strcat(request_body, body_buffer);  // Append the extra part to the request body
            }

            // Now handle the form submission with the full body
            handle_form_submission(client_fd, request_body);
        } else {
            // Bad request if we couldn't find the body
            const char *bad_request_response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(client_fd, bad_request_response, strlen(bad_request_response), 0);
        }
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
