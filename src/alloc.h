#ifndef __ALLOC_HEADER__
#define __ALLOC_HEADER__










typedef struct {
  void* nextBlock;
  void* prevBlock;

  void* allcPtr;
  void* basePtr;
  int top;
  int size;
  int lastTop;
}AllocBlock;










typedef struct {
  void* initBlock;
  void* lastBlock;
  int   blockSize;
}Allocator;










Allocator*  mkAllocator(int);
void  rmAllocator(Allocator*);
void* alloc      (Allocator*, int);
void* allocAlign (Allocator*, int, int);
void* frameAlloc (Allocator*, int);
void  popAlloc   (Allocator*);



#endif
