#include "cnfparser.h"
#include "stdio.h"
#include "stdlib.h"










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
  return 0;
}
