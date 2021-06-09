/****************************************************************************
 * apps/interpreters/bas/bas_token.h
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

#ifndef __APPS_EXAMPLES_BAS_BAS_TOKEN_H
#define __APPS_EXAMPLES_BAS_BAS_TOKEN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_autotypes.h"
#include "bas_value.h"
#include "bas_var.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TOKEN_ISBINARYOPERATOR(t)   (g_token_property[t]&1)
#define TOKEN_ISUNARYOPERATOR(t)    (g_token_property[t]&(1<<1))
#define TOKEN_BINARYPRIORITY(t)     ((g_token_property[t]>>2)&7)
#define TOKEN_UNARYPRIORITY(t)      ((g_token_property[t]>>5)&7)
#define TOKEN_ISRIGHTASSOCIATIVE(t) (g_token_property[t]&(1<<8))

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum SymbolType
{
  GLOBALVAR,
  GLOBALARRAY,
  LOCALVAR,
  BUILTINFUNCTION,
  USERFUNCTION
};

struct Symbol
{
  char *name;
  enum SymbolType type;
  union
  {
    struct Var var; /* GLOBALVAR, GLOBALARRAY */
    struct
    {
      int offset; /* LOCALVAR */
      enum ValueType type;
    } local;
    struct
    {
      union
      {
        struct /* BUILTINFUNCTION */
        {
          struct Value *(* call)(struct Value *value, struct Auto *stack);
          struct Symbol *next;
        } bltin;
        struct    /* USERFUNTION */
        {
          struct Scope scope;
          int localLength;
          enum ValueType *localTypes;
        } def;
      } u;
      int argLength;
      enum ValueType *argTypes;
      enum ValueType retType;
    } sub;
  } u;
  struct Symbol *next;
};

#include "bas_program.h"
#include "bas_str.h"

struct Identifier
{
  struct Symbol *sym;
  enum ValueType defaultType;
  char name[2/* ... */];
};

struct Next
{
  struct Pc fr;
  struct Pc var;
  struct Pc limit;
  struct Pc body;
};

struct On
{
  int pcLength;
  struct Pc *pc;
};

struct Selectcase
{
  struct Pc endselect;
  enum ValueType type;
  struct Pc nextcasevalue;
};

struct Casevalue
{
  struct Pc endselect;
  struct Pc nextcasevalue;
};

enum TokenType
{
  T_NOTOKEN = 0,
  T_ACCESS_READ,
  T_ACCESS_READ_WRITE,
  T_ACCESS_WRITE,
  T_AND,
  T_AS,
  T_CALL,
  T_CASEELSE,
  T_CASEVALUE,
  T_CHANNEL,
  T_CHDIR,
  T_CLEAR,
  T_CLOSE,
  T_CLS,
  T_COLON,
  T_COLOR,
  T_COMMA,
  T_CON,
  T_COPY,
  T_CP,
  T_DATA,
  T_DATAINPUT,
  T_DEC,
  T_DEFDBL,
  T_DEFFN,
  T_DEFINT,
  T_DEFPROC,
  T_DEFSTR,
  T_DELETE,
  T_DIM,
  T_DISPLAY,
  T_DIV,
  T_DO,
  T_DOUNTIL,
  T_DOWHILE,
  T_EDIT,
  T_ELSE,
  T_ELSEIFELSE,
  T_ELSEIFIF,
  T_END,
  T_ENDFN,
  T_ENDIF,
  T_ENDPROC,
  T_ENDSELECT,
  T_ENVIRON,
  T_EOL,
  T_EQ,
  T_EQV,
  T_ERASE,
  T_EXITDO,
  T_EXITFOR,
  T_FIELD,
  T_FNEND,
  T_FNEXIT,
  T_FNRETURN,
  T_FOR,
  T_FOR_INPUT,
  T_FOR_OUTPUT,
  T_FOR_APPEND,
  T_FOR_RANDOM,
  T_FOR_BINARY,
  T_FUNCTION,
  T_GE,
  T_GET,
  T_GOSUB,
  T_GOTO,
  T_GT,
  T_HEXINTEGER,
  T_OCTINTEGER,
  T_IDENTIFIER,
  T_IDIV,
  T_IDN,
  T_IF,
  T_IMAGE,
  T_IMP,
  T_INC,
  T_INPUT,
  T_INTEGER,
  T_INV,
  T_IS,
  T_JUNK,
  T_KILL,
  T_LE,
  T_LET,
  T_LINEINPUT,
  T_LIST,
  T_LLIST,
  T_LOAD,
  T_LOCAL,
  T_LOCATE,
  T_LOCK,
  T_LOCK_READ,
  T_LOCK_WRITE,
  T_LOOP,
  T_LOOPUNTIL,
  T_LPRINT,
  T_LSET,
  T_LT,
  T_MAT,
  T_MATINPUT,
  T_MATPRINT,
  T_MATREAD,
  T_MATREDIM,
  T_MATWRITE,
  T_MINUS,
  T_MKDIR,
  T_MOD,
  T_MULT,
  T_NAME,
  T_NE,
  T_NEW,
  T_NEXT,
  T_NOT,
  T_ON,
  T_ONERROR,
  T_ONERRORGOTO0,
  T_ONERROROFF,
  T_OP,
  T_OPEN,
  T_OPTIONBASE,
  T_OPTIONRUN,
  T_OPTIONSTOP,
  T_OR,
  T_OUT,
  T_PLUS,
  T_POKE,
  T_POW,
  T_PRINT,
  T_PUT,
  T_QUOTE,
  T_RANDOMIZE,
  T_READ,
  T_REAL,
  T_REM,
  T_RENAME,
  T_RENUM,
  T_REPEAT,
  T_RESTORE,
  T_RESUME,
  T_RETURN,
  T_RSET,
  T_RUN,
  T_SAVE,
  T_SELECTCASE,
  T_SEMICOLON,
  T_SHARED,
  T_SHELL,
  T_SLEEP,
  T_SPC,
  T_STEP,
  T_STOP,
  T_STRING,
  T_SUB,
  T_SUBEND,
  T_SUBEXIT,
  T_SWAP,
  T_SYSTEM,
  T_TAB,
  T_THEN,
  T_TO,
  T_TRN,
  T_TROFF,
  T_TRON,
  T_TRUNCATE,
  T_UNLOCK,
  T_UNNUM,
  T_UNNUMBERED,
  T_UNTIL,
  T_USING,
  T_WAIT,
  T_WEND,
  T_WHILE,
  T_WIDTH,
  T_WRITE,
  T_XOR,
  T_XREF,
  T_ZER,
  T_ZONE,
  T_LASTTOKEN=T_ZONE
};

