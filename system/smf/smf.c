/****************************************************************************
 * apps/system/smf/smf.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2021 The Chromium OS Authors
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <system/smf.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct internal_ctx
{
  bool new_state : 1;
  bool terminate : 1;
  bool is_exit : 1;
  bool handled : 1;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT

/****************************************************************************
 * Name: is_descendant_of
 ****************************************************************************/

static bool is_descendant_of(const struct smf_state *test_state,
                             const struct smf_state *target_state)
{
  const struct smf_state *state;

  for (state = test_state; state != NULL; state = state->parent)
    {
      if (target_state == state)
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: get_child_of
 ****************************************************************************/

static const struct smf_state *
get_child_of(const struct smf_state *states, const struct smf_state *parent)
{
  const struct smf_state *state = states;

  while (state != NULL)
    {
      if (state->parent == parent)
        {
          return state;
        }

      state = state->parent;
    }

  return NULL;
}

/****************************************************************************
 * Name: get_lca_of
 *
 * Description:
 *   Find the Least Common Ancestor (LCA) of two states that are not
 *   ancestors of one another.
 *
 ****************************************************************************/

static const struct smf_state *
get_lca_of(const struct smf_state *source, const struct smf_state *dest)
{
  const struct smf_state *ancestor;

  for (ancestor = source->parent; ancestor != NULL;
       ancestor = ancestor->parent)
    {
      /* First common ancestor. */

      if (is_descendant_of(dest, ancestor))
        {
          return ancestor;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: smf_execute_all_entry_actions
 ****************************************************************************/

static bool smf_execute_all_entry_actions(struct smf_ctx *const ctx,
                                          const struct smf_state *new_state,
                                          const struct smf_state *topmost)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;
  const struct smf_state *to_execute;

  if (new_state == topmost)
    {
      /* There are no child states, so do nothing. */

      return false;
    }

  for (to_execute = get_child_of(new_state, topmost);
       to_execute != NULL && to_execute != new_state;
       to_execute = get_child_of(new_state, to_execute))
    {
      /* Keep track of the executing entry action in case it calls
       * smf_set_state().
       */

      ctx->executing = to_execute;

      /* Execute every entry action EXCEPT that of the topmost state. */

      if (to_execute->entry != NULL)
        {
          to_execute->entry(ctx);

          /* No need to continue if terminate was set. */

          if (internal->terminate)
            {
              ctx->executing = ctx->current;
              return true;
            }
        }
    }

  /* Execute the new state entry action. */

  ctx->executing = new_state;
  if (new_state->entry != NULL)
    {
      new_state->entry(ctx);

      /* No need to continue if terminate was set. */

      if (internal->terminate)
        {
          ctx->executing = ctx->current;
          return true;
        }
    }

  ctx->executing = ctx->current;

  return false;
}

/****************************************************************************
 * Name: smf_execute_ancestor_run_actions
 ****************************************************************************/

static bool smf_execute_ancestor_run_actions(struct smf_ctx *const ctx)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;
  const struct smf_state *tmp_state;

  /* Execute all run actions in reverse order. */

  if (internal->terminate)
    {
      /* Return if the current state terminated. */

      return true;
    }

  if (internal->new_state || internal->handled)
    {
      /* The child state transitioned or handled it. Stop propagating. */

      return false;
    }

  /* Try to run parent run actions. */

  for (tmp_state = ctx->current->parent; tmp_state != NULL;
       tmp_state = tmp_state->parent)
    {
      enum smf_state_result rc;

      /* Keep track of where we are in case an ancestor calls
       * smf_set_state().
       */

      ctx->executing = tmp_state;

      /* Execute parent run action. */

      if (tmp_state->run != NULL)
        {
          rc = tmp_state->run(ctx);

          if (rc == SMF_EVENT_HANDLED)
            {
              internal->handled = true;
            }

          /* No need to continue if terminate was set. */

          if (internal->terminate)
            {
              ctx->executing = ctx->current;
              return true;
            }

          /* This state dealt with it. Stop propagating. */

          if (internal->new_state || internal->handled)
            {
              break;
            }
        }
    }

  /* All done executing the run actions. */

  ctx->executing = ctx->current;

  return false;
}

/****************************************************************************
 * Name: smf_execute_all_exit_actions
 ****************************************************************************/

static bool smf_execute_all_exit_actions(struct smf_ctx *const ctx,
                                         const struct smf_state *topmost)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;
  const struct smf_state *tmp_state;
  const struct smf_state *to_execute;

  tmp_state = ctx->executing;

  for (to_execute = ctx->current;
       to_execute != NULL && to_execute != topmost;
       to_execute = to_execute->parent)
    {
      if (to_execute->exit != NULL)
        {
          ctx->executing = to_execute;
          to_execute->exit(ctx);

          /* No need to continue if terminate was set in the exit action. */

          if (internal->terminate)
            {
              ctx->executing = tmp_state;
              return true;
            }
        }
    }

  ctx->executing = tmp_state;

  return false;
}

#endif /* CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT */

/****************************************************************************
 * Name: smf_clear_internal_state
 ****************************************************************************/

static void smf_clear_internal_state(struct smf_ctx *const ctx)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;

  internal->is_exit = false;
  internal->terminate = false;
  internal->handled = false;
  internal->new_state = false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smf_set_initial
 ****************************************************************************/

void smf_set_initial(struct smf_ctx *const ctx,
                     const struct smf_state *init_state)
{
#ifdef CONFIG_SYSTEM_SMF_INITIAL_TRANSITION
  /* The final target will be the deepest leaf state that the target
   * contains. Set that as the real target.
   */

  while (init_state->initial != NULL)
    {
      init_state = init_state->initial;
    }
#endif

  smf_clear_internal_state(ctx);
  ctx->current = init_state;
  ctx->previous = NULL;
  ctx->terminate_val = 0;

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  struct internal_ctx *const internal = (void *)&ctx->internal;
  const struct smf_state *topmost;

  ctx->executing = init_state;

  /* topmost is the root ancestor of init_state, its parent == NULL. */

  topmost = get_child_of(init_state, NULL);

  /* Execute topmost state entry action,
   * since smf_execute_all_entry_actions doesn't.
   */

  if (topmost->entry != NULL)
    {
      ctx->executing = topmost;
      topmost->entry(ctx);
      ctx->executing = init_state;

      if (internal->terminate)
        {
          /* No need to continue if terminate was set. */

          return;
        }
    }

  if (smf_execute_all_entry_actions(ctx, init_state, topmost))
    {
      /* No need to continue if terminate was set. */

      return;
    }
#else
  /* Execute entry action if it exists. */

  if (init_state->entry != NULL)
    {
      init_state->entry(ctx);
    }
#endif
}

/****************************************************************************
 * Name: smf_set_state
 ****************************************************************************/

void smf_set_state(struct smf_ctx *const ctx,
                   const struct smf_state *new_state)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;

  if (new_state == NULL)
    {
      printf("SMF ERR: new_state cannot be NULL\n");
      return;
    }

  /* It does not make sense to call smf_set_state in an exit phase of a
   * state since we are already in a transition; we would always ignore
   * the intended state to transition into.
   */

  if (internal->is_exit)
    {
      printf("SMF ERR: Calling %s from exit action\n", __func__);
      return;
    }

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  const struct smf_state *topmost;

  if (ctx->executing != new_state &&
      ctx->executing->parent == new_state->parent)
    {
      /* Optimize sibling transitions (different states under same parent). */

      topmost = ctx->executing->parent;
    }
  else if (is_descendant_of(ctx->executing, new_state))
    {
      /* New state is a parent of where we are now. */

      topmost = new_state;
    }
  else if (is_descendant_of(new_state, ctx->executing))
    {
      /* We are a parent of the new state. */

      topmost = ctx->executing;
    }
  else
    {
      /* Not directly related, find LCA. */

      topmost = get_lca_of(ctx->executing, new_state);
    }

  internal->is_exit = true;
  internal->new_state = true;

  /* Call all exit actions up to (but not including) the topmost. */

  if (smf_execute_all_exit_actions(ctx, topmost))
    {
      /* No need to continue if terminate was set in the exit action. */

      return;
    }

  /* If self-transition, call the exit action. */

  if (ctx->executing == new_state && new_state->exit != NULL)
    {
      new_state->exit(ctx);

      /* No need to continue if terminate was set in the exit action. */

      if (internal->terminate)
        {
          return;
        }
    }

  internal->is_exit = false;

  /* If self transition, call the entry action. */

  if (ctx->executing == new_state && new_state->entry != NULL)
    {
      new_state->entry(ctx);

      /* No need to continue if terminate was set in the entry action. */

      if (internal->terminate)
        {
          return;
        }
    }

#ifdef CONFIG_SYSTEM_SMF_INITIAL_TRANSITION
  /* The final target will be the deepest leaf state that the target
   * contains. Set that as the real target.
   */

  while (new_state->initial != NULL)
    {
      new_state = new_state->initial;
    }
#endif

  /* Update the state variables. */

  ctx->previous = ctx->current;
  ctx->current = new_state;
  ctx->executing = new_state;

  /* Call all entry actions (except those of topmost). */

  if (smf_execute_all_entry_actions(ctx, new_state, topmost))
    {
      /* No need to continue if terminate was set in the entry action. */

      return;
    }
#else
  /* Flat state machines have a very simple transition. */

  if (ctx->current->exit != NULL)
    {
      internal->is_exit = true;
      ctx->current->exit(ctx);

      /* No need to continue if terminate was set in the exit action. */

      if (internal->terminate)
        {
          return;
        }

      internal->is_exit = false;
    }

  /* Update the state variables. */

  ctx->previous = ctx->current;
  ctx->current = new_state;

  if (ctx->current->entry != NULL)
    {
      ctx->current->entry(ctx);

      /* No need to continue if terminate was set in the entry action. */

      if (internal->terminate)
        {
          return;
        }
    }
#endif
}

/****************************************************************************
 * Name: smf_set_terminate
 ****************************************************************************/

void smf_set_terminate(struct smf_ctx *const ctx, int32_t val)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;

  internal->terminate = true;
  ctx->terminate_val = val;
}

/****************************************************************************
 * Name: smf_run_state
 ****************************************************************************/

int32_t smf_run_state(struct smf_ctx *const ctx)
{
  struct internal_ctx *const internal = (void *)&ctx->internal;

  /* No need to continue if terminate was set. */

  if (internal->terminate)
    {
      return ctx->terminate_val;
    }

  /* Executing a state's run function could cause a transition, so clear
   * the internal state to ensure that the transition is handled correctly.
   */

  smf_clear_internal_state(ctx);

#ifdef CONFIG_SYSTEM_SMF_ANCESTOR_SUPPORT
  ctx->executing = ctx->current;

  if (ctx->current->run != NULL)
    {
      enum smf_state_result rc;

      rc = ctx->current->run(ctx);
      if (rc == SMF_EVENT_HANDLED)
        {
          internal->handled = true;
        }
    }

  if (smf_execute_ancestor_run_actions(ctx))
    {
      return ctx->terminate_val;
    }
#else
  if (ctx->current->run != NULL)
    {
      ctx->current->run(ctx);
    }
#endif

  return 0;
}
