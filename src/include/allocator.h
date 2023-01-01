//
//  allocator.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef allocator_h
#define allocator_h

#include <sys/mman.h>
#include "palloc.h"

/*
	Response Types
*/
/*
	Functionality
 */

struct allocator_metadata {
	int 	fd;
	size_t 	used_size;
	size_t 	multiplier;
};



#define TYPED_ALLOCATOR(name, type, max_count)\
TYPED_BLOCK(name, type, max_count)\
\
struct name##_allocator {\
	struct allocator_metadata	metadata;\
	struct name##_block *		blocks;\
};\
\
struct name##_allocator init_##name##_allocator(const char * file_name, const size_t multiplier);\
void name##_allocator_alloc(struct name##_allocator * allocator, uint8_t number_of_blocks);\
void name##_allocator_free(struct name##_allocator * allocator);\
void name##_allocator_add(struct name##_allocator * allocator, uint8_t block_no, type value);\
\
struct name##_allocator init_##name##_allocator(const char * file_name, const size_t multiplier) {\
	int fd = _open_file(file_name);\
	\
	size_t file_size;\
	if ((file_size = _file_size(fd)) == FILE_DOES_NOT_EXIST && _resize_file(fd, file_size) == FILE_UNABLE_TO_RESIZE) {\
		printf("[SMAC] - creating file\n");\
		file_size = 0;\
	}\
	\
	struct name##_allocator _init_val = {\
		{\
			.fd = fd,\
			.used_size = file_size / sizeof(struct name##_block),\
			.multiplier = multiplier\
		},\
		_palloc(fd, file_size, NULL, 0)\
	};\
	return _init_val;\
}\
void name##_allocator_alloc(struct name##_allocator * allocator, uint8_t number_of_blocks) {\
	allocator->blocks = _palloc(\
		allocator->metadata.fd,\
		(allocator->metadata.used_size + number_of_blocks) * sizeof(struct name##_block),\
		allocator->blocks,\
		allocator->metadata.used_size * sizeof(struct name##_block)\
	);\
}\
void name##_allocator_add(struct name##_allocator * allocator, uint8_t block_no, type value) {\
	insert_into_##name##_block(&allocator->blocks[block_no], value);\
}\
void name##_allocator_get(struct name##_allocator * allocator, uint8_t block_no, type * buffer) {\
	memmove(buffer, &(allocator->blocks[block_no].data), allocator->blocks[block_no].used_size * sizeof(type));\
}\
void name##_allocator_free(struct name##_allocator * allocator) {\
	munmap(allocator->blocks, allocator->metadata.used_size * sizeof(struct name##_block));\
}\



/*
 Remove item from block 
 */

static bool required_equal(int rhs, int lhs) {
	return rhs == lhs;
}

TYPED_ALLOCATOR(ruben, int, DEFAULT_BLOCK_SIZE);



#endif /* allocator_h */
