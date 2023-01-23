//
//  block.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef block_h
#define block_h

#include <string.h>
#include <stdbool.h>

#define MAX_COUNT 10

enum anyblock_insert_codes {
	INSERT_NEW_BLOCK,
	INSERT_GOTO_NEXT,
	INSERT_SUCCESS
};

enum anyblock_delete_codes {
	DELETE_NOT_FOUND,
	DELETE_SUCCESS
};


static bool block_val_equal(int rhs, int lhs) {
	return rhs == lhs;
}

// Placeholder type of type int
struct internal_block_data_store {
	int 	data;
};

struct any_block {
	struct internal_block_data_store 	data[MAX_COUNT];
	uint8_t 							used_size;
	uint8_t 							capacity;
	int64_t 							next;
	int64_t 							previous;
};

struct any_block init_block(void) {
	struct any_block _init_val = {
		.used_size = 0,
		.next = -1,
		.capacity = MAX_COUNT,
	};
	
	memset(_init_val.data, 0, sizeof(_init_val.data));
	return _init_val;
}

enum anyblock_insert_codes insert_into_block(struct any_block * block, struct internal_block_data_store value) {
	if (block->capacity <= block->used_size && block->next == -1) {
		return INSERT_NEW_BLOCK;
	} else if (block->capacity <= block->used_size) {\
		return INSERT_GOTO_NEXT;
	} else {
		block->data[block->used_size++] = value;
		return INSERT_SUCCESS;
	}
}

static void __block_delete_and_shift(struct any_block * block, size_t delete_index) {
	for (size_t i = delete_index + 1; i < block->used_size; i ++) {
		block->data[delete_index - 1] = block->data[delete_index];
	}
}

enum anyblock_delete_codes delete_from_block(struct any_block * block, struct internal_block_data_store value) {
	bool del_performed = false;
	for (int block_index = block->used_size - 1; block_index >= 0; block_index--) {
		if (block_val_equal(block->data[block_index].data, value.data)) {
			printf("DELETING\n");
			__block_delete_and_shift(block, block_index);
			del_performed = true;
		}
	}
	return del_performed ? DELETE_SUCCESS : DELETE_NOT_FOUND;
}




// I present to you: Why c++ can sometimes be easier:


/*
#define TYPED_BLOCK_DEF(name, type, max_count)\
struct name##_block {\
	type data[max_count];\
	uint8_t used_size;\
	uint8_t capacity; \
	int64_t next;\
	int64_t previous;\
};\
\
struct name##_block init_##name##_block(void);\
enum anyblock_insert_codes insert_into_##name##_block(struct name##_block * block, type value);\
enum anyblock_delete_codes delete_from_##name##_block(struct name##_block * block, type value);\

#define TYPED_BLOCK_IMPL(name, type, max_count)\
struct name##_block init_##name##_block() {\
	struct name##_block _init_val = {\
		.used_size = 0,\
		.capacity = max_count,\
		.next = -1,\
		.previous = -1,\
	};\
	return _init_val;\
}\
enum anyblock_insert_codes insert_into_##name##_block(struct name##_block * block, type value) {\
	if (block->capacity <= block->used_size && block->next == -1) {\
		return INSERT_NEW_BLOCK;\
	} else if (block->capacity <= block->used_size) {\
		return INSERT_GOTO_NEXT;\
	} else {\
		block->data[block->used_size++] = value;\
		return INSERT_SUCCESS;\
	}\
}\
enum anyblock_delete_codes delete_from_##name##_block(struct name##_block * block, type value) {\
	bool del_performed = false;\
	for (int block_index = block->used_size - 1; block_index >= 0; block_index--) {\
	if (name##_required_equal(block->data[block_index], value)) {\
			printf("DELETING\n");\
			memmove(\
				&block->data[block_index],\
				&block->data[block_index + 1],\
				sizeof(type) * ((block->used_size--) - (block_index + 1))\
			);\
			del_performed = true;\
		}\
	}\
	return del_performed ? DELETE_SUCCESS : DELETE_NOT_FOUND;\
}\

*/

#endif /* block_h */
