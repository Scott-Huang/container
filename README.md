# CONTAINER
Personal lightweight memory-based data structure for kernel developments since they get some strict mem control.
Mainly used for disk-based container as an abstraction of database storage interface.

### TODO:
1. concurrent lru cache (imagine concurrent but with log sync since we want usability...)
2. parallel hashmap (necessary for point 1, but we can use some existing tool to verify its worth)
3. coroutine generator for disk container (hard to make it useful since the container does not have file handler either)
4. thread pool (of course all databases have their own thread control, the major problem is about session and thread contexts)

## Benchmark Result
Benchmark code is in each folder of implementation code.

#### Vector
|           Test            | Vector    | std::vector |
|:-------------------------:|:---------:|:-----------:|
| push_back + pop_back 100m | 0.1233s   | 0.190329s   |

#### List
|      Test       | List      | DLList    | std::list |
|:---------------:|:---------:|:---------:|:---------:|
| push + pop 100m | 2.65554s  | 1.57707s  | 1.86868s  |

#### B+Tree
|            Test           | BPTree   | std::map |
|:-------------------------:|:-------:|:--------:|
| insert+search+remove 500w | 3.26705s | 9.29756s |

#### IntervalSet
|                Test               | IntervalSet | boost::icl::interval_set |
|:---------------------------------:|:-----------:|:------------------------:|
|     insert+contain range 500w     |   1.92069s  |         3.46236s         |
|     insert+contain random 500w    |  0.497938s  |         1.85847s         |
