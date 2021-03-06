# PyramidSearch

I'm writing a very specialized Constraint Programming solver for the PuyoPuyo puzzle game.
A huge number of "inner joins", identical to the ones found in Database systems,
are required to solve constraint programming problems. Researching the fastest modern
join algorithms led me to the "worst case optimal" algorithm Leapfrog Triejoin... and
later an improved version called Tributary Join. See the following papers for details:

* http://openproceedings.org/ICDT/2014/paper_13.pdf
* https://homes.cs.washington.edu/~chushumo/files/sigmod_15_join.pdf

In short, these algorithms implement a multi-way inner join between 3-or-more tables. 
Without going too far into the details, both require sorted lists for the relations, 
and both search for a particular table-entry many, many, many times.

An unordered Hash Table methodology was brought up hypothetically in one of the papers,
but it seems like sorting the values is hugely important for this particular problem.
A major operation in the algorithm is the "next" operator, so a sort has to take place
regardless.

There's no insertion or deletion in either of these algorithms. It is purely a 
read-only search problem as a new table is constructed.

Leapfrog Triejoin, the original, claims to use a B-tree like structure. Tributary Join 
uses a simple Binary-search,  and allegedly achieved higher performance than the 
original B-tree implementation. Which makes sense: arrays are far more cache and 
memory friendly than pointers.

Alas: Binary Search doesn't use the SIMD units (SSE or AVX on x86 platforms), while B-Trees 
have relatively simple SIMD implementation. Is it possible to achieve some of the linear
memory benefits of Binary Search, while still using the SIMD-units of modern processors?

In this repository, I'm trying to combine the SIMD-friendliness of B-Trees with the 
advantages of BinarySearch. This new methodology I call the SIMDPyramid data-structure.

Preliminary Results
===================

This "SIMD-Pyramid search" executes roughly 4x faster than std::binary_search (or really,
std::lower_bound) over 256-Million integers. However, it takes many microseconds to build
the SIMD-Pyramid (it is a O(n) operation to build the Pyramid: it requires walking and
extracting 1/128 elements of the array for each section of the pyramid, assuming a 
FACTOR=128).
