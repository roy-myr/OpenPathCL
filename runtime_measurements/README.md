# Runtime Measurements

This directory contains runtime measurements of 5 example requests in all the 4 algorithms.
The HTML-Files of the results where generated using the "Save Results" Button of the Output Map.

## Example Requests
| Request # | Route                                                                            |
|----------:|:---------------------------------------------------------------------------------|
|         1 | Short distance from Nordakademie to the train station with a tight bounding box. |
|         2 | Short distance from Nordakademie to the train station with a large bounding box. |
|         3 | Medium distance from SFSU to the Cheesecake Factory.                             |
|         4 | Long distance from the quay in Brake to NKT.                                     |
|         5 | Long distance from Nordakademie (Elmshorn) to Nordakademie (Dockland).           |

## Results

| Request | Number of Nodes | Number of Streets | Route Length | Dijkstra Graph Time | Dijkstra Route Time | Δ-Stepping Graph Time | Δ-Stepping Route Time | Parallelizable Graph Time | Parallelizable Route Time | Parallel Graph Time | Parallel Route Time |
|--------:|----------------:|------------------:|-------------:|--------------------:|--------------------:|----------------------:|----------------------:|--------------------------:|--------------------------:|--------------------:|--------------------:|
|       1 |            1486 |               400 |      974,06m |              0,004s |              0,005s |                0,004s |                0,000s |                    0,004s |                    0,001s |              0,004s |              0,073s |
|       2 |           63815 |             14381 |      982,94m |              4,287s |              0,262s |                4,497s |                0,011s |                    5,031s |                    0,092s |              4,230s |              0,326s |
|       3 |          192617 |             50343 |    10756,87m |             43,209s |             57,114s |               44,811s |                0,040s |                   46,723s |                    0,783s |             43,111s |              1,009s |
|       4 |           17216 |              3605 |    22634,78m |              0,330s |              0,511s |                0,308s |                0,003s |                    0,337s |                    0,023s |              0,306s |              0,217s |
|       5 |          194568 |             58413 |    32104,54m |             42,811s |             64,264s |               40,586s |                0,038s |                   44,121s |                    0,720s |             41,730s |              1,120s |

## Takeaways
- **Reproducibility**: For accurate analysis, measurements should ideally be repeated and averaged. However, due to minimal variations in graph creation times, each request was executed only once for simplicity.
- **Graph Size vs. Distance**: The distance between the start and end points does not impact the complexity of route planning. Instead, the number of nodes in the graph is the critical factor. For example, a longer route with fewer nodes can execute faster than a shorter route with a denser graph.
- **Graph Creation Time**: Except for Dijkstra's algorithm, graph creation consistently takes more time than the actual route planning. For Dijkstra, this is true only when using unnecessarily large bounding boxes.
- **Algorithm Efficiency**:
  - The serial Delta-Stepping algorithm was the fastest and most efficient solution across all tests.
  - Parallelization using OpenCL proved inefficient due to the absence of an early termination condition and the use of less efficient data structures for compatibility.
  - The fully parallel algorithm was consistently slower than the parallelizable version due to the additional overhead of data transfer.
- **Key Insight**: Parallelization adds overhead that outweighs its benefits, making serial Delta-Stepping the optimal choice for these scenarios.