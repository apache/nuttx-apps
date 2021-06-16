/****************************************************************************
 * apps/examples/nxflat/tests/struct/struct.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __EXAMPLES_NXFLAT_TESTS_STRUCT_STRUCT_H
#define __EXAMPLES_NXFLAT_TESTS_STRUCT_STRUCT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DUMMY_SCALAR_VALUE1 42
#define DUMMY_SCALAR_VALUE2 87
#define DUMMY_SCALAR_VALUE3 117

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*dummy_t)(void);

struct struct_dummy_s
{
  int  n;    /* This is a simple scalar value (DUMMY_SCALAR_VALUE3) */
};

struct struct_s
{
  int  n;                           /* This is a simple scalar value (DUMMY_SCALAR_VALUE1) */
  const int *pn;                    /* This is a pointer to a simple scalar value */
  const struct struct_dummy_s *ps;  /* This is a pointer to a structure */
  dummy_t pf;                       /* This is a pointer to a function */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int    dummy_scalar; /* (DUMMY_SCALAR_VALUE2) */
extern const struct struct_dummy_s dummy_struct;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern void dummyfunc(void);
extern const struct struct_s *getstruct(void);

#endif /* __EXAMPLES_NXFLAT_TESTS_STRUCT_STRUCT_H */
