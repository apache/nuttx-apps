#ifndef AUTO_H
#define AUTO_H

#include "program.h"
#include "var.h"
#include "token.h"

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
  int resumeable;

  struct Symbol *cur,*all;
};

union AutoSlot
{
  struct
  {
    long int framePointer;
    long int frameSize;
    struct Pc pc;
  } ret;
  struct Var var;
};

#endif
