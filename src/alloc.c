#include "alloc.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "global.h"










AllocBlock* mkAllocBlock(void* ptr, int size, AllocBlock* last){
  AllocBlock* ret = (AllocBlock*)ptr;
  ret->prevBlock = last;
  ret->nextBlock = NULL;

  ret->allcPtr   = ptr  + sizeof(AllocBlock);
  ret->basePtr   = ret->allcPtr;
  ret->top       = size - sizeof(AllocBlock);
  ret->size      = size - sizeof(AllocBlock);
  ret->lastFrame = NULL;

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
    #ifdef _TEST_MODE_
    printf("Alloc\n");
    #endif
    block->nextBlock = mkAllocBlock(malloc(block->size), block->size, block);
    *top = block->nextBlock;
    return blockAlloc(block->nextBlock, size, align, top);
  }else{
    block->allcPtr = (void*)newloc;
    return (void*)loc;
  }
}










void* alloc(Allocator* a, int size){
  return blockAlloc(a->lastBlock, size, 1, &a->lastBlock);
}










void* allocAlign(Allocator* a, int size, int align){
  return blockAlloc(a->lastBlock, size, align, &a->lastBlock);
}











void pushFrame(Allocator* a){
  AllocBlock* lastBlock = (AllocBlock*)a->lastBlock;
  if(lastBlock->allcPtr - (lastBlock->basePtr + lastBlock->size) < sizeof(void*)){
    //No room in the current frame!
    lastBlock->nextBlock = mkAllocBlock(malloc(lastBlock->size), lastBlock->size, lastBlock);
    a->lastBlock = lastBlock->nextBlock;
    AllocBlock* nextBlock = lastBlock->nextBlock;
    nextBlock->lastFrame = (void*)-1;
  }else{
    void** framePtr = (void**)alloc(a, sizeof(void*));
    *framePtr = lastBlock->lastFrame;
  }
}










void popFrame(Allocator* a){
  AllocBlock* lastBlock = (AllocBlock*)a->lastBlock;
  void* lastFrame  = lastBlock->lastFrame;
  int64_t frameInt = (int64_t)lastFrame;
  if(lastFrame == NULL){
    // Last frame is at the beginning of the frame
    lastBlock->allcPtr = lastBlock->basePtr;
  }else if(frameInt < 0){
    // Last frame is in previous block
    if(lastBlock == a->initBlock){
      //No previous block
      lastBlock->allcPtr = lastBlock->basePtr;
    }else{
      AllocBlock* last = (AllocBlock*)a->lastBlock;
      a->lastBlock = last->prevBlock;
      AllocBlock* newLastBlock = (AllocBlock*)a->lastBlock;
      newLastBlock->nextBlock = NULL;
      free(last);
      popFrame(a);
    }
  }else{
    // Last frame is in current block
    void** prevFrame = (void**)lastFrame;
    lastBlock->lastFrame = *prevFrame;
    lastBlock->allcPtr   =  lastFrame;
  }
}










void rmAllocator(Allocator* a){

  AllocBlock* current = (AllocBlock*)a->initBlock;
  do{
    AllocBlock* next  = (AllocBlock*)current->nextBlock;
    free(current);
    current =   next;
  }while(current != NULL);
  free(a);
}
