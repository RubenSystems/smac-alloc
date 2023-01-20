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

enum anyblock_insert_codes {
	INSERT_NEW_BLOCK,
	INSERT_GOTO_NEXT,
	INSERT_SUCCESS
};

enum anyblock_delete_codes {
	DELETE_NOT_FOUND,
	DELETE_SUCCESS
};

// I present to you: Why c++ can sometimes be easier:
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


#endif /* block_h */
