/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    // Vérifier que la taille à allouer est bien petite
    assert(size<=SMALLALLOC);

    if (!arena.chunkpool){
        // Construction de la liste chainée dans le cas où cette dernière est vide
        unsigned long bloc_size = mem_realloc_small();
        int i;
        for(i=0;i<bloc_size-CHUNKSIZE;i+=CHUNKSIZE){
            // Chaque noeud pointe vers le prochain
            *(void**) (arena.chunkpool+i) = (arena.chunkpool+CHUNKSIZE+i);
        }
    }
    // Stocker puis éliminer la tête de la liste chainée
    void* mem_ptr = arena.chunkpool; 
    arena.chunkpool =  *((void**)(mem_ptr));
    return mark_memarea_and_get_user_ptr(mem_ptr,CHUNKSIZE,SMALL_KIND);

}

void efree_small(Alloc a) {
    void* chunk_ptr = a.ptr;
    // Pointer le chunkpool depuis le nouveau chunk
    *((void**)chunk_ptr)=arena.chunkpool;
    // Mettre à jour la tête de la liste chainée
    arena.chunkpool = chunk_ptr;
}
