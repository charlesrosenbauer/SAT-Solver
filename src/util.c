#include "util.h"










void quicksort(IntPair* arr, int lo, int hi){
  int pivot, j, i;
  IntPair temp;
  if(lo < hi){
    pivot = lo;
    i = lo;
    j = hi;

    while(i < j){

      while((arr[i].x <= arr[pivot].x) && (i < hi)) i++;

      while(arr[j].x > arr[pivot].x) j--;

      if(i < j){
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }

    temp = arr[pivot];
    arr[pivot] = arr[j];
    arr[j] = temp;
    quicksort(arr, lo , j-1);
    quicksort(arr, j+1, hi );
  }
}










int cttz(uint64_t v){
  // This should probably be optimized. These if statements will kill branch performance.

  int c = 32;
  int64_t x = v;
  x &= -x;
  if (x) c--;
  if (x & 0x0000FFFF) c -= 16;
  if (x & 0x00FF00FF) c -= 8;
  if (x & 0x0F0F0F0F) c -= 4;
  if (x & 0x33333333) c -= 2;
  if (x & 0x55555555) c -= 1;

  return c;
}










uint64_t isomsb(uint64_t v){

  uint64_t x = v >> 1;
  x |= (x >>  1);
  x |= (x >>  2);
  x |= (x >>  4);
  x |= (x >>  8);
  x |= (x >> 16);
  x |= (x >> 32);
  
  return v & ~x;
}
