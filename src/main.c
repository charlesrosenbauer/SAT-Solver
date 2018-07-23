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
  t = clock() - t;

  TRANSLATION trans = reorderVars(&cnf, 320);
  printf("%f\n", translationScore(trans, cnf));

  for(int i = 0; i < trans.size; i++){
    trans.trans[i] = i;
  }
  printf("%f\n", translationScore(trans, cnf));

  //TABLE* table = initTable(&cnf, 0);

  printf("%i %i %p, %f seconds\n\n", cnf.varnum, cnf.clausenum, NULL, ((float)t / CLOCKS_PER_SEC));



  return 0;
}
