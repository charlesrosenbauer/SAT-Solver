#include "cnfparser.h"
#include "stdio.h"
#include "stdlib.h"
#include "global.h"
#include "time.h"
#include "table.h"
#include "util.h"
#include "predictor.h"










int main(int argc, char** argv){
  FILE*  pFile;
  long   lSize;
  char*  buffer;
  size_t result;

  if(argc < 2){
    printf("No file parameter.\n");
    return 1;
  }

  clock_t t = clock();

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

  CNF cnf = parseCNF(buffer, lSize, 0);


  TABLE* table = initTable(&cnf, 0);
  t = clock() - t;

  printf("%i %i %p, %f seconds\n\n", cnf.varnum, cnf.clausenum, NULL, ((float)t / CLOCKS_PER_SEC));

  SOLVERSTATE s = makeSolverState(&cnf);

  int x = getconstants(&s, &cnf, table);

  if(x == 0){
    printf("No trivial conflicts\n");
  }else{
    printf("Trivial conflict on literal #%i\n", x);
  }

  return 0;
}
