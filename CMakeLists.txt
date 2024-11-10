cmake_minimum_required(VERSION 3.29)
project(OpenPathCL C)

set(CMAKE_C_STANDARD 11)

# Include the OpenCL headers from Homebrew
include_directories("/usr/local/opt/opencl-headers/include")

# Find OpenCL package and include the directories
find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

# Find CURL package and include the directories
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find the cJSON library and include directories
find_package(cjson REQUIRED)

# ------ Open CL Check ------
add_executable(OpenCL_check
        src/check_opencl.c)

# Link OpenCL to the parallel version
target_link_libraries(OpenCL_check ${OpenCL_LIBRARIES})

# ------ Serial Dijkstra Version ------

# Add the serial Dijkstra version executable
add_executable(OpenPathCL_serial_dijkstra
        src/main_serial_dijkstra.c
        src/common.c
        src/common.h)

# Link CURL to the serial Dijkstra version
target_link_libraries(OpenPathCL_serial_dijkstra ${CURL_LIBRARIES})

# Add cJSON to the serial Dijkstra version
target_link_libraries(OpenPathCL_serial_dijkstra cjson)

# ------ Serial Delta Version ------

# Add the serial delta stepping version executable
add_executable(OpenPathCL_serial_delta
        src/main_serial_delta.c
        src/common.c
        src/common.h)

# Link CURL to the serial delta stepping version
target_link_libraries(OpenPathCL_serial_delta ${CURL_LIBRARIES})

# Add cJSON to the serial delta stepping version
target_link_libraries(OpenPathCL_serial_delta cjson)

# ------ Parallelizable Version ------

# Add the parallelizable version executable
add_executable(OpenPathCL_parallelizable
        src/main_parallelizable.c
        src/common.c
        src/common.h)

# Link CURL to the parallelizable version
target_link_libraries(OpenPathCL_parallelizable ${CURL_LIBRARIES})

# Add cJSON to the parallelizable version
target_link_libraries(OpenPathCL_parallelizable cjson)

# Link OpenCL to the parallelizable version
target_link_libraries(OpenPathCL_parallelizable ${OpenCL_LIBRARIES})

# ------ Parallel Version ------

# Add the parallel version executable
add_executable(OpenPathCL_parallel
        src/main_parallel.c
        src/common.c
        src/common.h)

# Link CURL to the parallel version
target_link_libraries(OpenPathCL_parallel ${CURL_LIBRARIES})

# Add cJSON to the parallel version
target_link_libraries(OpenPathCL_parallel cjson)

# Link OpenCL to the parallel version
target_link_libraries(OpenPathCL_parallel ${OpenCL_LIBRARIES})

# ------ Webserver ------

# Add the webserver executable
add_executable(OpenPathCL_webserver
        src/main_webserver.c)

# Add custom command to convert input_map.html to C header
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/src/input_map.h
        COMMAND xxd -i -n input_map_html ${CMAKE_SOURCE_DIR}/src/templates/input_map.html ${CMAKE_SOURCE_DIR}/src/input_map.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/templates/input_map.html
        COMMENT "Embedding input_map.html into input_map.h"
)

# Include the generated input_map header file
add_custom_target(embed_input_html DEPENDS ${CMAKE_SOURCE_DIR}/src/input_map.h)
add_dependencies(OpenPathCL_webserver embed_input_html)

# Add custom command to convert output_map.html to C header
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/src/output_map.h
        COMMAND xxd -i -n output_map_html ${CMAKE_SOURCE_DIR}/src/templates/output_map.html ${CMAKE_SOURCE_DIR}/src/output_map.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/templates/output_map.html
        COMMENT "Embedding input_map.html into input_map.h"
)

# Include the generated input_map header file
add_custom_target(embed_output_html DEPENDS ${CMAKE_SOURCE_DIR}/src/output_map.h)
add_dependencies(OpenPathCL_webserver embed_output_html)

# Add a custom command to convert all the images inside the IMAGES list into one images.h file
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/src/images.h
        COMMAND ${CMAKE_COMMAND} -E echo "Generating image header..."
        COMMAND bash ${CMAKE_SOURCE_DIR}/src/embed_images.sh ${CMAKE_SOURCE_DIR}/src/images.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/embed_images.sh
        COMMENT "Embedding images into images.h"
)

# Include the generated image header file
add_custom_target(embed_images DEPENDS ${CMAKE_SOURCE_DIR}/src/images.h)
add_dependencies(OpenPathCL_webserver embed_images)

# Add cJSON to the webserver
target_link_libraries(OpenPathCL_webserver cjson)