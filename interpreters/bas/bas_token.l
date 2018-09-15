/* Tokens and token sequence arrays. */
%{
#include <nuttx/config.h>

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "bas_auto.h"
#include "bas_token.h"
#include "bas_statement.h"

static int g_matchdata;
static int g_backslash_colon;
static int g_uppercase;
int yylex(void);
static struct Token *g_cur;

static void string(const char *text)
{
  if (g_cur)
  {
    const char *t;
    char *q;
    size_t l;

    for (t=text+1,l=0; *(t+1); ++t,++l)
    {
      if (*t=='"') ++t;
    }
    g_cur->u.string=malloc(sizeof(struct String));
    String_size(String_new(g_cur->u.string),l);
    for (t=text+1,q=g_cur->u.string->character; *(t+1); ++t,++q)
    {
      *q=*t;
      if (*t=='"') ++t;
    }
  }
}

static void string2(void)
{
  if (g_cur)
  {
    char *t,*q;
    size_t l;

    for (t=yytext+1,l=0; *t; ++t,++l)
    {
      if (*t=='"') ++t;
    }
    g_cur->u.string=malloc(sizeof(struct String));
    String_size(String_new(g_cur->u.string),l);
    for (t=yytext+1,q=g_cur->u.string->character; *t; ++t,++q)
    {
      *q=*t;
      if (*t=='"') ++t;
    }
  }
}

%}
 /* flex options and definitions */
%option noyywrap
%option nounput
%x DATAINPUT ELSEIF IMAGEFMT
REAL		([0-9]+("!"|"#"))|([0-9]+\.[0-9]*(e("+"|"-")?[0-9]+)?("!"|"#")?)|([0-9]*\.[0-9]+(e("+"|"-")?[0-9]+)?("!"|"#")?|([0-9]+e("+"|"-")?[0-9]+("!"|"#")?))
INTEGER		[0-9]+%?
HEXINTEGER	&H[0-9A-F]+
OCTINTEGER	&O[0-7]+
IDENTIFIER      ("fn"[ \t]+)?[A-Z][A-Z_0-9\.]*("$"|"%"|"#")?
STRING          \"([^"]|\"\")*\"
STRING2		\"([^"]|\"\")*$
REM		rem([^0-9A-Z_\.\n][^\n]*)?
QUOTE		("'"|"!")[^\n]*
ENDIF           end[ \t]*if
ENDPROC         end[ \t]*proc
ENDSELECT	end[ \t]*select
DOUNTIL		do[ \t]+until
DOWHILE		do[ \t]+while
EXITDO		exit[ \t]+do
EXITFOR		exit[ \t]+for
LINEINPUT	(line[ \t]+input)|linput
LOOPUNTIL	loop[ \t]+until
DATAITEM	[^ \t\n,:][^,:\n]*
ONERROR		on[ \t]+error
ONERROROFF	on[ \t]+error[ \t]+off
ONERRORGOTO0	on[ \t]+error[ \t]+goto[ \t]+0
SELECTCASE	select[ \t]+case

%%
 /* flex rules */
  if (g_matchdata) BEGIN(DATAINPUT);

"#"			return T_CHANNEL;
{REAL}          	{
			  int overflow;
			  double d;

                	  d=Value_vald(yytext,(char**)0,&overflow);
                	  if (overflow)
			  {
			    if (g_cur) g_cur->u.junk=yytext[0];
			    yyless(1);
			    return T_JUNK;
			  }
			  if (g_cur) g_cur->u.real=d;
                	  return T_REAL;
                	}
{INTEGER}       	{
			  int overflow;
			  long int n;

                	  n=Value_vali(yytext,(char**)0,&overflow);
			  if (overflow)
			  {
			    double d;

			    d=Value_vald(yytext,(char**)0,&overflow);
			    if (overflow)
			    {
			      if (g_cur) g_cur->u.junk=yytext[0];
			      yyless(1);
			      return T_JUNK;
			    }
			    if (g_cur) g_cur->u.real=d;
			    return T_REAL;
			  }
                	  if (g_cur) g_cur->u.integer=n;
                  	  return T_INTEGER;
                	}
{HEXINTEGER}    	{
			  int overflow;
			  long int n;

			  n=Value_vali(yytext,(char**)0,&overflow);
                	  if (overflow)
                	  {
                	    if (g_cur) g_cur->u.junk=yytext[0];
                	    yyless(1);
                            return T_JUNK;
                	  }
                	  if (g_cur) g_cur->u.hexinteger=n;
                	  return T_HEXINTEGER;
                	}
{OCTINTEGER}    	{
			  int overflow;
			  long int n;

                	  n=Value_vali(yytext,(char**)0,&overflow);
                	  if (overflow)
                	  {
                	    if (g_cur) g_cur->u.junk=yytext[0];
                	    yyless(1);
                            return T_JUNK;
                	  }
                	  if (g_cur) g_cur->u.octinteger=n;
                	  return T_OCTINTEGER;
                	}
{STRING}        	string(yytext); return T_STRING;
{STRING2}       	string2(); return T_STRING;
"("|"["             	return T_OP;
")"|"]"             	return T_CP;
"*"             	return T_MULT;
"+"             	return T_PLUS;
"-"             	return T_MINUS;
","             	return T_COMMA;
"/"             	return T_DIV;
"\\"			{
                          if (g_backslash_colon)
                          {
                            if (g_cur) g_cur->statement=stmt_COLON_EOL;
                            return T_COLON;
                          }
                          return T_IDIV;
                        }
":"             	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_COLON_EOL;
                          }
                          return T_COLON;
                        }
";"             	return T_SEMICOLON;
"<"             	return T_LT;
"<="            	return T_LE;
"=<"            	return T_LE;
"<>"|"><"            	return T_NE;
"="             	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EQ_FNRETURN_FNEND;
                          }
                          return T_EQ;
                        }
">"             	return T_GT;
">="            	return T_GE;
"=>"            	return T_GE;
"^"             	return T_POW;
"access"[ \t]+"read"    return T_ACCESS_READ;
"access"[ \t]+"read"[ \t]+"write"    return T_ACCESS_READ_WRITE;
"access"[ \t]+"write"   return T_ACCESS_WRITE;
"and"			return T_AND;
"as"			return T_AS;
"call"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CALL;
                          }
                          return T_CALL;
                        }
