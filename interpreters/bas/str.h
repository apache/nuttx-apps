#ifndef STR_H
#define STR_H

#include <sys/types.h>

struct String
{
  size_t length;
  char *character;
  struct StringField *field;
};

struct StringField
{
  struct String **refStrings;
  int refCount;
};

extern int cistrcmp(const char *s, const char *r);

extern struct String *String_new(struct String *this);
extern void String_destroy(struct String *this);
extern int String_joinField(struct String *this, struct StringField *field, char *character, size_t length);
extern void String_leaveField(struct String *this);
extern struct String *String_clone(struct String *this, const struct String *clon);
extern int String_appendString(struct String *this, const struct String *app);
extern int String_appendChar(struct String *this, char ch);
extern int String_appendChars(struct String *this, const char *ch);
extern int String_appendPrintf(struct String *this, const char *fmt, ...);
extern int String_insertChar(struct String *this, size_t where, char ch);
extern int String_delete(struct String *this, size_t where, size_t len);
extern void String_ucase(struct String *this);
extern void String_lcase(struct String *this);
extern int String_size(struct String *this, size_t length);
extern int String_cmp(const struct String *this, const struct String *s);
extern void String_lset(struct String *this, const struct String *s);
extern void String_rset(struct String *this, const struct String *s);
extern void String_set(struct String *this, size_t pos, const struct String *s, size_t length);

extern struct StringField *StringField_new(struct StringField *this);
extern void StringField_destroy(struct StringField *this);

#endif
