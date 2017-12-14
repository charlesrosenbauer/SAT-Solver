#ifndef __ALLOC_HEADER__
#define __ALLOC_HEADER__










typedef struct {
  void* nextBlock;
  void* prevBlock;

  void* allcPtr;
  void* basePtr;
  void* lastFrame;
  int top;
  int size;
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
void  pushFrame  (Allocator*);
void  popFrame   (Allocator*);



#endif
