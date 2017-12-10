#include "cnfparser.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"










typedef struct{
  int line, column, index, end;
  char *position, *start;
}ParserState;










void nextLine(ParserState* s){
  for (int i = s->index; i < s->end; i++){
    if (*s->position == '\n'){
      s->line++;
      s->column = 0;
      return;
    }
    s->position++;
    s->column++;
  }
}










void nextChar(ParserState* s){
  if (*s->position == '\n'){
    s->line++;
    s->column = 0;
  }else{
    s->column++;
  }
  s->position++;
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

    nextChar(s);
  }

  return number * sign;
}










Clause* parseClause(ParserState* s){
  int vals[512];
  int n;
  for(int i = 0; i < 512; i++){
    nextSymb(s);
    vals[i] = parseNum(s);
    if(vals[i] == 0){
      n = i;
      break;
    }
  }
  if(vals[n] != 0){
    printf("Exceeded maximum number of parameters per clause! Line:%i\n", s->line);
    exit(5);
  }

  //Everything went fine.

  //Temporary mallocs. Use faster allocator later.
  Clause* clause = malloc(sizeof(Clause));
  clause->vars = malloc(4 * n);
  clause->numvars = n;
  for(int i = 0; i < n; i++)
    clause->vars[i] = vals[i];

  return clause;
}









CNF parseCNF(char* input, int filesize){
  ParserState s = {0, 0, 0, filesize, input, input};
  CNF cnfState  = {0, 0, NULL};

  while(s.index < s.end){
    if(*s.position == 'c'){
      nextLine(&s);
    }else if (*s.position == 'p'){
      nextSymb(&s);
      if(strncmp(s.position, "cmp", 3)){
        nextSpace(&s);
        nextSymb(&s);
        cnfState.varnum    = parseNum(&s);
        //nextSpace(&s);
        nextSymb(&s);
        cnfState.clausenum = parseNum(&s);
      }
    }else{
      Clause* c = parseClause(&s);
    }
  }
  return cnfState;
}