"case"[ \t]+"else"	{
			  if (g_cur)
			  {
			    g_cur->statement=stmt_CASE;
			    g_cur->u.casevalue=malloc(sizeof(struct Casevalue));
			  }
			  return T_CASEELSE;
			}
"case"			{
			  if (g_cur)
			  {
			    g_cur->statement=stmt_CASE;
			    g_cur->u.casevalue=malloc(sizeof(struct Casevalue));
			  }
			  return T_CASEVALUE;
			}
"chdir"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CHDIR_MKDIR;
                          }
                          return T_CHDIR;
                        }
"clear"         	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CLEAR;
                          }
                          return T_CLEAR;
                        }
"close"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CLOSE;
                          }
                          return T_CLOSE;
                        }
"close"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CLOSE;
                          }
                          return T_CLOSE;
                        }
"cls"|"home"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CLS;
                          }
                          return T_CLS;
                        }
"color"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_COLOR;
                          }
                          return T_COLOR;
                        }
"con"			return T_CON;
"copy"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_COPY_RENAME;
                          }
                          return T_COPY;
                        }
"data"|"d."		{
                          BEGIN(DATAINPUT);
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DATA;
                          }
                          return T_DATA;
                        }
<DATAINPUT>{STRING}	string(yytext); return T_STRING;
<DATAINPUT>{STRING2}	string2(); return T_STRING;
<DATAINPUT>","		return T_COMMA;
<DATAINPUT>{DATAITEM}	{
	                  if (g_cur) g_cur->u.datainput=strcpy(malloc(strlen(yytext)+1),yytext);
			  return T_DATAINPUT;
			}
<DATAINPUT>[ \t]+
<DATAINPUT>\n		BEGIN(INITIAL);
<DATAINPUT>:		BEGIN(INITIAL); return T_COLON;
"dec"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DEC_INC;
                          }
                          return T_DEC;
                        }
"defdbl"        	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DEFINT_DEFDBL_DEFSTR;
                          }
                          return T_DEFDBL;
                        }
"defint"        	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DEFINT_DEFDBL_DEFSTR;
                          }
                          return T_DEFINT;
                        }
"defstr"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DEFINT_DEFDBL_DEFSTR;
                          }
                          return T_DEFSTR;
                        }
"def"/[ \t]+fn[ \t]*[A-Z_0-9\.] {
	                  if (g_cur)
	                  {
	                    g_cur->statement=stmt_DEFFN_DEFPROC_FUNCTION_SUB;
	                    g_cur->u.localSyms=(struct Symbol*)0;
	                  }
	                  return T_DEFFN;
	                }
"def"/[ \t]+proc[A-Z_0-9\.] {
	                  if (g_cur)
	                  {
	                    g_cur->statement=stmt_DEFFN_DEFPROC_FUNCTION_SUB;
	                    g_cur->u.localSyms=(struct Symbol*)0;
	                  }
	                  return T_DEFPROC;
	                }
"delete"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DELETE;
                          }
                          return T_DELETE;
                        }
"dim"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DIM;
                          }
                          return T_DIM;
                        }
"display"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DISPLAY;
                          }
                          return T_DISPLAY;
                        }
"do"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DO;
                          }
                          return T_DO;
                        }
{DOUNTIL}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DOcondition;
                          }
                          return T_DOUNTIL;
                        }
{DOWHILE}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DOcondition;
                          }
                          return T_DOWHILE;
                        }
"edit"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EDIT;
                          }
                          return T_EDIT;
                        }
"else"|"el."		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ELSE_ELSEIFELSE;
                          }
                          return T_ELSE;
                        }
"else"/"if"		{
                          BEGIN(ELSEIF);
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ELSE_ELSEIFELSE;
                          }
                          return T_ELSEIFELSE;
                        }
<ELSEIF>"if"		{
                          BEGIN(INITIAL);
                          if (g_cur)
                          {
                            g_cur->statement=stmt_IF_ELSEIFIF;
                          }
                          return T_ELSEIFIF;
                        }
end[ \t]+function	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDFN;
                          }
                          return T_ENDFN;
                        }
{ENDIF}         	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDIF;
                          }
                          return T_ENDIF;
                        }
{ENDPROC}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDPROC_SUBEND;
                          }
                          return T_ENDPROC;
                        }
{ENDSELECT}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDSELECT;
                          }
                          return T_ENDSELECT;
                        }
"end"[ \t]*"sub"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDPROC_SUBEND;
                          }
                          return T_SUBEND;
                        }
"end"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_END;
                          }
                          return T_END;
                        }
"environ"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENVIRON;
                          }
                          return T_ENVIRON;
                        }
"erase"         	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ERASE;
                          }
                          return T_ERASE;
                        }
"eqv"			return T_EQV;
{EXITDO}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EXITDO;
                          }
                          return T_EXITDO;
                        }
{EXITFOR}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EXITFOR;
                          }
                          return T_EXITFOR;
                        }
"exit"[ \t]+"function"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_FNEXIT;
                          }
                          return T_FNEXIT;
                        }
"exit"[ \t]+"sub"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SUBEXIT;
                          }
                          return T_SUBEXIT;
                        }
"field"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_FIELD;
                          }
                          return T_FIELD;
                        }
"field"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_FIELD;
                          }
                          return T_FIELD;
                        }
"fnend"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EQ_FNRETURN_FNEND;
                          }
                          return T_FNEND;
                        }
"fnreturn"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_EQ_FNRETURN_FNEND;
                          }
                          return T_FNRETURN;
                        }
"for"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_FOR;
                          }
                          return T_FOR;
                        }
"for"[ \t]+"input"      return T_FOR_INPUT;
"for"[ \t]+"output"     return T_FOR_OUTPUT;
"for"[ \t]+"append"     return T_FOR_APPEND;
"for"[ \t]+"random"     return T_FOR_RANDOM;
"for"[ \t]+"binary"     return T_FOR_BINARY;
"function"		{
	                  if (g_cur)
	                  {
	                    g_cur->statement=stmt_DEFFN_DEFPROC_FUNCTION_SUB;
	                    g_cur->u.localSyms=(struct Symbol*)0;
	                  }
	                  return T_FUNCTION;
	                }
"get"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_GET_PUT;
                          }
                          return T_GET;
                        }
