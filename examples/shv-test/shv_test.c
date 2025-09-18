/****************************************************************************
 * apps/examples/shv-test/shv_test.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_file_node.h>
#include <shv/tree/shv_connection.h>
#include <shv/tree/shv_methods.h>
#include <shv/tree/shv_clayer_posix.h>
#include <shv/tree/shv_dotdevice_node.h>
#include <shv/tree/shv_dotapp_node.h>

#include <nuttx/config.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <stdbool.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int shv_nuttxtesting_set(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid);
static int shv_nuttxtesting_get(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid);
static int shv_nuttxtesting_art(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid);

static void quit_handler(int signum);
static void print_help(char *name);

static struct shv_node *shv_tree_create_dynamically(int mode);
static void attention_cb(struct shv_con_ctx *shv_ctx,
                         enum shv_attention_reason r);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* An execution barrier */

static sem_t running;

/* Testing variable */

static int g_testing_val;

/* ------------------------- ROOT METHODS --------------------------------- */

static const struct shv_method_des * const shv_dev_root_dmap_items[] =
{
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

static const struct shv_dmap shv_dev_root_dmap =
  SHV_CREATE_NODE_DMAP(root, shv_dev_root_dmap_items);

/* ----------------------- nuttxtesting METHODS -------------------------- */

static const struct shv_method_des shv_dev_nuttxtesting_dmap_item_set =
{
  .name = "setTestingVal",
  .method = shv_nuttxtesting_set
};

static const struct shv_method_des shv_dev_nuttxtesting_dmap_item_get =
{
  .name = "getTestingVal",
  .method = shv_nuttxtesting_get
};

static const struct shv_method_des shv_dev_nuttxtesting_dmap_item_art =
{
  .name = "asciiArt",
  .method = shv_nuttxtesting_art
};

static const struct shv_method_des *const shv_dev_nuttxtesting_dmap_items[] =
{
  &shv_dev_nuttxtesting_dmap_item_art,
  &shv_dmap_item_dir,
  &shv_dev_nuttxtesting_dmap_item_get,
  &shv_dmap_item_ls,
  &shv_dev_nuttxtesting_dmap_item_set
};

static const struct shv_dmap shv_dev_nuttxtesting_dmap =
  SHV_CREATE_NODE_DMAP(nuttxtesting, shv_dev_nuttxtesting_dmap_items);

/* ------------------- Static const tree root creation ------------------- */

/* First, define all static nodes */

static const struct shv_dotdevice_node shv_static_node_dotdevice =
{
  .shv_node =
  {
    .name = ".device",
    .dir = UL_CAST_UNQ1(struct shv_dmap *, &shv_dotdevice_dmap),
    .children =
    {
      .mode = (SHV_NLIST_MODE_GSA | SHV_NLIST_MODE_STATIC)
    }
  },
  .devops =
  {
    .reset  = shv_dotdevice_node_posix_reset,
    .uptime = shv_dotdevice_node_posix_uptime
  },
  .name = "SHV Compatible Device",
  .serial_number = "0xDEADBEEF",
  .version = "0.1.0"
};

static const struct shv_dotapp_node shv_static_node_dotapp =
{
  .shv_node =
  {
    .name = ".app",
    .dir = UL_CAST_UNQ1(struct shv_dmap *, &shv_dotapp_dmap),
    .children =
    {
      .mode = (SHV_NLIST_MODE_GSA | SHV_NLIST_MODE_STATIC)
    }
  },
  .appops =
  {
    .date = NULL /* As of September 25, date parsing not implemented yet */
  },
  .name = "NuttX libshvc example",
  .version = "1.0.0"
};

static const struct shv_node shv_static_node_nuttxtesting =
{
  .name = "nuttxTesting",
  .dir = UL_CAST_UNQ1(struct shv_dmap *, &shv_dev_nuttxtesting_dmap),
  .children =
  {
    .mode = (SHV_NLIST_MODE_GSA | SHV_NLIST_MODE_STATIC)
  }
};

/* Now, define tree root's children */

const struct shv_node *const shv_static_tree_root_items[] =
{
  &shv_static_node_dotapp.shv_node,
  &shv_static_node_dotdevice.shv_node,
  &shv_static_node_nuttxtesting
};

/* Construct the root. Yes, it's a bit cumbersome,
 * but an automated code generator should have no problem with this!
 */

const struct shv_node shv_static_tree_root =
{
  .dir = UL_CAST_UNQ1(struct shv_dmap *, &shv_dev_root_dmap),
  .children =
  {
    .mode = (SHV_NLIST_MODE_GSA | SHV_NLIST_MODE_STATIC),
    .list =
    {
      .gsa =
      {
        .root =
        {
          .items = (void **)shv_static_tree_root_items,
          .count = sizeof(shv_static_tree_root_items) /
                   sizeof(shv_static_tree_root_items[0]),
          .alloc_count = 0,
        }
      }
    }
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int shv_nuttxtesting_set(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, &g_testing_val, 0);
  printf("Testing val set to %d\n", g_testing_val);
  shv_send_empty_response(shv_ctx, rid);
  return 0;
}

static int shv_nuttxtesting_get(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, g_testing_val);
  return 0;
}

static int shv_nuttxtesting_art(struct shv_con_ctx *shv_ctx,
                                struct shv_node *item, int rid)
{
  /* Generated from https://budavariam.github.io/asciiart-text/ */

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  puts("  ____  _   ___     __");
  puts(" / __ || | | \\ \\   / /");
  puts(" \\___ \\| |_| |\\ \\ / / ");
  puts("  ___) |  _  | \\ V /  ");
  puts(" |____/|_| |_|  \\_/   ");
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

static struct shv_node *shv_tree_create_dynamically(int mode)
{
  struct shv_node           *tree_root;
  struct shv_dotapp_node    *dotapp_node;
  struct shv_node           *nuttxtesting_node;
  struct shv_dotdevice_node *dotdevice_node;

  tree_root = shv_tree_node_new("", &shv_dev_root_dmap, mode);
  if (tree_root == NULL)
    {
      return NULL;
    }

  dotapp_node = shv_tree_dotapp_node_new(&shv_dotapp_dmap, mode);
  if (dotapp_node == NULL)
    {
      free(tree_root);
      return NULL;
    }

  dotapp_node->name = "NuttX libshvc example";
  dotapp_node->version = "1.0.0";

  shv_tree_add_child(tree_root, &dotapp_node->shv_node);

  dotdevice_node = shv_tree_dotdevice_node_new(&shv_dotdevice_dmap, mode);
  if (dotdevice_node == NULL)
    {
      free(tree_root);
      free(dotapp_node);
      return NULL;
    }

  dotdevice_node->name = "SHV Compatible Device";
  dotdevice_node->serial_number = "0xDEADBEEF";
  dotdevice_node->version = "0.1.0";
  shv_tree_add_child(tree_root, &dotdevice_node->shv_node);

  nuttxtesting_node = shv_tree_node_new("nuttxTesting",
                                        &shv_dev_nuttxtesting_dmap, mode);
  if (nuttxtesting_node == NULL)
    {
      free(tree_root);
      free(dotapp_node);
      free(dotdevice_node);
      return NULL;
    }

  shv_tree_add_child(tree_root, nuttxtesting_node);

  return tree_root;
}

static void quit_handler(int signum)
{
  puts("Stopping SHV FW Updater!");
  sem_post(&running);
}

static void print_help(char *name)
{
  printf("%s: [OPTIONS]\n", name);
  puts("Silicon Heaven NuttX example");
  puts("Showcases the tree creation.");
  puts("Mandatory options:");
  puts("  -a <0-2>: chooses the tree alloc type:");
  puts("    0: GAVL");
  puts("    1: GSA");
  puts("    2: GSA_STATIC: GSA with .rodata placed nodes");
  puts("  -m <mount-point>: mount point in the SHV broker");
  puts("  -i <ip-addr>: IPv4 address of the SHV broker");
  puts("  -p <passwd>: password to access the SHV broker");
  puts("  -t <port>: TCP/IP port of the SHV broker");
  puts("  -u <user>: user to access the SHV broker");
  puts("Nonmandatory options:");
  puts("  -h: print help");
}

static void attention_cb(struct shv_con_ctx *shv_ctx,
                         enum shv_attention_reason r)
{
  if (r == SHV_ATTENTION_ERROR)
    {
      printf("Error occurred in SHV, the reason is: %s\n",
             shv_errno_str(shv_ctx));
      sem_post(&running);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* Define the SHV Communication parameters */

  int ret;
  int opt;
  struct shv_connection connection;
  const struct shv_node *tree_root;
  struct shv_con_ctx *ctx;
  int alloc_mode = SHV_NLIST_MODE_GAVL;
  const char *user = NULL;
  const char *passwd = NULL;
  const char *mount = NULL;
  const char *ip = NULL;
  const char *port_s = NULL;
  int port = 0;

  /* Initialize the communication. But only if parameters are passed. */

  while ((opt = getopt(argc, argv, "a:hm:i:p:t:u:")) != -1)
    {
      switch (opt)
        {
          case 'a':
            alloc_mode = atoi(optarg);
            break;
          case 'h':
            print_help(argv[0]);
            return 0;
          case 'm':
            mount = optarg;
            break;
          case 'i':
            ip = optarg;
            break;
          case 'p':
            passwd = optarg;
            break;
          case 't':
            port_s = optarg;
            port = atoi(port_s);
            break;
          case 'u':
            user = optarg;
            break;
          case '?':
            print_help(argv[0]);
            return 1;
        }
    }

  if (!user || !passwd || !mount || !ip || !port_s)
    {
      print_help(argv[0]);
      return 1;
    }

  shv_connection_init(&connection, SHV_TLAYER_TCPIP);
  connection.broker_user =     user;
  connection.broker_password = passwd;
  connection.broker_mount =    mount;
  connection.reconnect_period = 10;
  connection.reconnect_retries = 0;
  if (shv_connection_tcpip_init(&connection, ip, port) < 0)
    {
      fprintf(stderr, "Have you supplied valid params to shv_connection?\n");
      return 1;
    }

  puts("SHV Connection Init OK");

  if (alloc_mode != SHV_NLIST_MODE_STATIC)
    {
      tree_root = shv_tree_create_dynamically(alloc_mode);
      if (tree_root == NULL)
        {
          fprintf(stderr, "Can't create the SHV tree.");
          return 1;
        }
    }
  else
    {
      tree_root = &shv_static_tree_root;
    }

  puts("SHV Tree created!");
  ctx = shv_com_init((struct shv_node *)tree_root, &connection,
                     attention_cb);
  if (ctx == NULL)
    {
      fprintf(stderr, "Can't establish the comm with the broker.\n");
      return 1;
    }

  ret = shv_create_process_thread(99, ctx);
  if (ret < 0)
    {
      fprintf(stderr, "%s\n", shv_errno_str(ctx));
      free(ctx);
      return 1;
    }

  sem_init(&running, 0, 0);
  signal(SIGTERM, quit_handler);

  sem_wait(&running);

  puts("Close the communication");
  shv_com_destroy(ctx);
  shv_tree_destroy(tree_root);

  return 0;
}
