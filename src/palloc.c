//
//  palloc.c
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 01/01/2023.
//

#include "include/palloc.h"

#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

int _open_file(const char * name) {
	return open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
}

size_t _file_size(int fd) {
	struct stat file_info;
	if (fstat(fd, &file_info) == -1 || file_info.st_size == 0) {
		return 0;
	}
	return file_info.st_size;
}

enum file_responses _resize_file(int fd, size_t size) {
	if (ftruncate(fd, size) == -1) {
		printf("[SMAC] - failed to resize_file");
		return FILE_UNABLE_TO_RESIZE;
	}
	return FILE_SUCCESS;
}

void * _palloc(int fd, size_t size, void * old_ptr, size_t old_ptr_size) {
	if (size == 0 && old_ptr_size > 0) {
		munmap(old_ptr, old_ptr_size);
		return NULL;
	} else if (size == old_ptr_size) {
		return old_ptr;
	}

	void * new_ptr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	_resize_file(fd, size);
	if (old_ptr != NULL && old_ptr_size != 0) {
		// Will truncate if old is larger then new
		memmove(new_ptr, old_ptr, min(old_ptr_size, size));
		munmap(old_ptr, old_ptr_size);
	}
	return new_ptr;
}
