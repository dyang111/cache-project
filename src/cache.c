//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Dongchen Yang";
const char *studentID   = "A59003267";
const char *email       = "doy001@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//
//here I referred to the implementation of 
//https://github.com/Fnjn


typedef struct Block
{
  struct Block *previous, *next;

  uint32_t value;
}Block;

typedef struct Set
{
  uint32_t size;
  Block *front, *back;
}Set;

Block* create_Block(uint32_t value)
{
  // define the block
  Block *block = (Block*)malloc(sizeof(Block));
  // input the block specifications
  
  //default to be null, will change later if we push
  block->previous = NULL;
  block->next = NULL;

  block->value = value;
  return block;
}


void setPush(Set* set,  Block *block)
{
  // If size of the set greater than 0
  if(set->size > 0)
  { 
    set->back->next = block;
    block->previous = set->back;
    set->back = block;
  }
  // If size = 0, which means it is empty
  else if (set->size ==0)
  { 
    set->front = block;
    set->back = block;
  }
  else
  {
    set->front = block;
    set->back = block;
  }
  (set->size)++;
}

// Pop front
void setPop(Set* set){
  // If set is empty
  if(set->size == 0)
    return;
//pop out the first one
  Block *b_pop = set->front;
  set->front = b_pop->next;

  if(set->front)
    set->front->previous = NULL;
  (set->size)--;
  free(b_pop);
}

Block* setPopIndex(Set* set, int index){
 // pop out a pecific
  if(index > set->size || index<0)
    return NULL;

  Block *b_pop = set->front;

  if(set->size == 1){
    set->front = NULL;
    set->back = NULL;
  }
  else if (index == 0)
  {
    set->front = b_pop->next;
    set->front->previous = NULL;
  }
  else if (index == set->size - 1)
  {
    b_pop = set->back;
    set->back = set->back->previous;
    set->back->next = NULL;
  }
  else{
    for(int i=0; i<index; i++)
      b_pop = b_pop->next;
    b_pop->previous->next = b_pop->next;
    b_pop->next->previous = b_pop->previous;
  }

  b_pop->next = NULL;
  b_pop->previous = NULL;

  (set->size)--;
  //Block ownership transfer to caller
  return b_pop;
}






uint32_t offset_size;
uint32_t offset_mask;

Set *icache;
uint32_t icache_index_mask;
uint32_t icache_index_size;

Set *dcache;
uint32_t dcache_index_mask;
uint32_t dcache_index_size;

Set *l2cache;
uint32_t l2cache_index_mask;
uint32_t l2cache_index_size;


//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //

  icache = (Set*)malloc(sizeof(Set) * icacheSets);
  dcache = (Set*)malloc(sizeof(Set) * dcacheSets);
  l2cache = (Set*)malloc(sizeof(Set) * l2cacheSets);

//init the sets
  for(int i=0; i<icacheSets; i++)
  {
    icache[i].size = 0;
    icache[i].front = NULL;
    icache[i].back = NULL;
  }
//init the sets
  for(int i=0; i<dcacheSets; i++)
  {
    dcache[i].size = 0;
    dcache[i].front = NULL;
    dcache[i].back = NULL;
  }
//init the sets
  for(int i=0; i<l2cacheSets; i++)
  {
    l2cache[i].size = 0;
    l2cache[i].front = NULL;
    l2cache[i].back = NULL;
  }

  offset_size = (uint32_t)log2(blocksize);
  offset_size += ((1<<offset_size)==blocksize)? 0 : 1;
  offset_mask = (1 << offset_size) - 1;

  icache_index_size = (uint32_t)log2(icacheSets);

  if ((1<<icache_index_size) == icacheSets){
      icache_index_size += 0;
  }
  else
  {
    icache_index_size += 1;
  }
 

  dcache_index_size = (uint32_t)log2(dcacheSets);

  if ((1<<dcache_index_size) == dcacheSets){
      dcache_index_size += 0;
  }
  else
  {
    dcache_index_size += 1;
  }



  l2cache_index_size = (uint32_t)log2(l2cacheSets);


  if ((1<<l2cache_index_size) == l2cacheSets){
      l2cache_index_size += 0;
  }
  else
  {
    l2cache_index_size += 1;
  }
 
  icache_index_mask = ((1 << icache_index_size) - 1) << offset_size;
  dcache_index_mask = ((1 << dcache_index_size) - 1) << offset_size;
  l2cache_index_mask = ((1 << l2cache_index_size) - 1) << offset_size;

}




// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //
// If not intialized, get it in l2
  if(icacheSets==0){
    return l2cache_access(addr);
  }
  // If intialized, go see if it is a hit

  icacheRefs++;

  // get offset, index, and tag trough masking and shifting
  uint32_t offset = offset_mask & addr;
  uint32_t index = (icache_index_mask & addr) >> offset_size;
  uint32_t tag = addr >> (icache_index_size + offset_size);

  Block *p = icache[index].front;
  //see if it is a hit
  for(int i=0; i<icache[index].size; i++){
    if(p->value == tag){ // Hit
      Block *b = setPopIndex(&icache[index], i); // Get the hit block
      setPush(&icache[index],  b); // move to end of set queue
      return icacheHitTime;
    }
    p = p->next;
  }
 //it is a miss
  icacheMisses++;

  uint32_t penalty = l2cache_access(addr);
  icachePenalties += penalty;

  // Miss replacement
  Block *b = create_Block(tag);

  if(icache[index].size == icacheAssoc) // sest filled, pop out the last element
    setPop(&icache[index]);
  setPush(&icache[index],  b);

  return icacheHitTime + penalty;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //TODO: Implement D$
  //
// If not intialized, get in l2
  if(dcacheSets==0){
    return l2cache_access(addr);
  }

  dcacheRefs++ ;
 // get offset, index, and tag trough masking and shifting
  uint32_t offset = offset_mask & addr;
  uint32_t index = (dcache_index_mask & addr) >> offset_size;
  uint32_t tag = addr >> (dcache_index_size + offset_size);

  Block *p = dcache[index].front;

  for(int i=0; i<dcache[index].size; i++){
    if(p->value == tag){ // Hit
      Block *b = setPopIndex(&dcache[index], i); 
      setPush(&dcache[index],  b); // move to end of sets
      return dcacheHitTime;
    }
    p = p->next;
  }

  dcacheMisses += 1;


  uint32_t penalty = l2cache_access(addr);
  dcachePenalties += penalty;

  // Miss replacement - LRU
  Block *b = create_Block(tag);

  if(dcache[index].size == dcacheAssoc) // set filled, replace LRU (front of set queue)
    setPop(&dcache[index]);
  setPush(&dcache[index],  b);

  return dcacheHitTime + penalty;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //

  if(l2cacheSets==0){
    return memspeed;
  }
  // If intialized, go
  l2cacheRefs ++;

  uint32_t offset = offset_mask & addr;
  uint32_t index = (l2cache_index_mask & addr) >> offset_size;
  uint32_t tag = addr >> (l2cache_index_size + offset_size);

  Block *p = l2cache[index].front;

  for(int i=0; i<l2cache[index].size; i++){
    if(p->value == tag){ // Hit
      Block *b = setPopIndex(&l2cache[index], i); // Get the hit block
      setPush(&l2cache[index],  b); // move to end of set queue
      return l2cacheHitTime;
    }
    p = p->next;
  }

  l2cacheMisses += 1;

  // Miss replacement - LRU
  Block *b = create_Block(tag);
  
  if(l2cache[index].size == l2cacheAssoc){ // set filled, replace LRU 

    setPop(&l2cache[index]);
    // else suppose it's a non-inclusive cache as for the requirement
  }
  setPush(&l2cache[index], b);

  l2cachePenalties += memspeed;
  return l2cacheHitTime + memspeed;
}
