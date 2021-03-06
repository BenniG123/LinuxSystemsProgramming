/**
 * @file	isu_mmu.c
 * @author	Kris Hall
 * @date	3/29/2015 - created
 * @brief	source file of isu_mmu.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "isu_mmu.h"
#include "llist/isu_llist.h"
#include "common/isu_error.h"
#include "common/isu_color.h"

/// defines for the delays for memory access, in nanoseconds
#define L1_DELAY 1
#define L1_SIZE 4
#define L2_DELAY 7
#define L2_SIZE 8
#define RAM_DELAY 75
#define RAM_SIZE 32
#define DISK_DELAY 5000000

/*****************************
 *Prototypes
 *****************************/
int isu_mmu_page_check(isu_mmu_t mem, int p);
int isu_mmu_page_fetch(isu_mmu_t mem, int p, unsigned long long *t);
int isu_mmu_page_swap(isu_mmu_t mem, int old, int new, int new_level, unsigned long long *t);
int isu_mmu_page_rep_fifo(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t);
int isu_mmu_page_rep_lru(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t);
int isu_mmu_page_rep_clock(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t);
int isu_mmu_page_rep_second_chance(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t);

struct ISU_MEM_PAGE_STRUCT{

	/// reference bit
	char ref;

	/// dirty bit
	char dirty;

	/// the page number
	int page;

	/// the time of last access
	unsigned long long access_time;

	/// the time of placement
	unsigned long long placement_time;
};

typedef struct ISU_MEM_PAGE_STRUCT *isu_mem_page_t;

struct ISU_MMU_STRUCT{
	/// array representing L1 cache
	isu_mem_page_t *L1;

	/// array representing L2 cache
	isu_mem_page_t *L2;

	/// array representing L3 cache
	isu_mem_page_t *RAM;
	
	/// the mode of operation for page replacement
	int rep_mode;

	/// the position of the hand for the clock algorithm
	int hand;
};

isu_mmu_t isu_mmu_create(int mode){
	int i;
	isu_mmu_t mmu;
	mmu = calloc(1, sizeof(struct ISU_MMU_STRUCT));
	if(mmu == NULL){
		isu_print(PRINT_ERROR, "calloc returned NULL");
		return NULL;
	}
	mmu->L1 = malloc(L1_SIZE * sizeof(isu_mem_page_t));
	mmu->L2 = malloc(L2_SIZE * sizeof(isu_mem_page_t));
	mmu->RAM = malloc(RAM_SIZE * sizeof(isu_mem_page_t));

	for(i = 0; i < L1_SIZE; i++){
		mmu->L1[i] = calloc(1, sizeof(struct ISU_MEM_PAGE_STRUCT));
		mmu->L1[i]->page = -1;
		mmu->L1[i]->access_time = -1;
		mmu->L1[i]->ref = 0;
		mmu->L1[i]->dirty = 0;
	}
	for(i = 0; i < L2_SIZE; i++){
		mmu->L2[i] = calloc(1, sizeof(struct ISU_MEM_PAGE_STRUCT));
		mmu->L2[i]->page = -1;
		mmu->L2[i]->access_time = -1;
		mmu->L2[i]->ref = 0;
		mmu->L2[i]->dirty = 0;
	}
	for(i = 0; i < RAM_SIZE; i++){
		mmu->RAM[i] = calloc(1, sizeof(struct ISU_MEM_PAGE_STRUCT));
		mmu->RAM[i]->page = -1;
		mmu->RAM[i]->access_time = -1;
		mmu->RAM[i]->ref = 0;
		mmu->RAM[i]->dirty = 0;
	}
	mmu->rep_mode = mode;
	mmu->hand = 0;
	isu_print(PRINT_DEBUG, "created new MMU");
	return mmu;
}

