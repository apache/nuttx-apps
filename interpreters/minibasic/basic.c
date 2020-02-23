/****************************************************************************
 * apps/interpreters/minibasic/basic.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *
 * This file was taken from Mini Basic, versino 1.0 developed by Malcolm
 * McLean, Leeds University.  Mini Basic version 1.0 was released the
 * Creative Commons Attribution license which, from my reading, appears to
 * be compatible with the NuttX BSD-style license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration */

#ifndef CONFIG_INTERPRETER_MINIBASIC_IOBUFSIZE
#  define CONFIG_INTERPRETER_MINIBASIC_IOBUFSIZE 1024
#endif

#define IOBUFSIZE CONFIG_INTERPRETER_MINIBASIC_IOBUFSIZE

/* Tokens defined */

#define EOS 0
#define VALUE 1
#define PI 2
#define E 3

#define DIV 10
#define MULT 11
#define OPAREN 12
#define CPAREN 13
#define PLUS 14
#define MINUS 15
#define SHRIEK 16
#define COMMA 17
#define MOD 200

#define SYNTAX_ERROR 20
#define EOL 21
#define EQUALS 22
#define STRID 23
#define FLTID 24
#define DIMFLTID 25
#define DIMSTRID 26
#define QUOTE 27
#define GREATER 28
#define LESS 29
#define SEMICOLON 30

#define PRINT 100
#define LET 101
#define DIM 102
#define IF 103
#define THEN 104
#define AND 105
#define OR 106
#define GOTO 107
#define INPUT 108
#define REM 109
#define FOR 110
#define TO 111
#define NEXT 112
#define STEP 113

#define SIN 5
#define COS 6
#define TAN 7
#define LN 8
#define POW 9
#define SQRT 18
#define ABS 201
#define LEN 202
#define ASCII 203
#define ASIN 204
#define ACOS 205
#define ATAN 206
#define INT 207
#define RND 208
#define VAL 209
#define VALLEN 210
#define INSTR 211

#define CHRSTRING 300
#define STRSTRING 301
#define LEFTSTRING 302
#define RIGHTSTRING 303
#define MIDSTRING 304
#define STRINGSTRING 305

/* Relational operators defined */

#define ROP_EQ 1                /* equals */
#define ROP_NEQ 2               /* doesn't equal */
#define ROP_LT 3                /* less than */
#define ROP_LTE 4               /* less than or equals */
#define ROP_GT 5                /* greater than */
#define ROP_GTE 6               /* greater than or equals */

/* Error codes (in BASIC script) defined */

#define ERR_CLEAR 0
#define ERR_SYNTAX 1
#define ERR_OUTOFMEMORY 2
#define ERR_IDTOOLONG 3
#define ERR_NOSUCHVARIABLE 4
#define ERR_BADSUBSCRIPT 5
#define ERR_TOOMANYDIMS 6
#define ERR_TOOMANYINITS 7
#define ERR_BADTYPE 8
#define ERR_TOOMANYFORS 9
#define ERR_NONEXT 10
#define ERR_NOFOR 11
#define ERR_DIVIDEBYZERO 12
#define ERR_NEGLOG 13
#define ERR_NEGSQRT 14
#define ERR_BADSINCOS 15
#define ERR_EOF 16
#define ERR_ILLEGALOFFSET 17
#define ERR_TYPEMISMATCH 18
#define ERR_INPUTTOOLONG 19
#define ERR_BADVALUE 20
#define ERR_NOTINT 21

#define MAXFORS 32              /* Maximum number of nested fors */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mb_line_s
{
  int no;                       /* Line number */
  FAR const char *str;          /* Points to start of line */
};

struct mb_variable_s
{
  char id[32];                  /* Id of variable */
  double dval;                  /* Its value if a real */
  FAR char *sval;               /* Its value if a string (malloced) */
};

struct mb_dimvar_s
{
  char id[32];                  /* Id of dimensioned variable */
  int type;                     /* Its type, STRID or FLTID */
  int ndims;                    /* Number of dimensions */
  int dim[5];                   /* Dimensions in x y order */
  FAR char **str;               /* Pointer to string data */
  FAR double *dval;             /* Pointer to real data */
};

struct mb_lvalue_s
{
  int type;                     /* Type of variable (STRID or FLTID or SYNTAX_ERROR) */
  FAR char **sval;              /* Pointer to string data */
  FAR double *dval;             /* Pointer to real data */
};

