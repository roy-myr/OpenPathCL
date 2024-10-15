#include <stdio.h>
#include "common.h"

// Utility function to print the path from source to target using Node IDs
void printPath(int prev[], int target_index, Node nodes[]) {
    if (prev[target_index] == -1) {
        printf("%d", nodes[target_index].id);
        return;
    }
    printPath(prev, prev[target_index], nodes);
    printf(" -> %d", nodes[target_index].id);
}
