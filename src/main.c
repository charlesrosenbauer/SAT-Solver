#include "cnfparser.h"
#include "stdio.h"
#include "stdlib.h"
#include "global.h"
#include "time.h"
#include "table.h"
#include "util.h"
#include "predictor.h"










int mainpass(char** argv, int argi){
  FILE* pFile;
  long   lSize;
  char*  buffer;
  size_t result;

  clock_t t = clock();

  pFile = fopen(argv[argi], "rb");
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

  SOLVERSTATE s = makeSolverState(&cnf);

  int x = getconstants(&s, &cnf, table);

  if(x == 0){
    printf("No trivial conflicts\n");
  }else if(x != -1){
    printf("Trivial conflict on literal #%i\n", x);
  }else{
    printf("Problem solved by constant propagation.\n");
    return 0;
  }
/*
  if(!x){
    x = approximator(&s, &cnf, table);

    if(x){
      printf("Approximation found a valid solution.\n");
    }else{
      printf("Approximation failed to find a valid solution. Continuing to full search.\n");
    }
  }*/

  t = clock() - t;

  printf("%i %i %p, %f seconds\n\n", cnf.varnum, cnf.clausenum, NULL, ((float)t / CLOCKS_PER_SEC));

  //freeTable(table);   // For some reason this causes segfaults. Debug this later.
  //free(table);

  //freeSolverState(&s);

  return 0;
}










int main(int argc, char** argv){
  if(argc < 2){
    printf("No file parameter.\n");
    return 1;
  }

  rngseed(54137678991, 1365781653);

  for(int i = 1; i < argc; i++){
    int err = mainpass(argv, i);

    switch(err){
      case  0: printf("%s successfully processed.\n", argv[i]);      break;
      case  1: printf("%s has had an unexpected error.\n", argv[i]); break;
      case  2: printf("%s cannot be loaded.\n", argv[i]);            break;
      case  3: printf("%s has had a memory error.\n", argv[i]);      break;
      case  4: printf("%s cannot be read.\n", argv[i]);              break;
      default: printf("%s has had an unknown error?\n", argv[i]);    break;
    }

    printf("\n\n\n");
  }

  return 0;
}
