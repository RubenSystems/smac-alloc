//
//  allocator.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef allocator_h
#define allocator_h

#include <stdlib.h>
#include <stdbool.h>
#include "block.h"

#define INITIAL_CAPACITY 				20
#define ALLOCATION_SCALE_OFFSET 		20
#define ALLOCATION_SCALE_AT 			10


struct persisted_allocator_metadata {
	size_t 	capacity;
	size_t 	used_size;
};

/*
	memory format:
		PERSISTED_METADATA | PRE_DATA | BLOCK_DATA
*/
struct in_memory_allocator_metadata {
	int 		fd;
	void * 		raw_data;
	size_t 		pre_data_size;
	uint64_t 	block_data_size;
	uint8_t		block_data_count;
};


struct smac_allocator {
	struct in_memory_allocator_metadata mdata;
};



struct smac_allocator init_allocator(int fd, void * pre_data, size_t pre_data_size, size_t block_data_size, size_t block_data_count);

void * smac_pre_data(struct smac_allocator *);

// If you deallocate (negative number_of_blocks) the result means very little.
size_t smac_allocate(struct smac_allocator * alloc, size_t number_of_blocks);

void smac_add(struct smac_allocator * alloc, size_t block_no, void * data);

size_t smac_get(struct smac_allocator * alloc, size_t block_no, size_t move_size, size_t buffer_offset, void * buffer);

void smac_delete(struct smac_allocator * alloc, size_t block_no, void * value, bool (* equatable)(void *, void *)) ;

void smac_free(struct smac_allocator * alloc);

#endif /* allocator_h */
