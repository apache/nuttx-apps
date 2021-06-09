/****************************************************************************
 * apps/interpreters/bas/bas_program.h
 *
 *   Copyright (c) 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_BAS_BAS_PROGRAM_H
#define __APPS_EXAMPLES_BAS_BAS_PROGRAM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_programtypes.h"
#include "bas_token.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Program *Program_new(struct Program *this);
void Program_destroy(struct Program *this);
void Program_norun(struct Program *this);
void Program_store(struct Program *this, struct Token *line,
                   long int where);
void Program_delete(struct Program *this, const struct Pc *from,
                    const struct Pc *to);
void Program_addScope(struct Program *this, struct Scope *scope);
struct Pc *Program_goLine(struct Program *this, long int line,
                          struct Pc *pc);
struct Pc *Program_fromLine(struct Program *this, long int line,
                            struct Pc *pc);
struct Pc *Program_toLine(struct Program *this, long int line,
                          struct Pc *pc);
int Program_scopeCheck(struct Program *this, struct Pc *pc, struct Pc *fn);
struct Pc *Program_dataLine(struct Program *this, long int line,
                            struct Pc *pc);
struct Pc *Program_imageLine(struct Program *this, long int line,
                             struct Pc *pc);
long int Program_lineNumber(const struct Program *this,
                            const struct Pc *pc);
struct Pc *Program_beginning(struct Program *this, struct Pc *pc);
struct Pc *Program_end(struct Program *this, struct Pc *pc);
struct Pc *Program_nextLine(struct Program *this, struct Pc *pc);
int Program_skipEOL(struct Program *this, struct Pc *pc, int dev, int tr);
void Program_trace(struct Program *this, struct Pc *pc, int dev, int tr);
void Program_PCtoError(struct Program *this, struct Pc *pc,
                       struct Value *v);
struct Value *Program_merge(struct Program *this, int dev,
                            struct Value *value);
int Program_lineNumberWidth(struct Program *this);
struct Value *Program_list(struct Program *this, int dev, int watchIntr,
                           struct Pc *from, struct Pc *to,
                           struct Value *value);
struct Value *Program_analyse(struct Program *this, struct Pc *pc,
                              struct Value *value);
void Program_renum(struct Program *this, int first, int inc);
void Program_unnum(struct Program *this);
int Program_setname(struct Program *this, const char *filename);
void Program_xref(struct Program *this, int chn);

#endif /* __APPS_EXAMPLES_BAS_BAS_PROGRAM_H */
