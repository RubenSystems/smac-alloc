//
//  main.c
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#include <stdio.h>
#include "src/include/block.h"
#include "src/include/allocator.h"

int main(int argc, const char * argv[]) {
	// insert code here...	
	struct ruben_allocator x = init_ruben_allocator("test.smac", 6);
	
	//	Ruben allocator has 3 blocks
//	ruben_allocator_alloc(&x, 3);
//	ruben_allocator_add(&x, 0, 10);
	
	int buffer[DEFAULT_BLOCK_SIZE];
//
	ruben_allocator_get(&x, 0, (int *)&buffer);
//
	printf("hi%i", buffer[0]);
//
//	ruben_allocator_free(&x);
//
//
	printf("Hello, World!\n");
	return 0;
}
