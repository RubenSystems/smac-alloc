//
//  allocator.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef allocator_h
#define allocator_h

#include <sys/mman.h>
#include <stdlib.h>
#include "palloc.h"
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


/*
 Helpers
 */


static size_t __single_block_size(size_t block_data_size, size_t block_data_count) {
	return sizeof(struct block_metadata) + (block_data_size * block_data_count);
}

static size_t __single_block_size_alloc(struct smac_allocator * alloc) {
	return __single_block_size(alloc->mdata.block_data_size, alloc->mdata.block_data_count);
}

static size_t __initial_size(size_t pre_data_size, size_t type_size, size_t number_of_items_p_block) {
	return sizeof(struct persisted_allocator_metadata) + pre_data_size + (INITIAL_CAPACITY * __single_block_size(type_size, number_of_items_p_block));
}

static size_t __pre_data_size(struct smac_allocator * alloc) {
	return sizeof(struct persisted_allocator_metadata) + alloc->mdata.pre_data_size;
}

static struct persisted_allocator_metadata * __metadata(void * raw_data) {
	return (struct persisted_allocator_metadata *)raw_data;
}

static void * __predata(void * raw_data) {
	size_t offset = sizeof(struct persisted_allocator_metadata);
	return (raw_data + offset);
}

static struct block_metadata * __block_md(struct smac_allocator * alloc, size_t index) {
	size_t offset = __pre_data_size(alloc);
	size_t entire_block_size = __single_block_size_alloc(alloc);
	return (struct block_metadata *)(alloc->mdata.raw_data + offset + (entire_block_size * index));
}

static void * __block_data(struct smac_allocator * alloc, size_t index) {
	//	printf("%li\n", (intptr_t)(__block_md(alloc, index)));
	//	printf("%li %i\n", (intptr_t)(__block_md(alloc, index) + sizeof(struct block_metadata)), sizeof(struct block_metadata));
	//	printf("%li\n", (intptr_t)(void *)(__block_md(alloc, index) + sizeof(struct block_metadata)));
	return ((void *)__block_md(alloc, index)) + sizeof(struct block_metadata);
}


struct smac_allocator init_allocator(const char * filename, void * pre_data, size_t pre_data_size, size_t block_data_size, size_t block_data_count) {
	
	int fd = _open_file(filename);
	
	size_t file_size;
	if ((file_size = _file_size(fd)) == 0) {
		printf("[SMAC] - creating file\n");
		file_size = 0;
	}
	
	struct smac_allocator _init = {
		.mdata  = {
			.fd = fd,
			.raw_data = _palloc(fd, file_size == 0 ? __initial_size(pre_data_size, block_data_size, block_data_count) : file_size, NULL, 0),
			.pre_data_size = pre_data_size,
			.block_data_size = block_data_size,
			.block_data_count = block_data_count,
		}
	};
	
	if (file_size == 0) {
		struct persisted_allocator_metadata _pdata = {
			.capacity = INITIAL_CAPACITY,
			.used_size = 0
		};
		*__metadata(_init.mdata.raw_data) = _pdata;
		
		if (pre_data != NULL)
			memmove(__predata(_init.mdata.raw_data), pre_data, pre_data_size);
	}

	return _init;
}

// If you deallocate (negative number_of_blocks) the result means very little.
size_t smac_allocate(struct smac_allocator * alloc, size_t number_of_blocks) {
	struct persisted_allocator_metadata * pdata = __metadata(alloc->mdata.raw_data);
	if (pdata->used_size + number_of_blocks > pdata->capacity
			|| pdata->used_size + number_of_blocks > pdata->capacity - ALLOCATION_SCALE_AT) {
		alloc->mdata.raw_data = _palloc(
			alloc->mdata.fd,
			__pre_data_size(alloc) + (__single_block_size_alloc(alloc) * (number_of_blocks + ALLOCATION_SCALE_OFFSET + pdata->capacity)),
			alloc->mdata.raw_data,
			__pre_data_size(alloc) + (__single_block_size_alloc(alloc) * pdata->capacity)
		);

		
		// PData has been deleted
		pdata = __metadata(alloc->mdata.raw_data);
		pdata->capacity += number_of_blocks + ALLOCATION_SCALE_OFFSET;
		
	}
	
	for (size_t nb_index = pdata->used_size; nb_index < pdata->used_size + number_of_blocks; nb_index ++) {
		*__block_md(alloc, nb_index) = init_block_metadata(alloc->mdata.block_data_size, alloc->mdata.block_data_count);
	}
	size_t first_block_index = pdata->used_size;
	pdata->used_size += number_of_blocks;
	
	return first_block_index;
}

