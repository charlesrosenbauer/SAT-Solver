#include "alloc.h"
#include "stdlib.h"










AllocBlock* mkAllocBlock(void* ptr, int size, AllocBlock* last){
  AllocBlock* ret = (AllocBlock*)ptr;
  ret->prevBlock = last;
  ret->nextBlock = NULL;

  ret->allcPtr   = ptr  + sizeof(AllocBlock);
  ret->basePtr   = ret->allcPtr;
  ret->top       = size - sizeof(AllocBlock);
  ret->size      = 0;
  ret->lastTop   = 0;

  return ret;
}










Allocator* mkAllocator(int size){
  Allocator* alloc = malloc(sizeof(Allocator));
  if(size <= sizeof(AllocBlock))
    return NULL;
  alloc->blockSize = size;
  alloc->initBlock = mkAllocBlock(malloc(size), size, NULL);
  alloc->lastBlock = alloc->initBlock;
  return alloc;
}









/*
void* blockAlloc(AllocBlock* block, int size, int align){

}*/
