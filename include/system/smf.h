/****************************************************************************
 * apps/include/system/smf.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_INCLUDE_SYSTEM_SMF_H
#define __APPS_INCLUDE_SYSTEM_SMF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
#  ifdef CONFIG_SYSTEM_SMF_INITIAL_TRANSITION
#    define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
       { \
         .entry = (_entry), \
         .run = (_run), \
         .exit = (_exit), \
         .parent = (_parent), \
         .initial = (_initial), \
       }
#  else
#    define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
       { \
         .entry = (_entry), \
         .run = (_run), \
         .exit = (_exit), \
         .parent = (_parent), \
       }
#  endif
#else
#  define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial) \
     { \
       .entry = (_entry), \
       .run = (_run), \
       .exit = (_exit), \
     }
#endif

#define SMF_CTX(o) ((struct smf_ctx *)(o))

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum smf_state_result
{
  SMF_EVENT_HANDLED,
  SMF_EVENT_PROPAGATE
};

typedef void (*state_method)(void *obj);

typedef enum smf_state_result (*state_execution)(void *obj);

struct smf_state
{
  /* Optional method that will be run when this state is entered. */

  state_method entry;

  /* Optional method that will be run repeatedly during the state machine
   * loop.
   */

  state_execution run;

  /* Optional method that will be run when this state exits. */

  state_method exit;

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  /* Optional parent state that contains common entry/run/exit
   * implementation among various child states.
   * entry: Parent function executes BEFORE child function.
   * run:   Parent function executes AFTER child function.
   * exit:  Parent function executes AFTER child function.
   * Note: When transitioning between two child states with a shared
   * parent, that parent's exit and entry functions do not execute.
   */

  const struct smf_state *parent;

#  ifdef CONFIG_SYSTEM_SMF_INITIAL_TRANSITION
  /* Optional initial transition state. NULL for leaf states. */

  const struct smf_state *initial;
#  endif /* CONFIG_SYSTEM_SMF_INITIAL_TRANSITION */
#endif /* CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT */
};

struct smf_ctx
{
  /* Current state the state machine is executing. */

  const struct smf_state *current;

  /* Previous state the state machine executed. */

  const struct smf_state *previous;

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  /* Currently executing state (which may be a parent). */

  const struct smf_state *executing;
#endif /* CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT */

  /* This value is set by the set_terminate function and should
   * terminate the state machine when its set to a value other than
   * zero when it's returned by the run_state function.
   */

  int32_t terminate_val;

  /* The state machine casts this to a "struct internal_ctx" and it's
   * used to track state machine context.
   */

  uint32_t internal;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: smf_set_initial
 *
 * Description:
 *   Initialize the state machine and set its initial state.
 *
 * Input Parameters:
 *   ctx - State machine context
 *   init_state - Initial state the state machine starts in
 *
 ****************************************************************************/

void smf_set_initial(struct smf_ctx *ctx,
                     const struct smf_state *init_state);

/****************************************************************************
 * Name: smf_set_state
 *
 * Description:
 *   Change the state machine state. This handles exiting the previous state
 *   and entering the target state. For HSMs the entry and exit actions of
 *   the Least Common Ancestor are not run.
 *
 * Input Parameters:
 *   ctx - State machine context
 *   new_state - State to transition to
 *
 ****************************************************************************/

void smf_set_state(struct smf_ctx *ctx, const struct smf_state *new_state);

/****************************************************************************
 * Name: smf_set_terminate
 *
 * Description:
 *   Terminate a state machine.
 *
 * Input Parameters:
 *   ctx - State machine context
 *   val - Non-zero termination value returned by smf_run_state
 *
 ****************************************************************************/

void smf_set_terminate(struct smf_ctx *ctx, int32_t val);

/****************************************************************************
 * Name: smf_run_state
 *
 * Description:
 *   Run one iteration of a state machine (including any parent states).
 *
 * Input Parameters:
 *   ctx - State machine context
 *
 * Returned Value:
 *   A non-zero value terminates the state machine. This non-zero value may
 *   represent a terminal state being reached or detection of an error.
 *
 ****************************************************************************/

int32_t smf_run_state(struct smf_ctx *ctx);

/****************************************************************************
 * Name: smf_get_current_leaf_state
 *
 * Description:
 *   Get the current leaf state. This may be a parent state if the HSM is
 *   malformed (initial transitions are not set up correctly).
 *
 ****************************************************************************/

static inline const struct smf_state *
smf_get_current_leaf_state(const struct smf_ctx *ctx)
{
  return ctx->current;
}

/****************************************************************************
 * Name: smf_get_current_executing_state
 *
 * Description:
 *   Get the state that is currently executing. This may be a parent state.
 *
 ****************************************************************************/

static inline const struct smf_state *
smf_get_current_executing_state(const struct smf_ctx *ctx)
{
#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  return ctx->executing;
#else
  return ctx->current;
#endif /* CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT */
}

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_SMF_H */
