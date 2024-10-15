cmake_minimum_required(VERSION 3.29)
project(OpenPathCL C)

set(CMAKE_C_STANDARD 11)

# Find OpenCL package and include the directories
# find_package(OpenCL REQUIRED)
# include_directories(${OpenCL_INCLUDE_DIRS})

# Add the serial version executable
add_executable(OpenPathCL_serial
        main_serial.c
        common.c
        common.h)
target_link_libraries(OpenPathCL_serial winhttp)

# Add the parallel version executable
add_executable(OpenPathCL_parallel
        main_parallel.c
        common.c
        common.h)
# target_link_libraries(OpenPathCL_parallel ${OpenCL_LIBRARIES})
