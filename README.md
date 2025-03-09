# Virtual Memory Allocator

## Project Overview
This project implements a virtual memory allocator with a progressive allocation strategy, supporting three different allocation sizes. It optimizes memory management through per-thread arenas, mmap-based growth, and efficient chunk management techniques.

## Features
- **Three Allocation Sizes**: Supports small, medium, and large allocations with a progressive strategy.
- **Per-Thread Arena**: Each thread has its own dedicated memory arena to reduce contention.
- **Thread-Local Growth**: Arena expansion per thread using `mmap` and a `_Thread_local` structure.
- **Recursive Doubling**: Efficient allocation of chunks and the main pool using a recursive doubling strategy.
- **Large Allocations (>128KB)**: Direct allocation using `mmap` for large memory requests.
- **Small Allocations (<64B)**: Uses 96-byte chunks for small memory allocations to reduce fragmentation.
- **Buddy System**: Implements a buddy allocation scheme for the main memory pool.
- **Block Tagging**:
  - Blocks are marked before and after allocation to store their actual size.
  - A unique magic integer is used for verification.
  - Uses the **LCG MMIX (64-bit) Linear Congruential Generator by Knuth** to generate a magic value based on the memory address.