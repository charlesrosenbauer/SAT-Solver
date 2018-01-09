#include "alloc.h"
#include "solver.h"
#include "stdlib.h"
#include "stdio.h"










int* countMentions(CNF* cnf){
  int* mentions = malloc(sizeof(int) * cnf->varnum);

  for(int i = 0; i < cnf->varnum; i++)
    mentions[i] = 0;

  for(int i = 0; i < cnf->clausenum; i++){
    Clause c = cnf->clauses[i];

    for(int j = 0; j < c.numvars; j++){
      int index = (c.vars[j] < 0)? -c.vars[j] : c.vars[j];
      mentions[index-1]++;
    }
  }

  return mentions;
}










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










IntPair* sortByMentions(CNF* cnf){

  int mentionsize = cnf->varnum;
  int* mentions   = countMentions(cnf);

  IntPair* pairs = malloc(sizeof(IntPair) * mentionsize);

  for(int i = 0; i < mentionsize; i++){
    pairs[i].val = i + 1;
    pairs[i].x   = mentions[i];
  }

  quicksort(pairs, 0, mentionsize-1);

  //for(int i = 0; i < mentionsize; i++)
  //  mentions[i] = pairs[i].val;

  free(mentions);
  return pairs;
}










PersistentByteArray* createByteArray(int size){
  PersistentByteArray* ret = malloc(sizeof(PersistentByteArray));
  ret->size  = size;
  ret->depth = 0;

  int bottomNodes = (size % 256 == 0)? (size / 256) : (size / 256)+1;
  char** base = malloc(sizeof(char*) * bottomNodes);
  for(int i = 0; i < bottomNodes; i++){
    base[i] = malloc(sizeof(char) * 256);

    // Testing
    for(int j = 0; j < 256; j++){
      base[i][j] = j + i;
    }
  }

  int topNodes = bottomNodes;
  void** prevLayer = (void**)base;
  while(topNodes > 32){
    int tmp = topNodes;
    topNodes = (topNodes % 32 == 0)? (topNodes / 32) : (topNodes / 32)+1;

    PersistentNode* topLayer = malloc(sizeof(PersistentNode) * topNodes);
    for(int i = 0; i < tmp; i++)
      topLayer[i/32].nodes[i%32] = prevLayer[i];

    prevLayer = (void**)topLayer;
    ret->depth++;
  }

  for(int i = 0; i < topNodes; i++)
    ret->nodes[i] = prevLayer[i];

  free(base);
  return ret;
}










unsigned char pbaRead(PersistentByteArray* pba, int index){
  if(index >= pba->size) return 0;
  void** buffer = (void**)pba->nodes;

  if(pba->depth == 0){
    unsigned char* lastBuffer = (unsigned char*)buffer[index >> 8];
    return lastBuffer[index % 256];
  }

  int adjustedIndex = index >> 8;

  for(int i = pba->depth; i > 0; i--){
    int thisIndex = (adjustedIndex >> (5 * i)) % 32;
    printf("%i -> ", adjustedIndex);
    buffer = ((PersistentNode*)buffer[thisIndex])->nodes;
  }
  unsigned char* lastBuffer = (unsigned char*)buffer;
  return lastBuffer[index%256];
}










unsigned char* pbaPointer(PersistentByteArray* pba, int index){
  if(index >= pba->size) return NULL;

  void** buffer = (void**)pba->nodes;
  for(int i = pba->depth; i > 0; i--){
    int thisIndex = (index >> ((5 * (i-1)) + 8)) % 32;
    //printf("T: %i %i\n", index, thisIndex);
    buffer = ((PersistentNode*)buffer[thisIndex])->nodes;
  }
  unsigned char* lastBuffer = (unsigned char*)buffer;
  return &(lastBuffer[index%256]);
}
