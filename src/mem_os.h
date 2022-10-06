//------------------------------------------------------------------------------
// Projet : TP CSE (malloc)
// Cours  : Conception des systèmes d'exploitation et programmation concurrente
// Cursus : Université Grenoble Alpes - UFRIM²AG - Master 1 - Informatique
// Année  : 2022-2023
//------------------------------------------------------------------------------

#ifndef MEM_OS_H
#define MEM_OS_H

//include stdlib pour definition du type size_t
#include <stdlib.h>

//Definie la structure the bloc libre
typedef struct mem_free_block_s block_t;

/* -----------------------------------------------*/
/* Interface de gestion de votre allocateur       */
/* -----------------------------------------------*/
// Initialisation
void mem_init(void);

// Définition du type mem_fit_function_t
// type des fonctions d'allocation
typedef block_t *(mem_fit_function_t)(block_t *, size_t);

// Choix de la fonction d'allocation
// = choix de la stratégie de l'allocation
void mem_set_fit_handler(mem_fit_function_t *);

// Stratégies de base (fonctions) d'allocation
mem_fit_function_t mem_first_fit;
mem_fit_function_t mem_worst_fit;
mem_fit_function_t mem_best_fit;

struct mem_free_block_s{
    size_t size;
    block_t *next;
};

typedef struct memPtr_t{
    mem_fit_function_t *strategy;
    block_t *freeBlocksHead;
    block_t *busyBlocksHead;
}memPtr_t;

/*
Description: function that look for the previous and next node of a given struct (busyblock/freeBlock) based on a giver address.
Return: void
Pre-conditions: head not NULL/Empty.
Post-conditions: prevBlk = node just after currBlk, nextBlk = node just before currBlk.
*/
void get_prev_next_blocks(block_t* head, block_t* currBlk, block_t** prevBlk, block_t** nextBlk);

/*
Description: function that calculat how many bytes are between two given params (beginning/end).
Return: number of bytes betweem end and beginning of type (size_t).
Pre-conditions: beginning & end not NULL, beginning > end.
Post-conditions: No side effect.
*/
size_t diff_size_bytes(char* beginning, char* end);

/*
Description: function that check if an address if held in a given data struct (busyblock/freeBlock).
Return: 1 if the address exists, 0 otherwise.
Pre-conditions: head not NULL/Empty address not NULL.
Post-conditions: No side effect.
*/
int check_valid_address(block_t *head, block_t* address);

/*
Description: function that to merge two free blocs into 1 bloc.
Return: 1 if the merge took place, 0 otherwise.
Pre-conditions: currBusyBlk not NULL !(leftFreeBlk < currBusyBlk < rightFreeBlk).
Post-conditions: leftFreeblk is the only bloc kept after size extenssion.
*/
int merge_free_blocks(block_t* currBusyBlk, block_t* leftFreeBlk, block_t* rightFreeBlk);

#endif /* MEM_OS_H */
