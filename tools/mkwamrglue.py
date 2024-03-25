#!/usr/bin/env python3
############################################################################
# apps/tools/mkwamrglue.py
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

import argparse
import os
import sys

NAME_INDEX = 0
HEADER_INDEX = 1
COND_INDEX = 2
RETTYPE_INDEX = 3
PARM1_INDEX = 4

POINTER_SIGNATURE = ['...', '*', '_sa_handler_t', ',mbstate_t']

BLACK_LIST = ['sigqueue', 'pthread_create', 'pthread_detach',
              'pthread_key_create', 'pthread_key_delete', 'pthread_getspecific',
              'pthread_setspecific', 'mallinfo', 'inet_ntoa']

VA_ADDITIONAL_FUNCS = [
    'asprintf', 'fprintf', 'fscanf', 'printf', 'snprintf', 'sprintf',
    'sprintf', 'sscanf', 'sscanf', 'swprintf', 'syslog',
]

VA_LIST_FUNCS = [
    'vasprintf', 'vfprintf', 'vprintf', 'vscanf', 'vsnprintf', 'vsprintf',
    'vsscanf', 'vsyslog',
]


def is_strip_function(name):
  for black in BLACK_LIST:
    if black == name:
      return True

  return False


def is_va_additional_function(name):
  for va in VA_ADDITIONAL_FUNCS:
    if va == name:
      return True

  return False


def is_va_list_function(name):
  for va in VA_LIST_FUNCS:
    if va == name:
      return True

  return False


def generate_include(csv, out):
  headers = []
  for line in csv.readlines():
    sline = line.strip()
    if sline == '':
      break
    args = []
    for arg in sline.split(","):
      args.append(arg.strip().strip('\"'))

    if is_strip_function(args[NAME_INDEX]):
      continue

    if len(args[HEADER_INDEX]) != 0 and args[HEADER_INDEX] not in headers:
      headers.append(args[HEADER_INDEX])

  headers.sort()

  for header in headers:
    out.write("#include <" + header + ">\n")

  out.write("\n")
  out.write("#include <wasm_export.h>\n")
  out.write("\n")
  csv.seek(0)


def generate_functions(csv, out):
  for line in csv.readlines():
    sline = line.strip()
    if sline == '':
      break
    args = []
    for arg in sline.split(","):
      args.append(arg.strip().strip('\"'))

    if is_strip_function(args[NAME_INDEX]):
      continue

    nodef = (len(args[COND_INDEX]) != 0)
    if nodef:
      out.write("#if " + args[COND_INDEX] + "\n\n")

    out.write("#ifndef GLUE_FUNCTION_" + args[NAME_INDEX] + "\n")
    out.write("#define GLUE_FUNCTION_" + args[NAME_INDEX] + "\n")

    noreturn = args[RETTYPE_INDEX] == 'void' or \
        args[RETTYPE_INDEX] == 'noreturn'

    if noreturn:
      out.write("void glue_" + args[NAME_INDEX] + "(wasm_exec_env_t env")
    else:
      out.write("uintptr_t glue_" +
                args[NAME_INDEX] + "(wasm_exec_env_t env")

    if is_va_additional_function(args[NAME_INDEX]):
      findex = len(args) - 2
      vprefix = 'v'
    elif is_va_list_function(args[NAME_INDEX]):
      findex = len(args) - 2
    else:
      findex = 0
      vprefix = ''

    for index in range(PARM1_INDEX, len(args)):
      if findex == index:
        out.write(", uintptr_t format")
      elif args[index] == '...':
        out.write(", va_list ap")
        break
      elif args[index] == 'va_list' and findex != 0:
        out.write(", va_list ap")
        break
      elif args[index] == 'void':
        continue
      else:
        out.write(", uintptr_t parm" + str(index - PARM1_INDEX + 1))

    out.write(")\n{\n")

    addrcov = False
    for index in range(PARM1_INDEX, len(args)):
      if '*' in args[index] or 'va_list' in args[index] or '...' in args[index]:
        addrcov = True

    if addrcov == False and '*' in args[RETTYPE_INDEX]:
      addrcov = True

    out.write("  wasm_module_inst_t module_inst = get_module_inst(env);\n")
    out.write("  uintptr_t ret;\n")

    if findex != 0:
      if 'scanf' in args[NAME_INDEX]:
        out.write("  scanf_begin(module_inst, ap);\n")
      else:
        out.write("  va_list_string2native(env, format, ap);\n")

    if noreturn:
      noret = ''
    else:
      noret = 'return'

    addrcov = ''
    if '*' in args[RETTYPE_INDEX]:
      addrcov = 'addr_native_to_app((uintptr_t)'

    if noreturn:
      out.write("  " + addrcov + str(vprefix) + args[NAME_INDEX] + "(")
    else:
      out.write("  ret = " + addrcov +
                str(vprefix) + args[NAME_INDEX] + "(")

    for index in range(PARM1_INDEX, len(args)):
      if index > PARM1_INDEX:
        out.write(", ")
      if args[index] == '...':
        if index < len(args) - 1 and args[NAME_INDEX] == 'ioctl':
          out.write(
              "(*(uintptr_t **)&ap != NULL && **(uintptr_t **)&ap != (uintptr_t)NULL) ? (uintptr_t)addr_app_to_native((uintptr_t)va_arg(ap, " + args[index + 1] + ")) : (uintptr_t)NULL")
        elif index < len(args) - 1:
          out.write(
              "(*(uintptr_t **)&ap != NULL && **(uintptr_t **)&ap != (uintptr_t)NULL) ? (uintptr_t)va_arg(ap, " + args[index + 1] + ") : (uintptr_t)NULL")
        else:
          out.write("ap")
        break
      if args[index] == 'va_list' and findex != 0:
        out.write("ap")
        break
      if args[index] == 'void':
        continue
      if '|' in args[index]:
        args[index] = args[index].split("|")[1]
      if findex == index:
        out.write("(" + args[index] + ")format")
      else:
        out.write("(" + args[index] + ")parm" +
                  str(index - PARM1_INDEX + 1))

    if addrcov != '':
      out.write(")")

    out.write(");\n")

    if findex != 0:
      if 'scanf' in args[NAME_INDEX]:
        out.write("  scanf_end(module_inst, ap);\n")
      else:
        out.write("  va_list_string2app(env, format, ap);\n")

    if noreturn:
      pass
    else:
      out.write("  return ret;\n")

    out.write("}\n\n")

    out.write("#endif /* GLUE_FUNCTION_" + args[NAME_INDEX] + " */\n")

    if nodef:
      out.write("#endif /* " + args[COND_INDEX] + " */\n\n")
  csv.seek(0)