void isu_mmu_destroy(isu_mmu_t mem){
	free(mem->L1);
	mem->L1 = 0;
	free(mem->L2);
	mem->L2 = 0;
	free(mem->RAM);
	mem->RAM = 0;
	free(mem);
	mem = 0;
}

int isu_mmu_handle_req(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t){
	// return value
	int ret;

	/// book keeping where we set the time of the request being started
	isu_mem_req_set_req_time(req, *t);
	// run certain replacement algorithms based on the value of mode
	switch(mem->rep_mode){
	case 1: ret = isu_mmu_page_rep_lru(mem, req, t);
		break;
	case 2: ret = isu_mmu_page_rep_clock(mem, req, t);
		break;
	default: ret = isu_mmu_page_rep_fifo(mem, req, t);
		break;
	}

	/// book keeping where we set the time of the request being handled
	isu_mem_req_set_handle_time(req, *t);

	return ret;
}

int isu_mmu_ref_clear(isu_mmu_t mem){
	int i;
	for(i = 0; i < L1_SIZE; i++){
		mem->L1[i]->ref = 0;
	}
	for(i = 0; i < L2_SIZE; i++){
		mem->L2[i]->ref = 0;
	}
	for(i = 0; i < RAM_SIZE; i++){
		mem->RAM[i]->ref = 0;
	}
}

/// checks if the page `page` exists in any level of memory in `mem`
/// returns 0 if in L1
/// returns 1 if in L2
/// returns 2 if in L3
/// returns -1 if not in `mem`
int isu_mmu_page_check(isu_mmu_t mem, int p){
	if(p < 0){
		isu_print(PRINT_ERROR, "Error in value of p: value not applicable");
	}

	int i;
	for(i = 0; i < L1_SIZE; i++){
		if(mem->L1[i]->page == p){
			mem->L1[i]->ref = 1;
			return 0;
		}
	}

	for(i = 0; i < L2_SIZE; i++){
		if(mem->L2[i]->page == p){
			mem->L2[i]->ref = 1;
			return 1;
		}
	}

	for(i = 0; i < RAM_SIZE; i++){
		if(mem->RAM[i]->page == p){
			mem->RAM[i]->ref = 1;
			return 2;
		}
	}
	// can't find it in `mem`
	return -1;
}