"get"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_GET_PUT;
                          }
                          return T_GET;
                        }
"go"[ \t]*"sub"        	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_GOSUB;
                          }
                          return T_GOSUB;
                        }
"go"[ \t]*"to"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RESUME_GOTO;
                          }
                          return T_GOTO;
                        }
"idn"			return T_IDN;
"if"            	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_IF_ELSEIFIF;
                          }
                          return T_IF;
                        }
"image"[ \t]*/[^"\n \t]	{
			  BEGIN(IMAGEFMT);
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_IMAGE;
                	  }
                	  return T_IMAGE;
			}
<IMAGEFMT>.*$		{
			  BEGIN(INITIAL);
                	  if (g_cur)
                	  {
                            size_t l;

                	    l=strlen(yytext);
                            g_cur->u.string=malloc(sizeof(struct String));
                            String_size(String_new(g_cur->u.string),l);
                            memcpy(g_cur->u.string->character,yytext,l);
                	  }
                	  return T_STRING;
                	}
"image"			{
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_IMAGE;
                	  }
                	  return T_IMAGE;
                	}
"imp"			return T_IMP;
"inc"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_DEC_INC;
                          }
                          return T_INC;
                        }
"input"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_INPUT;
                          }
                          return T_INPUT;
                        }
"input"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_INPUT;
                          }
                          return T_INPUT;
                        }
"inv"			return T_INV;
"is"			return T_IS;
"kill"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_KILL;
                          }
                          return T_KILL;
                        }
"let"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LET;
                          }
                          return T_LET;
                        }
"list"          	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LIST_LLIST;
                          }
                          return T_LIST;
                        }
"llist"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LIST_LLIST;
                          }
                          return T_LLIST;
                        }
"load"          	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOAD;
                          }
                          return T_LOAD;
                        }
"local"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOCAL;
                          }
                          return T_LOCAL;
                        }
"locate"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOCATE;
                          }
                          return T_LOCATE;
                        }
"lock"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOCK_UNLOCK;
                          }
                          return T_LOCK;
                        }
"lock"[ \t]+"read"	return T_LOCK_READ;
"lock"[ \t]+"write"	return T_LOCK_WRITE;
"loop"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOOP;
                          }
                          return T_LOOP;
                        }
{LOOPUNTIL}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOOPUNTIL;
                          }
                          return T_LOOPUNTIL;
                        }
"lprint"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_PRINT_LPRINT;
                          }
                          return T_LPRINT;
                        }
"lset"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LSET_RSET;
                          }
                          return T_LSET;
                        }
"mat"[ \t]+"input"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MATINPUT;
                          }
                          return T_MATINPUT;
                        }
"mat"[ \t]+"print"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MATPRINT;
                          }
                          return T_MATPRINT;
                        }
"mat"[ \t]+"read"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MATREAD;
                          }
                          return T_MATREAD;
                        }
"mat"[ \t]+"redim"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MATREDIM;
                          }
                          return T_MATREDIM;
                        }
"mat"[ \t]+"write"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MATWRITE;
                          }
                          return T_MATWRITE;
                        }
"mat"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_MAT;
                          }
                          return T_MAT;
                        }
"mkdir"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_CHDIR_MKDIR;
                          }
                          return T_MKDIR;
                        }
"mod"           	return T_MOD;
"new"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_NEW;
                          }
                          return T_NEW;
                        }
"name"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_NAME;
                          }
                          return T_NAME;
                        }
"next"          	{
                  	  if (g_cur)
                  	  {
                            g_cur->statement=stmt_NEXT;
                  	    g_cur->u.next=malloc(sizeof(struct Next));
                  	  }
			  return T_NEXT;
                	}
"not"			return T_NOT;
{ONERROROFF}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ONERROROFF;
                          }
                          return T_ONERROROFF;
                        }
{ONERRORGOTO0}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ONERRORGOTO0;
                          }
                          return T_ONERRORGOTO0;
                        }
{ONERROR}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ONERROR;
                          }
                          return T_ONERROR;
                        }
"on"			{
			  if (g_cur)
			  {
			    g_cur->statement=stmt_ON;
			    g_cur->u.on.pcLength=1;
			    g_cur->u.on.pc=(struct Pc*)0;
			  }
			  return T_ON;
			}
"open"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OPEN;
                          }
                          return T_OPEN;
                        }
"option"[ \t]+"base"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OPTIONBASE;
                          }
                          return T_OPTIONBASE;
                        }
"option"[ \t]+"run"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OPTIONRUN;
                          }
                          return T_OPTIONRUN;
                        }
"option"[ \t]+"stop"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OPTIONSTOP;
                          }
                          return T_OPTIONSTOP;
                        }
"or"			return T_OR;
"out"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OUT_POKE;
                          }
                          return T_OUT;
                        }
"print"|"p."|"?"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_PRINT_LPRINT;
                          }
                          return T_PRINT;
                        }
("print"|"p."|"?")/"#"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_PRINT_LPRINT;
                          }
                          return T_PRINT;
                        }
"poke"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_OUT_POKE;
                          }
                          return T_POKE;
                        }
"put"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_GET_PUT;
                          }
                          return T_PUT;
                        }
"put"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_GET_PUT;
                          }
                          return T_PUT;
                        }
"randomize"     	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RANDOMIZE;
                          }
                          return T_RANDOMIZE;
                        }
"read"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_READ;
                          }
                          return T_READ;
                        }
"renum"|"ren."		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RENUM;
                          }
                          return T_RENUM;
                        }
"repeat"|"rep."		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_REPEAT;
                          }
                          return T_REPEAT;
                        }
"restore"|"res."	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RESTORE;
                          }
                          return T_RESTORE;
                        }
"resume"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RESUME_GOTO;
                          }
                          return T_RESUME;
                        }
"return"|"r."   	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RETURN;
                          }
                          return T_RETURN;
                        }
"rset"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LSET_RSET;
                          }
                          return T_RSET;
                        }
"run"           	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_RUN;
                          }
                          return T_RUN;
                        }
"save"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SAVE;
                          }
                          return T_SAVE;
                        }
{SELECTCASE}		{
			  if (g_cur)
			  {
			    g_cur->statement=stmt_SELECTCASE;
			    g_cur->u.selectcase=malloc(sizeof(struct Selectcase));
			  }
			  return T_SELECTCASE;
			}
