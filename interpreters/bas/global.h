#ifndef GLOBAL_H
#define GLOBAL_H

#include "token.h"
#include "value.h"
#include "var.h"

#define GLOBAL_HASHSIZE 31

struct GlobalFunctionChain
{
  struct Pc begin,end;
  struct GlobalFunctionChain *next;
};

struct Global
{
  struct String command;
  struct Symbol *table[GLOBAL_HASHSIZE];
  struct GlobalFunctionChain *chain;
};

extern struct Global *Global_new(struct Global *this);
extern void Global_destroy(struct Global *this);
extern void Global_clear(struct Global *this);
extern void Global_clearFunctions(struct Global *this);
extern int Global_find(struct Global *this, struct Identifier *ident, int oparen);
extern int Global_function(struct Global *this, struct Identifier *ident, enum ValueType type, struct Pc *deffn, struct Pc *begin, int argTypesLength, enum ValueType *argTypes);
extern void Global_endfunction(struct Global *this, struct Identifier *ident, struct Pc *end);
extern int Global_variable(struct Global *this, struct Identifier *ident, enum ValueType type, enum SymbolType symbolType, int redeclare);

#endif
