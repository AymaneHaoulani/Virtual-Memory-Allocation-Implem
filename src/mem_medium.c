/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


// Une helper function qui va nous permettre de mener notre processus de récursion
void split_bloc(void* available_bloc, unsigned long current_index, unsigned long target_index){
    if (current_index==target_index){
        // Cette condition constitue l'état d'arrêt de notre processus de récursion
        return;
    }
    else{
        // Sinon, on se propose de positionner le compagnon en tête de la liste chainée de l'indice au dessous
        unsigned long new_bloc_size = 1 << (current_index-1);
        void* compagnon = (void*) ((unsigned long)(available_bloc) ^ new_bloc_size);
        *((void**) compagnon) = arena.TZL[current_index-1];
        arena.TZL[current_index-1]= compagnon;
        split_bloc(available_bloc, current_index-1,target_index);
    }

}


void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    
    // Considérer également l'espace occupé par 2*(size,magic_number)
    unsigned int target_index = puiss2(size + 4*sizeof(unsigned long));
    unsigned int current_index= target_index;
    void* available_bloc;

    if (!arena.TZL[target_index]){
        // Cas où aucun bloc de la taille souhaitée n'est disponible
        while (!arena.TZL[current_index] && (current_index<FIRST_ALLOC_MEDIUM_EXPOSANT+arena.medium_next_exponant)){
            // On progresse au sein de la TZL jusqu'à en trouver un
            current_index++;
        }
        if(current_index==FIRST_ALLOC_MEDIUM_EXPOSANT+arena.medium_next_exponant){
            // On fait appel à cette méthode si aucun espace possible n'a été retrouvé
            mem_realloc_medium();
        }
        // Division récursive du bloc disponible jusqu'à l'obtention d'un bloc de taille 2^(target_index)
        available_bloc = arena.TZL[current_index];
        arena.TZL[current_index] = *((void**) available_bloc);
        split_bloc(available_bloc,current_index,target_index);

    }
    else{
        // Cas simple où un bloc de la taille souhaitée est disponible
        available_bloc = arena.TZL[target_index];
        arena.TZL[target_index] = *((void**) available_bloc);
    }

    // Marquage et renvoi de l'adresse en question
    unsigned long real_size = 1 << target_index;
    return mark_memarea_and_get_user_ptr(available_bloc,real_size,MEDIUM_KIND);
}
    


void efree_medium(Alloc a) {

    void* current_bloc = a.ptr;

    // Détermination de l'indice qui nous intéresse
    unsigned int index = puiss2(a.size);

    while(index<FIRST_ALLOC_MEDIUM_EXPOSANT+arena.medium_next_exponant){
        
        // Calcul de l'adresse du buddy
        void* buddy = (void*) ((unsigned long)(current_bloc) ^ (1 << index));

    // Vérification de l'existence du buddy dans le bon endroit à la TZL
        void* current_ptr = arena.TZL[index];
        
        if(current_ptr==buddy){
            // Buddy retrouvé en tête de la liste chainée
            // On l'enlève de la liste et on réitère le processus global avec un bloc plus compact et un indice incrémenté 
            arena.TZL[index] = *((void**) buddy);
            if (current_bloc>buddy){
                current_bloc = buddy;
            }
        }
        else{
            void* previous_ptr;
            while((current_ptr) && (current_ptr != buddy)){
                previous_ptr = current_ptr;
                current_ptr = *((void**) current_ptr);
            }
            if(!(current_ptr)){
                // Cas où le buddy n'existe pas dans cette liste 
                *((void**) current_bloc) = arena.TZL[index];
                arena.TZL[index]= current_bloc;
                // On sort de la boucle while
                break;
            }
            if(current_ptr==buddy){
                // Buddy retouvé
                // On l'enlève de la liste et on réitère le processus global avec un bloc plus compact et un indice incrémenté 
                *((void**) previous_ptr)=*((void**) current_ptr);
                if (current_bloc>buddy){
                    current_bloc = buddy;
                }
            }
        }
        index+=1;
    }
    

}


