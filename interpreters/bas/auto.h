#ifndef AUTO_H
#define AUTO_H

#include "programtypes.h"
#include "var.h"

struct Auto
{
  long int stackPointer;
  long int stackCapacity;
  long int framePointer;
  long int frameSize;
  struct Pc onerror;
  union AutoSlot *slot;
  long int erl;
  struct Pc erpc;
  struct Value err;
  struct Value lastdet;
  struct Pc begindata;
  int resumeable;
  struct Symbol *cur,*all; /* should be hung off the funcs/procs */
};

struct AutoFrameSlot
{
  long int framePointer;
  long int frameSize;
  struct Pc pc;
};

struct AutoExceptionSlot
{
  struct Pc onerror;
  int resumeable;
};

union AutoSlot
{
  struct AutoFrameSlot retFrame;
  struct AutoExceptionSlot retException;
  struct Var var;
};

#include "token.h"

extern struct Auto *Auto_new(struct Auto *this);
extern void Auto_destroy(struct Auto *this);
extern struct Var *Auto_pushArg(struct Auto *this);
extern void Auto_pushFuncRet(struct Auto *this, int firstarg, struct Pc *pc);
extern void Auto_pushGosubRet(struct Auto *this, struct Pc *pc);
extern struct Var *Auto_local(struct Auto *this, int l);
extern int Auto_funcReturn(struct Auto *this, struct Pc *pc);
extern int Auto_gosubReturn(struct Auto *this, struct Pc *pc);
extern void Auto_frameToError(struct Auto *this, struct Program *program, struct Value *v);
extern void Auto_setError(struct Auto *this, long int line, struct Pc *pc, struct Value *v);

extern int Auto_find(struct Auto *this, struct Identifier *ident);
extern int Auto_variable(struct Auto *this, const struct Identifier *ident);
extern enum ValueType Auto_argType(const struct Auto *this, int l);
extern enum ValueType Auto_varType(const struct Auto *this, struct Symbol *sym);
extern void Auto_funcEnd(struct Auto *this);

#endif
