# smac_alloc

***serializable memory as chunks.***

Custom allocator for databases allowing large amounts of data to be held in a serialised format in and out of memory to improve read/write speed. 

Implements a custom garbage collection system which is based on automatic reference counting. This enables it to expand and contract fairly efficiently and with very little overhead. 
