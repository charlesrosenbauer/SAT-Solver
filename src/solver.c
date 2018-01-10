#include "alloc.h"
#include "solver.h"
#include "stdlib.h"
#include "stdio.h"
#include "global.h"










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
  ret->size = size;
  ret->depth = 0;
  int bottomNodes = (size % 256 == 0)? (size / 256) : (size / 256)+1;

  char** blockrefs = malloc(sizeof(char*) * bottomNodes);
  for(int i = 0; i < bottomNodes; i++){
    blockrefs[i] = malloc(sizeof(char) * 256);

    #ifdef _TEST_MODE_
    for(int j = 0; j < 256; j++)
      blockrefs[i][j] = j + i;
    #endif
  }

  if(bottomNodes <= 32){
    for(int i = 0; i < bottomNodes; i++)
      ret->nodes[i] = blockrefs[i];

    free(blockrefs);
    return ret;
  }else{

    int moveNodes   = bottomNodes;
    int middleNodes = (bottomNodes % 32 == 0)? (size / 32) : (size / 32)+1;
    PersistentNode** noderefs = malloc(sizeof(PersistentNode*) * middleNodes);

    while(middleNodes > 32){

      for(int i = 0; i < middleNodes; i++)
        noderefs[i] = malloc(sizeof(PersistentNode));

      for(int i = 0; i < moveNodes; i++)
        noderefs[i/32]->nodes[i%32] = blockrefs[i];

      void* tmppt = blockrefs;
      blockrefs = (char**)noderefs;
      noderefs  = (PersistentNode**)tmppt;

      moveNodes = middleNodes;
      middleNodes = (moveNodes % 32 == 0)? (moveNodes / 32) : (moveNodes / 32)+1;

      ret->depth++;
    }

    for(int i = 0; i < middleNodes; i++)
      ret->nodes[i] = noderefs[i];

    free(noderefs);
    free(blockrefs);
    return ret;
  }
}










unsigned char pbaRead(PersistentByteArray* pba, int index){
  if(index >= pba->size) return 0;
  void** buffer = (void**)pba->nodes;

  if(pba->depth == 0){
    unsigned char* lastBuffer = (unsigned char*)buffer[index >> 8];
    return lastBuffer[index % 256];
  }

  int adjustedIndex = index >> 8;

  int indexes[16];  // No way anyone's getting this high.
  for(int i = 0; i < pba->depth; i++)
    indexes[i] = (adjustedIndex >> (5 * i)) % 32;

  for(int i = pba->depth-1; i >= 0; i--)
    buffer = ((PersistentNode*)buffer[indexes[i]])->nodes;

  unsigned char* lastBuffer = (unsigned char*)buffer;
  return lastBuffer[index%256];
}










unsigned char* pbaPointer(PersistentByteArray* pba, int index){
  if(index >= pba->size) return 0;
  void** buffer = (void**)pba->nodes;

  if(pba->depth == 0){
    unsigned char* lastBuffer = (unsigned char*)buffer[index >> 8];
    return &(lastBuffer[index % 256]);
  }

  int adjustedIndex = index >> 8;

  int indexes[16];  // No way anyone's getting this high.
  for(int i = 0; i < pba->depth; i++)
    indexes[i] = (adjustedIndex >> (5 * i)) % 32;

  for(int i = pba->depth-1; i >= 0; i--)
    buffer = ((PersistentNode*)buffer[indexes[i]])->nodes;

  unsigned char* lastBuffer = (unsigned char*)buffer;
  return &(lastBuffer[index%256]);
}
