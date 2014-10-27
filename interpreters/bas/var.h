#ifndef VAR_H
#define VAR_H

#include "value.h"

struct Var
{
  unsigned int dim;
  unsigned int *geometry;
  struct Value *value;
  unsigned int size;
  enum ValueType type;
  char base;
};

#define VAR_SCALAR_VALUE(this) ((this)->value)

extern struct Var *Var_new(struct Var *this, enum ValueType type, unsigned int dim, const unsigned int *geometry, int base);
extern struct Var *Var_new_scalar(struct Var *this);
extern void Var_destroy(struct Var *this);
extern void Var_retype(struct Var *this, enum ValueType type);
extern struct Value *Var_value(struct Var *this, unsigned int dim, int idx[], struct Value *value);
extern void Var_clear(struct Var *this);
extern struct Value *Var_mat_assign(struct Var *this, struct Var *x, struct Value *err, int work);
extern struct Value *Var_mat_addsub(struct Var *this, struct Var *x, struct Var *y, int add, struct Value *err, int work);
extern struct Value *Var_mat_mult(struct Var *this, struct Var *x, struct Var *y, struct Value *err, int work);
extern struct Value *Var_mat_scalarMult(struct Var *this, struct Value *factor, struct Var *x, int work);
extern void Var_mat_transpose(struct Var *this, struct Var *x);
extern struct Value *Var_mat_invert(struct Var *this, struct Var *x, struct Value *det, struct Value *err);
extern struct Value *Var_mat_redim(struct Var *this, unsigned int dim, const unsigned int *geometry, struct Value *err);

#endif
