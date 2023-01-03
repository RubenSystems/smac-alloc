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
};



#define TYPED_ALLOCATOR(name, type, max_count)\
TYPED_BLOCK(name, type, max_count)\
\
struct name##_allocator {\
	struct allocator_metadata	metadata;\
	struct name##_block *		blocks;\
};\
\
struct 	name##_allocator init_##name##_allocator(const char * file_name);\
size_t 	name##_allocator_alloc(struct name##_allocator * allocator, uint8_t number_of_blocks);\
void 	name##_allocator_free(struct name##_allocator * allocator);\
size_t 	name##_allocator_get(struct name##_allocator * allocator, size_t block_no, size_t buffer_size, type * buffer);\
void 	name##_allocator_add(struct name##_allocator * allocator, size_t block_no, type * value);\
void __##name##_shift_last_block(struct name##_allocator * alloc, size_t block_to);\
\
struct name##_allocator init_##name##_allocator(const char * file_name) {\
	int fd = _open_file(file_name);\
	\
	size_t file_size;\
	if ((file_size = _file_size(fd)) == FILE_DOES_NOT_EXIST) {\
		printf("[SMAC] - creating file\n");\
		file_size = 0;\
	}\
	\
	struct name##_allocator _init_val = {\
		{\
			.fd = fd,\
			.used_size = file_size / sizeof(struct name##_block),\
		},\
		_palloc(fd, file_size, NULL, 0)\
	};\
	return _init_val;\
}\
size_t name##_allocator_alloc(struct name##_allocator * allocator, uint8_t number_of_blocks) {\
	allocator->blocks = _palloc(\
		allocator->metadata.fd,\
		(allocator->metadata.used_size + number_of_blocks) * sizeof(struct name##_block),\
		allocator->blocks,\
		allocator->metadata.used_size * sizeof(struct name##_block)\
	);\
	for (size_t block_idx = allocator->metadata.used_size; block_idx < allocator->metadata.used_size + number_of_blocks; block_idx ++) {\
		allocator->blocks[block_idx] = init_##name##_block();\
	}\
	size_t old_size = allocator->metadata.used_size;\
	allocator->metadata.used_size += number_of_blocks;\
	return old_size;\
}\
void name##_allocator_add(struct name##_allocator * alloc, size_t block_no, type * value) {\
	switch(insert_into_##name##_block(&alloc->blocks[block_no], *value)) {\
		case INSERT_NEW_BLOCK:\
			{\
				size_t new_block_index = name##_allocator_alloc(alloc, 1);\
				alloc->blocks[block_no].next = new_block_index;\
				alloc->blocks[new_block_index].previous = block_no;\
				name##_allocator_add(alloc, new_block_index, value);\
			}\
			break;\
		case INSERT_GOTO_NEXT:\
			return name##_allocator_add(alloc, alloc->blocks[block_no].next, value);\
		case INSERT_SUCCESS:\
			break;\
	}\
}\
size_t name##_allocator_get(struct name##_allocator * allocator, size_t block_no, size_t buffer_size, type * buffer) {\
	size_t moved_count = 0;\
	struct name##_block block;\
	do {\
		block = allocator->blocks[block_no];\
		block_no = block.next;\
		memmove(&buffer[moved_count], &block.data, block.used_size * sizeof(type));\
		moved_count += block.used_size;\
	} while (block_no != -1);\
	return moved_count;\
}\
void name##_allocator_free(struct name##_allocator * allocator) {\
	munmap(allocator->blocks, allocator->metadata.used_size * sizeof(struct name##_block));\
}\
void name##_allocator_delete(struct name##_allocator * alloc, size_t block_no, type * value) {\
	struct name##_block * block;\
	while (1) {\
		if (block_no == -1) {\
			break;\
		}\
		block = &alloc->blocks[block_no];\
		delete_from_##name##_block(block, *value);\
		if (block->previous != -1 && block->used_size == 0) {\
			size_t next_block = block->next;\
			__##name##_shift_last_block(alloc, block_no);\
			block_no = next_block;\
		} else {\
			block_no = block->next;\
		}\
	}\
}\
void __##name##_shift_last_block(struct name##_allocator * alloc, size_t block_to) {\
	struct name##_block * from = &alloc->blocks[alloc->metadata.used_size - 1];\
	struct name##_block * to = &alloc->blocks[block_to];\
	if (from->previous != -1) {\
		alloc->blocks[from->previous].next = block_to;\
	}\
	if (from->next != -1) {\
		alloc->blocks[from->next].previous = block_to;\
	}\
	if (to->previous != -1) {\
		alloc->blocks[to->previous].next = to->next;\
	}\
	if (to->next != -1) {\
		alloc->blocks[to->next].previous = to->previous;\
	}\
	*to = *from;\
	alloc->blocks = _palloc(\
		alloc->metadata.fd,\
		sizeof(struct name##_block) * (alloc->metadata.used_size - 1),\
		alloc->blocks,\
		sizeof(struct name##_block) * (alloc->metadata.used_size)\
	);\
	alloc->metadata.used_size--;\
}\


#endif /* allocator_h */