struct mb_forloop_s
{
  char id[32];                  /* Id of control variable */
  int nextline;                 /* Line below FOR to which control passes */
  double toval;                 /* Terminal value */
  double step;                  /* Step size */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTE: The use of these globals precludes the use of Mini Basic on
 * multiple threads (at least in a flat address environment).  If you
 * want multiple copies of Mini Basic to run, you would need to:
 * (1) Create a new struct mb_state_s that contains all of the following
 *     as fields.
 * (2) Allocate an instance of struct mb_state_s in basic() as part of the
 *     initialization logic. And,
 * (3) Pass the instance of struct mb_state_s to every Mini Basic function.
 */

static struct mb_forloop_s g_forstack[MAXFORS]; /* Stack for for loop control */
static int nfors;                               /* Number of fors on stack */

static FAR struct mb_variable_s *g_variables;   /* The script's variables */
static int g_nvariables;                        /* Number of variables */

static FAR struct mb_dimvar_s *g_dimvariables;  /* Dimensioned arrays */
static int g_ndimvariables;                     /* Number of dimensioned arrays */

static FAR struct mb_line_s *g_lines;           /* List of line starts */
static int nlines;                              /* Number of BASIC g_lines in program */

static FILE *g_fpin;                            /* Input stream */
static FILE *g_fpout;                           /* Output stream */
static FILE *g_fperr;                           /* Error stream */

static FAR const char *g_string;                /* String we are parsing */
static int g_token;                             /* Current token (lookahead) */
static int g_errorflag;                         /* Set when error in input encountered */
static char g_iobuffer[IOBUFSIZE];              /* I/O buffer */

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int setup(FAR const char *script);
static void cleanup(void);

static void reporterror(int lineno);
static int findline(int no);

static int line(void);
static void doprint(void);
static void dolet(void);
static void dodim(void);
static int doif(void);
static int dogoto(void);
static void doinput(void);
static void dorem(void);
static int dofor(void);
static int donext(void);

static void lvalue(FAR struct mb_lvalue_s *lv);

static int boolexpr(void);
static int boolfactor(void);
static int relop(void);

static double expr(void);
static double term(void);
static double factor(void);
static double instr(void);
static double variable(void);
static double dimvariable(void);

static FAR struct mb_variable_s *findvariable(FAR const char *id);
static FAR struct mb_dimvar_s *finddimvar(FAR const char *id);
static FAR struct mb_dimvar_s *dimension(FAR const char *id, int ndims, ...);
static FAR void *getdimvar(FAR struct mb_dimvar_s *dv, ...);
static FAR struct mb_variable_s *addfloat(FAR const char *id);
static FAR struct mb_variable_s *addstring(FAR const char *id);
static FAR struct mb_dimvar_s *adddimvar(FAR const char *id);

static FAR char *stringexpr(void);
static FAR char *chrstring(void);
static FAR char *strstring(void);
static FAR char *leftstring(void);
static FAR char *rightstring(void);
static FAR char *midstring(void);
static FAR char *stringstring(void);
static FAR char *stringdimvar(void);
static FAR char *stringvar(void);
static FAR char *stringliteral(void);

static int integer(double x);

static void match(int tok);
static void seterror(int errorcode);
static int getnextline(FAR const char *str);
static int gettoken(FAR const char *str);
static int tokenlen(FAR const char *str, int tokenid);

static int isstring(int tokenid);
static double getvalue(FAR const char *str, FAR int *len);
static void getid(FAR const char *str, FAR char *out, FAR int *len);

static void mystrgrablit(FAR char *dest, FAR const char *src);
static FAR char *mystrend(FAR const char *str, char quote);
static int mystrcount(FAR const char *str, char ch);
static FAR char *mystrdup(FAR const char *str);
static FAR char *mystrconcat(FAR const char *str, FAR const char *cat);
static double factorial(double x);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setup
 *
 * Description:
 *   Sets up all our globals, including the list of lines.
 *   Params: script - the script passed by the user
 *   Returns: 0 on success, -1 on failure
 *
 *
 ****************************************************************************/

static int setup(FAR const char *script)
{
  int i;

  nlines = mystrcount(script, '\n');
  g_lines = malloc(nlines * sizeof(struct mb_line_s));
  if (!g_lines)
    {
      if (g_fperr)
        {
          fprintf(g_fperr, "Out of memory\n");
        }

      return -1;
    }

  for (i = 0; i < nlines; i++)
    {
      if (isdigit(*script))
        {
          g_lines[i].str = script;
          g_lines[i].no = strtol(script, 0, 10);
        }
      else
        {
          i--;
          nlines--;
        }

      script = strchr(script, '\n');
      script++;
    }

  if (!nlines)
    {
      if (g_fperr)
        {
          fprintf(g_fperr, "Can't read program\n");
        }

      free(g_lines);
      return -1;
    }

  for (i = 1; i < nlines; i++)
    if (g_lines[i].no <= g_lines[i - 1].no)
      {
        if (g_fperr)
          {
            fprintf(g_fperr, "program lines %d and %d not in order\n",
                    g_lines[i - 1].no, g_lines[i].no);
          }

        free(g_lines);
        return -1;
      }

  g_nvariables = 0;
  g_variables = 0;

  g_dimvariables = 0;
  g_ndimvariables = 0;

  return 0;
}

/****************************************************************************
 * Name: cleanup
 *
 * Description:
 *   Frees all the memory we have allocated
 *
 ****************************************************************************/

static void cleanup(void)
{
  int i;
  int ii;
  int size;

  for (i = 0; i < g_nvariables; i++)
    {
      if (g_variables[i].sval)
        {
          free(g_variables[i].sval);
        }
    }

  if (g_variables)
    {
      free(g_variables);
    }

  g_variables = 0;
  g_nvariables = 0;

  for (i = 0; i < g_ndimvariables; i++)
    {
      if (g_dimvariables[i].type == STRID)
        {
          if (g_dimvariables[i].str)
            {
              size = 1;
              for (ii = 0; ii < g_dimvariables[i].ndims; ii++)
                {
                  size *= g_dimvariables[i].dim[ii];
                }

              for (ii = 0; ii < size; ii++)
                {
                  if (g_dimvariables[i].str[ii])
                    {
                      free(g_dimvariables[i].str[ii]);
                    }
                }

              free(g_dimvariables[i].str);
            }
        }
      else if (g_dimvariables[i].dval)
        {

          free(g_dimvariables[i].dval);
        }
    }

  if (g_dimvariables)
    {
      free(g_dimvariables);
    }

  g_dimvariables = 0;
  g_ndimvariables = 0;

  if (g_lines)
    {
      free(g_lines);
    }

  g_lines = 0;
  nlines = 0;
}

/****************************************************************************
 * Name: reporterror
 *
 * Description:
 *   Frror report function.
 *   For reporting errors in the user's script.
 *   Checks the global g_errorflag.
 *   Writes to g_fperr.
 *   Params: lineno - the line on which the error occurred
 *
 ****************************************************************************/

static void reporterror(int lineno)
{
  if (!g_fperr)
    {
      return;
    }

  switch (g_errorflag)
    {
    case ERR_CLEAR:
      assert(0);
      break;

    case ERR_SYNTAX:
      fprintf(g_fperr, "Syntax error line %d\n", lineno);
      break;

    case ERR_OUTOFMEMORY:
      fprintf(g_fperr, "Out of memory line %d\n", lineno);
      break;

    case ERR_IDTOOLONG:
      fprintf(g_fperr, "Identifier too long line %d\n", lineno);
      break;

    case ERR_NOSUCHVARIABLE:
      fprintf(g_fperr, "No such variable line %d\n", lineno);
      break;

    case ERR_BADSUBSCRIPT:
      fprintf(g_fperr, "Bad subscript line %d\n", lineno);
      break;

    case ERR_TOOMANYDIMS:
      fprintf(g_fperr, "Too many dimensions line %d\n", lineno);
      break;

    case ERR_TOOMANYINITS:
      fprintf(g_fperr, "Too many initialisers line %d\n", lineno);
      break;

    case ERR_BADTYPE:
      fprintf(g_fperr, "Illegal type line %d\n", lineno);
      break;

    case ERR_TOOMANYFORS:
      fprintf(g_fperr, "Too many nested fors line %d\n", lineno);
      break;

    case ERR_NONEXT:
      fprintf(g_fperr, "For without matching next line %d\n", lineno);
      break;

    case ERR_NOFOR:
      fprintf(g_fperr, "Next without matching for line %d\n", lineno);
      break;

    case ERR_DIVIDEBYZERO:
      fprintf(g_fperr, "Divide by zero lne %d\n", lineno);
      break;

    case ERR_NEGLOG:
      fprintf(g_fperr, "Negative logarithm line %d\n", lineno);
      break;

    case ERR_NEGSQRT:
      fprintf(g_fperr, "Negative square root line %d\n", lineno);
      break;

    case ERR_BADSINCOS:
      fprintf(g_fperr, "Sine or cosine out of range line %d\n", lineno);
      break;

    case ERR_EOF:
      fprintf(g_fperr, "End of input file %d\n", lineno);
      break;

    case ERR_ILLEGALOFFSET:
      fprintf(g_fperr, "Illegal offset line %d\n", lineno);
      break;

    case ERR_TYPEMISMATCH:
      fprintf(g_fperr, "Type mismatch line %d\n", lineno);
      break;

    case ERR_INPUTTOOLONG:
      fprintf(g_fperr, "Input too long line %d\n", lineno);
      break;

    case ERR_BADVALUE:
      fprintf(g_fperr, "Bad value at line %d\n", lineno);
      break;

    case ERR_NOTINT:
      fprintf(g_fperr, "Not an integer at line %d\n", lineno);
      break;

    default:
      fprintf(g_fperr, "ERROR line %d\n", lineno);
      break;
    }
}

/****************************************************************************
 * Name: findline
 *
 * Description:
 *   Binary search for a line
 *   Params: no - line number to find
 *   Returns: index of the line, or -1 on fail.
 *
 ****************************************************************************/

static int findline(int no)
{
  int high;
  int low;
  int mid;

  low = 0;
  high = nlines - 1;
  while (high > low + 1)
    {
      mid = (high + low) / 2;
      if (g_lines[mid].no == no)
        {
          return mid;
        }

      if (g_lines[mid].no > no)
        {
          high = mid;
        }
      else
        {
          low = mid;
        }
    }

  if (g_lines[low].no == no)
    {
      mid = low;
    }
  else if (g_lines[high].no == no)
    {
      mid = high;
    }
  else
    {
      mid = -1;
    }

  return mid;
}

/****************************************************************************
 * Name: line
 *
 * Description:
 *   Parse a line. High level parse function
 *
 ****************************************************************************/

static int line(void)
{
  int answer = 0;
  FAR const char *str;

  match(VALUE);

  switch (g_token)
    {
    case PRINT:
      doprint();
      break;

    case LET:
      dolet();
      break;

    case DIM:
      dodim();
      break;

    case IF:
      answer = doif();
      break;

    case GOTO:
      answer = dogoto();
      break;

    case INPUT:
      doinput();
      break;

    case REM:
      dorem();
      return 0;

    case FOR:
      answer = dofor();
      break;

    case NEXT:
      answer = donext();
      break;

    default:
      seterror(ERR_SYNTAX);
      break;
    }

  if (g_token != EOS)
    {
      /* match(VALUE); */
      /* check for a newline */

      str = g_string;
      while (isspace(*str))
        {
          if (*str == '\n')
            {
              break;
            }

          str++;
        }

      if (*str != '\n')
        {
          seterror(ERR_SYNTAX);
        }
    }

  return answer;
}

/****************************************************************************
 * Name: doprint
 *
 * Description:
 *   The PRINT statement
 *
 ****************************************************************************/

static void doprint(void)
{
  FAR char *str;
  double x;

  match(PRINT);

  while (1)
    {
      if (isstring(g_token))
        {
          str = stringexpr();
          if (str)
            {
              fprintf(g_fpout, "%s", str);
              free(str);
            }
        }
      else
        {
          x = expr();
          fprintf(g_fpout, "%g", x);
        }

      if (g_token == COMMA)
        {
          fprintf(g_fpout, " ");
          match(COMMA);
        }
      else
        {
          break;
        }
    }

  if (g_token == SEMICOLON)
    {
      match(SEMICOLON);
      fflush(g_fpout);
    }
  else
    {
      fprintf(g_fpout, "\n");
    }
}

/****************************************************************************
 * Name: dolet
 *
 * Description:
 *   The LET statement
 *
 ****************************************************************************/

static void dolet(void)
{
  struct mb_lvalue_s lv;
  FAR char *temp;

  match(LET);
  lvalue(&lv);
  match(EQUALS);
  switch (lv.type)
    {
    case FLTID:
      *lv.dval = expr();
      break;

    case STRID:
      temp = *lv.sval;
      *lv.sval = stringexpr();
      if (temp)
        {
          free(temp);
        }

      break;

    default:
      break;
    }
}

/****************************************************************************
 * Name: dodim
 *
 * Description:
 *   The DIM statement
 *
 ****************************************************************************/

static void dodim(void)
{
  int ndims = 0;
  double dims[6];
  char name[32];
  int len;
  FAR struct mb_dimvar_s *dimvar;
  int i;
  int size = 1;

  match(DIM);

  switch (g_token)
    {
    case DIMFLTID:
    case DIMSTRID:
      getid(g_string, name, &len);
      match(g_token);
      dims[ndims++] = expr();
      while (g_token == COMMA)
        {
          match(COMMA);
          dims[ndims++] = expr();
          if (ndims > 5)
            {
              seterror(ERR_TOOMANYDIMS);
              return;
            }
        }

      match(CPAREN);

      for (i = 0; i < ndims; i++)
        {
          if (dims[i] < 0 || dims[i] != (int)dims[i])
            {
              seterror(ERR_BADSUBSCRIPT);
              return;
            }
        }

      switch (ndims)
        {
        case 1:
          dimvar = dimension(name, 1, (int)dims[0]);
          break;

        case 2:
          dimvar = dimension(name, 2, (int)dims[0], (int)dims[1]);
          break;

        case 3:
          dimvar = dimension(name, 3, (int)dims[0], (int)dims[1], (int)dims[2]);
          break;

        case 4:
          dimvar =
            dimension(name, 4, (int)dims[0], (int)dims[1], (int)dims[2],
                      (int)dims[3]);
          break;

        case 5:
          dimvar =
            dimension(name, 5, (int)dims[0], (int)dims[1], (int)dims[2],
                      (int)dims[3], (int)dims[4]);
          break;
        }
      break;

    default:
      seterror(ERR_SYNTAX);
      return;
    }

  if (dimvar == 0)
    {
      /* Out of memory */

      seterror(ERR_OUTOFMEMORY);
      return;
    }

  if (g_token == EQUALS)
    {
      match(EQUALS);

      for (i = 0; i < dimvar->ndims; i++)
        {
          size *= dimvar->dim[i];
        }

      switch (dimvar->type)
        {
        case FLTID:
          i = 0;
          dimvar->dval[i++] = expr();
          while (g_token == COMMA && i < size)
            {
              match(COMMA);
              dimvar->dval[i++] = expr();
              if (g_errorflag)
                {
                  break;
                }
            }
          break;

        case STRID:
          i = 0;
          if (dimvar->str[i])
            {
              free(dimvar->str[i]);
            }

          dimvar->str[i++] = stringexpr();

          while (g_token == COMMA && i < size)
            {
              match(COMMA);
              if (dimvar->str[i])
                {
                  free(dimvar->str[i]);
                }

              dimvar->str[i++] = stringexpr();
              if (g_errorflag)
                {
                  break;
                }
            }
          break;
        }

      if (g_token == COMMA)
        {
          seterror(ERR_TOOMANYINITS);
        }
    }
}

/****************************************************************************
 * Name: doif
 *
 * Description:
 *   The IF statement.
 *   If jump taken, returns new line no, else returns 0
 *
 ****************************************************************************/

static int doif(void)
{
  int condition;
  int jump;

  match(IF);
  condition = boolexpr();
  match(THEN);
  jump = integer(expr());
  if (condition)
    {
      return jump;
    }
  else
    {
      return 0;
    }
}

/****************************************************************************
 * Name: dogoto
 *
 * Description:
 *   The GOTO satement
 *   Returns new line number
 *
 ****************************************************************************/

static int dogoto(void)
{
  match(GOTO);
  return integer(expr());
}

/****************************************************************************
 * Name: dofor
 *
 * Description:
 *   The FOR statement.
 *
 *   Pushes the for stack.
 *   Returns line to jump to, or -1 to end program
 *
 ****************************************************************************/

static int dofor(void)
{
  struct mb_lvalue_s lv;
  char id[32];
  char nextid[32];
  int len;
  double initval;
  double toval;
  double stepval;
  FAR const char *savestring;
  int answer;

  match(FOR);
  getid(g_string, id, &len);

  lvalue(&lv);
  if (lv.type != FLTID)
    {
      seterror(ERR_BADTYPE);
      return -1;
    }

  match(EQUALS);
  initval = expr();
  match(TO);
  toval = expr();

  if (g_token == STEP)
    {
      match(STEP);
      stepval = expr();
    }
  else
    {
      stepval = 1.0;
    }

  *lv.dval = initval;

  if (nfors > MAXFORS - 1)
    {
      seterror(ERR_TOOMANYFORS);
      return -1;
    }

  if ((stepval < 0 && initval < toval) ||
      (stepval > 0 && initval > toval))
    {
      savestring = g_string;
      while ((g_string = strchr(g_string, '\n')) != NULL)
        {
          g_errorflag = 0;
          g_token = gettoken(g_string);
          match(VALUE);
          if (g_token == NEXT)
            {
              match(NEXT);
              if (g_token == FLTID || g_token == DIMFLTID)
                {
                  getid(g_string, nextid, &len);
                  if (!strcmp(id, nextid))
                    {
                      answer = getnextline(g_string);
                      g_string = savestring;
                      g_token = gettoken(g_string);
                      return answer ? answer : -1;
                    }
                }
            }
        }

      seterror(ERR_NONEXT);
      return -1;
    }
  else
    {
      strcpy(g_forstack[nfors].id, id);
      g_forstack[nfors].nextline = getnextline(g_string);
      g_forstack[nfors].step = stepval;
      g_forstack[nfors].toval = toval;
      nfors++;
      return 0;
    }
}

/****************************************************************************
 * Name: donext
 *
 * Description:
 *   The NEXT statement
 *   Updates the counting index, and returns line to jump to
 *
 ****************************************************************************/

static int donext(void)
{
  char id[32];
  int len;
  struct mb_lvalue_s lv;

  match(NEXT);

  if (nfors)
    {
      getid(g_string, id, &len);
      lvalue(&lv);
      if (lv.type != FLTID)
        {
          seterror(ERR_BADTYPE);
          return -1;
        }

      *lv.dval += g_forstack[nfors - 1].step;
      if ((g_forstack[nfors - 1].step < 0 &&
           *lv.dval < g_forstack[nfors - 1].toval) ||
          (g_forstack[nfors - 1].step > 0 &&
           *lv.dval > g_forstack[nfors - 1].toval))
        {
          nfors--;
          return 0;
        }
      else
        {
          return g_forstack[nfors - 1].nextline;
        }
    }
  else
    {
      seterror(ERR_NOFOR);
      return -1;
    }
}

/****************************************************************************
 * Name: doinput
 *
 * Description:
 *   The INPUT statement
 *
 ****************************************************************************/

static void doinput(void)
{
  struct mb_lvalue_s lv;
  FAR char *end;

  match(INPUT);
  lvalue(&lv);

  switch (lv.type)
    {
    case FLTID:
      {
        FAR char *ptr;
        int nch;

        /* Copy floating point number to a g_iobuffer.  Skip over leading
         * spaces and terminate with a NUL when a space, tab, newline, EOF,
         * or comma is detected.
         */

        for (nch = 0, ptr = g_iobuffer; nch < (IOBUFSIZE-1); nch++)
          {
            int ch = fgetc(g_fpin);
            if (ch == EOF)
              {
                seterror(ERR_EOF);
                return;
              }

            if (ch == ' ' || ch == '\t' || ch == ',' || ch == '\n')
              {
                ungetc(ch, g_fpin);
                break;
              }
          }

        *ptr = '\0';

        /* Use strtod() to get the floating point value from the substring
         * in g_iobuffer.
         */

        *lv.dval = strtod(g_iobuffer, &ptr);
        if (ptr == g_iobuffer)
          {
            seterror(ERR_SYNTAX);
            return;
          }
      }
      break;

    case STRID:
      {
        if (*lv.sval)
          {
            free(*lv.sval);
            *lv.sval = NULL;
          }

        if (fgets(g_iobuffer, IOBUFSIZE, g_fpin) == 0)
          {
            seterror(ERR_EOF);
            return;
          }

        end = strchr(g_iobuffer, '\n');
        if (!end)
          {
            seterror(ERR_INPUTTOOLONG);
            return;
          }

        *end = 0;
        *lv.sval = mystrdup(g_iobuffer);
        if (!*lv.sval)
          {
            seterror(ERR_OUTOFMEMORY);
            return;
          }
      }
      break;

    default:
      break;
    }
}

/****************************************************************************
 * Name: dorem
 *
 * Description:
 *   The REM statement.
 *   Note is unique as the rest of the line is not parsed
 *
 ****************************************************************************/

static void dorem(void)
{
  match(REM);
  return;
}

/****************************************************************************
 * Name: lvalue
 *
 * Description:
 *   Get an lvalue from the environment
 *   Params: lv - structure to fill.
 *   Notes: missing variables (but not out of range subscripts)
 *          are added to the variable list.
 *
 ****************************************************************************/

static void lvalue(FAR struct mb_lvalue_s *lv)
{
  char name[32];
  int len;
  FAR struct mb_variable_s *var;
  FAR struct mb_dimvar_s *dimvar;
  int index[5];
  FAR void *valptr = 0;
  int type;

  lv->type = SYNTAX_ERROR;
  lv->dval = NULL;
  lv->sval = NULL;

  switch (g_token)
    {
    case FLTID:
      {
        getid(g_string, name, &len);
        match(FLTID);
        var = findvariable(name);
        if (!var)
          {
            var = addfloat(name);
          }

        if (!var)
          {
            seterror(ERR_OUTOFMEMORY);
            return;
          }

        lv->type = FLTID;
        lv->dval = &var->dval;
        lv->sval = NULL;
      }
      break;

    case STRID:
      {
        getid(g_string, name, &len);
        match(STRID);
        var = findvariable(name);
        if (!var)
          {
            var = addstring(name);
          }

        if (!var)
          {
            seterror(ERR_OUTOFMEMORY);
            return;
          }

        lv->type = STRID;
        lv->sval = &var->sval;
        lv->dval = NULL;
      }
      break;

    case DIMFLTID:
    case DIMSTRID:
      {
        type = (g_token == DIMFLTID) ? FLTID : STRID;
        getid(g_string, name, &len);
        match(g_token);
        dimvar = finddimvar(name);
        if (dimvar)
          {
            switch (dimvar->ndims)
              {
              case 1:
                {
                  index[0] = integer(expr());
                  if (g_errorflag == 0)
                    {
                      valptr = getdimvar(dimvar, index[0]);
                    }
                }
                break;

              case 2:
                {
                  index[0] = integer(expr());
                  match(COMMA);
                  index[1] = integer(expr());
                  if (g_errorflag == 0)
                    {
                      valptr = getdimvar(dimvar, index[0], index[1]);
                    }
                }
                break;

              case 3:
                {
                  index[0] = integer(expr());
                  match(COMMA);
                  index[1] = integer(expr());
                  match(COMMA);
                  index[2] = integer(expr());
                  if (g_errorflag == 0)
                    {
                      valptr = getdimvar(dimvar, index[0], index[1], index[2]);
                    }
                }
                break;

              case 4:
                {
                  index[0] = integer(expr());
                  match(COMMA);
                  index[1] = integer(expr());
                  match(COMMA);
                  index[2] = integer(expr());
                  match(COMMA);
                  index[3] = integer(expr());
                  if (g_errorflag == 0)
                    {
                      valptr =
                        getdimvar(dimvar, index[0], index[1], index[2], index[3]);
                    }
                }
                break;

              case 5:
                {
                  index[0] = integer(expr());
                  match(COMMA);
                  index[1] = integer(expr());
                  match(COMMA);
                  index[2] = integer(expr());
                  match(COMMA);
                  index[3] = integer(expr());
                  match(COMMA);
                  index[4] = integer(expr());
                  if (g_errorflag == 0)
                    {
                      valptr =
                        getdimvar(dimvar, index[0], index[1], index[2], index[3]);
                    }
                }
                break;
             }

           match(CPAREN);
         }
      else
        {
          seterror(ERR_NOSUCHVARIABLE);
          return;
        }

      if (valptr)
        {
          lv->type = type;
          if (type == FLTID)
            {
              lv->dval = valptr;
            }
          else if (type == STRID)
            {
              lv->sval = valptr;
            }
          else
            {
              assert(0);
            }
        }
      }
      break;

    default:
      seterror(ERR_SYNTAX);
    }
}

/****************************************************************************
 * Name: boolexpr
 *
 * Description:
 *   Parse a boolean expression
 *   Consists of expressions or strings and relational operators,
 *   and parentheses
 *
 ****************************************************************************/

static int boolexpr(void)
{
  int left;
  int right;

  left = boolfactor();

  while (1)
    {
      switch (g_token)
        {
        case AND:
          match(AND);
          right = boolexpr();
          return (left && right) ? 1 : 0;

        case OR:
          match(OR);
          right = boolexpr();
          return (left || right) ? 1 : 0;

        default:
          return left;
        }
    }
}

/****************************************************************************
 * Name: boolfactor
 *
 * Description:
 *   Boolean factor, consists of expression relop expression
 *   or string relop string, or ( boolexpr() )
 *
 ****************************************************************************/

static int boolfactor(void)
{
  int answer;
  double left;
  double right;
  int op;
  FAR char *strleft;
  FAR char *strright;
  int cmp;

  switch (g_token)
    {
    case OPAREN:
      match(OPAREN);
      answer = boolexpr();
      match(CPAREN);
      break;

    default:
      if (isstring(g_token))
        {
          strleft = stringexpr();
          op = relop();
          strright = stringexpr();
          if (!strleft || !strright)
            {
              if (strleft)
                {
                  free(strleft);
                }

              if (strright)
                {
                  free(strright);
                }

              return 0;
            }
          cmp = strcmp(strleft, strright);
          switch (op)
            {
            case ROP_EQ:
              answer = cmp == 0 ? 1 : 0;
              break;

            case ROP_NEQ:
              answer = cmp == 0 ? 0 : 1;
              break;

            case ROP_LT:
              answer = cmp < 0 ? 1 : 0;
              break;

            case ROP_LTE:
              answer = cmp <= 0 ? 1 : 0;
              break;

            case ROP_GT:
              answer = cmp > 0 ? 1 : 0;
              break;

            case ROP_GTE:
              answer = cmp >= 0 ? 1 : 0;
              break;

            default:
              answer = 0;
            }

          free(strleft);
          free(strright);
        }
      else
        {
          left = expr();
          op = relop();
          right = expr();
          switch (op)
            {
            case ROP_EQ:
              answer = (left == right) ? 1 : 0;
              break;

            case ROP_NEQ:
              answer = (left != right) ? 1 : 0;
              break;

            case ROP_LT:
              answer = (left < right) ? 1 : 0;
              break;

            case ROP_LTE:
              answer = (left <= right) ? 1 : 0;
              break;

            case ROP_GT:
              answer = (left > right) ? 1 : 0;
              break;

            case ROP_GTE:
              answer = (left >= right) ? 1 : 0;
              break;

            default:
              g_errorflag = 1;
              return 0;
            }
        }
    }

  return answer;
}

/****************************************************************************
 * Name: relop
 *
 * Description:
 *   Get a relational operator
 *   Returns operator parsed or SYNTAX_ERROR
 *
 ****************************************************************************/

static int relop(void)
{
  switch (g_token)
    {
    case EQUALS:
      match(EQUALS);
      return ROP_EQ;

    case GREATER:
      match(GREATER);
      if (g_token == EQUALS)
        {
          match(EQUALS);
          return ROP_GTE;
        }

      return ROP_GT;

    case LESS:
      match(LESS);
      if (g_token == EQUALS)
        {
          match(EQUALS);
          return ROP_LTE;
        }
      else if (g_token == GREATER)
        {
          match(GREATER);
          return ROP_NEQ;
        }

      return ROP_LT;

    default:
      seterror(ERR_SYNTAX);
      return SYNTAX_ERROR;
    }
}

/****************************************************************************
 * Name: expr
 *
 * Description:
 *   Parses an expression
 *
 ****************************************************************************/

static double expr(void)
{
  double left;
  double right;

  left = term();

  while (1)
    {
      switch (g_token)
        {
        case PLUS:
          match(PLUS);
          right = term();
          left += right;
          break;

        case MINUS:
          match(MINUS);
          right = term();
          left -= right;
          break;

        default:
          return left;
        }
    }
}

/****************************************************************************
 * Name: term
 *
 * Description:
 *   Parses a term
 *
 ****************************************************************************/

static double term(void)
{
  double left;
  double right;

  left = factor();

  while (1)
    {
      switch (g_token)
        {
        case MULT:
          match(MULT);
          right = factor();
          left *= right;
          break;

        case DIV:
          match(DIV);
          right = factor();
          if (right != 0.0)
            {
              left /= right;
            }
          else
            {
              seterror(ERR_DIVIDEBYZERO);
            }
          break;

        case MOD:
          match(MOD);
          right = factor();
          left = fmod(left, right);
          break;

        default:
          return left;
        }
    }
}

/****************************************************************************
 * Name: factor
 *
 * Description:
 *   Parses a factor
 *
 ****************************************************************************/

static double factor(void)
{
  double answer = 0;
  FAR char *str;
  FAR char *end;
  int len;

  switch (g_token)
    {
    case OPAREN:
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      break;

    case VALUE:
      answer = getvalue(g_string, &len);
      match(VALUE);
      break;

    case MINUS:
      match(MINUS);
      answer = -factor();
      break;

    case FLTID:
      answer = variable();
      break;

    case DIMFLTID:
      answer = dimvariable();
      break;

    case E:
      answer = exp(1.0);
      match(E);
      break;

    case PI:
      answer = acos(0.0) * 2.0;
      match(PI);
      break;

    case SIN:
      match(SIN);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = sin(answer);
      break;

    case COS:
      match(COS);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = cos(answer);
      break;

    case TAN:
      match(TAN);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = tan(answer);
      break;

    case LN:
      match(LN);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      if (answer > 0)
        {
          answer = log(answer);
        }
      else
        {
          seterror(ERR_NEGLOG);
        }
      break;

    case POW:
      match(POW);
      match(OPAREN);
      answer = expr();
      match(COMMA);
      answer = pow(answer, expr());
      match(CPAREN);
      break;

    case SQRT:
      match(SQRT);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      if (answer >= 0.0)
        {
          answer = sqrt(answer);
        }
      else
        {
          seterror(ERR_NEGSQRT);
        }
      break;

    case ABS:
      match(ABS);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = fabs(answer);
      break;

    case LEN:
      match(LEN);
      match(OPAREN);
      str = stringexpr();
      match(CPAREN);
      if (str)
        {
          answer = strlen(str);
          free(str);
        }
      else
        {
          answer = 0;
        }
      break;

    case ASCII:
      match(ASCII);
      match(OPAREN);
      str = stringexpr();
      match(CPAREN);
      if (str)
        {
          answer = *str;
          free(str);
        }
      else
        {
          answer = 0;
        }
      break;

    case ASIN:
      match(ASIN);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      if (answer >= -1 && answer <= 1)
        {
          answer = asin(answer);
        }
      else
        {
          seterror(ERR_BADSINCOS);
        }
      break;

    case ACOS:
      match(ACOS);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      if (answer >= -1 && answer <= 1)
        {
          answer = acos(answer);
        }
      else
        {
          seterror(ERR_BADSINCOS);
        }
      break;

    case ATAN:
      match(ATAN);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = atan(answer);
      break;

    case INT:
      match(INT);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = floor(answer);
      break;

    case RND:
      match(RND);
      match(OPAREN);
      answer = expr();
      match(CPAREN);
      answer = integer(answer);
      if (answer > 1)
        {
          answer = floor(rand() / (RAND_MAX + 1.0) * answer);
        }
      else if (answer == 1)
        {
          answer = rand() / (RAND_MAX + 1.0);
        }
      else
        {
          if (answer < 0)
            {
              srand((unsigned)-answer);
            }
          answer = 0;
        }
      break;

    case VAL:
      match(VAL);
      match(OPAREN);
      str = stringexpr();
      match(CPAREN);
      if (str)
        {
          answer = strtod(str, 0);
          free(str);
        }
      else
        {
          answer = 0;
        }
      break;

    case VALLEN:
      match(VALLEN);
      match(OPAREN);
      str = stringexpr();
      match(CPAREN);
      if (str)
        {
          strtod(str, &end);
          answer = end - str;
          free(str);
        }
      else
        {
          answer = 0.0;
        }
      break;

    case INSTR:
      answer = instr();
      break;

    default:
      if (isstring(g_token))
        {
          seterror(ERR_TYPEMISMATCH);
        }
      else
        {
          seterror(ERR_SYNTAX);
        }
      break;
    }

  while (g_token == SHRIEK)
    {
      match(SHRIEK);
      answer = factorial(answer);
    }

  return answer;
}

/****************************************************************************
 * Name: instr
 *
 * Description:
 *   Calculate the INSTR() function.
 *
 ****************************************************************************/

static double instr(void)
{
  FAR char *str;
  FAR char *substr;
  FAR char *end;
  double answer = 0;
  int offset;

  match(INSTR);
  match(OPAREN);
  str = stringexpr();
  match(COMMA);
  substr = stringexpr();
  match(COMMA);
  offset = integer(expr());
  offset--;
  match(CPAREN);

  if (!str || !substr)
    {
      if (str)
        {
          free(str);
        }

      if (substr)
        {
          free(substr);
        }

      return 0;
    }

  if (offset >= 0 && offset < (int)strlen(str))
    {
      end = strstr(str + offset, substr);
      if (end)
        {
          answer = end - str + 1.0;
        }
    }

  free(str);
  free(substr);
  return answer;
}

/****************************************************************************
 * Name: variable
 *
 * Description:
 *   Get the value of a scalar variable from string
 *   matches FLTID
 *
 *
 ****************************************************************************/

static double variable(void)
{
  FAR struct mb_variable_s *var;
  char id[32];
  int len;

  getid(g_string, id, &len);
  match(FLTID);
  var = findvariable(id);
  if (var)
    {
      return var->dval;
    }
  else
    {
      seterror(ERR_NOSUCHVARIABLE);
      return 0.0;
    }
}

/****************************************************************************
 * Name: dimvariable
 *
 * Description:
 *   Get value of a dimensioned variable from string.
 *   matches DIMFLTID
 *
 ****************************************************************************/

static double dimvariable(void)
{
  FAR struct mb_dimvar_s *dimvar;
  char id[32];
  int len;
  int index[5];
  FAR double *answer = NULL;

  getid(g_string, id, &len);
  match(DIMFLTID);
  dimvar = finddimvar(id);
  if (!dimvar)
    {
      seterror(ERR_NOSUCHVARIABLE);
      return 0.0;
    }

  if (dimvar)
    {
      switch (dimvar->ndims)
        {
        case 1:
          index[0] = integer(expr());
          answer = getdimvar(dimvar, index[0]);
          break;

        case 2:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1]);
          break;

        case 3:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1], index[2]);
          break;

        case 4:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          match(COMMA);
          index[3] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
          break;

        case 5:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          match(COMMA);
          index[3] = integer(expr());
          match(COMMA);
          index[4] = integer(expr());
          answer =
            getdimvar(dimvar, index[0], index[1], index[2], index[3], index[4]);
          break;
        }

      match(CPAREN);
    }

  if (answer != NULL)
    {
      return *answer;
    }

  return 0.0;
}