"shared"		return T_SHARED;
"shell"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SHELL;
                          }
                          return T_SHELL;
                        }
"sleep"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SLEEP;
                          }
                          return T_SLEEP;
                        }
"spc"			return T_SPC;
"step"          	return T_STEP;
"stop"          	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_STOP;
                          }
                          return T_STOP;
                        }
"sub"[ \t]*"end"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ENDPROC_SUBEND;
                          }
                          return T_SUBEND;
                        }
"sub"[ \t]*"exit"	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SUBEXIT;
                          }
                          return T_SUBEXIT;
                        }
"sub"			{
	                  if (g_cur)
	                  {
	                    g_cur->statement=stmt_DEFFN_DEFPROC_FUNCTION_SUB;
	                    g_cur->u.localSyms=(struct Symbol*)0;
	                  }
	                  return T_SUB;
	                }
"swap"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SWAP;
                          }
                          return T_SWAP;
                        }
"system"|"bye"        	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_SYSTEM;
                          }
                          return T_SYSTEM;
                        }
"then"|"th."		return T_THEN;
"tab"			return T_TAB;
"to"            	return T_TO;
"trn"			return T_TRN;
"troff"         	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_TROFF;
                          }
                          return T_TROFF;
                        }
"tron"          	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_TRON;
                          }
                          return T_TRON;
                        }
"truncate"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_TRUNCATE;
                          }
                          return T_TRUNCATE;
                        }
"unlock"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LOCK_UNLOCK;
                          }
                          return T_UNLOCK;
                        }
"unnum"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_UNNUM;
                          }
                          return T_UNNUM;
                        }
"until"         	{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_UNTIL;
                          }
                          return T_UNTIL;
                        }
"using"			return T_USING;
"wait"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_WAIT;
                          }
                          return T_WAIT;
                        }
"wend"          	{
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_WEND;
                	    g_cur->u.whilepc=malloc(sizeof(struct Pc));
                	  }
                	  return T_WEND;
                	}
"while"         	{
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_WHILE;
                	    g_cur->u.afterwend=malloc(sizeof(struct Pc));
                	  }
                	  return T_WHILE;
                	}
"width"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_WIDTH;
                          }
                          return T_WIDTH;
                        }
"width"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_WIDTH;
                          }
                          return T_WIDTH;
                        }
"write"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_WRITE;
                          }
                          return T_WRITE;
                        }
"write"/"#"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_WRITE;
                          }
                          return T_WRITE;
                        }
"xor"			return T_XOR;
"xref"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_XREF;
                          }
                          return T_XREF;
                        }
"zer"			return T_ZER;
"zone"			{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_ZONE;
                          }
                          return T_ZONE;
                        }
{REM}           	{
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_QUOTE_REM;
                	    g_cur->u.rem=strcpy(malloc(strlen(yytext+3)+1),yytext+3);
                	  }
                	  return T_REM;
                	}
"rename"		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_COPY_RENAME;
                          }
                          return T_RENAME;
                        }
{QUOTE}          	{
                	  if (g_cur)
                	  {
                	    g_cur->statement=stmt_QUOTE_REM;
                	    strcpy(g_cur->u.rem=malloc(strlen(yytext+1)+1),yytext+1);
                          }	
                	  return T_QUOTE;
                	}
{LINEINPUT}		{
                          if (g_cur)
                          {
                            g_cur->statement=stmt_LINEINPUT;
                          }
                          return T_LINEINPUT;
                        }
{IDENTIFIER}		{
			  if (g_cur)
			  {
                            size_t len;
                            char *s;
                            int fn;

                            g_cur->statement=stmt_IDENTIFIER;
                            if (tolower(yytext[0])=='f' && tolower(yytext[1])=='n')
                            {
                              for (len=2,s=&yytext[2]; *s==' ' || *s=='\t'; ++s);
                              fn=1;
                            }
                            else
                            {
                              len=0;
                              s=yytext;
                              fn=0;
                            }
                            len+=strlen(s);
			    g_cur->u.identifier=malloc(offsetof(struct Identifier,name)+len+1);
                            if (fn)
                            {
                              memcpy(g_cur->u.identifier->name,yytext,2);
                              strcpy(g_cur->u.identifier->name+2,s);
                            }
                            else
                            {
                              strcpy(g_cur->u.identifier->name,s);
                            }
			    switch (yytext[yyleng-1])
			    {
			      case '$': g_cur->u.identifier->defaultType=V_STRING; break;
			      case '%': g_cur->u.identifier->defaultType=V_INTEGER; break;
			      default: g_cur->u.identifier->defaultType=V_REAL; break;
			    }
			  }
			  return T_IDENTIFIER;
			}
[ \t\n]+
.			{
			  if (g_cur) g_cur->u.junk=yytext[0];
			  return T_JUNK;
			}

%%

int g_token_property[T_LASTTOKEN];

struct Token *Token_newCode(const char *ln)
{
  int l,lasttok,thistok,addNumber=0,sawif;
  struct Token *result;
  YY_BUFFER_STATE buf;

  g_cur=(struct Token*)0;
  buf=yy_scan_string(ln);
  /* determine number of tokens */
  g_matchdata=sawif=0;
  for (lasttok=T_EOL,l=1; (thistok=yylex()); ++l)
  {
    if (l==1 && thistok!=T_INTEGER) { addNumber=1; ++l; }
    if ((lasttok==T_THEN || lasttok==T_ELSE) && thistok==T_INTEGER) ++l;
    if (thistok==T_IF) sawif=1;
    if (thistok==T_THEN) sawif=0;
    if (thistok==T_GOTO && sawif) ++l;
    lasttok=thistok;
  }
  if (l==1) { addNumber=1; ++l; }

