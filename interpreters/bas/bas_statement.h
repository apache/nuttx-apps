/****************************************************************************
 * apps/interpreters/bas/statement.h
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

#ifndef __APPS_EXAMPLES_BAS_STATEMENT_H
#define __APPS_EXAMPLES_BAS_STATEMENT_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Value *stmt_CALL(struct Value *value);
struct Value *stmt_CASE(struct Value *value);
struct Value *stmt_CHDIR_MKDIR(struct Value *value);
struct Value *stmt_CLEAR(struct Value *value);
struct Value *stmt_CLOSE(struct Value *value);
struct Value *stmt_CLS(struct Value *value);
struct Value *stmt_COLOR(struct Value *value);
struct Value *stmt_DATA(struct Value *value);
struct Value *stmt_DEFFN_DEFPROC_FUNCTION_SUB(struct Value *value);
struct Value *stmt_DEC_INC(struct Value *value);
struct Value *stmt_DEFINT_DEFDBL_DEFSTR(struct Value *value);
struct Value *stmt_DELETE(struct Value *value);
struct Value *stmt_DIM(struct Value *value);
struct Value *stmt_DISPLAY(struct Value *value);
struct Value *stmt_DO(struct Value *value);
struct Value *stmt_DOcondition(struct Value *value);
struct Value *stmt_EDIT(struct Value *value);
struct Value *stmt_ELSE_ELSEIFELSE(struct Value *value);
struct Value *stmt_END(struct Value *value);
struct Value *stmt_ENDIF(struct Value *value);
struct Value *stmt_ENDFN(struct Value *value);
struct Value *stmt_ENDPROC_SUBEND(struct Value *value);
struct Value *stmt_ENDSELECT(struct Value *value);
struct Value *stmt_ENVIRON(struct Value *value);
struct Value *stmt_FNEXIT(struct Value *value);
struct Value *stmt_COLON_EOL(struct Value *value);
struct Value *stmt_QUOTE_REM(struct Value *value);
struct Value *stmt_EQ_FNRETURN_FNEND(struct Value *value);
struct Value *stmt_ERASE(struct Value *value);
struct Value *stmt_EXITDO(struct Value *value);
struct Value *stmt_EXITFOR(struct Value *value);
struct Value *stmt_FIELD(struct Value *value);
struct Value *stmt_FOR(struct Value *value);
struct Value *stmt_GET_PUT(struct Value *value);
struct Value *stmt_GOSUB(struct Value *value);
struct Value *stmt_RESUME_GOTO(struct Value *value);
struct Value *stmt_KILL(struct Value *value);
struct Value *stmt_LET(struct Value *value);
struct Value *stmt_LINEINPUT(struct Value *value);
struct Value *stmt_LIST_LLIST(struct Value *value);
struct Value *stmt_LOAD(struct Value *value);
struct Value *stmt_LOCAL(struct Value *value);
struct Value *stmt_LOCATE(struct Value *value);
struct Value *stmt_LOCK_UNLOCK(struct Value *value);
struct Value *stmt_LOOP(struct Value *value);
struct Value *stmt_LOOPUNTIL(struct Value *value);
struct Value *stmt_LSET_RSET(struct Value *value);
struct Value *stmt_IDENTIFIER(struct Value *value);
struct Value *stmt_IF_ELSEIFIF(struct Value *value);
struct Value *stmt_IMAGE(struct Value *value);
struct Value *stmt_INPUT(struct Value *value);
struct Value *stmt_MAT(struct Value *value);
struct Value *stmt_MATINPUT(struct Value *value);
struct Value *stmt_MATPRINT(struct Value *value);
struct Value *stmt_MATREAD(struct Value *value);
struct Value *stmt_MATREDIM(struct Value *value);
struct Value *stmt_MATWRITE(struct Value *value);
struct Value *stmt_NAME(struct Value *value);
struct Value *stmt_NEW(struct Value *value);
struct Value *stmt_NEXT(struct Value *value);
struct Value *stmt_ON(struct Value *value);
struct Value *stmt_ONERROR(struct Value *value);
struct Value *stmt_ONERRORGOTO0(struct Value *value);
struct Value *stmt_ONERROROFF(struct Value *value);
struct Value *stmt_OPEN(struct Value *value);
struct Value *stmt_OPTIONBASE(struct Value *value);
struct Value *stmt_OPTIONRUN(struct Value *value);
struct Value *stmt_OPTIONSTOP(struct Value *value);
struct Value *stmt_OUT_POKE(struct Value *value);
struct Value *stmt_PRINT_LPRINT(struct Value *value);
struct Value *stmt_RANDOMIZE(struct Value *value);
struct Value *stmt_READ(struct Value *value);
struct Value *stmt_COPY_RENAME(struct Value *value);
struct Value *stmt_RENUM(struct Value *value);
struct Value *stmt_REPEAT(struct Value *value);
struct Value *stmt_RESTORE(struct Value *value);
struct Value *stmt_RETURN(struct Value *value);
struct Value *stmt_RUN(struct Value *value);
struct Value *stmt_SAVE(struct Value *value);
struct Value *stmt_SELECTCASE(struct Value *value);
struct Value *stmt_SHELL(struct Value *value);
struct Value *stmt_SLEEP(struct Value *value);
struct Value *stmt_STOP(struct Value *value);
struct Value *stmt_SUBEXIT(struct Value *value);
struct Value *stmt_SWAP(struct Value *value);
struct Value *stmt_SYSTEM(struct Value *value);

struct Value *stmt_TROFF(struct Value *value);
struct Value *stmt_TRON(struct Value *value);
struct Value *stmt_TRUNCATE(struct Value *value);
struct Value *stmt_UNNUM(struct Value *value);
struct Value *stmt_UNTIL(struct Value *value);
struct Value *stmt_WAIT(struct Value *value);
struct Value *stmt_WHILE(struct Value *value);
struct Value *stmt_WEND(struct Value *value);
struct Value *stmt_WIDTH(struct Value *value);
struct Value *stmt_WRITE(struct Value *value);
struct Value *stmt_XREF(struct Value *value);
struct Value *stmt_ZONE(struct Value *value);

#endif /* __APPS_EXAMPLES_BAS_STATEMENT_H */
