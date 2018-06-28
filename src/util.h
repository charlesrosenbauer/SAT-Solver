#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__

#include "stdint.h"










typedef struct{
  int val, x;
}IntPair;










void quicksort(IntPair*, int, int);
int cttz(uint64_t);
uint64_t isomsb(uint64_t);
int ctlz(uint64_t);





#endif
