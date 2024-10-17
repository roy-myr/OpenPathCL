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

## How it works
The process of the route planning can be broken down into 4 basic steps.

### Step 1: Getting the Data
The Data for the Route planning is gathered by requesting it from the Overpass API public instance.
The [Overpass API](https://overpass-api.de/) is a powerful tool that allows users to query and extract specific data 
from the OpenStreetMap (OSM) database using a flexible query language. 
It is used to get all the nodes and ways inside a bounding box that 
is constructed using the user given start and endpoint coordinates.

In the OpenStreetMap (OSM) database, a road is represented as a way, which is a sequence of multiple nodes, 
where each node is a point defined by a specific longitude and latitude coordinate.

This requested data is parsed and stored in two arrays of structs. One for the nodes and one for the roads.

### Step 2: Constructing the Graph
After the data has been requested and parsed, it has to be represented as a graph in oder to run the dijkstra algorithm.
The Graph representation is done as a nxn matrix. The deminsion n is equal to the number of nodes.
A node represents a vertices of the graph. 

