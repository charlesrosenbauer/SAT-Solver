#include "alloc.h"
#include "stdlib.h"
#include "stdio.h"










AllocBlock* mkAllocBlock(void* ptr, int size, AllocBlock* last){
  AllocBlock* ret = (AllocBlock*)ptr;
  ret->prevBlock = last;
  ret->nextBlock = NULL;

  ret->allcPtr   = ptr  + sizeof(AllocBlock);
  ret->basePtr   = ret->allcPtr;
  ret->top       = size - sizeof(AllocBlock);
  ret->size      = size - sizeof(AllocBlock);
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










// Assumes align is an exponent of 2
void* blockAlloc(AllocBlock* block, int size, int align, void** top){
  long loc = (long)block->allcPtr;
  if (loc & (align - 1))
    loc = (loc + align) & (~(align - 1));

  long newloc = loc + size;
  if((newloc - (long)block->basePtr) >= block->size){
    //Overflow! Go to next block!
    block->nextBlock = mkAllocBlock(malloc(block->size), block->size, block);
    *top = block->nextBlock;
    return blockAlloc(block->nextBlock, size, align, top);
  }else{
    block->allcPtr = (void*)newloc;
    return (void*)loc;
  }
  return NULL;
}










void* alloc(Allocator* a, int size){
  return blockAlloc(a->lastBlock, size, 1, &a->lastBlock);
}










void* allocAlign(Allocator* a, int size, int align){
  return blockAlloc(a->lastBlock, size, align, &a->lastBlock);
}