/****************************************************************************
 * Name: findvariable
 *
 * Description:
 *   Find a scalar variable invariables list
 *   Params: id - id to get
 *   Returns: pointer to that entry, 0 on fail
 *
 ****************************************************************************/

static FAR struct mb_variable_s *findvariable(FAR const char *id)
{
  int i;

  for (i = 0; i < g_nvariables; i++)
    {
      if (!strcmp(g_variables[i].id, id))
        {
          return &g_variables[i];
        }
    }

  return 0;
}

/****************************************************************************
 * Name: finddimvar
 *
 * Description:
 *   Get a dimensioned array by name
 *   Params: id (includes opening parenthesis)
 *   Returns: pointer to array entry or 0 on fail
 *
 ****************************************************************************/

static struct mb_dimvar_s *finddimvar(FAR const char *id)
{
  int i;

  for (i = 0; i < g_ndimvariables; i++)
    {
      if (!strcmp(g_dimvariables[i].id, id))
        {
          return &g_dimvariables[i];
        }
    }

  return 0;
}

/****************************************************************************
 * Name: dimension
 *
 * Description:
 *   Dimension an array.
 *   Params: id - the id of the array (include leading ()
 *           ndims - number of dimension (1-5)
 *         ... - integers giving dimension size,
 *
 ****************************************************************************/

