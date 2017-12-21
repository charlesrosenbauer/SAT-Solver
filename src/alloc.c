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
    printf("Alloc\n");
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
  void** frameHeader = (void**)allocAlign(a, sizeof(void*), 2);
  long frameHeaderBits = (long)frameHeader;
  AllocBlock* lastBlockRef = (AllocBlock*)a->lastBlock;
  long basePtrBits = (long)lastBlockRef->basePtr;

  if((frameHeaderBits - basePtrBits) < 4){  // Check if near the bottom
    //Allocated on new block. Make minor changes.
    printf("Pushblock\n");
    lastBlockRef->lastFrame = (void*)frameHeader;
    *frameHeader = NULL;  // NULL indicates that the frame pointer is in the previous block.
  }else{
    printf("Pushhere\n");
    *frameHeader = lastBlockRef->lastFrame;
    lastBlockRef->lastFrame = frameHeader;
  }
}










void popFrame(Allocator* a){
  AllocBlock* lastBlockRef = (AllocBlock*)a->lastBlock;
  void** frameHeader = (void**)lastBlockRef->lastFrame;

  if(*frameHeader == NULL){   // Dereferencing a NULL pointer
    // Remove top block, rewind
    AllocBlock* top = a->lastBlock;
    a->lastBlock = top->prevBlock;
    if(a->lastBlock != NULL){
      printf("Popback\n");
      free(top);
      return popFrame(a);
    }else{
      // Cannot rewind further, just reset first block
      printf("Poppound\n");
      mkAllocBlock(a->initBlock, a->blockSize, NULL);
      a->lastBlock = a->initBlock;
      return;
    }
  }else{
    printf("Popnormal\n");
    lastBlockRef = (AllocBlock*)a->lastBlock;
    lastBlockRef->allcPtr = frameHeader;
  }
}
