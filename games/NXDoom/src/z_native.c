/****************************************************************************
 * apps/games/NXDoom/src/z_native.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Zone Memory Allocation. Neat.
 *
 *  This is an implementation of the zone memory API which
 *  uses native calls to malloc() and free().
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_system.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ZONEID 0x1d4a11

#ifdef TESTING
#define malloc test_malloc
#define free test_free
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct memblock_s memblock_t;

struct memblock_s
{
  int id; /* = ZONEID */
  int tag;
  int size;
  void **user;
  memblock_t *prev;
  memblock_t *next;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Linked list of allocated blocks for each tag type */

static memblock_t *allocated_blocks[PU_NUM_TAGS];

#ifdef TESTING
static int test_malloced = 0;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: z_insert_block
 *
 * Description:
 *  Add a block into the linked list for its type.
 *
 ****************************************************************************/

static void z_insert_block(memblock_t *block)
{
  block->prev = NULL;
  block->next = allocated_blocks[block->tag];
  allocated_blocks[block->tag] = block;

  if (block->next != NULL)
    {
      block->next->prev = block;
    }
}

/****************************************************************************
 * Name: z_insert_block
 *
 * Description:
 *  Remove a block from its linked list.
 *
 ****************************************************************************/

static void z_remove_block(memblock_t *block)
{
  /* Unlink from list */

  if (block->prev == NULL)
    {
      /* Start of list */

      allocated_blocks[block->tag] = block->next;
    }
  else
    {
      if (block->prev->next != block)
        {
          i_error("Z_RemoveBlock: Doubly-linked list corrupted!");
        }

      block->prev->next = block->next;
    }

  if (block->next != NULL)
    {
      if (block->next->prev != block)
        {
          i_error("Z_RemoveBlock: Doubly-linked list corrupted!");
        }

      block->next->prev = block->prev;
    }
}

/****************************************************************************
 * Name: clear_cache
 *
 * Description:
 *  Empty data from the cache list to allocate enough data of the size
 *  required.
 *
 * Return:
 *  Returns true if any blocks were freed.
 *
 ****************************************************************************/