static FAR struct mb_dimvar_s *dimension(FAR const char *id, int ndims, ...)
{
  FAR struct mb_dimvar_s *dv;
  va_list vargs;
  int size = 1;
  int oldsize = 1;
  int i;
  int dimensions[5];
  FAR double *dtemp;
  FAR char **stemp;

  assert(ndims <= 5);
  if (ndims > 5)
    {
      return 0;
    }

  dv = finddimvar(id);
  if (!dv)
    {
      dv = adddimvar(id);
    }

  if (!dv)
    {
      seterror(ERR_OUTOFMEMORY);
      return 0;
    }

  if (dv->ndims)
    {
      for (i = 0; i < dv->ndims; i++)
        {
          oldsize *= dv->dim[i];
        }
    }
  else
    {
      oldsize = 0;
    }

  va_start(vargs, ndims);
  for (i = 0; i < ndims; i++)
    {
      dimensions[i] = va_arg(vargs, int);
      size *= dimensions[i];
    }

  va_end(vargs);

  switch (dv->type)
    {
    case FLTID:
      dtemp = realloc(dv->dval, size * sizeof(double));
      if (dtemp)
        {
          dv->dval = dtemp;
        }
      else
        {
          seterror(ERR_OUTOFMEMORY);
          return 0;
        }
      break;

    case STRID:
      if (dv->str)
        {
          for (i = size; i < oldsize; i++)
            {
              if (dv->str[i])
                {
                  free(dv->str[i]);
                  dv->str[i] = 0;
                }
            }
        }

      stemp = realloc(dv->str, size * sizeof(char *));
      if (stemp)
        {
          dv->str = stemp;
          for (i = oldsize; i < size; i++)
            {
              dv->str[i] = 0;
            }
        }
      else
        {
          for (i = 0; i < oldsize; i++)
            {
              if (dv->str[i])
                {
                  free(dv->str[i]);
                  dv->str[i] = 0;
                }
            }

          seterror(ERR_OUTOFMEMORY);
          return 0;
        }
      break;

    default:
      assert(0);
    }

  for (i = 0; i < 5; i++)
    {
      dv->dim[i] = dimensions[i];
    }

  dv->ndims = ndims;

  return dv;
}