  yy_delete_buffer(buf);
  g_cur=result=malloc(sizeof(struct Token)*l);
  if (addNumber)
  {
    g_cur->type=T_UNNUMBERED;
    ++g_cur;
  }
  buf=yy_scan_string(ln);
  lasttok=T_EOL;
  g_matchdata=sawif=0;
  while (g_cur->statement=NULL,(g_cur->type=yylex()))
  {
    if (g_cur->type==T_IF) sawif=1;
    if (g_cur->type==T_THEN) sawif=0;
    if (g_cur->type==T_GOTO && sawif)
    {
      sawif=0;
      *(g_cur+1)=*g_cur;
      g_cur->type=T_THEN;
      lasttok=T_GOTO;
      g_cur+=2;
    }
    else if ((lasttok==T_THEN || lasttok==T_ELSE) && g_cur->type==T_INTEGER)
    {
      *(g_cur+1)=*g_cur;
      g_cur->type=T_GOTO;
      g_cur->statement=stmt_RESUME_GOTO;
      lasttok=T_INTEGER;
      g_cur+=2;
    }
    else
    {
      lasttok=g_cur->type;
      ++g_cur;
    }
  }
  g_cur->type=T_EOL;
  g_cur->statement=stmt_COLON_EOL;
  yy_delete_buffer(buf);
  return result;
}

struct Token *Token_newData(const char *ln)
{
  int l;
  struct Token *result;
  YY_BUFFER_STATE buf;

  g_cur=(struct Token*)0;
  buf=yy_scan_string(ln);
  g_matchdata=1;
  for (l=1; yylex(); ++l);
  yy_delete_buffer(buf);
  g_cur=result=malloc(sizeof(struct Token)*l);
  buf=yy_scan_string(ln);
  g_matchdata=1;
  while (g_cur->statement=NULL,(g_cur->type=yylex())) ++g_cur;
  g_cur->type=T_EOL;
  g_cur->statement=stmt_COLON_EOL;
  yy_delete_buffer(buf);
  return result;
}

void Token_destroy(struct Token *token)
{
  struct Token *r=token;

  do
  {
    switch (r->type)
    {
      case T_ACCESS_READ:       break;
      case T_ACCESS_WRITE:      break;
      case T_AND:               break;
      case T_AS:                break;
      case T_CALL:              break;
      case T_CASEELSE:
      case T_CASEVALUE:         free(r->u.casevalue); break;
      case T_CHANNEL:           break;
      case T_CHDIR:             break;
      case T_CLEAR:             break;
      case T_CLOSE:             break;
      case T_CLS:               break;
      case T_COLON:             break;
      case T_COLOR:             break;
      case T_COMMA:             break;
      case T_CON:               break;
      case T_COPY:              break;
      case T_CP:                break;
      case T_DATA:              break;
      case T_DATAINPUT:         free(r->u.datainput); break;
      case T_DEC:               break;
      case T_DEFFN:             break;
      case T_DEFDBL:            break;
      case T_DEFINT:            break;
      case T_DEFPROC:           break;
      case T_DEFSTR:            break;
      case T_DELETE:            break;
      case T_DIM:               break;
      case T_DISPLAY:           break;
      case T_DIV:               break;
      case T_DO:                break;
      case T_DOUNTIL:           break;
      case T_DOWHILE:           break;
      case T_EDIT:              break;
      case T_ELSE:              break;
      case T_ELSEIFELSE:        break;
      case T_ELSEIFIF:          break;
      case T_END:               break;
      case T_ENDFN:             break;
      case T_ENDIF:             break;
      case T_ENDPROC:           break;
      case T_ENDSELECT:         break;
      case T_ENVIRON:           break;
      case T_EOL:               break;
      case T_EQ:                break;
      case T_EQV:               break;
      case T_ERASE:             break;
      case T_EXITDO:            break;
      case T_EXITFOR:           break;
      case T_FIELD:             break;
      case T_FNEND:             break;
      case T_FNEXIT:            break;
      case T_FNRETURN:          break;
      case T_FOR:               break;
      case T_FOR_INPUT:         break;
      case T_FOR_OUTPUT:        break;
      case T_FOR_APPEND:        break;
      case T_FOR_RANDOM:        break;
      case T_FOR_BINARY:        break;
      case T_FUNCTION:          break;
      case T_GE:                break;
      case T_GET:               break;
      case T_GOSUB:             break;
      case T_GOTO:              break;
      case T_GT:                break;
      case T_HEXINTEGER:        break;
      case T_OCTINTEGER:        break;
      case T_IDENTIFIER:        free(r->u.identifier); break;
      case T_IDIV:              break;
      case T_IDN:               break;
      case T_IF:                break;
      case T_IMAGE:             break;
      case T_IMP:               break;
      case T_INC:               break;
      case T_INPUT:             break;
      case T_INTEGER:           break;
      case T_INV:               break;
      case T_IS:                break;
      case T_JUNK:              break;
      case T_KILL:              break;
      case T_LE:                break;
      case T_LET:               break;
      case T_LINEINPUT:         break;
      case T_LIST:              break;
      case T_LLIST:             break;
      case T_LOAD:              break;
      case T_LOCAL:             break;
      case T_LOCATE:            break;
      case T_LOCK:              break;
      case T_LOCK_READ:         break;
      case T_LOCK_WRITE:        break;
      case T_LOOP:              break;
      case T_LOOPUNTIL:         break;
      case T_LPRINT:            break;
      case T_LSET:              break;
      case T_LT:                break;
      case T_MAT:               break;
      case T_MATINPUT:          break;
      case T_MATPRINT:          break;
      case T_MATREAD:           break;
      case T_MATREDIM:          break;
      case T_MATWRITE:          break;
      case T_MINUS:             break;
      case T_MKDIR:             break;
      case T_MOD:               break;
      case T_MULT:              break;
      case T_NAME:              break;
      case T_NE:                break;
      case T_NEW:               break;
      case T_NEXT:              free(r->u.next); break;
      case T_NOT:               break;
      case T_ON:                if (r->u.on.pc) free(r->u.on.pc); break;
      case T_ONERROR:           break;
      case T_ONERRORGOTO0:      break;
      case T_ONERROROFF:        break;
      case T_OP:                break;
      case T_OPEN:              break;
      case T_OPTIONBASE:        break;
      case T_OPTIONRUN:         break;
      case T_OPTIONSTOP:        break;
      case T_OR:                break;
      case T_OUT:		break;
      case T_PLUS:              break;
      case T_POKE:		break;
      case T_POW:               break;
      case T_PRINT:             break;
      case T_PUT:               break;
      case T_QUOTE:             free(r->u.rem); break;
      case T_RANDOMIZE:         break;
      case T_READ:              break;
      case T_REAL:              break;
      case T_REM:               free(r->u.rem); break;
      case T_RENAME:            break;
      case T_RENUM:             break;
      case T_REPEAT:            break;
      case T_RESTORE:           break;
      case T_RESUME:            break;
      case T_RETURN:            break;
      case T_RSET:              break;
      case T_RUN:               break;
      case T_SAVE:              break;
      case T_SELECTCASE:        free(r->u.selectcase); break;
      case T_SEMICOLON:         break;
      case T_SHARED:            break;
      case T_SHELL:             break;
      case T_SLEEP:             break;
      case T_SPC:               break;
      case T_STEP:              break;
      case T_STOP:              break;
      case T_STRING:            String_destroy(r->u.string); free(r->u.string); break;
      case T_SUB:               break;
      case T_SUBEND:            break;
      case T_SUBEXIT:           break;
      case T_SWAP:              break;
      case T_SYSTEM:            break;
      case T_TAB:               break;
      case T_THEN:              break;
      case T_TO:                break;
      case T_TRN:               break;
      case T_TROFF:             break;
      case T_TRON:              break;
      case T_TRUNCATE:          break;
      case T_UNLOCK:            break;
      case T_UNNUM:             break;
      case T_UNNUMBERED:        break;
      case T_UNTIL:             break;
      case T_USING:             break;
      case T_WAIT:              break;
      case T_WEND:              free(r->u.whilepc); break;
      case T_WHILE:             free(r->u.afterwend); break;
      case T_WIDTH:             break;
      case T_WRITE:             break;
      case T_XOR:               break;
      case T_XREF:              break;
      case T_ZER:               break;
      case T_ZONE:              break;
      default:                  assert(0);
    }
  } while ((r++)->type!=T_EOL);
  free(token);
}

