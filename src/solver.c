#include "alloc.h"
#include "solver.h"
#include "stdlib.h"










int* countMentions(CNF* cnf){
  int* mentions = malloc(sizeof(int) * cnf->varnum);

  for(int i = 0; i < cnf->varnum; i++)
    mentions[i] = 0;

  for(int i = 0; i < cnf->clausenum; i++){
    Clause c = cnf->clauses[i];

    for(int j = 0; j < c.numvars; j++)
      mentions[c.vars[j]]++;

  }

  return mentions;
}










typedef struct{
  int val, x;
}IntPair;


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










int* sortByMentions(CNF* cnf){

  int mentionsize = cnf->varnum;
  int* mentions   = countMentions(cnf);

  IntPair* pairs = malloc(sizeof(IntPair) * mentionsize);

  for(int i = 0; i < mentionsize; i++){
    pairs[i].val = i + 1;
    pairs[i].x   = mentions[i];
  }

  quicksort(pairs, 0, mentionsize-1);

  for(int i = 0; i < mentionsize; i++)
    mentions[i] = pairs[i].val;

  free(pairs);
  return mentions;
}