/****************************************************************************
 * Name: getdimvar
 *
 * Description:
 *   Get the address of a dimensioned array element.
 *   Works for both string and real arrays.
 *   Params: dv - the array's entry in variable list
 *           ... - integers telling which array element to get
 *   Returns: the address of that element, 0 on fail
 *
 ****************************************************************************/

static FAR void *getdimvar(FAR struct mb_dimvar_s *dv, ...)
{
  va_list vargs;
  int index[5];
  int i;
  FAR void *answer = 0;

  va_start(vargs, dv);
  for (i = 0; i < dv->ndims; i++)
    {
      index[i] = va_arg(vargs, int);
      index[i]--;
    }

  va_end(vargs);

  for (i = 0; i < dv->ndims; i++)
    {
      if (index[i] >= dv->dim[i] || index[i] < 0)
        {
          seterror(ERR_BADSUBSCRIPT);
          return 0;
        }
    }

  if (dv->type == FLTID)
    {
      switch (dv->ndims)
        {
        case 1:
          answer = &dv->dval[index[0]];
          break;

        case 2:
          answer = &dv->dval[index[1] * dv->dim[0] + index[0]];
          break;
        case 3:
          answer = &dv->dval[index[2] * (dv->dim[0] * dv->dim[1])
                             + index[1] * dv->dim[0] + index[0]];
          break;

        case 4:
          answer =
            &dv->dval[index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) +
                      index[2] * (dv->dim[0] * dv->dim[1]) +
                      index[1] * dv->dim[0] + index[0]];
          break;

        case 5:
          answer =
            &dv->dval[index[4] *
                      (dv->dim[0] + dv->dim[1] + dv->dim[2] +
                       dv->dim[3]) + index[3] * (dv->dim[0] + dv->dim[1] +
                                                 dv->dim[2]) +
                      index[2] * (dv->dim[0] + dv->dim[1]) +
                      index[1] * dv->dim[0] + index[0]];
          break;
        }
    }
  else if (dv->type == STRID)
    {
      switch (dv->ndims)
        {
        case 1:
          answer = &dv->str[index[0]];
          break;

        case 2:
          answer = &dv->str[index[1] * dv->dim[0] + index[0]];
          break;

        case 3:
          answer = &dv->str[index[2] * (dv->dim[0] * dv->dim[1])
                            + index[1] * dv->dim[0] + index[0]];
          break;

        case 4:
          answer =
            &dv->str[index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) +
                     index[2] * (dv->dim[0] * dv->dim[1]) +
                     index[1] * dv->dim[0] + index[0]];
          break;

        case 5:
          answer =
            &dv->str[index[4] *
                     (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3]) +
                     index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) +
                     index[2] * (dv->dim[0] + dv->dim[1]) +
                     index[1] * dv->dim[0] + index[0]];
          break;
        }
    }

  return answer;
}

/****************************************************************************
 * Name: addfloat
 *
 * Description:
 *   Add a real variable to our variable list
 *   Params: id - id of variable to add.
 *   Returns: pointer to new entry in table
 *
 ****************************************************************************/