struct Token
{
  enum TokenType type;
  struct Value *(*statement)(struct Value *value);
  union
  {
    /* T_ACCESS_READ        */
    /* T_ACCESS_READ_WRITE  */
    /* T_ACCESS_WRITE       */
    /* T_AND                */
    /* T_AS                 */
    /* T_CALL               */
    /* T_CASEELSE           */ struct Casevalue *casevalue;
    /* T_CASEIS             */ /* struct Casevalue *casevalue; */
    /* T_CASEVALUE          */ /* struct Casevalue *casevalue; */
    /* T_CHANNEL            */
    /* T_CHDIR              */
    /* T_CLEAR              */
    /* T_CLOSE              */
    /* T_CLS                */
    /* T_COLON              */
    /* T_COLOR              */
    /* T_COMMA              */
    /* T_CON                */
    /* T_COPY               */
    /* T_CP                 */
    /* T_DATA               */ struct Pc nextdata;
    /* T_DATAINPUT          */ char *datainput;
    /* T_DEFFN              */ struct Symbol *localSyms;
    /* T_DEFDBL             */
    /* T_DEFINT             */
    /* T_DEFPROC            */ /* struct Symbol *localSyms; */
    /* T_DELETE             */
    /* T_DIM                */
    /* T_DIV                */
    /* T_DO                 */ struct Pc exitdo;
    /* T_DOUNTIL            */ /* struct Pc exitdo; */
    /* T_DOWHILE            */ /* struct Pc exitdo; */
    /* T_EDIT               */
    /* T_ELSE               */ struct Pc endifpc;
    /* T_ELSEIFELSE         */ /* struct Pc endifpc; */
    /* T_ELSEIFIF           */ struct Pc elsepc;
    /* T_END                */ struct Pc endpc;
    /* T_ENDFN              */
    /* T_ENDIF              */
    /* T_ENDPROC            */
    /* T_ENDSELECT          */
    /* T_ENVIRON            */
    /* T_EOL                */
    /* T_EQ                 */ enum ValueType type;
    /* T_EQV                */
    /* T_ERASE              */
    /* T_EXITDO             */ /* struct Pc exitdo; */
    /* T_EXITFOR            */ struct Pc exitfor;
    /* T_FIELD              */
    /* T_FNEND              */
    /* T_FNRETURN           */
    /* T_FOR                */ /* struct Pc exitfor */
    /* T_FOR_INPUT          */
    /* T_FOR_OUTPUT         */
    /* T_FOR_APPEND         */
    /* T_FOR_RANDOM         */
    /* T_FOR_BINARY         */
    /* T_FUNCTION           */ /* struct Symbol *localSyms; */
    /* T_GE                 */
    /* T_GET                */
    /* T_GOSUB              */ struct Pc gosubpc;
    /* T_GOTO               */ struct Pc gotopc;
    /* T_GT                 */
    /* T_HEXINTEGER         */ long int hexinteger;
    /* T_OCTINTEGER         */ long int octinteger;
    /* T_IDENTIFIER         */ struct Identifier *identifier;
    /* T_IDIV               */
    /* T_IDN                */
    /* T_IF                 */ /* struct Pc elsepc; */
    /* T_IMAGE              */ /* struct String *string; */
    /* T_IMP                */
    /* T_INPUT              */
    /* T_INTEGER            */ long int integer;
    /* T_INV                */
    /* T_IS                 */
    /* T_JUNK               */ char junk;
    /* T_KILL               */
    /* T_LE                 */
    /* T_LEN                */
    /* T_LET                */
    /* T_LINEINPUT          */
    /* T_LIST               */
    /* T_LLIST              */
    /* T_LOAD               */
    /* T_LOCAL              */
    /* T_LOCATE             */
    /* T_LOCK               */
    /* T_LOCK_READ          */
    /* T_LOCK_WRITE         */
    /* T_LOOP               */ struct Pc dopc;
    /* T_LOOPUNTIL          */ /* struct Pc dopc; */
    /* T_LPRINT             */
    /* T_LSET               */
    /* T_LT                 */
    /* T_MAT                */
    /* T_MATINPUT           */
    /* T_MATPRINT           */
    /* T_MATREAD            */
    /* T_MATREDIM           */
    /* T_MINUS              */
    /* T_MKDIR              */
    /* T_MOD                */
    /* T_MULT               */
    /* T_NAME               */
    /* T_NE                 */
    /* T_NEW                */
    /* T_NEXT               */ struct Next *next;
    /* T_NOT                */
    /* T_ON                 */ struct On on;
    /* T_ONERROR            */
    /* T_ONERRORGOTO0       */
    /* T_ONERROROFF         */
    /* T_OP                 */
    /* T_OPEN               */
    /* T_OPTIONBASE         */
    /* T_OR                 */
    /* T_OUT                */
    /* T_PLUS               */
    /* T_POKE               */
    /* T_POW                */
    /* T_PRINT              */
    /* T_PUT                */
    /* T_QUOTE              */ /* char *rem; */
    /* T_RANDOMIZE          */
    /* T_READ               */
    /* T_REAL               */ double real;
    /* T_REM                */ char *rem;
    /* T_RENAME             */
    /* T_RENUM              */
    /* T_REPEAT             */
    /* T_RESTORE            */ struct Pc restore;
    /* T_RESUME             */ /* struct Pc gotopc; */
    /* T_RETURN             */
    /* T_RSET               */
    /* T_RUN                */
    /* T_SAVE               */
    /* T_SELECTCASE         */ struct Selectcase *selectcase;
    /* T_SEMICOLON          */
    /* T_SHARED             */
    /* T_SHELL              */
    /* T_SLEEP              */
    /* T_SPC                */
    /* T_STEP               */
    /* T_STOP               */
    /* T_STRING             */ struct String *string;
    /* T_SUB                */ /* struct Symbol *localSyms; */
    /* T_SUBEND             */
    /* T_SUBEXIT            */
    /* T_SWAP               */
    /* T_SYSTEM             */
    /* T_TAB                */
    /* T_THEN               */
    /* T_TO                 */
    /* T_TRN                */
    /* T_TROFF              */
    /* T_TRON               */
    /* T_TRUNCATE           */
    /* T_UNLOCK             */
    /* T_UNNUM              */
    /* T_UNNUMBERED         */
    /* T_UNTIL              */ struct Pc until;
    /* T_USING              */ struct Pc image;
    /* T_WAIT               */
    /* T_WEND               */ struct Pc *whilepc;
    /* T_WHILE              */ struct Pc *afterwend;
    /* T_WIDTH              */
    /* T_WRITE              */
    /* T_XOR                */
    /* T_XREF               */
    /* T_ZER                */
    /* T_ZONE               */
  } u;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int g_token_property[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Token *Token_newCode(const char *ln);
struct Token *Token_newData(const char *ln);
void Token_destroy(struct Token *token);
struct String *Token_toString(struct Token *token, struct Token *spaceto,
                              struct String *s, int *indent, int full);
void Token_init(int backslash_colon, int uppercase);

#endif /* __APPS_EXAMPLES_BAS_BAS_TOKEN_H */
