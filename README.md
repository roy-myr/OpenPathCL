# OpenPathCL

Route planing based on Open Street Map data. Parallelized with OpenCL.


## Background

This project was part of a portfolio exam in the Parallel Systems module at the Nordakademie.
The task was to implement two route planning algorithms in C, one serial and one parallel, and to compare their performance.

## Requirements

> [!IMPORTANT]
> This project uses the OpenCL, Libcurl and cJSON libraries. They must be installed to use this project.

> [!NOTE]
> This code has only been tested on macOS. It may need to be adapted to run on your platform.


## How it Works

The route planning process is broken down into four essential steps:


### Step 1: Getting the Data

The data required for route planning is sourced from the [Overpass API](https://overpass-api.de/), a powerful tool 
that allows users to query and extract specific geographic data from the OpenStreetMap (OSM) database. 
The Overpass API provides flexible queries to retrieve **nodes** and **ways** (roads) within a bounding box, 
which is defined using the user-provided start and end coordinates.

In OSM, roads are represented as **ways** — a sequence of nodes, where each node corresponds to a geographical point 
with specific latitude and longitude coordinates. These nodes define the road's shape and structure.

Once the data is retrieved, it is parsed and stored in two arrays of structs: 
one for **nodes** (vertices) and one for **roads** (edges). Nodes represent the points, and roads connect these points.


### Step 2: Constructing the Graph

After parsing the data, it is converted into a graph representation so that algorithms can be run on it. 
The graph is modeled as an [adjacency list](https://en.wikipedia.org/wiki/Adjacency_list), where each node contains 
a linked list of edges, wich are connected to it.

- Each node corresponds to a vertex in the graph.
- Roads provide the information needed to define edges between the nodes (vertices).
- If a node is part of a road, an edge exists between that node and the next node along the road. 
  This edge is bidirectional, meaning it represents two-way movement.

The [Haversine algorithm](https://en.wikipedia.org/wiki/Haversine_formula) is used to calculate the distance between 
two nodes, and this distance serves as the weight of the edge connecting them. 
These weights are stored in the edge struct inside the adjacency list.


### Step 3: Calculating the shortest distance

The shortest path between two nodes is calculated using a *Single-Source Shortest Paths (SSSP)* algorithm.

- For the *serial* implementation, the [Dijkstra algorithm](https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm) 
  is employed to find the shortest path.
- In the *parallel* implementation, [Delta-Stepping](https://en.wikipedia.org/wiki/Parallel_single-source_shortest_path_algorithm), 
  a parallelized version of Dijkstra's algorithm, is used for improved performance on larger graphs.

These algorithms work by progressively exploring nodes, calculating the minimal cumulative distance from the start node 
to the destination node, while updating the shortest known distances.


### Step 4: Displaying the Result

Once the shortest path is computed, the result is displayed in two ways:

- *Terminal Output*: The list of nodes forming the path is printed to the terminal, showing the sequence of nodes 
  traversed to reach the destination.
- *Map Display*: The path is also visualized on an interactive map using [Leaflet](https://leafletjs.com/), 
  a lightweight JavaScript library for displaying maps using OSM data. The map shows the path by plotting the geographic 
  coordinates (latitude and longitude) of each node along the computed route.

An HTML page with embedded JavaScript code is used to display the map, and the path is drawn by connecting the 
corresponding nodes on the map.


## Serial vs Parallel


### Differences


### Performance Comparison


## Improvements

### Larger distances:

At the moment the Algorithm can not handle really high distances. # ToDo: Find limit and reason

### Bounding Box:
At the moment the bounding box, so the area where Nodes and Ways are searched in, is only within a 300m radius within 
the direct path between that start and end koordinate. In Some cases this is not enough since there is no a valid path 
inside this region. The Bounding box would need to be improved to fix this issue.

### Selection of Ways
At the Moment the ways that are taken into account when planning the route are not really practical. They include 
such ways that can only be taken by foot as well as highways wich definitely should not be taken by foot. So it`s 
neither a good route to take by car, nor is it one to walk. The Request to the Overpass API would have to be adjusted 
to filter out only the way types that are wanted.

### One-Ways
At the moment every way is considered two-ways. One-way roads would need to be detected and the adjacency matrix would 
have to represent them.

### Better weight
At the moment the weight for the graph and the route planning algorithm is the distance between the nodes. Often this 
is not really the value that is interesting for a user. The user wants to take the route that takes the least amount 
of time, not the one that is the shortest distance. The weight would have to be adjusted to take this into account.
To do that the time that it takes to travel between weights could be calculated using the speed limit and the distance.
However sometimes the speed limit is not saved inside the OSM database wich would be a problem.

### User Input
At the moment a user has to enter the start and destination coordinates. This is not really practically. Alternatives 
could be to enter an address or use a map to select a start and destination.