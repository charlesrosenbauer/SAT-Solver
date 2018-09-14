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










inline int cttz(uint64_t v){
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










inline uint64_t isomsb(uint64_t v){

  uint64_t x = v >> 1;
  x |= (x >>  1);
  x |= (x >>  2);
  x |= (x >>  4);
  x |= (x >>  8);
  x |= (x >> 16);
  x |= (x >> 32);

  return v & ~x;
}










inline int ctlz (uint64_t v){
  return cttz(isomsb(v));
}










inline int popcount(uint64_t x){
  const uint64_t m0 = 0x5555555555555555;
  const uint64_t m1 = 0x3333333333333333;
  const uint64_t m2 = 0x0f0f0f0f0f0f0f0f;
  const uint64_t m3 = 0x0101010101010101;
  x -= (x >> 1) & m0;
  x = (x & m1) + ((x >> 2) & m1);
  x = (x + (x >> 4)) & m2;
  return (x * m3) >> 56;
}










uint64_t rng64(){
  rseed[0] = (rseed[0] * 135713786917) + 1519657161;
  rseed[1] = (rseed[1] * 513761798575) + 3671967169;
  rseed[0] =  rseed[0] ^  rseed[1] - 1;
  return rseed[0];
}










void rngseed(uint64_t a, uint64_t b){
  rseed[0] = a;
  rseed[1] = b;
}
