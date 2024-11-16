# ![OpenPathCL icon](./src/templates/images/favicon.png) OpenPathCL 

Route planing based on Open Street Map data. Parallelized with OpenCL.


## Background

This project was part of a portfolio exam in the Parallel Systems module at the Nordakademie.
The task was to implement two route planning algorithms in C, one serial and one parallel, and to compare their performance.

## Requirements

> [!IMPORTANT]
> This project uses the OpenCL, Libcurl and cJSON libraries. They must be installed to use this project.

> [!NOTE]
> This code has only been tested on macOS 15.1. It may need to be adapted to run on your platform.

## OpenCL Version

In order to run this code on your platform you need to use the correct OpenCL version, platform and device. 
The OpenCL_check can help you with that. It returned the following data on the used Hardware:

| ID    | Type         | Name                                          | Supported Version | Max Work Group Size | Max Work Item Sizes | Global Mem Size (MB) | Max Mem Alloc Size (MB) |
|-------|--------------|-----------------------------------------------|-------------------|---------------------|---------------------|----------------------|-------------------------|
| 0     | CPU          | Intel(R) Core(TM) i9-9880H CPU @ 2.30GHz      | OpenCL 1.2        | 1024                | 1024 x 1 x 1        | 32768.00             | 8192.00                 |
| 1     | GPU          | Intel(R) UHD Graphics 630                     | OpenCL 1.2        | 256                 | 256 x 256 x 256     | 1536.00              | 384.00                  |
| 2     | GPU          | AMD Radeon Pro 5500M Compute Engine           | OpenCL 1.2        | 256                 | 256 x 256 x 256     | 8176.00              | 2044.00                 |


## How it Works

**OpenPathCL** consists of a **Webserver** used for the user Inputs 
as well as multiple executables for the algorithms for the route planning.

### The Webserver

The web server for OpenPathCL provides the frontend of the application. Below are the main functionalities:

### Frontend Interface:

- A map interface (powered by [Leaflet](https://leafletjs.com/) and [Leaflet Draw](https://github.com/Leaflet/Leaflet.draw)) 
  allows users to select start and end points, draw a bounding box (rectangle or polygon), and choose the algorithm.
- Tooltips explain the interface and steps for ease of use.
- Validates input and disables the "Send Data" button if no valid points are within the bounding box.

### Route Calculation:

- Inputs (start, end points, bounding box, algorithm) are saved locally and sent to the server for processing.
- The server executes the selected algorithm and processes the route based on input coordinates.
- Results are returned in JSON format and displayed on the output map with a loading screen during processing.


### Output and Comparison:
- The output map shows the calculated route, coordinates, and metadata (e.g., route length, OpenStreetMap Node IDs, runtime statistics).
- Users can re-run the same request with other algorithms for comparison.

### Save Results:
- Results, including an interactive map and input/output data, can be saved locally as an HTML file.


### The Route Planning

The route planning process can be broken down into four essential steps:


#### Step 1: Getting the Data

The data required for route planning is sourced from the [Overpass API](https://overpass-api.de/), a powerful tool 
that allows users to query and extract specific geographic data from the OpenStreetMap (OSM) database. 
The Overpass API provides flexible queries to retrieve **nodes** and **ways** (roads) within a bounding box, 
which is defined using the user-provided start and end coordinates.

In OSM, roads are represented as **ways** — a sequence of nodes, where each node corresponds to a geographical point 
with specific latitude and longitude coordinates. These nodes define the road's shape and structure.

Once the data is retrieved, it is parsed and stored in two arrays of structs: 
one for **nodes** (vertices) and one for **roads** (edges). Nodes represent the points, and roads connect these points.


#### Step 2: Constructing the Graph

After parsing the data, it is converted into a graph representation so that algorithms can be run on it. 
The graph is modeled as an [adjacency list](https://en.wikipedia.org/wiki/Adjacency_list), where each node contains 
a linked list of edges, wich are connected to it.

- Each node corresponds to a vertex in the graph.
- Roads provide the information needed to define edges between the nodes (vertices).
- If a node is part of a road, an edge exists between that node and the next node along the road. 
  This edge is bidirectional, meaning it represents two-way movement.
- For the `parallel` and `parallizable` Algorithms the adjacency list is further converted into a data structure that 
  can be handled by OpenCL.

The [Haversine algorithm](https://en.wikipedia.org/wiki/Haversine_formula) is used to calculate the distance between 
two nodes, and this distance serves as the weight of the edge connecting them. 
These weights are stored in the edge struct inside the adjacency list.


#### Step 3: Calculating the shortest distance

The shortest path between two nodes is calculated using a *Single-Source Shortest Paths (SSSP)* algorithm. 
Four different Algorithms where implemented.

- A *serial* [Dijkstra algorithm](https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm) called `serial_dijkstra`
- A *serial* [Delta-Stepping](https://en.wikipedia.org/wiki/Parallel_single-source_shortest_path_algorithm) called `serial_delta`
- A *serial* Delta-Stepping Algorithm that is prepared to be parallelized called `parallelizable`
- A *parallel* Delta-Stepping Algorithm that was implemented using OpenCL called `parallel`

These algorithms work by progressively exploring nodes, calculating the minimal cumulative distance from the start node 
to the destination node, while updating the shortest known distances.


#### Step 4: Outputting the Result

Once the shortest path is computed, the result is displayed by printing it as a JSON format to the terminal. 
It contains the following data:
```
{
  "startNode": <OSM Node ID>,
  "destNode": <OSM Node ID>,
  "nodesRequest": <Link to your request in Overpass Turbo>,
  "nodesInBoundingBox": <# of OSM Nodes in the Bounding Box>,
  "roadsInBoundingBox": <# of OSM Ways in the Bounding Box>,
  "graphTime": <time of the graph construction in ms>,
  "route": [[53.754658, 9.659376], [53.754673, 9.659469], [53.754700, 9.659646], [53.754723, 9.659819], [53.754723, 9.659968], [53.754719, 9.660052], [53.754734, 9.660138], [53.754723, 9.660207], [53.754742, 9.660327], [53.754784, 9.660624], [53.754864, 9.661155], [53.754913, 9.661497], [53.755009, 9.662143], [53.755096, 9.662725], [53.755108, 9.662831], [53.755146, 9.663155], [53.755203, 9.663681], [53.755245, 9.664149], [53.755257, 9.664288], [53.755138, 9.664310], [53.755154, 9.664432], [53.755104, 9.664442], [53.754951, 9.664523], [53.754852, 9.664608], [53.754803, 9.664664], [53.754730, 9.664741], [53.754658, 9.664875], [53.754612, 9.664978], [53.754539, 9.665108], [53.754463, 9.665253], [53.754292, 9.665545], [53.754162, 9.665758], [53.753956, 9.666151], [53.754303, 9.666784], [53.754147, 9.667552], [53.754047, 9.667919], [53.753910, 9.668625], [53.753819, 9.669232], [53.753799, 9.669564], [53.753960, 9.670014], [53.754120, 9.670578], [53.754192, 9.670688], [53.754166, 9.670873], [53.754150, 9.671057], [53.754250, 9.671284], [53.754311, 9.671404], [53.754372, 9.671523], [53.754456, 9.671798], [53.754459, 9.671914], [53.754498, 9.671979], [53.754528, 9.672123]],
  "routeLength": <distance of the shortest path>,
  "routingTime": <time of the routing algorithm in ms>,
  "totalTime": <total runtime in ms>,
  "success": true
}
```
