# CONTAINER
Personal lightweight memory-based data structure for kernel developments since they get some strict mem control.
Mainly used for disk-based container as an abstraction of database storage interface.

All code is in headers files, just grab and include them. Modify `definition.h` for various momory context compatibility.

Compiled and tested in c++23. c++20 is required to compile them at minimum, but it only takes one small fix to support c++11.

### TODO:
1. concurrent lambda hash map
2. thread pool (all databases have their own thread control, the major problem is about session and thread contexts)

## Benchmark Result
Benchmark code is in each folder of corresponding implementation.

#### Concurrent LRU Cache
ConcurrentLRUArrayCache is an array-based LRU cache which move an accessed element some steps (expected to be cache_size / nsegment) away from eviction rather than placing it at the end of list. So it somehow acts like an LRU-k-ish (k = nsegment - 1) cache but still init an element at the end of queue.

The ConcurrentLRUArrayCache requires about 1.6 times more cache size and additional nsegment(default 8) * cache_size * sizeof(uint32_t) space to how much list-based LRU cache takes in order to achieve similar hit rate.
|      Task      | ConcurrentLRUArrayCache | ConcurrentLRUCache | boost lru_cache<br>+<br>mutex | LRUCache<br>+<br>mutex |
|:--------------:|:-----------------------:|:------------------:|:-----------------------------:|:----------------------:|
| access 32 * 1m |         18.0701s        |      207.985s      |            349.722s           |        147.642s        |

#### LRU cache
|     Test    |   LRUCache  | boost lru_cache |
|:-----------:|:-----------:|:---------------:|
| access 10m  |  0.186879s  |    2.42022s     |

#### B+Tree
|           Test          |  BPTree  | std::map |
|:-----------------------:|:--------:|:--------:|
| insert+search+remove 5m | 3.26705s | 9.29756s |

#### IntervalSet
|             Test           | IntervalSet | boost::icl::interval_set |
|:--------------------------:|:-----------:|:------------------------:|
|  insert+contain range 5m   |   1.92069s  |         3.46236s         |
|  insert+contain random 5m  |  0.497938s  |         1.85847s         |

#### ConcurrentList
ConcurrentList keeps iterator valid after move_back while std::list only support erase + push_back a new element
|       Test      |   ConcurrentList  | std::list + std::mutex |
|:---------------:|:-----------------:|:----------------------:|
| push_back 50*1m |      3.71496s     |        19.8754s        |
| move_back 50*1m |      38.567s      |        27.2621s        |
|   erase 50*1m   |      0.985751s    |        29.1263s        |

#### List
|      Test       | List      | DLList    | std::list |
|:---------------:|:---------:|:---------:|:---------:|
| push + pop 100m | 2.65554s  | 1.57707s  | 1.86868s  |

#### Vector
|           Test            | Vector    | std::vector |
|:-------------------------:|:---------:|:-----------:|
| push_back + pop_back 100m | 0.1233s   | 0.190329s   |