/*
 Todo:
	undo recursive code
 */

void smac_add(struct smac_allocator * alloc, size_t block_no, void * data) {
	void * block_data = __block_data(alloc, block_no);
	struct block_metadata * meta = __block_md(alloc, block_no);

 	switch (insert_into_block(block_data, meta, data)) {
		case INSERT_NEW_BLOCK: {
				size_t new_block_index = smac_allocate(alloc, 1);
				// Can't use meta bec it it has been freed 
				__block_md(alloc, new_block_index)->previous = block_no;
				__block_md(alloc, block_no)->next = new_block_index;
				smac_add(alloc, block_no, data);
			}
			break;
		case INSERT_GOTO_NEXT:
			smac_add(alloc, meta->next, data);
			break;
		case INSERT_SUCCESS:
			break;
	}
}


size_t smac_get(struct smac_allocator * alloc, size_t block_no, size_t move_size, size_t buffer_offset, void * buffer) {
	void * block_data = __block_data(alloc, block_no);
	struct block_metadata * meta = __block_md(alloc, block_no);
	memmove(
		buffer + (alloc->mdata.block_data_size * buffer_offset),
		block_data,
		alloc->mdata.block_data_size * min(move_size, meta->used_size)
	);
	move_size -= min(move_size, meta->used_size);
	buffer_offset += min(move_size, meta->used_size);
	if (move_size <= 0 || meta->next == -1) {
		return buffer_offset;
	} else {
		return smac_get(alloc, meta->next, move_size, buffer_offset, buffer);
	}
}

void smac_delete(struct smac_allocator * alloc, size_t block_no, void * value, bool (*equal)(void *, void *)) {
	void * block_data = __block_data(alloc, block_no);
	struct block_metadata * meta = __block_md(alloc, block_no);
	
	delete_from_block(block_data, meta, value, equal);
	
	if (meta->next != -1) {
		smac_delete(alloc, meta->next, value, equal);
	}
}

/*
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
struct name##_block * __##name##_alloc_get_block_ptr(struct name##_allocator * allocator);\

#define TYPED_ALLOCATOR_IMPL(name, type, max_count)\
TYPED_BLOCK_IMPL(name, type, max_count)\
struct name##_block * __##name##_alloc_get_block_ptr(struct name##_allocator * allocator) {\
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
		memmove(&buffer[moved_count], &block.data,\
			block.used_size < (buffer_size - moved_count) ?\
			block.used_size * sizeof(type) :\
			(buffer_size - moved_count) * sizeof(type)\
		);\
		moved_count += block.used_size < (buffer_size - moved_count) ? block.used_size :(buffer_size - moved_count);\
	} while (block_no != -1 && moved_count < buffer_size);\
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
	while (block_no != -1) {\
		block = &__##name##_alloc_get_block_ptr(alloc)[block_no];\
		delete_from_##name##_block(block, *value);\
		if (block->previous != -1 && block->used_size == 0) {\
			size_t prev_block = block->previous;\
			__##name##_shift_last_block(alloc, block_no);\
			block_no = __##name##_alloc_get_block_ptr(alloc)[prev_block].next ;\
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
	if (from->previous == block_to) {\
		from->previous = to->previous;\
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
*/
#endif /* allocator_h */