int isu_mmu_page_move(isu_mmu_t mem, int p, int from_level, unsigned long long *t){
	int i;
	unsigned long long temp_time;
	int replace_index = INT_MAX;
	/// if the move is to occur from L1
	if(from_level == 0){
		/// first, check if there are any open slots in L2
		for(i = 0; i < L2_SIZE; i++){
			/// if there is, then we move the passed in page into the open
			/// slot
			if(mem->L2[i]->page == -1){
				/// add delay of writing to L2
				*t += L2_DELAY;
				/// write over current slot in L2
				mem->L2[i]->page = p;
				mem->L2[i]->placement_time = *t;
				mem->L2[i]->access_time = *t;
				mem->L2[i]->ref = 0;
				mem->L2[i]->dirty = 0;
				return 0;
			}
		}
		/// if there wasn't an open slot then we would have to move a page from
		/// the current level down to RAM. Choosing the page to be moved based
		/// on when page was last accessed as chances are that if a page hasn't
		/// been accessed in a while, it won't be accessed again
		temp_time = mem->L2[0]->access_time;
		for(i = 0; i < L2_SIZE; i++){
			if(!(mem->L2[i]->ref) && temp_time > mem->L2[i]->access_time){
				temp_time = mem->L2[i]->access_time;
				replace_index = i;
			}
		}
		/// if a move candidate was not found
		if(replace_index == INT_MAX){
			/// we loop through L2 again, this time, not worrying about reference bit
			replace_index = 0;
			for(i = 0; i < L2_SIZE; i++){
				if(temp_time > mem->L2[i]->access_time){
					temp_time = mem->L2[i]->access_time;
					replace_index = i;
				}
			}
			/// now we are guaranteed that there will be a valid replace index
		}

		isu_mmu_page_move(mem, mem->L2[replace_index]->page, 1, t);
		/// once move is complete, we put our page that we want into
		/// the `replace_index` slot

		/// add the delay of writing to L2
		*t += L2_DELAY;
		mem->L2[replace_index]->page = p;
		mem->L2[replace_index]->placement_time = *t;
		mem->L2[replace_index]->access_time = *t;
		mem->L2[replace_index]->ref = 0;
		mem->L2[replace_index]->dirty = 0;
		return 0;
	}else if(from_level == 1){	
		/// first, check if there are any open slots in RAM
		for(i = 0; i < L2_SIZE; i++){
			/// if there is, then we move the passed in page into the open
			/// slot
			if(mem->RAM[i]->page == -1){
				/// add delay of writing to RAM
				*t += RAM_DELAY;
				/// write over current slot in RAM
				mem->RAM[i]->page = p;
				mem->RAM[i]->placement_time = *t;
				mem->RAM[i]->access_time = *t;
				mem->RAM[i]->ref = 0;
				mem->RAM[i]->dirty = 0;
				return 0;
			}
		}
		/// if there wasn't an open slot then we would have to move a page from
		/// the current level down to disk. Choosing the page to be moved based
		/// on when page was last accessed as chances are that if a page hasn't
		/// been accessed in a while, it won't be accessed again
		temp_time = mem->RAM[0]->access_time;
		for(i = 0; i < RAM_SIZE; i++){
			if(!(mem->RAM[i]->ref) && temp_time > mem->RAM[i]->access_time){
				temp_time = mem->RAM[i]->access_time;
				replace_index = i;
			}
		}
		/// if a move candidate was not found
		if(replace_index == INT_MAX){
			/// we loop through RAM again, this time, not worrying about reference bit
			replace_index = 0;
			for(i = 0; i < RAM_SIZE; i++){
				if(temp_time > mem->RAM[i]->access_time){
					temp_time = mem->RAM[i]->access_time;
					replace_index = i;
				}
			}
			/// now we are guaranteed that there will be a valid replace index
		}

		/// since there is no access to disk here, it just disappears from RAM
		/// once move is complete, we put our page that we want into
		/// the `replace_index` slot

		/// add the delay of writing to RAM
		*t += RAM_DELAY;
		mem->RAM[replace_index]->page = p;
		mem->RAM[replace_index]->placement_time = *t;
		mem->RAM[replace_index]->access_time = *t;
		mem->RAM[replace_index]->ref = 0;
		mem->RAM[replace_index]->dirty = 0;
		return 0;
	}
}

