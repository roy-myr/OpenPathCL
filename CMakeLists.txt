cmake_minimum_required(VERSION 3.29)
project(OpenPathCL C)

set(CMAKE_C_STANDARD 11)

# Find OpenCL package and include the directories
find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

# Find CURL package and include the directories
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Add the serial version executable
add_executable(OpenPathCL_serial
        main_serial.c
        common.c
        common.h)

# Link CURL to the serial version
target_link_libraries(OpenPathCL_serial ${CURL_LIBRARIES})

# Add the parallel version executable
add_executable(OpenPathCL_parallel
        main_parallel.c
        common.c
        common.h)

# Link CURL to the parallel version
target_link_libraries(OpenPathCL_parallel ${CURL_LIBRARIES})

# Link OpenCL to the parallel version
target_link_libraries(OpenPathCL_parallel ${OpenCL_LIBRARIES})
