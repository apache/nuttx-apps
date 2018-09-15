/****************************************************************************
 * apps/interpreters/bas/bas_error.h
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
 * Adapted to NuttX and re-released under a 3-clause BSD license:
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Alan Carvalho de Assis <Alan Carvalho de Assis>
 *            Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __APPS_EXAMPLES_BAS_BAS_ERROR_H
#define __APPS_EXAMPLES_BAS_BAS_ERROR_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

#define STATIC 100

#define ALREADYDECLARED    STATIC+ 0, _("Formal parameter already declared")
#define ALREADYLOCAL       STATIC+ 1, _("Variable already declared as `local'")
#define BADIDENTIFIER      STATIC+ 2, _("Identifier can not be declared as %s")
#define BADRANGE           STATIC+ 3, _("Ranges must be constructed from single letter identifiers")
#define INVALIDLINE        STATIC+ 4, _("Missing line number at the beginning of text line %d")
#define INVALIDUOPERAND    STATIC+ 5, _("Invalid unary operand")
#define INVALIDOPERAND     STATIC+ 6, _("Invalid binary operand")
#define MISSINGAS          STATIC+ 7, _("Missing `as'")
#define MISSINGCOLON       STATIC+ 8, _("Missing colon `:'")
#define MISSINGCOMMA       STATIC+ 9, _("Missing comma `,'")
#define MISSINGCP          STATIC+10, _("Missing right parenthesis `)'")
#define MISSINGDATAINPUT   STATIC+11, _("Missing `data' input")
#define MISSINGDECINCIDENT STATIC+12, _("Missing `dec'/`inc' variable identifier")
#define MISSINGEQ          STATIC+13, _("Missing equal sign `='")
#define MISSINGEXPR        STATIC+14, _("Expected %s expression")
#define MISSINGFILE        STATIC+15, _("Missing `file'")
#define MISSINGGOTOSUB     STATIC+16, _("Missing `goto' or `gosub'")
#define MISSINGVARIDENT    STATIC+17, _("Missing variable identifier")
#define MISSINGPROCIDENT   STATIC+18, _("Missing procedure identifier")
#define MISSINGFUNCIDENT   STATIC+19, _("Missing function identifier")
#define MISSINGARRIDENT    STATIC+20, _("Missing array variable identifier")
#define MISSINGSTRIDENT    STATIC+21, _("Missing string variable identifier")
#define MISSINGLOOPIDENT   STATIC+22, _("Missing loop variable identifier")
#define MISSINGFORMIDENT   STATIC+23, _("Missing formal parameter identifier")
#define MISSINGREADIDENT   STATIC+24, _("Missing `read' variable identifier")
#define MISSINGSWAPIDENT   STATIC+25, _("Missing `swap' variable identifier")
#define MISSINGMATIDENT    STATIC+26, _("Missing matrix variable identifier")
#define MISSINGINCREMENT   STATIC+27, _("Missing line increment")
#define MISSINGLEN         STATIC+28, _("Missing `len'")
#define MISSINGLINENUMBER  STATIC+29, _("Missing line number")
#define MISSINGOP          STATIC+30, _("Missing left parenthesis `('")
#define MISSINGSEMICOLON   STATIC+31, _("Missing semicolon `;'")
#define MISSINGSEMICOMMA   STATIC+32, _("Missing semicolon `;' or comma `,'")
#define MISSINGMULT        STATIC+33, _("Missing star `*'")
#define MISSINGSTATEMENT   STATIC+34, _("Missing statement")
#define MISSINGTHEN        STATIC+35, _("Missing `then'")
#define MISSINGTO          STATIC+36, _("Missing `to'")
#define NESTEDDEFINITION   STATIC+37, _("Nested definition")
#define NOPROGRAM          STATIC+38, _("No program")
#define NOSUCHDATALINE     STATIC+39, _("No such `data' line")
#define NOSUCHLINE         STATIC+40, _("No such line")
#define REDECLARATION      STATIC+41, _("Redeclaration as different kind of symbol")
#define STRAYCASE          STATIC+42, _("`case' without `select case'")
#define STRAYDO            STATIC+43, _("`do' without `loop'")
#define STRAYDOcondition   STATIC+44, _("`do while' or `do until' without `loop'")
#define STRAYELSE1         STATIC+45, _("`else' without `if'")
#define STRAYELSE2         STATIC+46, _("`else' without `end if'")
#define STRAYENDIF         STATIC+47, _("`end if' without multiline `if' or `else'")
#define STRAYSUBEND        STATIC+49, _("`subend', `end sub' or `endproc' without `sub' or `def proc' inside %s")
#define STRAYSUBEXIT       STATIC+50, _("`subexit' without `sub' inside %s")
#define STRAYENDSELECT     STATIC+51, _("`end select' without `select case'")
#define STRAYENDFN         STATIC+52, _("`end function' without `def fn' or `function'")
#define STRAYENDEQ         STATIC+53, _("`=' returning from function without `def fn'")
#define STRAYEXITDO        STATIC+54, _("`exit do' without `do'")
#define STRAYEXITFOR       STATIC+55, _("`exit for' without `for'")
#define STRAYFNEND         STATIC+56, _("`fnend' without `def fn'")
#define STRAYFNEXIT        STATIC+57, _("`exit function' outside function declaration")
#define STRAYFNRETURN      STATIC+58, _("`fnreturn' without `def fn'")
#define STRAYFOR           STATIC+59, _("`for' without `next'")
#define STRAYFUNC          STATIC+60, _("Function/procedure declaration without end")
#define STRAYIF            STATIC+61, _("`if' without `end if'")
#define STRAYLOCAL         STATIC+62, _("`local' without `def fn' or `def proc'")
#define STRAYLOOP          STATIC+63, _("`loop' without `do'")
#define STRAYLOOPUNTIL     STATIC+64, _("`loop until' without `do'")
#define STRAYNEXT          STATIC+65, _("`next' without `for' inside %s")
#define STRAYREPEAT        STATIC+66, _("`repeat' without `until'")
#define STRAYSELECTCASE    STATIC+67, _("`select case' without `end select'")
#define STRAYUNTIL         STATIC+68, _("`until' without `repeat'")
#define STRAYWEND          STATIC+69, _("`wend' without `while' inside %s")
#define STRAYWHILE         STATIC+70, _("`while' without `wend'")
#define SYNTAX             STATIC+71, _("Syntax")
#define TOOFEW             STATIC+72, _("Too few parameters")
#define TOOMANY            STATIC+73, _("Too many parameters")
#define TYPEMISMATCH1      STATIC+74, _("Type mismatch (has %s, need %s)")
#define TYPEMISMATCH2      STATIC+75, _("Type mismatch of argument %d")
#define TYPEMISMATCH3      STATIC+76, _("%s of argument %d")
#define TYPEMISMATCH4      STATIC+77, _("Type mismatch (need string variable)")
#define TYPEMISMATCH5      STATIC+78, _("Type mismatch (need numeric variable)")
#define TYPEMISMATCH6      STATIC+79, _("Type mismatch (need numeric value)")
#define UNDECLARED         STATIC+80, _("Undeclared function or variable")
#define UNNUMBERED         STATIC+81, _("Use `renum' to number program first")
#define OUTOFSCOPE         STATIC+82, _("Line out of scope")
#define VOIDVALUE          STATIC+83, _("Procedures do not return values")
#define UNREACHABLE        STATIC+84, _("Unreachable statement")
#define WRONGMODE          STATIC+85, _("Wrong access mode")
#define FORMISMATCH        STATIC+86, _("`next' variable does not match `for' variable")
#define NOSUCHIMAGELINE    STATIC+87, _("No such `image' line")
#define MISSINGFMT         STATIC+88, _("Missing `image' format")
#define MISSINGRELOP       STATIC+89, _("Missing relational operator")

#define RUNTIME 200

#define MISSINGINPUTDATA   RUNTIME+0, _("Missing `input' data")
#define MISSINGCHARACTER   RUNTIME+1, _("Missing character after underscore `_' in format string")
#define NOTINDIRECTMODE    RUNTIME+2, _("Not allowed in interactive mode")
#define NOTINPROGRAMMODE   RUNTIME+3, _("Not allowed in program mode")
#define BREAK              RUNTIME+4, _("Break")
#define UNDEFINED          RUNTIME+5, _("%s is undefined")
#define OUTOFRANGE         RUNTIME+6, _("%s is out of range")
#define STRAYRESUME        RUNTIME+7, _("`resume' without exception")
#define STRAYRETURN        RUNTIME+8, _("`return' without `gosub'")
#define BADCONVERSION      RUNTIME+9, _("Bad %s conversion")
#define IOERROR            RUNTIME+10,_("Input/Output error (%s)")
#define IOERRORCREATE      RUNTIME+10,_("Input/Output error (Creating `%s' failed: %s)")
#define IOERRORCLOSE       RUNTIME+10,_("Input/Output error (Closing `%s' failed: %s)")
#define IOERROROPEN        RUNTIME+10,_("Input/Output error (Opening `%s' failed: %s)")
#define ENVIRONFAILED      RUNTIME+11,_("Setting environment variable failed (%s)")
#define REDIM              RUNTIME+12,_("Trying to redimension existing array")
#define FORKFAILED         RUNTIME+13,_("Forking child process failed (%s)")
#define BADMODE            RUNTIME+14,_("Invalid mode")
#define ENDOFDATA          RUNTIME+15,_("end of `data'")
#define DIMENSION          RUNTIME+16,_("Dimension mismatch")
#define NOMATRIX           RUNTIME+17,_("Variable dimension must be 2 (is %d), base must be 0 or 1 (is %d)")
#define SINGULAR           RUNTIME+18,_("Singular matrix")
#define BADFORMAT          RUNTIME+19,_("Syntax error in print format")
#define OUTOFMEMORY        RUNTIME+20,_("Out of memory")
#define RESTRICTED         RUNTIME+21,_("Restricted")
#define NOTAVAILABLE       RUNTIME+22,_("Feature not available")

#endif /* __APPS_EXAMPLES_BAS_BAS_ERROR_H */
