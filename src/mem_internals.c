/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    // Définition du premier bout de la zone mémoire à marquer
    unsigned long* first_end = ptr;

    // Première étape du marquage (premiers 16 bits)
    *first_end = size;
        // Calcul de la valeur magique et écriture en mémoire
    unsigned long magic_number = (knuth_mmix_one_round((unsigned long)(first_end)) & ~(0b11UL)) | k ;
    *(first_end+1)=magic_number;

    // Deuxième étape
        // On utilise le fait que size est la taille totale allouée
    unsigned long* second_end = ptr+size-2*sizeof(unsigned long);
    *second_end = magic_number;
    *(second_end+1) = size; 
    
    return (void *)(ptr+2*sizeof(unsigned long));
}

Alloc mark_check_and_get_alloc(void *ptr)
{
    // Extraction des informations du premier bout de la zone mémoire dédiée
    unsigned long* real_ptr = ptr-2*sizeof(unsigned long);
    unsigned long size = *real_ptr;
    unsigned long magic_number = *(real_ptr+1);
    
    // Deuxième bout
    unsigned long* second_end = (void*)real_ptr +size-2*sizeof(unsigned long);
    unsigned long magic_number_to_verify = *(second_end);
    unsigned long size_to_verify = *(second_end+1);
    

    // Vérification de la cohérence des résultats obtenus
    assert((size==size_to_verify)&&(magic_number==magic_number_to_verify));

    // Détermination du type de l'allocation
    MemKind k = magic_number & (0b11UL);

    // Rassemblement des informations dans la structure a de type Alloc  
    Alloc a;
    a.ptr = real_ptr;
    a.size = size;
    a.kind = k;

    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
