//
//  palloc.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 01/01/2023.
//

#ifndef palloc_h
#define palloc_h

#include <stdio.h>

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

// Returns file descriptor of the file that was opened
enum file_responses {
	FILE_DOES_NOT_EXIST = -1,
	FILE_UNABLE_TO_RESIZE = -2,
	FILE_SUCCESS = 0
};

int _open_file(const char * name);

size_t _file_size(int fd);

enum file_responses _resize_file(int fd, size_t size);

/*
	Requires file is appropiatly sized. See above
*/
void * _palloc(int fd, size_t size, void * old_ptr, size_t old_ptr_size);

#endif /* palloc_h */