struct String *Token_toString(struct Token *token, struct Token *spaceto, struct String *s, int *indent, int width)
{
  int ns=0,infn=0;
  int thisindent=0,thisnotindent=0,nextindent=0;
  size_t oldlength=s->length;
  struct Token *t;
  static struct
  {
    const char *text;
    char space;
  } table[]=
  {
    /* 0                    */ {(const char*)0,-1},
    /* T_ACCESS_READ        */ {"access read",1},
    /* T_ACCESS_READ_WRITE  */ {"access read write",1},
    /* T_ACCESS_WRITE       */ {"access write",1},
    /* T_AND                */ {"and",1},
    /* T_AS                 */ {"as",1},
    /* T_CALL               */ {"call",1},
    /* T_CASEELSE           */ {"case else",1},
    /* T_CASEVALUE          */ {"case",1},
    /* T_CHANNEL            */ {"#",0},
    /* T_CHDIR              */ {"chdir",1},
    /* T_CLEAR              */ {"clear",1},
    /* T_CLOSE              */ {"close",1},
    /* T_CLS                */ {"cls",1},
    /* T_COLON              */ {":",1},
    /* T_COLOR              */ {"color",1},
    /* T_COMMA              */ {",",0},
    /* T_CON                */ {"con",0},
    /* T_COPY               */ {"copy",1},
    /* T_CP                 */ {")",0},
    /* T_DATA               */ {"data",1},
    /* T_DATAINPUT          */ {(const char*)0,0},
    /* T_DEC                */ {"dec",1},
    /* T_DEFDBL             */ {"defdbl",1},
    /* T_DEFFN              */ {"def",1},
    /* T_DEFINT             */ {"defint",1},
    /* T_DEFPROC            */ {"def",1},
    /* T_DEFSTR             */ {"defstr",1},
    /* T_DELETE             */ {"delete",1},
    /* T_DIM                */ {"dim",1},
    /* T_DISPLAY            */ {"display",1},
    /* T_DIV                */ {"/",0},
    /* T_DO                 */ {"do",1},
    /* T_DOUNTIL            */ {"do until",1},
    /* T_DOWHILE            */ {"do while",1},
    /* T_EDIT               */ {"edit",1},
    /* T_ELSE               */ {"else",1},
    /* T_ELSEIFELSE         */ {"elseif",1},
    /* T_ELSEIFIF           */ {(const char*)0,0},
    /* T_END                */ {"end",1},
    /* T_ENDFN              */ {"end function",1},
    /* T_ENDIF              */ {"end if",1},
    /* T_ENDPROC            */ {"end proc",1},
    /* T_ENDSELECT          */ {"end select",1},
    /* T_ENVIRON            */ {"environ",1},
    /* T_EOL                */ {"\n",0},
    /* T_EQ                 */ {"=",0},
    /* T_EQV                */ {"eqv",0},
    /* T_ERASE              */ {"erase",1},
    /* T_EXITDO             */ {"exit do",1},
    /* T_EXITFOR            */ {"exit for",1},
    /* T_FIELD              */ {"field",1},
    /* T_FNEND              */ {"fnend",1},
    /* T_FNEXIT             */ {"exit function",1},
    /* T_FNRETURN           */ {"fnreturn",1},
    /* T_FOR                */ {"for",1},
    /* T_FOR_INPUT          */ {"for input",1},
    /* T_FOR_OUTPUT         */ {"for output",1},
    /* T_FOR_APPEND         */ {"for append",1},
    /* T_FOR_RANDOM         */ {"for random",1},
    /* T_FOR_BINARY         */ {"for binary",1},
    /* T_FUNCTION           */ {"function",1},
    /* T_GE                 */ {">=",0},
    /* T_GET                */ {"get",1},
    /* T_GOSUB              */ {"gosub",1},
    /* T_GOTO               */ {"goto",1},
    /* T_GT                 */ {">",0},
    /* T_HEXINTEGER         */ {(const char*)0,0},
    /* T_OCTINTEGER         */ {(const char*)0,0},
    /* T_IDENTIFIER         */ {(const char*)0,0},
    /* T_IDIV               */ {"\\",0},
    /* T_IDN                */ {"idn",0},
    /* T_IF                 */ {"if",1},
    /* T_IMAGE              */ {"image",1},
    /* T_IMP                */ {"imp",0},
    /* T_INC                */ {"inc",1},
    /* T_INPUT              */ {"input",1},
    /* T_INTEGER            */ {(const char*)0,0},
    /* T_INV                */ {"inv",0},
    /* T_IS                 */ {"is",1},
    /* T_JUNK               */ {(const char*)0,0},
    /* T_KILL               */ {"kill",1},
    /* T_LE                 */ {"<=",0},
    /* T_LET                */ {"let",1},
    /* T_LINEINPUT          */ {"line input",1},
    /* T_LIST               */ {"list",1},
    /* T_LLIST              */ {"llist",1},
    /* T_LOAD               */ {"load",1},
    /* T_LOCAL              */ {"local",1},
    /* T_LOCATE             */ {"locate",1},
    /* T_LOCK               */ {"lock",1},
    /* T_LOCK_READ          */ {"lock read",1},
    /* T_LOCK_WRITE         */ {"lock write",1},
    /* T_LOOP               */ {"loop",1},
    /* T_LOOPUNTIL          */ {"loop until",1},
    /* T_LPRINT             */ {"lprint",1},
    /* T_LSET               */ {"lset",1},
    /* T_LT                 */ {"<",0},
    /* T_MAT                */ {"mat",1},
    /* T_MATINPUT           */ {"mat input",1},
    /* T_MATPRINT           */ {"mat print",1},
    /* T_MATREAD            */ {"mat read",1},
    /* T_MATREDIM           */ {"mat redim",1},
    /* T_MATWRITE           */ {"mat write",1},
    /* T_MINUS              */ {"-",0},
    /* T_MKDIR              */ {"mkdir",1},
    /* T_MOD                */ {"mod",0},
    /* T_MULT               */ {"*",0},
    /* T_NAME               */ {"name",1},
    /* T_NE                 */ {"<>",0},
    /* T_NEW                */ {"new",1},
    /* T_NEXT               */ {"next",1},
    /* T_NOT                */ {"not",0},
    /* T_ON                 */ {"on",1},
    /* T_ONERROR            */ {"on error",1},
    /* T_ONERRORGOTO0       */ {"on error goto 0",1},
    /* T_ONERROROFF         */ {"on error off",1},
    /* T_OP                 */ {"(",0},
    /* T_OPEN               */ {"open",1},
    /* T_OPTIONBASE         */ {"option base",1},
    /* T_OPTIONRUN          */ {"option run",1},
    /* T_OPTIONSTOP         */ {"option stop",1},
    /* T_OR                 */ {"or",1},
    /* T_OUT                */ {"out",1},
    /* T_PLUS               */ {"+",0},
    /* T_POKE               */ {"poke",1},
    /* T_POW                */ {"^",0},
    /* T_PRINT              */ {"print",1},
    /* T_PUT                */ {"put",1},
    /* T_QUOTE              */ {(const char*)0,1},
    /* T_RANDOMIZE          */ {"randomize",1},
    /* T_READ               */ {"read",1},
    /* T_REAL               */ {(const char*)0,0},
    /* T_REM                */ {(const char*)0,1},
    /* T_RENAME             */ {"rename",1},
    /* T_RENUM              */ {"renum",1},
    /* T_REPEAT             */ {"repeat",1},
    /* T_RESTORE            */ {"restore",1},
    /* T_RESUME             */ {"resume",1},
    /* T_RETURN             */ {"return",1},
    /* T_RSET               */ {"rset",1},
    /* T_RUN                */ {"run",1},
    /* T_SAVE               */ {"save",1},
    /* T_SELECTCASE         */ {"select case",1},
    /* T_SEMICOLON          */ {";",0},
    /* T_SHARED             */ {"shared",1},
    /* T_SHELL              */ {"shell",1},
    /* T_SLEEP              */ {"sleep",1},
    /* T_SPC                */ {"spc",0},
    /* T_STEP               */ {"step",1},
    /* T_STOP               */ {"stop",1},
    /* T_STRING             */ {(const char*)0,0},
    /* T_SUB                */ {"sub",1},
    /* T_SUBEND             */ {"subend",1},
    /* T_SUBEXIT            */ {"subexit",1},
    /* T_SWAP               */ {"swap",1},
    /* T_SYSTEM             */ {"system",1},
    /* T_TAB                */ {"tab",0},
    /* T_THEN               */ {"then",1},
    /* T_TO                 */ {"to",1},
    /* T_TRN                */ {"trn",0},
    /* T_TROFF              */ {"troff",1},
    /* T_TRON               */ {"tron",1},
    /* T_TRUNCATE           */ {"truncate",1},
    /* T_UNLOCK             */ {"unlock",1},
    /* T_UNNUM              */ {"unnum",1},
    /* T_UNNUMBERED         */ {"",0},
    /* T_UNTIL              */ {"until",1},
    /* T_USING              */ {"using",0},
    /* T_WAIT               */ {"wait",1},
    /* T_WEND               */ {"wend",1},
    /* T_WHILE              */ {"while",1},
    /* T_WIDTH              */ {"width",1},
    /* T_WRITE              */ {"write",1},
    /* T_XOR                */ {"xor",0},
    /* T_XREF               */ {"xref",0},
    /* T_ZER                */ {"zer",0},
    /* T_ZONE               */ {"zone",1},
  };