int isu_mmu_page_fetch(isu_mmu_t mem, int p, unsigned long long *t){
	/// counter
	int i;
	/// variable to keep track of state
	int ret;
	/// after this line, the variables are for handling when an empty slot cannot be found
	/// temporary variable to store the time when a page is placed in the slot
	unsigned long long temp_time;
	/// the index of the slot that will be replaced
	int replace_index = 0;
	/// loop through L1 cache to find an empty slot
	for(i = 0; i < L1_SIZE; i++){
		/// if the current slot in L1 is empty
		if(-1 == mem->L1[i]->page){
			/// increment time due to reading stuff from disk
			*t += DISK_DELAY;
			/// place our page here
			mem->L1[i]->page = p;
			mem->L1[i]->placement_time = *t;
			mem->L1[i]->access_time = *t;
			if(mem->rep_mode < 2){
				mem->L1[i]->ref = 1;
			}else{
				mem->L1[i]->ref = 0;
			}
			mem->L1[i]->dirty = 0;
			return 0;
		}
	}
	
	/// if there were no empty slots in L1, we must replace a page in L1 with the page
	/// we want. To do that, we first need to move the page based on the replacement
	/// algorithm(the case for the clock algorithm can change)
	switch(mem->rep_mode){
	case 0: /// fifo page replacement
		temp_time = mem->L1[0]->placement_time;
		for(i = 0; i < L1_SIZE; i++){
			/// if the page of the current slot has no references
			if(!(mem->L1[i]->ref)){
				/// if the page at the current slot has been around longer
				/// than the last one
				if(temp_time > mem->L1[i]->placement_time){
					temp_time = mem->L1[i]->placement_time;
					replace_index = i;
				}
			}
		}
		/// once we know the replace index, we call the move function to move the
		/// page in L1 that we just found to a lower level of cache
		isu_mmu_page_move(mem, mem->L1[replace_index]->page, 0, t);

		/// move was successful, now we place our new page in the place of
		/// the moved page
		/// increment time due to reading stuff from disk
		*t += DISK_DELAY;
		mem->L1[replace_index]->page = p;
		mem->L1[replace_index]->placement_time = *t;
		mem->L1[replace_index]->access_time = *t;
		mem->L1[replace_index]->ref = 1;
		mem->L1[replace_index]->dirty = 0;
		break;
	case 1: /// LRU page replacement
		temp_time = mem->L1[0]->access_time;
		for(i = 0; i < L1_SIZE; i++){
			/// if the page of the current slot has no references
			if(!(mem->L1[i]->ref)){
				/// if the page at the current slot has been used more recently, replace it
				if(temp_time > mem->L1[i]->access_time){
					temp_time = mem->L1[i]->access_time;
					replace_index = i;
				}
			}
		}
		/// once we know the replace index, we call the move function to move the
		/// page in L1 that we just found to a lower level of cache
		isu_mmu_page_move(mem, mem->L1[replace_index]->page, 0, t);

		/// move was successful, now we place our new page in the place of
		/// the moved page
		/// increment time due to reading stuff from disk
		*t += DISK_DELAY;
		mem->L1[replace_index]->page = p;
		mem->L1[replace_index]->placement_time = *t;
		mem->L1[replace_index]->access_time = *t;
		mem->L1[replace_index]->ref = 1;
		mem->L1[replace_index]->dirty = 0;
		break;
	case 2:	/// clock page replacement
		/// once we know the replace index, we call the move function to move the
		/// page in L1 that we just found to a lower level of cache
		isu_mmu_page_move(mem, mem->L1[mem->hand]->page, 0, t);

		/// move was successful, now we place our new page in the place of
		/// the moved page
		/// increment time due to reading stuff from disk
		*t += DISK_DELAY;
		mem->L1[mem->hand]->page = p;
		mem->L1[mem->hand]->placement_time = *t;
		mem->L1[mem->hand]->access_time = *t;
		mem->L1[mem->hand]->ref = 1;
		mem->L1[mem->hand]->dirty = 0;
		break;
	}
	return 0;
}