static FAR struct mb_variable_s *addfloat(FAR const char *id)
{
  FAR struct mb_variable_s *vars;

  vars =
    realloc(g_variables, (g_nvariables + 1) * sizeof(struct mb_variable_s));
  if (vars)
    {
      g_variables = vars;
      strcpy(g_variables[g_nvariables].id, id);
      g_variables[g_nvariables].dval = 0.0;
      g_variables[g_nvariables].sval = NULL;
      g_nvariables++;
      return &g_variables[g_nvariables - 1];
    }
  else
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return 0;
}

/****************************************************************************
 * Name: addstring
 *
 * Description:
 *   Add a string variable to table.
 *   Params: id - id of variable to get (including trailing $)
 *   Returns: pointer to new entry in table, 0 on fail.
 *
 ****************************************************************************/

static FAR struct mb_variable_s *addstring(FAR const char *id)
{
  FAR struct mb_variable_s *vars;

  vars =
    realloc(g_variables, (g_nvariables + 1) * sizeof(struct mb_variable_s));
  if (vars)
    {
      g_variables = vars;
      strcpy(g_variables[g_nvariables].id, id);
      g_variables[g_nvariables].sval = NULL;
      g_variables[g_nvariables].dval = 0.0;
      g_nvariables++;
      return &g_variables[g_nvariables - 1];
    }
  else
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return 0;
}

/****************************************************************************
 * Name: adddimvar
 *
 * Description:
 *   Add a new array to our symbol table.
 *   Params: id - id of array (include leading ()
 *   Returns: pointer to new entry, 0 on fail.
 *
 ****************************************************************************/

static FAR struct mb_dimvar_s *adddimvar(FAR const char *id)
{
  FAR struct mb_dimvar_s *vars;

  vars =
    realloc(g_dimvariables, (g_ndimvariables + 1) * sizeof(struct mb_dimvar_s));
  if (vars)
    {
      g_dimvariables = vars;
      strcpy(g_dimvariables[g_ndimvariables].id, id);
      g_dimvariables[g_ndimvariables].dval  = NULL;
      g_dimvariables[g_ndimvariables].str   = NULL;
      g_dimvariables[g_ndimvariables].ndims = 0;
      g_dimvariables[g_ndimvariables].type  = strchr(id, '$') ? STRID : FLTID;
      g_ndimvariables++;
      return &g_dimvariables[g_ndimvariables - 1];
    }
  else
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return 0;
}

/****************************************************************************
 * Name: stringexpr
 *
 * Description:
 *   High level string parsing function.
 *   Returns: a malloced pointer, or 0 on error condition.
 *   caller must free!
 *
 ****************************************************************************/

static FAR char *stringexpr(void)
{
  FAR char *left;
  FAR char *right;
  FAR char *temp;

  switch (g_token)
    {
    case DIMSTRID:
      left = mystrdup(stringdimvar());
      break;

    case STRID:
      left = mystrdup(stringvar());
      break;

    case QUOTE:
      left = stringliteral();
      break;

    case CHRSTRING:
      left = chrstring();
      break;

    case STRSTRING:
      left = strstring();
      break;

    case LEFTSTRING:
      left = leftstring();
      break;

    case RIGHTSTRING:
      left = rightstring();
      break;

    case MIDSTRING:
      left = midstring();
      break;

    case STRINGSTRING:
      left = stringstring();
      break;

    default:
      if (!isstring(g_token))
        {
          seterror(ERR_TYPEMISMATCH);
        }
      else
        {
          seterror(ERR_SYNTAX);
        }

      return mystrdup("");
    }

  if (!left)
    {
      seterror(ERR_OUTOFMEMORY);
      return 0;
    }

  switch (g_token)
    {
    case PLUS:
      match(PLUS);
      right = stringexpr();
      if (right)
        {
          temp = mystrconcat(left, right);
          free(right);
          if (temp)
            {
              free(left);
              left = temp;
            }
          else
            {
              seterror(ERR_OUTOFMEMORY);
            }
        }
      else
        {
          seterror(ERR_OUTOFMEMORY);
        }

      break;

    default:
      return left;
    }

  return left;
}

/****************************************************************************
 * Name: chrstring
 *
 * Description:
 *   Parse the CHR$ token
 *
 ****************************************************************************/

static FAR char *chrstring(void)
{
  double x;
  FAR char *answer;

  match(CHRSTRING);
  match(OPAREN);
  x = integer(expr());
  match(CPAREN);

  g_iobuffer[0] = (char)x;
  g_iobuffer[1] = 0;
  answer = mystrdup(g_iobuffer);

  if (!answer)
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return answer;
}

/****************************************************************************
 * Name: strstring
 *
 * Description:
 *   Parse the STR$ token
 *
 ****************************************************************************/

static FAR char *strstring(void)
{
  double x;
  FAR char *answer;

  match(STRSTRING);
  match(OPAREN);
  x = expr();
  match(CPAREN);

  sprintf(g_iobuffer, "%g", x);
  answer = mystrdup(g_iobuffer);
  if (!answer)
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return answer;
}

/****************************************************************************
 * Name: leftstring
 *
 * Description:
 *   Parse the LEFT$ token
 *
 ****************************************************************************/

static FAR char *leftstring(void)
{
  FAR char *str;
  int x;
  FAR char *answer;

  match(LEFTSTRING);
  match(OPAREN);
  str = stringexpr();
  if (!str)
    {
      return 0;
    }

  match(COMMA);
  x = integer(expr());
  match(CPAREN);

  if (x > (int)strlen(str))
    {
      return str;
    }

  if (x < 0)
    {
      seterror(ERR_ILLEGALOFFSET);
      return str;
    }

  str[x] = 0;
  answer = mystrdup(str);
  free(str);
  if (!answer)
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return answer;
}

/****************************************************************************
 * Name: rightstring
 *
 * Description:
 *   Parse the RIGHT$ token
 *
 ****************************************************************************/

static FAR char *rightstring(void)
{
  int x;
  FAR char *str;
  FAR char *answer;

  match(RIGHTSTRING);
  match(OPAREN);
  str = stringexpr();
  if (!str)
    {
      return 0;
    }

  match(COMMA);
  x = integer(expr());
  match(CPAREN);

  if (x > (int)strlen(str))
    {
      return str;
    }

  if (x < 0)
    {
      seterror(ERR_ILLEGALOFFSET);
      return str;
    }

  answer = mystrdup(&str[strlen(str) - x]);
  free(str);
  if (!answer)
    {
      seterror(ERR_OUTOFMEMORY);
    }

  return answer;
}

/****************************************************************************
 * Name: midstring
 *
 * Description:
 *   Parse the MID$ token
 *
 ****************************************************************************/

static FAR char *midstring(void)
{
  FAR char *str;
  int x;
  int len;
  FAR char *answer;
  FAR char *temp;

  match(MIDSTRING);
  match(OPAREN);
  str = stringexpr();
  match(COMMA);
  x = integer(expr());
  match(COMMA);
  len = integer(expr());
  match(CPAREN);

  if (!str)
    {
      return 0;
    }

  if (len == -1)
    {
      len = strlen(str) - x + 1;
    }

  if (x > (int)strlen(str) || len < 1)
    {
      free(str);
      answer = mystrdup("");
      if (!answer)
        {
          seterror(ERR_OUTOFMEMORY);
        }

      return answer;
    }

  if (x < 1.0)
    {
      seterror(ERR_ILLEGALOFFSET);
      return str;
    }

  temp = &str[x - 1];

  answer = malloc(len + 1);
  if (!answer)
    {
      seterror(ERR_OUTOFMEMORY);
      return str;
    }

  strncpy(answer, temp, len);
  answer[len] = 0;
  free(str);
  return answer;
}

/****************************************************************************
 * Name: stringstring
 *
 * Description:
 *   Parse the string$ token
 *
 ****************************************************************************/

static FAR char *stringstring(void)
{
  int x;
  FAR char *str;
  FAR char *answer;
  int len;
  int N;
  int i;

  match(STRINGSTRING);
  match(OPAREN);
  x = integer(expr());
  match(COMMA);
  str = stringexpr();
  match(CPAREN);

  if (!str)
    {
      return 0;
    }

  N = x;

  if (N < 1)
    {
      free(str);
      answer = mystrdup("");
      if (!answer)
        {
          seterror(ERR_OUTOFMEMORY);
        }

      return answer;
    }

  len = strlen(str);
  answer = malloc(N * len + 1);
  if (!answer)
    {
      free(str);
      seterror(ERR_OUTOFMEMORY);
      return 0;
    }

  for (i = 0; i < N; i++)
    {
      strcpy(answer + len * i, str);
    }

  free(str);
  return answer;
}

/****************************************************************************
 * Name: stringdimvar
 *
 * Description:
 *   Read a dimensioned string variable from input.
 *   Returns: pointer to string (not malloced)
 *
 ****************************************************************************/

static FAR char *stringdimvar(void)
{
  char id[32];
  int len;
  FAR struct mb_dimvar_s *dimvar;
  FAR char **answer = NULL;
  int index[5];

  getid(g_string, id, &len);
  match(DIMSTRID);
  dimvar = finddimvar(id);

  if (dimvar)
    {
      switch (dimvar->ndims)
        {
        case 1:
          index[0] = integer(expr());
          answer = getdimvar(dimvar, index[0]);
          break;

        case 2:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1]);
          break;

        case 3:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1], index[2]);
          break;

        case 4:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          match(COMMA);
          index[3] = integer(expr());
          answer = getdimvar(dimvar, index[0], index[1], index[2], index[3]);
          break;

        case 5:
          index[0] = integer(expr());
          match(COMMA);
          index[1] = integer(expr());
          match(COMMA);
          index[2] = integer(expr());
          match(COMMA);
          index[3] = integer(expr());
          match(COMMA);
          index[4] = integer(expr());
          answer =
            getdimvar(dimvar, index[0], index[1], index[2], index[3], index[4]);
          break;
        }

      match(CPAREN);
    }
  else
    {
      seterror(ERR_NOSUCHVARIABLE);
    }

  if (!g_errorflag)
    {
      if (answer != NULL && *answer != NULL)
        {
          return *answer;
        }
    }

  return "";
}

