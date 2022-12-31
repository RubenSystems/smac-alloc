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
	
	if (old_ptr != NULL) {
		memmove(new_ptr, old_ptr, old_ptr_size);
		munmap(old_ptr, old_ptr_size);
	}
	return new_ptr;
}

static inline ssize_t calculate_capacity_from_used_size(size_t used_size, size_t multiplier){
	return used_size + (multiplier - (used_size % multiplier));
}

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
struct name##_allocator init_allocator(const char * file_name, const size_t multiplier) {\
	int fd = _open_file(file_name);\
	\
	size_t file_size = 0;\
	if ((file_size = _file_size(fd)) == FILE_DOES_NOT_EXIST && _resize_file(fd, file_size) == FILE_UNABLE_TO_RESIZE) {\
		exit(0);\
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


/*
 Create block(s)
 Add item to block
 Remove item from block
 Return all items in block
 
 */

static bool required_equal(int rhs, int lhs) {
	return rhs == lhs;
}

TYPED_ALLOCATOR(ruben, int, DEFAULT_BLOCK_SIZE);



#endif /* allocator_h */