  /* precompute indentation */
  if (indent) thisindent=nextindent=*indent;
  t=token;
  do
  {
    switch (t->type)
    {
      case T_CASEELSE:
      case T_CASEVALUE:
      {
        if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        break;
      }
      case T_DEFFN:
      case T_FUNCTION:
      {
        struct Token *cp;

        for (cp=t; cp->type!=T_EOL && cp->type!=T_CP; ++cp);
        if ((cp+1)->type!=T_EQ)
        {
          ++thisnotindent;
          ++nextindent;
        }
        infn=1;
        break;
      }
      case T_COLON: infn=0; break;
      case T_DEFPROC:
      case T_DO:
      case T_DOUNTIL:
      case T_DOWHILE:
      case T_REPEAT:
      case T_SUB:
      case T_WHILE: ++thisnotindent; ++nextindent; break;
      case T_FOR:
      {
        if ((t>token && ((t-1)->type==T_COLON || (t-1)->type==T_INTEGER || (t-1)->type==T_UNNUMBERED)))
        {
          ++thisnotindent; ++nextindent;
        }
        break;
      }
      case T_SELECTCASE: thisnotindent+=2; nextindent+=2; break;
      case T_EQ:
      {
        if (infn || (t>token && ((t-1)->type==T_COLON || (t-1)->type==T_INTEGER || (t-1)->type==T_UNNUMBERED)))
        {
          if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
          if (nextindent) --nextindent;
        }
        infn=0;
        break;
      }
      case T_ENDFN:
      case T_FNEND:
      case T_ENDIF:
      case T_ENDPROC:
      case T_SUBEND:
      case T_LOOP:
      case T_LOOPUNTIL:
      case T_UNTIL:
      case T_WEND:
      {
        if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        if (nextindent) --nextindent;
        break;
      }
      case T_ENDSELECT:
      {
        if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        if (nextindent) --nextindent;
        if (nextindent) --nextindent;
        break;
      }
      case T_NEXT:
      {
        ++t;
        while (1)
        {
          if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
          if (nextindent) --nextindent;
          if (t->type==T_IDENTIFIER)
          {
            ++t;
            if (t->type==T_OP)
            {
              int par=0;

              do
              {
                if (t->type==T_OP) ++par;
                else if (t->type==T_CP) --par;
                if (t->type!=T_EOL) ++t;
                else break;
              } while (par);
            }
            if (t->type==T_COMMA) ++t;
            else break;
          }
          else break;
        }
        break;
      }
      case T_THEN: if ((t+1)->type==T_EOL) { ++thisnotindent; ++nextindent; } break;
      case T_ELSE:
      {
        if (t==token+1)
        {
          if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        }
        break;
      }
      case T_ELSEIFELSE:
      {
        if (t==token+1)
        {
          if (thisnotindent) --thisnotindent; else if (thisindent) --thisindent;
        }
        if (nextindent) --nextindent;
        break;
      }
      default: break;
    }
  } while (t++->type!=T_EOL);

