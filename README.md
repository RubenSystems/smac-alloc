# smac_alloc

***serializable memory as chunks.***

Custom allocator for databases allowing large amounts of data to be held in a serialised format in and out of memory to improve read/write speed. 

Implements a custom garbage collection system which is a combination of doubly linked lists, reference counting and automatic stale address deallocation. This enables it to expand and contract fairly efficiently and with very little overhead - unlike garbage collection systems such as the one used by several implementations of the JVM, it runs in near constant time (depending on the implementation of mmap). 
