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
#include "block.h"


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



#define TYPED_ALLOCATOR_DEF(name, type, max_count)\
TYPED_BLOCK_DEF(name, type, max_count)\
\
struct name##_allocator {\
	struct allocator_metadata	metadata;\
	size_t 						pre_data_size;\
	void *						raw_data;\
};\
\
struct 	name##_allocator init_##name##_allocator(const char * file_name, void * pre_data, size_t pre_data_size);\
struct 	name##_allocator init_##name##_allocator_pre_open(int fd, void * pre_data, size_t pre_data_size);\
size_t 	name##_allocator_alloc(struct name##_allocator * allocator, size_t number_of_blocks);\
void 	name##_allocator_free(struct name##_allocator * allocator);\
size_t 	name##_allocator_get(struct name##_allocator * allocator, size_t block_no, size_t buffer_size, type * buffer);\
void 	name##_allocator_add(struct name##_allocator * allocator, size_t block_no, type * value);\
void 	__##name##_shift_last_block(struct name##_allocator * alloc, size_t block_to);\

#define TYPED_ALLOCATOR_IMPL(name, type, max_count)\
TYPED_BLOCK_IMPL(name, type, max_count)\
static struct name##_block * __##name##_alloc_get_block_ptr(struct name##_allocator * allocator) {\
	return (struct name##_block *)(allocator->raw_data + allocator->pre_data_size);\
}\
\
struct name##_allocator init_##name##_allocator_pre_open(int fd, void * pre_data, size_t pre_data_size) {\
	size_t file_size;\
	if ((file_size = _file_size(fd)) == 0) {\
		printf("[SMAC] - creating file\n");\
		file_size = pre_data_size;\
	}\
	\
	struct name##_allocator _init_val = {\
		{\
			.fd = fd,\
			.used_size = (file_size - pre_data_size) / sizeof(struct name##_block),\
		},\
		.pre_data_size = pre_data_size,\
		.raw_data = _palloc(fd, file_size, NULL, 0)\
	};\
	if (pre_data != NULL) {\
		memmove(_init_val.raw_data, pre_data, pre_data_size);\
	}\
	return _init_val;\
}\
\
struct name##_allocator init_##name##_allocator(const char * file_name, void * pre_data, size_t pre_data_size) {\
	int fd = _open_file(file_name);\
	return init_##name##_allocator_pre_open(fd, pre_data, pre_data_size);\
}\
size_t name##_allocator_alloc(struct name##_allocator * allocator, size_t number_of_blocks) {\
	allocator->raw_data = _palloc(\
		allocator->metadata.fd,\
		((allocator->metadata.used_size + number_of_blocks) * sizeof(struct name##_block)) + allocator->pre_data_size,\
		allocator->raw_data,\
		(allocator->metadata.used_size * sizeof(struct name##_block) + allocator->pre_data_size)\
	);\
	for (size_t block_idx = allocator->metadata.used_size; block_idx < allocator->metadata.used_size + number_of_blocks; block_idx ++) {\
		__##name##_alloc_get_block_ptr(allocator)[block_idx] = init_##name##_block();\
	}\
	size_t old_size = allocator->metadata.used_size;\
	allocator->metadata.used_size += number_of_blocks;\
	return old_size;\
}\
void name##_allocator_add(struct name##_allocator * alloc, size_t block_no, type * value) {\
	switch(insert_into_##name##_block(&__##name##_alloc_get_block_ptr(alloc)[block_no], *value)) {\
		case INSERT_NEW_BLOCK:\
			{\
				size_t new_block_index = name##_allocator_alloc(alloc, 1);\
				__##name##_alloc_get_block_ptr(alloc)[block_no].next = new_block_index;\
				__##name##_alloc_get_block_ptr(alloc)[new_block_index].previous = block_no;\
				name##_allocator_add(alloc, new_block_index, value);\
			}\
			break;\
		case INSERT_GOTO_NEXT:\
			return name##_allocator_add(alloc, __##name##_alloc_get_block_ptr(alloc)[block_no].next, value);\
		case INSERT_SUCCESS:\
			break;\
	}\
}\
size_t name##_allocator_get(struct name##_allocator * allocator, size_t block_no, size_t buffer_size, type * buffer) {\
	size_t moved_count = 0;\
	struct name##_block block;\
	do {\
		block = __##name##_alloc_get_block_ptr(allocator)[block_no];\
		block_no = block.next;\
		memmove(&buffer[moved_count], &block.data, block.used_size * sizeof(type));\
		moved_count += block.used_size;\
	} while (block_no != -1);\
	return moved_count;\
}\
void name##_allocator_free(struct name##_allocator * alloc) {\
	alloc->raw_data = _palloc(\
		alloc->metadata.fd,\
		0,\
		__##name##_alloc_get_block_ptr(alloc),\
		(sizeof(struct name##_block) * (alloc->metadata.used_size--)) + alloc->pre_data_size\
	);\
}\
void name##_allocator_delete(struct name##_allocator * alloc, size_t block_no, type * value) {\
	struct name##_block * block;\
	while (1) {\
		if (block_no == -1) {\
			break;\
		}\
		block = &__##name##_alloc_get_block_ptr(alloc)[block_no];\
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
	struct name##_block * from = &__##name##_alloc_get_block_ptr(alloc)[alloc->metadata.used_size - 1];\
	struct name##_block * to = &__##name##_alloc_get_block_ptr(alloc)[block_to];\
	if (from->previous != -1) {\
		__##name##_alloc_get_block_ptr(alloc)[from->previous].next = block_to;\
	}\
	if (from->next != -1) {\
		__##name##_alloc_get_block_ptr(alloc)[from->next].previous = block_to;\
	}\
	if (to->previous != -1) {\
		__##name##_alloc_get_block_ptr(alloc)[to->previous].next = to->next;\
	}\
	if (to->next != -1) {\
		__##name##_alloc_get_block_ptr(alloc)[to->next].previous = to->previous;\
	}\
	*to = *from;\
	alloc->raw_data = _palloc(\
		alloc->metadata.fd,\
		(sizeof(struct name##_block) * (alloc->metadata.used_size - 1)) + alloc->pre_data_size,\
		alloc->raw_data,\
		(sizeof(struct name##_block) * (alloc->metadata.used_size--)) + alloc->pre_data_size \
	);\
}\

#endif /* allocator_h */
