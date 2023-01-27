//
//  allocator.c
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 26/01/2023.
//

#include "include/allocator.h"
#include "include/palloc.h"

#include <sys/mman.h>



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
	return ((void *)__block_md(alloc, index)) + sizeof(struct block_metadata);
}


void * smac_pre_data(struct smac_allocator * alloc) {
	return __predata(alloc->mdata.raw_data);
}

struct smac_allocator init_allocator(int fd, void * pre_data, size_t pre_data_size, size_t block_data_size, size_t block_data_count) {
	
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
	buffer_offset += min(move_size, meta->used_size);
	move_size -= min(move_size, meta->used_size);
	if (move_size <= 0 || meta->next == -1) {
		return buffer_offset;
	} else {
		return smac_get(alloc, meta->next, move_size, buffer_offset, buffer);
	}
}


// Delete by removing references.
static void __delete_block(struct smac_allocator * alloc, size_t block_no){
	struct block_metadata * meta = __block_md(alloc, block_no);
	
	if (meta->previous != -1)
		__block_md(alloc, meta->previous)->next = meta->next;
	
	if (meta->next != -1)
		__block_md(alloc, meta->next)->previous = meta->previous;
}

static void __shift_last_block(struct smac_allocator * alloc, size_t to_position){
	struct persisted_allocator_metadata * pdata = __metadata(alloc->mdata.raw_data);
	struct block_metadata * meta = __block_md(alloc, pdata->used_size - 1);
	
	if (meta->previous != -1)
		__block_md(alloc, meta->previous)->next = to_position;
	
	memmove(__block_md(alloc, to_position), meta, __single_block_size_alloc(alloc));
	pdata->used_size--;
	// TODO: - CALL DEALLOCATOR
}

void smac_delete(struct smac_allocator * alloc, size_t block_no, void * value, bool (*equal)(void *, void *)) {
	void * block_data = __block_data(alloc, block_no);
	struct block_metadata * meta = __block_md(alloc, block_no);
	
	
	delete_from_block(block_data, meta, value, equal);
	
//
//	if (__block_md(alloc, block_no)->used_size == 0) {
//		size_t prev_jump = meta->previous;
//		__delete_block(alloc, block_no);
//		__shift_last_block(alloc, block_no);
//		meta = __block_md(alloc, prev_jump);
//	}
//
	if (meta->next != -1) {
		smac_delete(alloc, meta->next, value, equal);
	}
}

void smac_free(struct smac_allocator * alloc) {
	struct persisted_allocator_metadata * pdata = __predata(alloc->mdata.raw_data);
	munmap(alloc->mdata.raw_data, __single_block_size_alloc(alloc) * pdata->capacity);
}