int isu_mmu_page_swap(isu_mmu_t mem, int old, int new, int new_level, unsigned long long *t){
	/// if the page values are out of range, return -1(error)
	if(old < 0 || new < 0){
		return -1;
	}else{
		/// loop through L1 cache to find the index of `old`
		int i;
		int replace_index = INT_MAX;
		for(i = 0; i < L1_SIZE; i++){
			if(mem->L1[i]->page == old){
				replace_index = i;
				break;
			}
		}
		switch(new_level){
		case 1: /// `new` is in L2 cache
			/// find the spot of `new`
			for(i = 0; i < L2_SIZE; i++){
				if(mem->L2[i]->page == new){
					/// switch the places of `old` and `new`
					mem->L2[i]->page = old;
					mem->L1[replace_index]->page = new;
					*t += L2_DELAY;
					mem->L1[replace_index]->access_time = *t;
					mem->L1[replace_index]->placement_time = *t;
					*t += L2_DELAY;
					mem->L2[i]->placement_time = *t;
					mem->L2[i]->access_time = *t;
					if(mem->rep_mode < 2){
						mem->L1[replace_index]->ref = 1;
					}else{
						mem->L2[replace_index]->ref = 0;
					}
					mem->L2[i]->ref = 0;
					mem->L1[replace_index]->dirty = 0;
					mem->L2[i]->dirty = 1;
				}
			}
			break;
		case 2: /// `new` is in RAM
			/// find the spot of `new`
			for(i = 0; i < RAM_SIZE; i++){
				if(mem->RAM[i]->page == new){
					/// switch the places of `old` and `new`
					mem->RAM[i]->page = old;
					mem->L1[replace_index]->page = new;
					*t += RAM_DELAY;
					mem->L1[replace_index]->access_time = *t;
					mem->L1[replace_index]->placement_time = *t;
					*t += RAM_DELAY;
					mem->RAM[i]->access_time = *t;
					mem->RAM[i]->placement_time = *t;
					if(mem->rep_mode < 2){
						mem->L1[replace_index]->ref = 1;
					}else{
						mem->L1[replace_index]->ref = 0;
					}
					mem->RAM[i]->ref = 0;
					mem->L1[replace_index]->dirty = 0;
					mem->RAM[i]->dirty = 1;
				}
			}
			break;
		}
	}
	return 0;
}

int isu_mmu_page_rep_fifo(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t){
	int ret;
	int new;
	int old;
	int i;

	/// book keeping purposes, copying all the pages in L1 cache
	for(i = 0; i < L1_SIZE; i++){
		isu_mem_req_add_page(req, mem->L1[i]->page);
	}
	
	//first, calculate the page the address is in
	unsigned short addr = isu_mem_req_get_address(req);
	int page = addr / 4096;

	//once we have the page number, check if it is in memory
	//if the page is in L1 cache, it is a hit, otherwise it is a miss
	//	if it is a miss, but it is still in memory, we swap it out
	//	if it is not in memory, we have to go fetch it
	char hit = (char)isu_mmu_page_check(mem, page);
	/// if `hit` is 0, it is a hit, and that the memory is in L1
	if(0 == hit){
		isu_mem_req_set_access_hit(req, 1);
		for(i = 0; i < L1_SIZE; i++){
			if(mem->L1[i]->page == page){
				mem->L1[i]->access_time = *t;
				mem->L1[i]->ref = 1;
			}
		}
		ret = 0;
	/// if `hit` is 1 then `page` is in L2; if `hit` is 2 then `page` is in RAM
	}else if(0 < hit){
		/// the new page to be put in the working set is `page`
		new = page;

		/// now we figure out which one is to be replaced
		/// first find the one with the earliest placement time
		unsigned long long temp = mem->L1[0]->placement_time;
		old = mem->L1[0]->page;
		for(i = 0; i < L1_SIZE; i++){
			if(temp > mem->L1[i]->placement_time){
				temp = mem->L1[i]->placement_time;
				old = mem->L1[i]->page;
			}
		}
		
		/// once the loop is complete, we know the `old` page to be replaced with
		/// the `new` page, and `hit` tells us the which level to look for `new`
		ret = isu_mmu_page_swap(mem, old, new, hit, t);
	}else{
		/// the page is not in memory, so we have to fetch it
		ret = isu_mmu_page_fetch(mem, page, t);
	}

	return ret;
}

