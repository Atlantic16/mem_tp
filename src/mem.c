//------------------------------------------------------------------------------
// Projet : TP CSE (malloc)
// Cours  : Conception des systèmes d'exploitation et programmation concurrente
// Cursus : Université Grenoble Alpes - UFRIM²AG - Master 1 - Informatique
// Année  : 2022-2023
//------------------------------------------------------------------------------

#include "mem.h"
#include "mem_space.h"
#include "mem_os.h"
#include <assert.h>
#include <stdio.h>


////////////////////
memPtr_t *head;

void get_prev_next_blocks(block_t* head, block_t* currBlk, block_t** prevBlk, block_t** nextBlk){
	if (head == NULL) return;
	block_t *ctrl = head;
	*prevBlk = NULL;
	*nextBlk = currBlk->next;
	
	//Looking for the prev
	while(ctrl < currBlk && ctrl != NULL){
		*prevBlk = ctrl;
		ctrl = ctrl->next;
	}

	//To avoid reflexive node
	if(ctrl != currBlk)
		*nextBlk = ctrl;
}

size_t diff_size_bytes(char* beginning, char* end){
	if(beginning != NULL && end != NULL && end > beginning)
		return end - beginning;
	
	return 0;
}

int check_valid_address(block_t *head, block_t* address){
	block_t *ctrl = head;
	int valid = 0;
	while (ctrl != NULL){
		if((block_t*)address == ctrl){
			valid = 1;
			break;
		}
		ctrl = ctrl->next;
	}
	 
	return valid;
}

int merge_free_blocks(block_t* currBusyBlk, block_t* leftFreeBlk, block_t* rightFreeBlk){
	if(rightFreeBlk != NULL && leftFreeBlk != NULL && !(currBusyBlk > leftFreeBlk && currBusyBlk < rightFreeBlk)){
		leftFreeBlk->next = rightFreeBlk->next;
		leftFreeBlk->size += rightFreeBlk->size + sizeof(block_t);
		return 1;
	}

	return 0;
}
////////////////////

//-------------------------------------------------------------
// mem_init
//-------------------------------------------------------------
/**
 * Initialize the memory allocator.
 * If already init it will re-init.
**/
void mem_init() {
	block_t *ini;
	head  = mem_space_get_addr();
	//Maybe the easiest way ?
	head->strategy = mem_first_fit;
	
	head->busyBlocksHead = NULL;
	ini = (block_t*)(head+1);
	ini->size = mem_space_get_size() - sizeof(memPtr_t) - sizeof(block_t);
	ini->next = NULL;
	head->freeBlocksHead = ini;
}

//-------------------------------------------------------------
// mem_alloc
//-------------------------------------------------------------
/**
 * Allocate a bloc of the given size.
**/
void *mem_alloc(size_t size) {
	//Variable declaration
	block_t *newBb, *prevBb, *nextBb;
	block_t *newFb, *prevFb, *nextFb;
	size_t fitSize = 0;
	
	newBb = prevBb = nextBb = newFb = prevFb = nextFb = NULL;
	
	if(size < 0) return NULL;
	
	newBb = head->strategy(head->freeBlocksHead, size);

	//Check if there is any available slot, the function is ok for size = 0
	if(newBb != NULL && newBb->size >= size){
		get_prev_next_blocks(head->busyBlocksHead, newBb, &prevBb, &nextBb);
		get_prev_next_blocks(head->freeBlocksHead, newBb, &prevFb, &nextFb);

		newBb->size = size;
		newBb->next = nextBb;

		if(prevBb == NULL) head->busyBlocksHead = newBb;
		else prevBb->next = newBb;
		
		//Check if the size would be calculated between two busy nodes or a node and the end of the memory
		if(nextBb)
			fitSize = diff_size_bytes((char*)(newBb+1) + size, (char*)nextBb);
		else
			fitSize = diff_size_bytes((char*)(newBb+1) + size, (char*)mem_space_get_addr() + (mem_space_get_size()));
		

		if(fitSize >= sizeof(block_t)){
			//Create free node for unocuppied space
			newFb = (block_t*)((char*)(newBb+1) + size);
			newFb->size = fitSize - sizeof(block_t);
		
			if(prevFb == NULL) head->freeBlocksHead = newFb;
			else{
				prevFb->next = newFb;
				newFb->next = nextFb;
			}
		}
		else{
			if(prevFb != NULL)
				prevFb->next = nextFb;
			else
				head->freeBlocksHead = nextFb;

			newBb->size += fitSize;
		}
	}

	return (newBb) ? newBb+1 : NULL;
}

