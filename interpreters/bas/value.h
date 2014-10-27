#ifndef VALUE_H
#define VALUE_H

#include "str.h"

enum ValueType
{
  V_ERROR=1,
  V_INTEGER,
  V_NIL,
  V_REAL,
  V_STRING,
  V_VOID
};

struct Value
{
  enum ValueType type;
  union
  {
    /* V_ERROR   */ struct { char *msg; long int code; } error;
    /* V_INTEGER */ long int integer;
    /* V_NIL     */
    /* V_REAL    */ double real;
    /* V_STRING  */ struct String string;
    /* V_VOID    */
  } u;
};

extern const enum ValueType Value_commonType[V_VOID+1][V_VOID+1];

#define VALUE_NEW_INTEGER(this,n) ((this)->type=V_INTEGER,(this)->u.integer=(n))
#define VALUE_NEW_REAL(this,n) ((this)->type=V_REAL,(this)->u.real=(n))
#define VALUE_RETYPE(v,t) ((v)->type==(t) ? (v) : Value_retype(v,t))
#define VALUE_DESTROY(this) assert((this)!=(struct Value*)0); \
  switch ((this)->type) \
  { \
    case V_ERROR: free((this)->u.error.msg); break; \
    case V_INTEGER: break; \
    case V_NIL: break; \
    case V_REAL: break; \
    case V_STRING: String_destroy(&(this)->u.string); break; \
    case V_VOID: break; \
    default: assert(0); \
  } \
  (this)->type=0;


#ifndef HAVE_LRINT
extern long int lrint(double d);
#endif
extern double Value_trunc(double d);
extern double Value_round(double d);
extern long int Value_toi(double d, int *overflow);
extern long int Value_vali(const char *s, char **end, int *overflow);
extern double Value_vald(const char *s, char **end, int *overflow);

extern struct Value *Value_new_NIL(struct Value *this);
extern struct Value *Value_new_ERROR(struct Value *this, int code, const char *error, ...);
extern struct Value *Value_new_INTEGER(struct Value *this, int n);
extern struct Value *Value_new_REAL(struct Value *this, double n);
extern struct Value *Value_new_STRING(struct Value *this);
extern struct Value *Value_new_VOID(struct Value *this);
extern struct Value *Value_new_null(struct Value *this, enum ValueType type);
extern int Value_isNull(const struct Value *this);
extern void Value_destroy(struct Value *this);
extern void Value_errorPrefix(struct Value *this, const char *prefix);
extern void Value_errorSuffix(struct Value *this, const char *suffix);
extern struct Value *Value_new_typeError(struct Value *this, enum ValueType t1, enum ValueType t2);
extern struct Value *Value_retype(struct Value *this, enum ValueType type);
extern struct Value *Value_clone(struct Value *this, const struct Value *original);
extern struct Value *Value_uplus(struct Value *this, int calc);
extern struct Value *Value_uneg(struct Value *this, int calc);
extern struct Value *Value_unot(struct Value *this, int calc);
extern struct Value *Value_add(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_sub(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_mult(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_div(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_idiv(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_mod(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_pow(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_and(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_or(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_xor(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_eqv(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_imp(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_lt(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_le(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_eq(struct Value *this, struct Value *s, int calc);
extern struct Value *Value_ge(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_gt(struct Value *this, struct Value *x, int calc);
extern struct Value *Value_ne(struct Value *this, struct Value *x, int calc);
extern int Value_exitFor(struct Value *this, struct Value *limit, struct Value *step);
extern struct String *Value_toString(struct Value *this, struct String *s, char pad, int headingsign, size_t width, int commas, int dollar, int dollarleft, int precision, int exponent, int trailingsign);
extern struct Value *Value_toStringUsing(struct Value *this, struct String *s, struct String *using, size_t *usingpos);
extern struct String *Value_toWrite(struct Value *this, struct String *s);
extern struct Value *Value_nullValue(enum ValueType type);

#endif
