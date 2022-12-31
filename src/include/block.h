//
//  block.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef block_h
#define block_h

#define DEFAULT_BLOCK_SIZE 64
#include <string.h>
#include <stdbool.h>

enum anyblock_insert_codes {
	INSERT_NEW_BLOCK,
	INSERT_GOTO_NEXT,
	INSERT_SUCCESS
};

enum anyblock_delete_codes {
	DELETE_NOT_FOUND,
	DELETE_SUCCESS
};

#define TYPED_BLOCK(name, type, max_count)\
struct name {\
	uint8_t used_size;\
	uint8_t capacity; \
	int64_t next;\
	int64_t previous;\
	type data[max_count];\
};\
\
struct name init_##name(void);\
enum anyblock_insert_codes insert_into_##name(struct name * block, type value);\
enum anyblock_delete_codes delete_from##name(struct name * block, type value);\
\
struct name init_##name() {\
	struct name _init_val = {\
		.used_size = 0,\
		.capacity = max_count,\
		.next = -1,\
		.previous = 1,\
	};\
	return _init_val;\
}\
enum anyblock_insert_codes insert_into_##name(struct name * block, type value) {\
	if (block->capacity == block->used_size + 1 && block->next == -1) {\
		return INSERT_NEW_BLOCK;\
	} else if (block->capacity == block->used_size + 1) {\
		return INSERT_GOTO_NEXT;\
	}\
	block->data[block->used_size++] = value;\
	return INSERT_SUCCESS;\
}\
enum anyblock_delete_codes delete_from##name(struct name * block, type value) {\
	for (uint8_t block_index = 0; block_index < block->used_size; block_index ++) {\
		if (required_equal(block->data[block_index], value)) {\
			memmove(\
				&block->data[block_index],\
				&block->data[block_index + 1],\
				sizeof(type) * ((block->used_size--) - block_index)\
			);\
			return DELETE_SUCCESS;\
		}\
	}\
	return DELETE_NOT_FOUND;\
}\

static bool required_equal(int a, int b){
	return a == b;
}

TYPED_BLOCK(ruben, int, DEFAULT_BLOCK_SIZE);




#endif /* block_h */