//-------------------------------------------------------------
// mem_get_size
//-------------------------------------------------------------
size_t mem_get_size(void * zone)
{
	if(zone != NULL){
		if(check_valid_address(head->busyBlocksHead, zone) || check_valid_address(head->freeBlocksHead, zone))
			return ((block_t*)zone)->size;
	}
		return 0;
}

//-------------------------------------------------------------
// mem_free
//-------------------------------------------------------------
/**
 * Free an allocaetd bloc.
**/
void mem_free(void *zone) {

	block_t *prevBb, *nextBb;
	block_t *newFb, *prevFb, *nextFb;

	prevBb = nextBb = newFb = prevFb = nextFb = NULL;

	//Point on the header to get informations (size)
	zone = (block_t*)zone-1;
    if(! check_valid_address(head->busyBlocksHead, zone)){
		fprintf(stderr, "Not a valid zone\n");
		return;
	}

	get_prev_next_blocks(head->freeBlocksHead, zone, &prevFb, &nextFb);
	get_prev_next_blocks(head->busyBlocksHead, zone, &prevBb, &nextBb);
	
	newFb = (block_t*) zone;

	//Linking update
	if(prevFb == NULL){
		head->freeBlocksHead = newFb;
		newFb->next = nextFb;
	}
	else{
		prevFb->next = newFb;
		newFb->next = nextFb;
	}

	if(prevBb == NULL)
		head->busyBlocksHead = nextBb;
	else
		prevBb->next = nextBb;

	//Merge left and right side of newFb
	if(merge_free_blocks(prevBb, prevFb, newFb) && prevBb != NULL)
		merge_free_blocks(prevBb->next, prevFb, newFb->next);
	else{ 
		if((prevBb && newFb))
			merge_free_blocks(prevBb->next, newFb, newFb->next);
		else
			merge_free_blocks(nextBb, newFb, newFb->next);
	}
}

//-------------------------------------------------------------
// Itérateur(parcours) sur le contenu de l'allocateur
// mem_show
//-------------------------------------------------------------
void mem_show(void (*print)(void *, size_t, int free)) {
	block_t *freeBlock = head->freeBlocksHead;
	block_t *busyBlock = head->busyBlocksHead;

	//For more organisation, show free blocks firstly then busy blocks
	printf("\x1b[32mFree blocks:\n\x1b[0m");
	while(freeBlock != NULL){
		print((void*)((void*)freeBlock - mem_space_get_addr()), freeBlock->size, 1);
		freeBlock = freeBlock->next;
	}
	printf("\x1b[32m===================\n\x1b[0m");

	printf("\x1b[31mBusy blocks:\n\x1b[0m");
	while(busyBlock != NULL){	
		print((void*)((void*)busyBlock - mem_space_get_addr()), busyBlock->size, 0);
		busyBlock = busyBlock->next;
	}
	printf("\x1b[31m===================\n\x1b[0m");
}

//-------------------------------------------------------------
// mem_fit
//-------------------------------------------------------------
void mem_set_fit_handler(mem_fit_function_t *mff) {
	//mff = mem_first_fit;
	head->strategy = mem_first_fit;
}

//-------------------------------------------------------------
// Stratégies d'allocation
//-------------------------------------------------------------
block_t *mem_first_fit(block_t *first_free_block, size_t wanted_size) {
    block_t *ctrl = first_free_block;
	while(ctrl != NULL){
		if(ctrl->size >= wanted_size) {return ctrl;}
		else{ctrl = ctrl->next;}
	}
	return NULL;
}
//-------------------------------------------------------------
block_t *mem_best_fit(block_t *first_free_block, size_t wanted_size) {
    block_t *ctrl, *bestBlockAddress;
	ctrl = bestBlockAddress = first_free_block;
	size_t bestBlockSize = ctrl->size;

	while(ctrl != NULL){
		if(ctrl->size >= wanted_size && bestBlockSize > ctrl->size) {
			bestBlockAddress = ctrl;
			bestBlockSize = ctrl->size;
		}
		ctrl = ctrl->next;
	}

	return bestBlockAddress;
}

//-------------------------------------------------------------
block_t *mem_worst_fit(block_t *first_free_block, size_t wanted_size) {
    block_t *ctrl, *WorstBlockAddress;
	ctrl = WorstBlockAddress = first_free_block;
	size_t WorstBlockSize = ctrl->size;

	while(ctrl != NULL){
		if(ctrl->size >= wanted_size && WorstBlockSize < ctrl->size) {
			WorstBlockAddress = ctrl;
			WorstBlockSize = ctrl->size;
		}
		ctrl = ctrl->next;
	}

	return WorstBlockAddress;
}