static boolean clear_cache(int size)
{
  memblock_t *block;
  memblock_t *next_block;
  int remaining;

  block = allocated_blocks[PU_CACHE];

  if (block == NULL)
    {
      /* Cache is already empty. */

      return false;
    }

  /* Search to the end of the PU_CACHE list.  The blocks at the end
   * of the list are the ones that have been free for longer and
   * are more likely to be unneeded now.
   */

  while (block->next != NULL)
    {
      block = block->next;
    }

  /* Search backwards through the list freeing blocks until we have
   * freed the amount of memory required.
   */

  remaining = size;

  while (remaining > 0)
    {
      if (block == NULL)
        {
          break; /* No blocks left to free; we've done our best. */
        }

      next_block = block->prev;

      z_remove_block(block);

      remaining -= block->size;

      if (block->user)
        {
          *block->user = NULL;
        }

      free(block);

      block = next_block;
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef TESTING
void *test_malloc(size_t size)
{
  int *result;

  if (test_malloced + size > 2 * 1024 * 1024)
    {
      return NULL;
    }

  test_malloced += size;

  result = malloc(size + sizeof(int));

  *result = size;

  return result + 1;
}

void test_free(void *data)
{
  int *i;

  i = ((int *)data) - 1;

  test_malloced -= *i;

  free(i);
}
#endif /* #ifdef TESTING */

/****************************************************************************
 * Name: z_init
 ****************************************************************************/

void z_init(void)
{
  memset(allocated_blocks, 0, sizeof(allocated_blocks));
  printf("zone memory: Using native C allocator.\n");
}

/****************************************************************************
 * Name: z_free
 ****************************************************************************/

void z_free(void *ptr)
{
  memblock_t *block;

  block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->id != ZONEID)
    {
      i_error("z_free: freed a pointer without ZONEID");
    }

  if (block->tag != PU_FREE && block->user != NULL)
    {
      /* clear the user's mark */

      *block->user = NULL;
    }

  z_remove_block(block);

  /* Free back to system */

  free(block);
}

/****************************************************************************
 * Name: z_malloc
 *
 * Description:
 *  You can pass a NULL user if the tag is < PU_PURGELEVEL.
 *
 * Return:
 *   The translated key code.
 *
 ****************************************************************************/

void *z_malloc(int size, int tag, void *user)
{
  memblock_t *newblock;
  unsigned char *data;
  void *result;

  if (tag < 0 || tag >= PU_NUM_TAGS || tag == PU_FREE)
    {
      i_error("z_malloc: attempted to allocate a block with an invalid "
              "tag: %i",
              tag);
    }

  if (user == NULL && tag >= PU_PURGELEVEL)
    {
      i_error("z_malloc: an owner is required for purgeable blocks");
    }

  /* Malloc a block of the required size */

  newblock = NULL;

  while (newblock == NULL)
    {
      newblock = (memblock_t *)malloc(sizeof(memblock_t) + size);

      if (newblock == NULL)
        {
          if (!clear_cache(sizeof(memblock_t) + size))
            {
              i_error("z_malloc: failed on allocation of %i bytes", size);
            }
        }
    }

  newblock->tag = tag;

  /* Hook into the linked list for this tag type */

  newblock->id = ZONEID;
  newblock->user = user;
  newblock->size = size;

  z_insert_block(newblock);

  data = (unsigned char *)newblock;
  result = data + sizeof(memblock_t);

  if (user != NULL)
    {
      *newblock->user = result;
    }

  return result;
}

/****************************************************************************
 * Name: z_free_tags
 ****************************************************************************/

void z_free_tags(int lowtag, int hightag)
{
  int i;

  for (i = lowtag; i <= hightag; ++i)
    {
      memblock_t *block;
      memblock_t *next;

      /* Free all in this chain */

      for (block = allocated_blocks[i]; block != NULL; )
        {
          next = block->next;

          /* Free this block */

          if (block->user != NULL)
            {
              *block->user = NULL;
            }

          free(block);

          /* Jump to the next in the chain */

          block = next;
        }

      /* This chain is empty now */

      allocated_blocks[i] = NULL;
    }
}

/****************************************************************************
 * Name: z_dump_heap
 ****************************************************************************/

void z_dump_heap(int lowtag, int hightag)
{
  /* WARN: broken */

#if 0
  memblock_t *block;

  printf("zone size: %i  location: %p\n", mainzone->size, mainzone);

  printf("tag range: %i to %i\n", lowtag, hightag);

  for (block = mainzone->blocklist.next; ; block = block->next)
    {
      if (block->tag >= lowtag && block->tag <= hightag)
        printf("block:%p    size:%7i    user:%p    tag:%3i\n", block,
               block->size, block->user, block->tag);

      if (block->next == &mainzone->blocklist)
        {
          /* all blocks have been hit */

          break;
        }

      if ((byte *)block + block->size != (byte *)block->next)
        printf("ERROR: block size does not touch the next block\n");

      if (block->next->prev != block)
        printf("ERROR: next block doesn't have proper back link\n");

      if (block->tag == PU_FREE && block->next->tag == PU_FREE)
        printf("ERROR: two consecutive free blocks\n");
    }
#endif
}

/****************************************************************************
 * Name: z_file_dump_heap
 ****************************************************************************/

void z_file_dump_heap(FILE *f)
{
  /* WARN: broken */
#if 0
  memblock_t *block;

  fprintf(f, "zone size: %i  location: %p\n", mainzone->size, mainzone);

  for (block = mainzone->blocklist.next; ; block = block->next)
    {
      fprintf(f, "block:%p    size:%7i    user:%p    tag:%3i\n", block,
              block->size, block->user, block->tag);

      if (block->next == &mainzone->blocklist)
        {
          /* all blocks have been hit */

          break;
        }

      if ((byte *)block + block->size != (byte *)block->next)
        fprintf(f, "ERROR: block size does not touch the next block\n");

      if (block->next->prev != block)
        fprintf(f, "ERROR: next block doesn't have proper back link\n");

      if (block->tag == PU_FREE && block->next->tag == PU_FREE)
        fprintf(f, "ERROR: two consecutive free blocks\n");
    }
#endif
}

/****************************************************************************
 * Name: z_check_heap
 ****************************************************************************/

void z_check_heap(void)
{
  memblock_t *block;
  memblock_t *prev;
  int i;

  /* Check all chains */

  for (i = 0; i < PU_NUM_TAGS; ++i)
    {
      prev = NULL;

      for (block = allocated_blocks[i]; block != NULL; block = block->next)
        {
          if (block->id != ZONEID)
            {
              i_error("z_check_heap: Block without a ZONEID!");
            }

          if (block->prev != prev)
            {
              i_error("z_check_heap: Doubly-linked list corrupted!");
            }

          prev = block;
        }
    }
}

/****************************************************************************
 * Name: z_change_tag2
 ****************************************************************************/

void z_change_tag2(void *ptr, int tag, const char *file, int line)
{
  memblock_t *block;

  block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->id != ZONEID)
    i_error("%s:%i: z_change_tag: block without a ZONEID!", file, line);

  if (tag >= PU_PURGELEVEL && block->user == NULL)
    i_error("%s:%i: z_change_tag: an owner is required "
            "for purgeable blocks",
            file, line);

  /* Remove the block from its current list, and rehook it into
   * its new list.
   */

  z_remove_block(block);
  block->tag = tag;
  z_insert_block(block);
}

void z_change_user(void *ptr, void **user)
{
  memblock_t *block;

  block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

  if (block->id != ZONEID)
    {
      i_error("z_change_user: Tried to change user for invalid block!");
    }

  block->user = user;
  *user = ptr;
}

/****************************************************************************
 * Name: z_free_memory
 ****************************************************************************/

int z_free_memory(void)
{
  return -1; /* Limited by the system?? */
}

/****************************************************************************
 * Name: z_zone_size
 ****************************************************************************/

unsigned int z_zone_size(void)
{
  return 0;
}
