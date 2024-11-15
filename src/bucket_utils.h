#ifndef BUCKET_UTILS_H
#define BUCKET_UTILS_H

// Define a struct to manage dynamic buckets used in delta stepping
typedef struct {
    int **buckets;  // Array of integer arrays (each array is a bucket)
    int *bucketSizes;  // Array to store the size (number of nodes) in each bucket
    int numBuckets;  // Number of buckets currently allocated
} BucketsArray;

// Bucket functions
void initializeBuckets(BucketsArray *bucketsArray);
void resizeBuckets(BucketsArray *bucketsArray, const int newSize);
void addNodeToBucket(BucketsArray *bucketsArray, const int bucketIndex, const int node_id);
void freeBuckets(BucketsArray *bucketsArray);

// Debug functions
void printBucket(const int *bucket, const int size);

#endif //BUCKET_UTILS_H
