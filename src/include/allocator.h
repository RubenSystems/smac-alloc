//
//  allocator.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef allocator_h
#define allocator_h

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

/*
	Static helpers
*/

#define max(a, b) (a > b) ? a : b
#define min(a, b) (a < b) ? a : b

// Returns file descriptor of the file that was opened
enum file_responses {
	FILE_DOES_NOT_EXIST = -1,
	FILE_UNABLE_TO_RESIZE = -2,
	FILE_SUCCESS = 0
};

static int _open_file(const char * name) {
	return open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
}

static size_t _file_size(int fd) {
	struct stat file_info;
	if (fstat(fd, &file_info) == -1 || file_info.st_size == 0) {
		return FILE_DOES_NOT_EXIST;
	}
	return file_info.st_size;
}

static enum file_responses _resize_file(int fd, size_t size) {
	if (ftruncate(fd, size) == -1) {
		return FILE_UNABLE_TO_RESIZE;
	}
	return FILE_SUCCESS;
}

/*
	Requires file is appropiatly sized. See above function.
*/
static void * _palloc(int fd, size_t size, void * old_ptr, size_t old_ptr_size) {
	void * new_ptr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	_resize_file(fd, size);
	if (old_ptr != NULL) {
		// Will truncate if old is larger then new
		memmove(new_ptr, old_ptr, max(old_ptr_size, size));
		munmap(old_ptr, old_ptr_size);
	}
	return new_ptr;
}

static inline ssize_t calculate_capacity_from_used_size(size_t used_size, size_t multiplier){
	return used_size + (multiplier - (used_size % multiplier));
}

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
 Add item to block
 Remove item from block
 Return all items in block
 
 */

static bool required_equal(int rhs, int lhs) {
	return rhs == lhs;
}

TYPED_ALLOCATOR(ruben, int, DEFAULT_BLOCK_SIZE);



#endif /* allocator_h */
