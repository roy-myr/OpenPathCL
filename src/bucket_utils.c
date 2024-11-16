#include <stdio.h>
#include <stdlib.h>
#include "bucket_utils.h"

// Function to initialize the BucketsArray
void initializeBuckets(BucketsArray *bucketsArray) {
    bucketsArray->buckets = NULL;
    bucketsArray->bucketSizes = NULL;
    bucketsArray->numBuckets = 0;
}

// Function to resize the outer buckets array
void resizeBuckets(BucketsArray *bucketsArray, const int newSize) {
    bucketsArray->buckets = (int **)realloc(bucketsArray->buckets, newSize * sizeof(int *));
    bucketsArray->bucketSizes = (int *)realloc(bucketsArray->bucketSizes, newSize * sizeof(int));
    for (int i = bucketsArray->numBuckets; i < newSize; i++) {
        bucketsArray->buckets[i] = NULL;    // Initialize new buckets to NULL
        bucketsArray->bucketSizes[i] = 0;   // Initialize sizes to 0
    }
    bucketsArray->numBuckets = newSize;
}

// Function to add a node to a specific bucket, resizing if necessary
void addNodeToBucket(BucketsArray *bucketsArray, const int bucketIndex, const int node_id) {
    // Resize buckets array if needed
    if (bucketIndex >= bucketsArray->numBuckets) {
        resizeBuckets(bucketsArray, bucketIndex + 1);
    }

    // Get the current size of the bucket
    int currentSize = bucketsArray->bucketSizes[bucketIndex];

    // Resize the specific bucket to add another node
    bucketsArray->buckets[bucketIndex] = (int *)realloc(bucketsArray->buckets[bucketIndex], (currentSize + 1) * sizeof(int));
    bucketsArray->buckets[bucketIndex][currentSize] = node_id; // Add the new node
    bucketsArray->bucketSizes[bucketIndex]++;  // Update the size
}

// Function to free the bucket memory
void freeBuckets(BucketsArray *bucketsArray) {
    if (bucketsArray == NULL || bucketsArray->buckets == NULL) {
        return; // Nothing to free if the pointer is NULL
    }

    // Free each bucket's allocated memory
    for (int i = 0; i < bucketsArray->numBuckets; i++) {
        free(bucketsArray->buckets[i]);
    }

    // Free the array of buckets
    free(bucketsArray->buckets);
    bucketsArray->buckets = NULL; // Set to NULL to avoid dangling pointers
}

// Function to print a bucket's content
void printBucket(const int *bucket, const int size) {
    if (bucket == NULL || size == 0) {
        printf("Bucket is empty.\n");
        return;
    }
    printf("Bucket contents: [");
    for (int i = 0; i < size; i++) {
        printf("%d ", bucket[i]);
    }
    printf("]\n");
}
