# GPU Sorting Algorithms in OpenCL

This is the result of a freestyle assignment for a GPU-Computing class at KIT in Summer 2016.
Some of the source code (mostly the common code folder) was provided.
I coded the sorting algorithms mergesort, bitonic mergesort and a bubble sort sorting network for OpenCL-
Also there is a implementation of mergesort on CPU for comparison.
See [Sort.cl](Code/Sort.cl) for Kernel-Code, [CSortTask.cpp](Code/CSortTask.cpp) for most of the host code and [CSortingMain.cpp](Code/CSortingMain.cpp) for changing local work size and array size.


## Standard Mergesort for GPU
Mostly not recommended because it does not benefit that well from parallelization. With bigger array size the graphics card will eventually run out of memory.

## Bubble Sort Sorting Network
Called Simple Sorting Network (SSN) in Code. Not recommended because it is inefficient and slow.

## Bitonic Mergesort
Recommended sorting variant of these three. Is fast and benefits well from parallelization.


## How to Build
Best way is to use cmake with the [Code](Code/) folder as source folder. Use a 64-Bit compiler as otherwise bigger array sizes won't work.

## Measurements
All measurements with randomly generated arrays of the given size. Times are in milliseconds.

Size             | CPU      | Mergesort        |             | SSN        |             | bitonic      | Mergesort
---------------: | -------: | ---------------: | ----------: | ---------: | ----------: | -----------: | --------:
-                | Time     |  Time            |  Speedup    |  Time      | Speedup     |  Time        | Speedup
4 * 1024         | 0.313    |  2.365           |  0.132      |  0.996     | 0.314       |  0.434       | 0.721
64 * 1024        | 6.906    |  36.999          |  0.187      |  48.628    | 0.142       |  0.872       | 7.719
256 * 1024       | 28.498   |  177.689         |  0.160      |  703.28    | 0.041       |  2.018       | 14.122
2 * 1024 * 1024  | 253.282  |  1450.660        |  0.175      | -          | -           |  24.680      | 10.262
64 *1024 * 1024  | 8861.330 | -                | -           | -          | -           |  1192.820    | 7.429