/****************************************************************************
 * Name: stringvar
 *
 * Description:
 *   Parse a string variable.
 *   Returns: pointer to string (not malloced)
 *
 ****************************************************************************/

static FAR char *stringvar(void)
{
  char id[32];
  int len;
  FAR struct mb_variable_s *var;

  getid(g_string, id, &len);
  match(STRID);
  var = findvariable(id);
  if (var)
    {
      if (var->sval)
        {
          return var->sval;
        }

      return "";
    }

  seterror(ERR_NOSUCHVARIABLE);
  return "";
}

/****************************************************************************
 * Name: stringliteral
 *
 * Description:
 *   Parse a string literal
 *   Returns: malloced string literal
 *   Notes: newlines aren't allowed in literals, but blind
 *          concatenation across newlines is.
 *
 ****************************************************************************/

static FAR char *stringliteral(void)
{
  int len = 1;
  FAR char *answer = 0;
  FAR char *temp;
  FAR char *substr;
  FAR char *end;

  while (g_token == QUOTE)
    {
      while (isspace(*g_string))
        {
          g_string++;
        }

      end = mystrend(g_string, '"');
      if (end)
        {
          len = end - g_string;
          substr = malloc(len);
          if (!substr)
            {
              seterror(ERR_OUTOFMEMORY);
              return answer;
            }

          mystrgrablit(substr, g_string);
          if (answer)
            {
              temp = mystrconcat(answer, substr);
              free(substr);
              free(answer);
              answer = temp;
              if (!answer)
                {
                  seterror(ERR_OUTOFMEMORY);
                  return answer;
                }
            }
          else
            {
              answer = substr;
            }

          g_string = end;
        }
      else
        {
          seterror(ERR_SYNTAX);
          return answer;
        }

      match(QUOTE);
    }

  return answer;
}

/****************************************************************************
 * Name: integer
 *
 * Description:
 *   Cast a double to an integer, triggering errors if out of range
 *
 ****************************************************************************/

static int integer(double x)
{
  if (x < INT_MIN || x > INT_MAX)
    {
      seterror(ERR_BADVALUE);
    }

  if (x != floor(x))
    {
      seterror(ERR_NOTINT);
    }

  return (int)x;
}

/****************************************************************************
 * Name: match
 *
 * Description:
 *   Check that we have a token of the passed type (if not set the g_errorflag)
 *   Move parser on to next token. Sets token and string.
 *
 ****************************************************************************/

static void match(int tok)
{
  if (g_token != tok)
    {
      seterror(ERR_SYNTAX);
      return;
    }

  while (isspace(*g_string))
    {
      g_string++;
    }

  g_string += tokenlen(g_string, g_token);
  g_token = gettoken(g_string);
  if (g_token == SYNTAX_ERROR)
    {
      seterror(ERR_SYNTAX);
    }
}

/****************************************************************************
 * Name: seterror
 *
 * Description:
 *   Set the errorflag.
 *   Params: errorcode - the error.
 *   Notes: ignores error cascades
 *
 ****************************************************************************/

static void seterror(int errorcode)
{
  if (g_errorflag == 0 || errorcode == 0)
    {
      g_errorflag = errorcode;
    }
}

/****************************************************************************
 * Name: getnextline
 *
 * Description:
 *   Get the next line number
 *   Params: str - pointer to parse string
 *   Returns: line no of next line, 0 if end
 *   Notes: goes to newline, then finds
 *          first line starting with a digit.
 *
 ****************************************************************************/

static int getnextline(FAR const char *str)
{
  while (*str)
    {
      while (*str && *str != '\n')
        {
          str++;
        }

      if (*str == 0)
        {
          return 0;
        }

      str++;
      if (isdigit(*str))
        {
          return atoi(str);
        }
    }

  return 0;
}

/****************************************************************************
 * Name: gettoken
 *
 * Description:
 *   Get a token from the string
 *   Params: str - string to read token from
 *   Notes: ignores white space between tokens
 *
 ****************************************************************************/

static int gettoken(FAR const char *str)
{
  while (isspace(*str))
    {
      str++;
    }

  if (isdigit(*str))
    {
      return VALUE;
    }

  switch (*str)
    {
    case 0:
      return EOS;

    case '\n':
      return EOL;

    case '/':
      return DIV;

    case '*':
      return MULT;

    case '(':
      return OPAREN;

    case ')':
      return CPAREN;

    case '+':
      return PLUS;

    case '-':
      return MINUS;

    case '!':
      return SHRIEK;

    case ',':
      return COMMA;

    case ';':
      return SEMICOLON;

    case '"':
      return QUOTE;

    case '=':
      return EQUALS;

    case '<':
      return LESS;

    case '>':
      return GREATER;

    default:
      if (!strncmp(str, "e", 1) && !isalnum(str[1]))
        {
          return E;
        }

      if (isupper(*str))
        {
          if (!strncmp(str, "SIN", 3) && !isalnum(str[3]))
            {
              return SIN;
            }

          if (!strncmp(str, "COS", 3) && !isalnum(str[3]))
            {
              return COS;
            }

          if (!strncmp(str, "TAN", 3) && !isalnum(str[3]))
            {
              return TAN;
            }

          if (!strncmp(str, "LN", 2) && !isalnum(str[2]))
            {
              return LN;
            }

          if (!strncmp(str, "POW", 3) && !isalnum(str[3]))
            {
              return POW;
            }

          if (!strncmp(str, "PI", 2) && !isalnum(str[2]))
            {
              return PI;
            }

          if (!strncmp(str, "SQRT", 4) && !isalnum(str[4]))
            {
              return SQRT;
            }

          if (!strncmp(str, "PRINT", 5) && !isalnum(str[5]))
            {
              return PRINT;
            }

          if (!strncmp(str, "LET", 3) && !isalnum(str[3]))
            {
              return LET;
            }

          if (!strncmp(str, "DIM", 3) && !isalnum(str[3]))
            {
              return DIM;
            }

          if (!strncmp(str, "IF", 2) && !isalnum(str[2]))
            {
              return IF;
            }

          if (!strncmp(str, "THEN", 4) && !isalnum(str[4]))
            {
              return THEN;
            }

          if (!strncmp(str, "AND", 3) && !isalnum(str[3]))
            {
              return AND;
            }

          if (!strncmp(str, "OR", 2) && !isalnum(str[2]))
            {
              return OR;
            }

          if (!strncmp(str, "GOTO", 4) && !isalnum(str[4]))
            {
              return GOTO;
            }

          if (!strncmp(str, "INPUT", 5) && !isalnum(str[5]))
            {
              return INPUT;
            }

          if (!strncmp(str, "REM", 3) && !isalnum(str[3]))
            {
              return REM;
            }

          if (!strncmp(str, "FOR", 3) && !isalnum(str[3]))
            {
              return FOR;
            }

          if (!strncmp(str, "TO", 2) && !isalnum(str[2]))
            {
              return TO;
            }

          if (!strncmp(str, "NEXT", 4) && !isalnum(str[4]))
            {
              return NEXT;
            }

          if (!strncmp(str, "STEP", 4) && !isalnum(str[4]))
            {
              return STEP;
            }

          if (!strncmp(str, "MOD", 3) && !isalnum(str[3]))
            {
              return MOD;
            }

          if (!strncmp(str, "ABS", 3) && !isalnum(str[3]))
            {
              return ABS;
            }

          if (!strncmp(str, "LEN", 3) && !isalnum(str[3]))
            {
              return LEN;
            }

          if (!strncmp(str, "ASCII", 5) && !isalnum(str[5]))
            {
              return ASCII;
            }

          if (!strncmp(str, "ASIN", 4) && !isalnum(str[4]))
            {
              return ASIN;
            }

          if (!strncmp(str, "ACOS", 4) && !isalnum(str[4]))
            {
              return ACOS;
            }

          if (!strncmp(str, "ATAN", 4) && !isalnum(str[4]))
            {
              return ATAN;
            }

          if (!strncmp(str, "INT", 3) && !isalnum(str[3]))
            {
              return INT;
            }

          if (!strncmp(str, "RND", 3) && !isalnum(str[3]))
            {
              return RND;
            }

          if (!strncmp(str, "VAL", 3) && !isalnum(str[3]))
            {
              return VAL;
            }

          if (!strncmp(str, "VALLEN", 6) && !isalnum(str[6]))
            {
              return VALLEN;
            }

          if (!strncmp(str, "INSTR", 5) && !isalnum(str[5]))
            {
              return INSTR;
            }

          if (!strncmp(str, "CHR$", 4))
            {
              return CHRSTRING;
            }

          if (!strncmp(str, "STR$", 4))
            {
              return STRSTRING;
            }

          if (!strncmp(str, "LEFT$", 5))
            {
              return LEFTSTRING;
            }

          if (!strncmp(str, "RIGHT$", 6))
            {
              return RIGHTSTRING;
            }

          if (!strncmp(str, "MID$", 4))
            {
              return MIDSTRING;
            }

          if (!strncmp(str, "STRING$", 7))
            {
              return STRINGSTRING;
            }
        }

      /* end isupper() */

      if (isalpha(*str))
        {
          while (isalnum(*str))
            {
              str++;
            }

          switch (*str)
            {
            case '$':
              return str[1] == '(' ? DIMSTRID : STRID;

            case '(':
              return DIMFLTID;

            default:
              return FLTID;
            }
        }

      return SYNTAX_ERROR;
    }
}

