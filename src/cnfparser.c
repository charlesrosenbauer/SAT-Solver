#include "cnfparser.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"










typedef struct{
  int line, column, index, end;
  char *position, *start;
}ParserState;










void nextChar(ParserState* s){
  if (*s->position == '\n'){
    s->line++;
    s->column = 0;
  }else{
    s->column++;
  }
  s->position++;
}










void nextLine(ParserState* s){
  for (int i = s->index; i < s->end; i++){
/*
    if (*s->position == '\n'){
      s->line++;
      s->column = 0;
      return;
    }
    s->position++;
    s->column++;*/
    nextChar(s);
    if(s->column == 0) return;
  }
}










void nextSpace(ParserState* s){
  nextChar(s);
  for (int i = s->index; i < s->end; i++){
    if ((*s->position == ' ') || (*s->position == '\n'))
      return;
    nextChar(s);
  }
}











void nextSymb(ParserState* s){
  nextChar(s);
  for (int i = s->index; i < s->end; i++){
    if ((*s->position != ' ') && (*s->position != '\n'))
      return;
    nextChar(s);
  }
}










int parseNum(ParserState* s){
  int number = 0;
  int sign   = 1;
  if(*s->position == '-'){
    sign = -1;
    nextChar(s);
  }

  int cond = 1;
  while (cond) {

    switch(*s->position){
      case '0': number *= 10;
      break;

      case '1': number = (number * 10) + 1;
      break;

      case '2': number = (number * 10) + 2;
      break;

      case '3': number = (number * 10) + 3;
      break;

      case '4': number = (number * 10) + 4;
      break;

      case '5': number = (number * 10) + 5;
      break;

      case '6': number = (number * 10) + 6;
      break;

      case '7': number = (number * 10) + 7;
      break;

      case '8': number = (number * 10) + 8;
      break;

      case '9': number = (number * 10) + 9;
      break;

      default: cond = 0;
    }

    if(cond) nextChar(s);
  }

  return number * sign;
}






int ct = 0;



Clause* parseClause(ParserState* s){
  ct++;
  int vals[512];
  int n = 511;
  for(int i = 0; i < 512; i++){
    nextSymb(s);
    vals[i] = parseNum(s);
    if(vals[i] == 0){
      n = i;
      goto next;
    }
  }

  next:
  if(vals[n] != 0){
    printf("Exceeded maximum number of parameters per clause! Line:%i.\n", s->line);
    exit(5);
  }

  if(n == 0) return NULL;

  //Everything went fine.
  printf("L: %i :: ", s->line);
  for(int i = 0; i < n; i++)
    printf("%i ", vals[i]);
  printf("\n");

  //Temporary mallocs. Use faster allocator later.
  Clause* clause = malloc(sizeof(Clause));
  clause->vars = malloc(sizeof(int) * n);
  clause->numvars = n;
  for(int i = 0; i < n; i++)
    clause->vars[i] = vals[i];

  return clause;
}









CNF parseCNF(char* input, int filesize){
  ParserState s = {1, 1, 0, filesize, input, input};
  CNF cnfState  = {0, 0, NULL};

  int hasFoundParameters = 0;
  int clausesFound = 0;

  while(s.index < s.end){
    if(*s.position == 'c'){
      nextLine(&s);
    }else if (*s.position == 'p'){
      if(hasFoundParameters == 1){
        printf("Repeated parameter line at %i.\n", s.line);
        exit(6);
      }
      nextSymb(&s);
      if(strncmp(s.position, "cmp", 3)){
        nextSpace(&s);
        nextSymb(&s);
        cnfState.varnum    = parseNum(&s);
        nextChar(&s);
        cnfState.clausenum = parseNum(&s);
        nextChar(&s);
        cnfState.clauses   = malloc(sizeof(Clause) * cnfState.clausenum);
        hasFoundParameters = 1;
        printf("p cnf %i %i\n", cnfState.varnum, cnfState.clausenum);
      }else{
        printf("Invalid syntax at line %i", s.line);
        exit(7);
      }
    }else{
      if((clausesFound >= cnfState.clausenum) && (cnfState.clausenum != 0)) return cnfState;
      Clause* c = parseClause(&s);
      if(c == NULL){
        nextChar(&s);
      }else{
        cnfState.clauses[clausesFound] = *c;
        free(c);    // Replace with more efficient free later
        clausesFound++;
      }
    }
  }
  return cnfState;
}
