#include "cnfparser.h"
#include "stdio.h"
#include "stdlib.h"
#include "alloc.h"










int main(int argc, char** argv){
  FILE*  pFile;
  long   lSize;
  char*  buffer;
  size_t result;

  if(argc < 2){
    printf("No file parameter.\n");
    return 1;
  }

  pFile = fopen(argv[1], "rb");
  if(pFile == NULL) {
    printf("Unable to load file.\n");
    return 2;
  }

  fseek(pFile , 0 , SEEK_END);
  lSize = ftell(pFile);
  rewind(pFile);

  buffer = (char*) malloc(sizeof(char)*lSize);
  if (buffer == NULL) {
    printf("Memory error.");
    return 3;
  }

  result = fread(buffer, 1, lSize, pFile);
  if (result != lSize){
    printf("Unable to read file.\n");
    return 4;
  }


  CNF cnf = parseCNF(buffer, lSize);
  printf("%i %i %p\n", cnf.varnum, cnf.clausenum, cnf.clauses);

  /*
  Allocator* allocator = mkAllocator(16384);
  for(int i = 0; i < 1024; i++){
    int* x = (int*)alloc(allocator, 400);
    *x = i;
    if(i % 64 == 0){
      printf("%p %i\n", x, *x);
    }
  }

  AllocBlock* last = allocator->lastBlock;
  AllocBlock* init = allocator->initBlock;
  AllocBlock* current = init;

  printf("%p %p\n", last, init);
  do{
    printf("Still looking. %p\n", current);
    current = current->nextBlock;
  }while(current != last);
  */
  
  return 0;
}
