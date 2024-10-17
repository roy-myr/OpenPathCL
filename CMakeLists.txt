cmake_minimum_required(VERSION 3.29)
project(OpenPathCL C)

set(CMAKE_C_STANDARD 11)

# Find OpenCL package and include the directories
# find_package(OpenCL REQUIRED)
# include_directories(${OpenCL_INCLUDE_DIRS})

# Find CURL package and include the directories
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find the cJSON library and include directories
find_package(cjson REQUIRED)

# Add the serial version executable
add_executable(OpenPathCL_serial
        src/main_serial.c
        src/common.c
        src/common.h)

# Link CURL to the serial version
target_link_libraries(OpenPathCL_serial ${CURL_LIBRARIES})

# Add cJSON to the serial version
target_link_libraries(OpenPathCL_serial cjson)

# Add the parallel version executable
add_executable(OpenPathCL_parallel
        src/main_parallel.c
        src/common.c
        src/common.h)

# Link CURL to the parallel version
target_link_libraries(OpenPathCL_parallel ${CURL_LIBRARIES})

# Link OpenCL to the parallel version
target_link_libraries(OpenPathCL_parallel ${OpenCL_LIBRARIES})

# Add cJSON to the parallel version
target_link_libraries(OpenPathCL_serial cjson)