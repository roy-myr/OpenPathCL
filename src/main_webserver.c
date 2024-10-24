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
#define PATH_MAX 1024

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

// Function to get the current working dir
char *get_current_directory() {
    static char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return cwd;
    } else {
        perror("getcwd() error");
        return NULL;
    }
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

    // determine the path of the program
    const char *program_name = NULL;
    char full_program_path[PATH_MAX];
    char *cwd = get_current_directory();
    if (cwd == NULL) {
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to determine current directory.");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Construct the full path to the executable
    if (strcmp(algorithm->valuestring, "parallel") == 0) {
        snprintf(full_program_path, PATH_MAX, "%s/OpenPathCL_parallel", cwd);
    } else if (strcmp(algorithm->valuestring, "serial") == 0) {
        snprintf(full_program_path, PATH_MAX, "%s/OpenPathCL_serial", cwd);
    } else {
        printf("Invalid algorithm specified.\n");
        fflush(stdout);
        cJSON_Delete(json);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Calculate the number of arguments
    int bbox_size = cJSON_GetArraySize(bbox);
    int num_bbox_args = bbox_size * 2;  // Each bounding box entry is a pair of coordinates (lat, lng)
    int num_args = 4 + num_bbox_args;   // start(2), dest(2), bbox(num_bbox_args), plus the program name

    // Allocate memory for the argument array dynamically
    char **args = malloc((num_args + 2) * sizeof(char *));  // +2 for program name and NULL termination
    int arg_idx = 0;

    // First argument: Program name (with full path)
    args[arg_idx++] = strdup(full_program_path);

    // Add start coordinates
    cJSON *start_lat = cJSON_GetArrayItem(start, 0);
    cJSON *start_lng = cJSON_GetArrayItem(start, 1);
    if (cJSON_IsNumber(start_lat) && cJSON_IsNumber(start_lng)) {
        char start_lat_str[20], start_lng_str[20];
        sprintf(start_lat_str, "%f", start_lat->valuedouble);
        sprintf(start_lng_str, "%f", start_lng->valuedouble);

        args[arg_idx++] = strdup(start_lat_str);
        args[arg_idx++] = strdup(start_lng_str);
    } else {
        printf("Invalid start coordinates.\n");
        fflush(stdout);
        cJSON_Delete(json);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        free(args);
        return;
    }

    // Add destination coordinates
    cJSON *dest_lat = cJSON_GetArrayItem(dest, 0);
    cJSON *dest_lng = cJSON_GetArrayItem(dest, 1);
    if (cJSON_IsNumber(dest_lat) && cJSON_IsNumber(dest_lng)) {
        char dest_lat_str[20], dest_lng_str[20];
        sprintf(dest_lat_str, "%f", dest_lat->valuedouble);
        sprintf(dest_lng_str, "%f", dest_lng->valuedouble);

        args[arg_idx++] = strdup(dest_lat_str);
        args[arg_idx++] = strdup(dest_lng_str);
    } else {
        printf("Invalid destination coordinates.\n");
        fflush(stdout);
        cJSON_Delete(json);
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        free(args);
        return;
    }

    // Add bounding box coordinates (each bbox element is a [lat, lng] pair)
    for (int i = 0; i < bbox_size; i++) {
        cJSON *bbox_pair = cJSON_GetArrayItem(bbox, i);
        if (cJSON_IsArray(bbox_pair) && cJSON_GetArraySize(bbox_pair) == 2) {
            cJSON *bbox_lat = cJSON_GetArrayItem(bbox_pair, 0);  // Latitude
            cJSON *bbox_lng = cJSON_GetArrayItem(bbox_pair, 1);  // Longitude

            if (cJSON_IsNumber(bbox_lat) && cJSON_IsNumber(bbox_lng)) {
                char bbox_lat_str[20], bbox_lng_str[20];
                sprintf(bbox_lat_str, "%f", bbox_lat->valuedouble);
                sprintf(bbox_lng_str, "%f", bbox_lng->valuedouble);

                args[arg_idx++] = strdup(bbox_lat_str);
                args[arg_idx++] = strdup(bbox_lng_str);
            } else {
                printf("Invalid bounding box coordinates.\n");
                fflush(stdout);
                cJSON_Delete(json);
                sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
                send(client_fd, response, strlen(response), 0);
                free(args);
                return;
            }
        } else {
            printf("Invalid bounding box structure.\n");
            fflush(stdout);
            cJSON_Delete(json);
            sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
            send(client_fd, response, strlen(response), 0);
            free(args);
            return;
        }
    }

    // Null-terminate the argument list for execvp
    args[arg_idx] = NULL;

    // Print the arguments before calling execvp
    printf("Program to execute:\n");
    for (int i = 0; i < arg_idx; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
    fflush(stdout);

    // Create a pipe to capture the output of the external program
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\nPipe creation failed.");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Fork a child process to execute the external program
    pid_t pid = fork();
    if (pid == 0) {
        // In child process
        close(pipefd[0]);  // Close the read end of the pipe

        // Redirect stdout to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);  // Close the write end after dup2

        execvp(args[0], args);  // Replace the current process image with the new one

        // If execvp returns, there was an error
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Fork failed
        perror("fork failed");
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        send(client_fd, response, strlen(response), 0);
        return;
    } else {
        // In parent process, wait for the child process to complete
        close(pipefd[1]);  // Close the write end of the pipe
        int status;

        // Wait for the child process to finish
        waitpid(pid, &status, 0);

        // Check if the program executed successfully
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Success: capture the output from the pipe
            char buffer[BUFFER_SIZE];
            ssize_t count = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (count > 0) {
                buffer[count] = '\0';  // Null-terminate the output string

                // Send the response to the client
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
                send(client_fd, response, strlen(response), 0);
                send(client_fd, buffer, count, 0);  // Send the captured JSON output
            } else {
                // No output or error reading from pipe
                sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to read output.");
                send(client_fd, response, strlen(response), 0);
            }
        } else {
            // Error during execution
            sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\nExecution failed.");
            send(client_fd, response, strlen(response), 0);
        }

        close(pipefd[0]);  // Close the read end of the pipe
    }

    // Clean up
    cJSON_Delete(json);
    for (int i = 0; i < arg_idx; i++) {
        free(args[i]);  // Free the dynamically allocated argument strings
    }
    free(args);  // Free the argument array

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
    } else if (strstr(buffer, "POST /run ")) {
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
