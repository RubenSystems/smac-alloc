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

struct block_metadata {
	uint8_t used_size;
	uint8_t	capacity;
	int64_t next;
	int64_t previous;
	size_t 	item_size;
};

#define BLOCK_TYPE(type, name, max_size)\
struct name {\
	struct block_metadata	metadata;\
	type					data[max_size];\
};\
struct name init_block(void) {\
	struct name _init_val = {\
		{\
			.used_size = 0,\
			.next = -1,\
			.previous = -1,\
			.capacity = max_size,\
			.item_size = sizeof(type)\
		}\
	};\
	memset(_init_val.data, 0, sizeof(_init_val.data));\
	return _init_val;\
}\

enum anyblock_insert_codes insert_into_block(void * block_data, struct block_metadata * meta, void * value);

enum anyblock_delete_codes delete_from_block(void * block_data, struct block_metadata * meta, void * value, bool (* equal)(void *, void *));


#endif /* block_h */
