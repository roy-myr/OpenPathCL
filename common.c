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