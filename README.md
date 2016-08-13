# GPU Sorting Algorithms in OpenCL

This is the result of a freestyle assignment for a GPU-Computing class at KIT in Summer 2016.
Some of the source code (mostly the common code folder) was provided.
I coded the sorting algorithms merge-sort, bitonic merge-sort and a bubble sort sorting network for OpenCL-
Also there is a implementation of merge-sort on CPU for comparison.
See [Sort.cl](Code/Sort.cl) for Kernel-Code, [CSortTask.cpp](Code/CSortTask.cpp) for most of the host code and [CSortingMain.cpp](Code/CSortingMain.cpp) for changing local work size and array size.


## Standard Merge-sort for GPU
Mostly not recommended because it does not benefit that well from parallelization. With bigger arraysize the graphics card will eventually run out of memory.

## Bubble Sort Sorting Network
Called Simple Sorting Network in Code. Not recommended because it is inefficient and slow.

## Bitonic Merge-sort
Recommended sorting variant of these three. Is fast and benefits well from parallelization.


## How to Build
Best way is to use cmake with the Code folder as source folder. Use a 64-Bit compiler as otherwise bigger array sizes won't work.
