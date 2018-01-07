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
  printf("%i %i %p\n\n", cnf.varnum, cnf.clausenum, cnf.clauses);

  IntPair* mentions = sortByMentions(&cnf);

  PersistentByteArray* arr = createByteArray(cnf.varnum);
  printf("%i\n", pbaRead(arr, 10));

  /*
  // I'll worry about the allocator later
  Allocator* allocator = mkAllocator(16384);
  pushFrame(allocator);
  for(int i = 0; i < 1048576; i++){
    int* x = (int*)alloc(allocator, 400);
    *x = i;
    if(i % 4096 == 0) printf("!!\n");
    if(i % 65536 == 0){
      printf("---\n");

      popFrame(allocator);
      pushFrame(allocator);
    }
  }
  */

  return 0;
}
