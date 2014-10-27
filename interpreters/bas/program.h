#ifndef PROGRAM_H
#define PROGRAM_H

#include "programtypes.h"
#include "token.h"

extern struct Program *Program_new(struct Program *this);
extern void Program_destroy(struct Program *this);
extern void Program_norun(struct Program *this);
extern void Program_store(struct Program *this, struct Token *line, long int where);
extern void Program_delete(struct Program *this, const struct Pc *from, const struct Pc *to);
extern void Program_addScope(struct Program *this, struct Scope *scope);
extern struct Pc *Program_goLine(struct Program *this, long int line, struct Pc *pc);
extern struct Pc *Program_fromLine(struct Program *this, long int line, struct Pc *pc);
extern struct Pc *Program_toLine(struct Program *this, long int line, struct Pc *pc);
extern int Program_scopeCheck(struct Program *this, struct Pc *pc, struct Pc *fn);
extern struct Pc *Program_dataLine(struct Program *this, long int line, struct Pc *pc);
extern struct Pc *Program_imageLine(struct Program *this, long int line, struct Pc *pc);
extern long int Program_lineNumber(const struct Program *this, const struct Pc *pc);
extern struct Pc *Program_beginning(struct Program *this, struct Pc *pc);
extern struct Pc *Program_end(struct Program *this, struct Pc *pc);
extern struct Pc *Program_nextLine(struct Program *this, struct Pc *pc);
extern int Program_skipEOL(struct Program *this, struct Pc *pc, int dev, int tr);
extern void Program_trace(struct Program *this, struct Pc *pc, int dev, int tr);
extern void Program_PCtoError(struct Program *this, struct Pc *pc, struct Value *v);
extern struct Value *Program_merge(struct Program *this, int dev, struct Value *value);
extern int Program_lineNumberWidth(struct Program *this);
extern struct Value *Program_list(struct Program *this, int dev, int watchIntr, struct Pc *from, struct Pc *to, struct Value *value);
extern struct Value *Program_analyse(struct Program *this, struct Pc *pc, struct Value *value);
extern void Program_renum(struct Program *this, int first, int inc);
extern void Program_unnum(struct Program *this);
extern int Program_setname(struct Program *this, const char *filename);
extern void Program_xref(struct Program *this, int chn);

#endif
