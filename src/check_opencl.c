#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

int main(int argc, char* const argv[]) {
    cl_uint num_devices, i;
    clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);

    cl_device_id* devices = calloc(num_devices, sizeof(cl_device_id));
    clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);

    // Print the Markdown table header
    printf("| %-5s | %-12s | %-45s | %-17s | %-19s | %-19s | %-20s | %-23s |\n",
           "ID", "Type", "Name", "Supported Version", "Max Work Group Size",
           "Max Work Item Sizes", "Global Mem Size (MB)", "Max Mem Alloc Size (MB)");
    printf("|-------|--------------|-----------------------------------------------|-------------------|---------------------|---------------------|----------------------|-------------------------|\n");

    char name_buf[128];
    char version_buf[128];
    for (i = 0; i < num_devices; i++) {
        // Get device name
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(name_buf), name_buf, NULL);

        // Get device type
        cl_device_type device_type;
        clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);

        // Determine human-readable device type
        char* device_type_str;
        if (device_type & CL_DEVICE_TYPE_CPU) {
            device_type_str = "CPU";
        } else if (device_type & CL_DEVICE_TYPE_GPU) {
            device_type_str = "GPU";
        } else if (device_type & CL_DEVICE_TYPE_ACCELERATOR) {
            device_type_str = "Accelerator";
        } else if (device_type & CL_DEVICE_TYPE_DEFAULT) {
            device_type_str = "Default";
        } else {
            device_type_str = "Unknown";
        }

        // Get device version
        clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, sizeof(version_buf), version_buf, NULL);

        // Get maximum work group size
        size_t max_work_group_size;
        clGetDeviceInfo(devices[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);

        // Get maximum work item sizes
        size_t max_work_item_sizes[3];
        clGetDeviceInfo(devices[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item_sizes), max_work_item_sizes, NULL);

        // Format max work item sizes as a string
        char work_item_sizes_str[50];
        snprintf(work_item_sizes_str, sizeof(work_item_sizes_str), "%zu x %zu x %zu",
                 max_work_item_sizes[0], max_work_item_sizes[1], max_work_item_sizes[2]);

        // Get global memory size
        cl_ulong global_mem_size;
        clGetDeviceInfo(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
        double global_mem_size_mb = global_mem_size / (1024.0 * 1024.0);

        // Get maximum memory allocation size
        cl_ulong max_mem_alloc_size;
        clGetDeviceInfo(devices[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_mem_alloc_size), &max_mem_alloc_size, NULL);
        double max_mem_alloc_size_mb = max_mem_alloc_size / (1024.0 * 1024.0);

        // Print the device information in Markdown table format with fixed-width fields
        printf("| %-5u | %-12s | %-45s | %-17s | %-19zu | %-19s | %-20.2f | %-23.2f |\n",
               i, device_type_str, name_buf, version_buf, max_work_group_size, work_item_sizes_str,
               global_mem_size_mb, max_mem_alloc_size_mb);
    }

    free(devices);
    return 0;
}
