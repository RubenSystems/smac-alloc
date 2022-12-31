//
//  tower.h
//  rs_smac_allocator
//
//  Created by Ruben Ticehurst-James on 31/12/2022.
//

#ifndef tower_h
#define tower_h

struct tower {
	//	Number of pages in the system 
	size_t 	size;
	void *	blocks;
};

#endif /* tower_h */
