#ifndef PROGRAMTYPES_H
#define PROGRAMTYPES_H

#include "str.h"

struct Pc
{
  int line;
  struct Token *token;
};

struct Scope
{
  struct Pc start;
  struct Pc begin;
  struct Pc end;
  struct Scope *next;
};

struct Program
{
  int trace;
  int numbered;
  int size;
  int capacity;
  int runnable;
  int unsaved;
  struct String name;
  struct Token **code;
  struct Scope *scope;
};

#endif