int isu_mmu_page_rep_lru(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t){
	/// First we want to do some book keeping, copy the elements in L1
	/// to the memory request class
	int ret;
	int new;
	int old;
	int i;

	/// book keeping purposes, copying all the pages in L1 cache
	for(i = 0; i < L1_SIZE; i++){
		isu_mem_req_add_page(req, mem->L1[i]->page);
	}
	
	//first, calculate the page the address is in
	unsigned short addr = isu_mem_req_get_address(req);
	int page = addr / 4096;

	//once we have the page number, check if it is in memory
	//if the page is in L1 cache, it is a hit, otherwise it is a miss
	//	if it is a miss, but it is still in memory, we swap it out
	//	if it is not in memory, we have to go fetch it
	char hit = (char)isu_mmu_page_check(mem, page);
	/// if `hit` is 0, it is a hit, and that the memory is in L1
	if(0 == hit){
		isu_mem_req_set_access_hit(req, 1);
		for(i = 0; i < L1_SIZE; i++){
			if(mem->L1[i]->page == page){
				mem->L1[i]->access_time = *t;
				mem->L1[i]->ref = 1;
			}
		}
		ret = 0;
	/// if `hit` is 1 then `page` is in L2; if `hit` is 2 then `page` is in RAM
	}else if(0 < hit){
		/// the new page to be put in the working set is `page`
		new = page;

		/// now we figure out which one is to be replaced
		/// first find the one with the earliest access time
		unsigned long long temp = mem->L1[0]->access_time;
		old = mem->L1[0]->page;
		for(i = 0; i < L1_SIZE; i++){
			if(temp > mem->L1[i]->access_time){
				temp = mem->L1[i]->access_time;
				old = mem->L1[i]->page;
			}
		}
		
		/// once the loop is complete, we know the `old` page to be replaced with
		/// the `new` page, and `hit` tells us the which level to look for `new`
		ret = isu_mmu_page_swap(mem, old, new, hit, t);
	}else{
		/// the page is not in memory, so we have to fetch it
		ret = isu_mmu_page_fetch(mem, page, t);
	}

	return ret;
}

int isu_mmu_page_rep_clock(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t){
	/// First we want to do some book keeping, copy the elements in L1
	/// to the memory request class
	int ret;
	int new;
	int old;
	int i;

	/// book keeping purposes, copying all the pages in L1 cache
	for(i = 0; i < L1_SIZE; i++){
		isu_mem_req_add_page(req, mem->L1[i]->page);
	}
	
	//first, calculate the page the address is in
	unsigned short addr = isu_mem_req_get_address(req);
	int page = addr / 4096;

	//once we have the page number, check if it is in memory
	//if the page is in L1 cache, it is a hit, otherwise it is a miss
	//	if it is a miss, but it is still in memory, we swap it out
	//	if it is not in memory, we have to go fetch it
	char hit = (char)isu_mmu_page_check(mem, page);
	/// if `hit` is 0, it is a hit, and that the memory is in L1
	if(0 == hit){
		isu_mem_req_set_access_hit(req, 1);
		for(i = 0; i < L1_SIZE; i++){
			if(mem->L1[i]->page == page){
				mem->L1[i]->access_time = *t;
				mem->L1[i]->ref = 1;
			}
		}
		ret = 0;
	} else {
		/// the new page to be put in the working set is `page`
		new = page;

		/// now we figure out which one is to be replaced
		while (1){
			/// if we're at the end of the circular list, reset
			if (mem->hand == L1_SIZE) {
				mem->hand = 0;
			}
			
			/// if the page of the current slot has no references, pick it and move the hand
			if((mem->L1[mem->hand]->ref) == 0){
				old = mem->L1[mem->hand]->page;
				break;
			}
			/// otherwise set references to 0 and move the hand
			else {
				mem->L1[mem->hand]->ref = 0;
				mem->hand++;
			}
		}
		
		/// once the loop is complete, we know the `old` page to be replaced with
		/// the `new` page, and `hit` tells us the which level to look for `new`
		if(0 < hit){
			/// if `hit` is 1 then `page` is in L2; if `hit` is 2 then `page` is in RAM
			ret = isu_mmu_page_swap(mem, old, new, hit, t);
		}
		else {
			/// the page is not in memory, so we have to fetch it
			ret = isu_mmu_page_fetch(mem, page, t);
		}
		mem->hand++;
	}

	return ret;
}

int isu_mmu_page_rep_second_chance(isu_mmu_t mem, isu_mem_req_t req, unsigned long long *t){
	/// TODO (maybe)
}