  if (width>=0) /* whole line */
  {
    if (width) /* nicely formatted listing */
    {
      assert (token->type==T_UNNUMBERED || token->type==T_INTEGER);
      if (token->type==T_INTEGER) String_appendPrintf(s,"%*ld ",width,token->u.integer);
      else String_appendPrintf(s,"%*s ",width,"");
    }
    else assert (token->type==T_UNNUMBERED);
    ++token;
  }
  while (thisindent--) String_appendPrintf(s,"  ");
  do
  {
    if (s->length>oldlength && token->type!=T_EOL)
    {
      const char *keyword;

      if ((keyword=table[token->type].text)==(const char*)0) keyword="X";
      if (ns && s->character[s->length-1]!=' ')
      {
        String_appendPrintf(s," ");
      }
      else if (isalnum((int)(s->character[s->length-1])) && isalnum((int)*keyword))
      {
        String_appendPrintf(s," ");
      }
      else if (s->character[s->length-1]!=' ' && table[token->type].space)
      {
        String_appendChar(s,' ');
      }
    }
    if (spaceto && token==spaceto) break;
    switch (token->type)
    {
      case T_DATAINPUT: String_appendChars(s,token->u.datainput); break;
      case T_ELSEIFIF: break;
      case T_IDENTIFIER: String_appendChars(s,token->u.identifier->name); break;
      case T_INTEGER: String_appendPrintf(s,"%ld",token->u.integer); break;
      case T_HEXINTEGER: String_appendPrintf(s,"&h%lx",token->u.hexinteger); break;
      case T_OCTINTEGER: String_appendPrintf(s,"&o%lo",token->u.octinteger); break;
      case T_JUNK: String_appendChar(s,token->u.junk); break;
      case T_REAL:
      {
        String_appendPrintf(s,"%.*g",DBL_DIG,token->u.real);
        if ((token->u.real<((double)LONG_MIN)) || (token->u.real>((double)LONG_MAX))) String_appendChar(s,'!');
        break;
      }
      case T_REM: String_appendPrintf(s,"%s%s",g_uppercase?"REM":"rem",token->u.rem); break;
      case T_QUOTE: String_appendPrintf(s,"'%s",token->u.rem); break;
      case T_STRING:
      {
        size_t l=token->u.string->length;
        char *data=token->u.string->character;

        String_appendPrintf(s,"\"");
        while (l--)
        {
          if (*data=='"') String_appendPrintf(s,"\"");
          String_appendPrintf(s,"%c",*data);
          ++data;
        }
        String_appendPrintf(s,"\"");
        break;
      }

      default:
      {
        if (g_uppercase)
        {
          struct String u;

          String_new(&u);
          String_appendChars(&u,table[token->type].text);
          String_ucase(&u);
          String_appendString(s,&u);
          String_destroy(&u);
        }
        else String_appendChars(s,table[token->type].text);
      }
    }
    ns=table[token->type].space;
  } while (token++->type!=T_EOL);
  if (indent) *indent=nextindent;
  if (spaceto && s->length>oldlength) memset(s->character+oldlength,' ',s->length-oldlength);
  return s;
}

void Token_init(int b_c, int uc)
{
#define PROPERTY(t,assoc,unary_priority,binary_priority,is_unary,is_binary) \
  g_token_property[t]=(assoc<<8)|(unary_priority<<5)|(binary_priority<<2)|(is_unary<<1)|is_binary

  g_backslash_colon=b_c;
  g_uppercase=uc;
  PROPERTY(T_POW,  1,0,7,0,1);
  PROPERTY(T_MULT, 0,0,5,0,1);
  PROPERTY(T_DIV,  0,0,5,0,1);
  PROPERTY(T_IDIV, 0,0,5,0,1);
  PROPERTY(T_MOD,  0,0,5,0,1);
  PROPERTY(T_PLUS, 0,6,4,1,1);
  PROPERTY(T_MINUS,0,6,4,1,1);
  PROPERTY(T_LT,   0,0,3,0,1);
  PROPERTY(T_LE,   0,0,3,0,1);
  PROPERTY(T_EQ,   0,0,3,0,1);
  PROPERTY(T_GE,   0,0,3,0,1);
  PROPERTY(T_GT,   0,0,3,0,1);
  PROPERTY(T_NE,   0,0,3,0,1);
  PROPERTY(T_NOT,  0,2,0,1,0);
  PROPERTY(T_AND,  0,0,1,0,1);
  PROPERTY(T_OR,   0,0,0,0,1);
  PROPERTY(T_XOR,  0,0,0,0,1);
  PROPERTY(T_EQV,  0,0,0,0,1);
  PROPERTY(T_IMP,  0,0,0,0,1);
}

