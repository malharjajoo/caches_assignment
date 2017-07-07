# Initial Problems - 
- Setting up Data Structures for the Cache and Main memory was crucial since it was important
  to realize that the data structures had to be dynamic since the configuration inputs were obtained at runtime.

# Implementation -
- The complete cache is represented/implemented as a C++ std::vector of cache sets.

      - The cache sets are a std::vector of cache blocks.
      - Each blocks is a struct containing a vector of words and other fields like tag bits and valid bit.
      - Finally,each word is a vector of bytes.
      - the sizes of all of these dynamic structures is determined using the configuration input at runtime.
  
# Debugging - 

- Used the Debug Response that was created for the Debug command to debug my Cache Content.
  It displays all the cache contents (even the bytes).
- Some of the built in types like uint8_t caused some issues such as – Not being able to cout it’s value.
- Since the words are stored as vector of words, There were quite a few issues with reading and writing the data.Some of the       inbuilt types are automatically sign extended when you cast it .Ended up removing the casts from the code.

# Testing - 
- Created a Shell Script and tested many samples of possible inputs with edge cases like – 
    - Write Miss Optimization in 1 word/block cache.
    - 1 byte cache.
    - 8 way set associative cache – **A Complete Test** of all the features of a write back cache and the LRU replacement policy.

# Learning Outcome - 
- Learnt how to use Scripts to automate testing. Also learnt Linux and Scripting basics in the process.
- Wrote program for a write back cache simulator with LRU replacement policy in C++.
- Understood how a cache deals with read/write hit/miss and how main memory is updated.
- handling of dirty and valid flags in caches.