/****************************************************************************
 * Name: tokenlen
 *
 * Description:
 *   Get the length of a token.
 *   Params: str - pointer to the string containing the token
 *           token - the type of the token read
 *   Returns: length of the token, or 0 for EOL to prevent
 *            it being read past.
 *
 ****************************************************************************/

static int tokenlen(FAR const char *str, int tokenid)
{
  int len = 0;

  switch (tokenid)
    {
    case EOS:
      return 0;

    case EOL:
      return 1;

    case VALUE:
      getvalue(str, &len);
      return len;

    case DIMSTRID:
    case DIMFLTID:
    case STRID:
      getid(str, g_iobuffer, &len);
      return len;

    case FLTID:
      getid(str, g_iobuffer, &len);
      return len;

    case PI:
      return 2;

    case E:
      return 1;

    case SIN:
      return 3;

    case COS:
      return 3;

    case TAN:
      return 3;

    case LN:
      return 2;

    case POW:
      return 3;

    case SQRT:
      return 4;

    case DIV:
      return 1;

    case MULT:
      return 1;

    case OPAREN:
      return 1;

    case CPAREN:
      return 1;

    case PLUS:
      return 1;

    case MINUS:
      return 1;

    case SHRIEK:
      return 1;

    case COMMA:
      return 1;

    case QUOTE:
      return 1;

    case EQUALS:
      return 1;

    case LESS:
      return 1;

    case GREATER:
      return 1;

    case SEMICOLON:
      return 1;

    case SYNTAX_ERROR:
      return 0;

    case PRINT:
      return 5;

    case LET:
      return 3;

    case DIM:
      return 3;

    case IF:
      return 2;

    case THEN:
      return 4;

    case AND:
      return 3;

    case OR:
      return 2;

    case GOTO:
      return 4;

    case INPUT:
      return 5;

    case REM:
      return 3;

    case FOR:
      return 3;

    case TO:
      return 2;

    case NEXT:
      return 4;

    case STEP:
      return 4;

    case MOD:
      return 3;

    case ABS:
      return 3;

    case LEN:
      return 3;

    case ASCII:
      return 5;

    case ASIN:
      return 4;

    case ACOS:
      return 4;

    case ATAN:
      return 4;

    case INT:
      return 3;

    case RND:
      return 3;

    case VAL:
      return 3;

    case VALLEN:
      return 6;

    case INSTR:
      return 5;

    case CHRSTRING:
      return 4;

    case STRSTRING:
      return 4;

    case LEFTSTRING:
      return 5;

    case RIGHTSTRING:
      return 6;

    case MIDSTRING:
      return 4;

    case STRINGSTRING:
      return 7;

    default:
      assert(0);
      return 0;
    }
}

/****************************************************************************
 * Name: isstring
 *
 * Description:
 *   Test if a token represents a string expression
 *   Params: token - token to test
 *   Returns: 1 if a string, else 0
 *
 ****************************************************************************/

static int isstring(int tokenid)
{
  if (tokenid == STRID || tokenid == QUOTE || tokenid == DIMSTRID
      || tokenid == CHRSTRING || tokenid == STRSTRING
      || tokenid == LEFTSTRING || tokenid == RIGHTSTRING
      || tokenid == MIDSTRING || tokenid == STRINGSTRING)
    {
      return 1;
    }

  return 0;
}

/****************************************************************************
 * Name: getvalue
 *
 * Description:
 *   Get a numerical value from the parse string
 *   Params: str - the string to search
 *           len - return pinter for no chars read
 *   Returns: the value of the string.
 *
 ****************************************************************************/

static double getvalue(FAR const char *str, FAR int *len)
{
  double answer;
  FAR char *end;

  answer = strtod(str, &end);
  assert(end != str);
  *len = end - str;
  return answer;
}

/****************************************************************************
 * Name: getid
 *
 * Description:
 *   Get an id from the parse string:
 *   Params: str - string to search
 *           out - id output [32 chars max ]
 *         len - return pointer for id length
 *   Notes: triggers an error if id > 31 chars
 *          the id includes the $ and ( qualifiers.
 *
 ****************************************************************************/

static void getid(FAR const char *str, FAR char *out, FAR int *len)
{
  int nread = 0;
  while (isspace(*str))
    {
      str++;
    }

  assert(isalpha(*str));
  while (isalnum(*str))
    {
      if (nread < 31)
        {
          out[nread++] = *str++;
        }
      else
        {
          seterror(ERR_IDTOOLONG);
          break;
        }
    }

  if (*str == '$')
    {
      if (nread < 31)
        {
          out[nread++] = *str++;
        }
      else
        {
          seterror(ERR_IDTOOLONG);
        }
    }

  if (*str == '(')
    {
      if (nread < 31)
        {
          out[nread++] = *str++;
        }
      else
        {
          seterror(ERR_IDTOOLONG);
        }
    }

  out[nread] = 0;
  *len = nread;
}

/****************************************************************************
 * Name: mystrgrablit
 *
 * Description:
 *   Grab a literal from the parse string.
 *   Params: dest - destination string
 *           src - source string
 *   Notes: strings are in quotes, double quotes the escape
 *
 ****************************************************************************/

static void mystrgrablit(FAR char *dest, FAR const char *src)
{
  assert(*src == '"');
  src++;

  while (*src)
    {
      if (*src == '"')
        {
          if (src[1] == '"')
            {
              *dest++ = *src;
              src++;
              src++;
            }
          else
            {
              break;
            }
        }
      else
        {
          *dest++ = *src++;
        }
    }

  *dest++ = 0;
}

/****************************************************************************
 * Name: mystrend
 *
 * Description:
 *   Find where a source string literal ends
 *   Params: src - string to check (must point to quote)
 *           quote - character to use for quotation
 *   Returns: pointer to quote which ends string
 *   Notes: quotes escape quotes
 *
 ****************************************************************************/

static FAR char *mystrend(FAR const char *str, char quote)
{
  assert(*str == quote);
  str++;

  while (*str)
    {
      while (*str != quote)
        {
          if (*str == '\n' || *str == 0)
            return 0;
          str++;
        }

      if (str[1] == quote)
        {
          str += 2;
        }
      else
        {
          break;
        }
    }

  return (char *)(*str ? str : 0);
}

/****************************************************************************
 * Name: mystrcount
 *
 * Description:
 *   Count the instances of ch in str
 *   Params: str - string to check
 *           ch - character to count
 *   Returns: no time chs occurs in str.
 *
 ****************************************************************************/

static int mystrcount(FAR const char *str, char ch)
{
  int answer = 0;

  while (*str)
    {
      if (*str++ == ch)
        {
          answer++;
        }
    }

  return answer;
}

/****************************************************************************
 * Name: mystrdup
 *
 * Description:
 *   Duplicate a string:
 *   Params: str - string to duplicate
 *   Returns: malloced duplicate.
 *
 ****************************************************************************/

static FAR char *mystrdup(FAR const char *str)
{
  FAR char *answer;

  answer = malloc(strlen(str) + 1);
  if (answer)
    {
      strcpy(answer, str);
    }

  return answer;
}

/****************************************************************************
 * Name: mystrconcat
 *
 * Description:
 *   Concatenate two strings
 *   Params: str - firsts string
 *           cat - second string
 *   Returns: malloced string.
 *
 ****************************************************************************/

static FAR char *mystrconcat(FAR const char *str, FAR const char *cat)
{
  int len;
  FAR char *answer;

  len = strlen(str) + strlen(cat);
  answer = malloc(len + 1);
  if (answer)
    {
      strcpy(answer, str);
      strcat(answer, cat);
    }

  return answer;
}

/****************************************************************************
 * Name: factorial
 *
 * Description:
 *   Compute x!
 *
 ****************************************************************************/

static double factorial(double x)
{
  double answer = 1.0;
  double t;

  if (x > 1000.0)
    {
      x = 1000.0;
    }

  for (t = 1; t <= x; t += 1.0)
    {
      answer *= t;
    }

  return answer;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: basic
 *
 * Description:
 *   Interpret a BASIC script
 *
 * Input Parameters:
 *   script - the script to run
 *   in     - input stream
 *   out    - output stream
 *   err    - error stream
 *
 * Returned Value:
 *   Returns: 0 on success, 1 on error condition.
 *
 ****************************************************************************/

int basic(FAR const char *script, FILE * in, FILE * out, FILE * err)
{
  int curline = 0;
  int nextline;
  int answer = 0;

  g_fpin = in;
  g_fpout = out;
  g_fperr = err;

  if (setup(script) == -1)
    {
      return 1;
    }

  while (curline != -1)
    {
      g_string = g_lines[curline].str;
      g_token = gettoken(g_string);
      g_errorflag = 0;

      nextline = line();
      if (g_errorflag)
        {
          reporterror(g_lines[curline].no);
          answer = 1;
          break;
        }

      if (nextline == -1)
        {
          break;
        }

      if (nextline == 0)
        {
          curline++;
          if (curline == nlines)
            break;
        }
      else
        {
          curline = findline(nextline);
          if (curline == -1)
            {
              if (g_fperr)
                {
                  fprintf(g_fperr, "line %d not found\n", nextline);
                }
              answer = 1;
              break;
            }
        }
    }

  cleanup();
  return answer;
}