def generate_table(csv, out):
  out.write("#ifndef native_function\n")
  out.write("#define native_function(func_name, signature) \\\n")
  out.write("  { #func_name, glue_##func_name, signature, NULL }\n\n")
  out.write("#endif\n")

  out.write("static NativeSymbol g_" +
            os.path.splitext(os.path.basename(csv.name))[0] + "_native_symbols[] =\n{\n")

  for line in csv.readlines():
    sline = line.strip()
    if sline == '':
      break
    args = []
    for arg in sline.split(","):
      args.append(arg.strip().strip('\"'))

    if is_strip_function(args[NAME_INDEX]):
      continue

    nodef = (len(args[COND_INDEX]) != 0)
    if nodef:
      out.write("#if " + args[COND_INDEX] + "\n")

    out.write("#ifndef GLUE_ENTRY_" + args[NAME_INDEX] + "\n")
    out.write("#define GLUE_ENTRY_" + args[NAME_INDEX] + "\n")

    out.write("  native_function(" + args[NAME_INDEX] + ",\"(")

    noreturn = args[RETTYPE_INDEX] == 'void' or \
        args[RETTYPE_INDEX] == 'noreturn'

    for index in range(PARM1_INDEX, len(args)):
      if '(*)' in args[index]:
        out.write("i")
      elif 'float' in args[index]:
        out.write("f")
      elif 'double' in args[index]:
        out.write("F")
      elif 'long long' in args[index]:
        out.write("I")
      elif 'char *' in args[index]:
        out.write("$")
      elif 'off_t' in args[index]:
        out.write("I")
      elif 'uintptr_t' in args[index]:
        out.write("i")
      elif 'va_list' in args[index]:
        out.write("*")
      else:
        defval = True
        for sig in POINTER_SIGNATURE:
          if sig in args[index]:
            out.write("*")
            defval = False
            break

        if defval:
          out.write("i")
        if '...' in args[index]:
          break

    out.write(")")

    if not noreturn:
      if '*' in args[RETTYPE_INDEX]:
        out.write("i")
      elif 'off_t' in args[RETTYPE_INDEX]:
        out.write("I")
      elif 'time_t' in args[RETTYPE_INDEX]:
        out.write("I")
      elif 'double' in args[RETTYPE_INDEX]:
        out.write("F")
      elif 'float' in args[RETTYPE_INDEX]:
        out.write("f")
      elif 'long long' in args[RETTYPE_INDEX]:
        out.write("I")
      else:
        out.write("i")

    out.write("\"),\n")

    out.write("#endif /* GLUE_ENTRY_" + args[NAME_INDEX] + " */\n")

    if nodef:
      out.write("#endif /* " + args[COND_INDEX] + " */\n\n")

  out.write("};\n")
  csv.seek(0)


def generate_glue(csv, out):
  generate_include(csv, out)
  generate_functions(csv, out)
  generate_table(csv, out)


def parse_args():
  global args
  parser = argparse.ArgumentParser(
      description=__doc__,
      formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)
  parser.add_argument("-i", "--input", required=True,
                      help="CSV(Comma-Separated-Value) file")
  parser.add_argument(
      "-o", "--output", help="Output header file", default="glue.c")
  parser.add_argument("-v", "--verbose", action="count", default=0,
                      help="Verbose Output")
  args = parser.parse_args()


def main():
  parse_args()
  if not os.path.isfile(args.input):
    sys.exit(1)

  infile = open(args.input, "r")
  outfile = open(args.output, "w")

  outfile.write("#include <nuttx/config.h>\n")
  outfile.write("#include <stdint.h>\n")

  generate_glue(infile, outfile)

  infile.close()
  outfile.close()


if __name__ == "__main__":
  main()
