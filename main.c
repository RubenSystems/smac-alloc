//
//  main.c
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#include <stdio.h>
#include "src/include/block.h"
#include "src/include/allocator.h"


static bool required_equal(int rhs, int lhs) {
	return rhs == lhs;
}

TYPED_ALLOCATOR(ruben, int, 2);


int main(int argc, const char * argv[]) {
	// insert code here...

	struct ruben_allocator x = init_ruben_allocator("test.smac");

	
	
	//	Ruben allocator has 3 blocks
	ruben_allocator_alloc(&x, 1);
	for (int value = 0; value < 10; value ++) {
		ruben_allocator_add(&x, 0, &value);
	}
	int val1 = 4;
	ruben_allocator_add(&x, 0, &val1);
	int buffer[1000];
	uint8_t count = ruben_allocator_get(&x, 0, 1000, (int *)&buffer);
	for (int i = 0; i < count; i ++){
		printf("hi%i\n", buffer[i]);
	}

	int value = 4;
	ruben_allocator_delete(&x, 0, &value);
//	int value2 = 6;
//	ruben_allocator_delete(&x, 0, &value2);
	printf("YSED%i\n", x.metadata.used_size);
	
	count = ruben_allocator_get(&x, 0, 1000, (int *)&buffer);
	for (int i = 0; i < count; i ++){
		printf("hi%i\n", buffer[i]);
	}
	

	ruben_allocator_free(&x);


	return 0;
}
