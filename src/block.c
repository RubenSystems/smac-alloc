//
//  block.c
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 23/01/2023.
//

#include "include/block.h"


struct block_metadata init_block_metadata(size_t item_size, size_t capacity) {
	struct block_metadata _init_val = {
		.used_size = 0,
		.next = -1,
		.previous = -1,
		.capacity = capacity,
		.item_size = item_size
	};
	return _init_val;
}

enum anyblock_insert_codes insert_into_block(void * block_data, struct block_metadata * meta, void * value) {
	if (meta->capacity <= meta->used_size && meta->next == -1) {
		return INSERT_NEW_BLOCK;
	} else if (meta->capacity <= meta->used_size) {\
		return INSERT_GOTO_NEXT;
	} else {
		memmove(block_data + (meta->item_size * meta->used_size++), value, meta->item_size);
		return INSERT_SUCCESS;
	}
}

static void __block_delete_and_shift(void * block_data, struct block_metadata * meta, size_t delete_index) {
	memmove(\
		block_data + (delete_index * meta->item_size),\
		block_data + (meta->item_size * (delete_index + 1)),\
		meta->item_size * ((meta->used_size--) - (delete_index + 1))\
	);\
}

enum anyblock_delete_codes delete_from_block(void * block_data, struct block_metadata * meta, void * value, bool (* equal)(void *, void *)) {
	bool del_performed = false;
	for (int block_index = meta->used_size - 1; block_index >= 0; block_index--) {
		if (equal(block_data + (meta->item_size * block_index), value)) {
			__block_delete_and_shift(block_data, meta, block_index);
			del_performed = true;
		}
	}
	return del_performed ? DELETE_SUCCESS : DELETE_NOT_FOUND;
}
