/****************************************************************************
 * apps/games/NXDoom/src/net_gui.c
 *
 * SPDX-License-Identifier: GPLv2
 *
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
 * Graphical stuff related to the networking code:
 *
 *  * The client waiting screen when we are waiting for the server to
 *    start the game.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "doomkeys.h"

#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_misc.h"

#include "net_client.h"
#include "net_gui.h"
#include "net_query.h"
#include "net_server.h"

#include "textscreen.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static txt_window_t *window;
static int old_max_players;
static txt_label_t *player_labels[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
static txt_label_t *ip_labels[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
static txt_label_t *drone_label;
static txt_label_t *master_msg_label;
static boolean had_warning;

/* Number of players we expect to be in the game. When the number is
 * reached, we auto-start the game (if we're the controller). If
 * zero, do not autostart.
 */

static int expected_nodes;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void escape_pressed(TXT_UNCAST_ARG(widget), void *unused)
{
  txt_shutdown();
  i_quit();
}

static void start_game(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
  net_cl_launch_game();
}

static void open_wait_dialog(void)
{
  txt_window_action_t *cancel;

  txt_set_desktop_title(PACKAGE_STRING);

  window = txt_new_window("Waiting for game start...");

  txt_add_widget(window, txt_new_label("\nPlease wait...\n\n"));

  cancel = txt_new_window_action(KEY_ESCAPE, "Cancel");
  txt_signal_connect(cancel, "pressed", escape_pressed, NULL);

  txt_set_window_action(window, TXT_HORIZ_LEFT, cancel);
  txt_set_window_position(window, TXT_HORIZ_CENTER, TXT_VERT_BOTTOM,
                          TXT_SCREEN_W / 2, TXT_SCREEN_H - 9);

  old_max_players = 0;
}

static void build_window(void)
{
  char buf[50];
  txt_table_t *table;
  int i;

  txt_clear_table(window);
  table = txt_new_table(3);
  txt_add_widget(window, table);

  /* Add spacers */

  txt_add_widget(table, NULL);
  txt_add_widget(table, txt_new_strut(25, 1));
  txt_add_widget(table, txt_new_strut(17, 1));

  /* Player labels */

  for (i = 0; i < net_client_wait_data.max_players; ++i)
    {
      snprintf(buf, sizeof(buf), " %i. ", i + 1);
      txt_add_widget(table, txt_new_label(buf));
      player_labels[i] = txt_new_label("");
      ip_labels[i] = txt_new_label("");
      txt_add_widget(table, player_labels[i]);
      txt_add_widget(table, ip_labels[i]);
    }

  drone_label = txt_new_label("");

  txt_add_widget(window, drone_label);
}

static void update_gui(void)
{
  txt_window_action_t *startgame;
  char buf[50];
  unsigned int i;

  /* If the value of max_players changes, we must rebuild the
   * contents of the window. This includes when the first
   * waiting data packet is received.
   */

  if (net_client_received_wait_data)
    {
      if (net_client_wait_data.max_players != old_max_players)
        {
          build_window();
        }
    }
  else
    {
      return;
    }

  for (i = 0; i < net_client_wait_data.max_players; ++i)
    {
      txt_color_t color = TXT_COLOR_BRIGHT_WHITE;

      if ((signed)i == net_client_wait_data.consoleplayer)
        {
          color = TXT_COLOR_YELLOW;
        }

      txt_set_fg_colour(player_labels[i], color);
      txt_set_fg_colour(ip_labels[i], color);

      if (i < net_client_wait_data.num_players)
        {
          txt_set_label(player_labels[i],
                       net_client_wait_data.player_names[i]);
          txt_set_label(ip_labels[i], net_client_wait_data.player_addrs[i]);
        }
      else
        {
          txt_set_label(player_labels[i], "");
          txt_set_label(ip_labels[i], "");
        }
    }

  if (net_client_wait_data.num_drones > 0)
    {
      snprintf(buf, sizeof(buf), " (+%i observer clients)",
               net_client_wait_data.num_drones);
      txt_set_label(drone_label, buf);
    }
  else
    {
      txt_set_label(drone_label, "");
    }

  if (net_client_wait_data.is_controller)
    {
      startgame = txt_new_window_action(' ', "Start game");
      txt_signal_connect(startgame, "pressed", start_game, NULL);
    }
  else
    {
      startgame = NULL;
    }

  txt_set_window_action(window, TXT_HORIZ_RIGHT, startgame);
}

static void build_master_status_window(void)
{
  txt_window_t *master_window;

  master_window = txt_new_window(NULL);
  master_msg_label = txt_new_label("");
  txt_add_widget(master_window, master_msg_label);

  /* This window is here purely for information, so it should be
   * in the background.
   */

  txt_lower_window(master_window);
  txt_set_window_position(master_window, TXT_HORIZ_CENTER, TXT_VERT_CENTER,
                          TXT_SCREEN_W / 2, TXT_SCREEN_H - 4);
  txt_set_window_action(master_window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(master_window, TXT_HORIZ_CENTER, NULL);
  txt_set_window_action(master_window, TXT_HORIZ_RIGHT, NULL);
}

static void check_master_status(void)
{
  boolean added;

  if (!net_query_check_added_to_master(&added))
    {
      return;
    }

  if (master_msg_label == NULL)
    {
      build_master_status_window();
    }

  if (added)
    {
      txt_set_label(
          master_msg_label,
          "Your server is now registered with the global master server.\n"
          "Other players can find your server online.");
    }
  else
    {
      txt_set_label(
          master_msg_label,
          "Failed to register with the master server. Your server is not\n"
          "publicly accessible. You may need to reconfigure your Internet\n"
          "router to add a port forward for UDP port 2342. Look up\n"
          "information on port forwarding online.");
    }
}

static void print_sha1_digest(const char *s, const byte *digest)
{
  unsigned int i;

  printf("%s: ", s);

  for (i = 0; i < sizeof(sha1_digest_t); ++i)
    {
      printf("%02x", digest[i]);
    }

  printf("\n");
}

static void close_window(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(p_window))
{
  TXT_CAST_ARG(txt_window_t, p_window);

  txt_close_window(p_window);
}

static void check_sha1_sums(void)
{
  boolean correct_wad;
  boolean correct_deh;
  boolean same_freedoom;
  txt_window_t *l_window;
  txt_window_action_t *cont_button;

  if (!net_client_received_wait_data || had_warning)
    {
      return;
    }

  correct_wad =
      memcmp(net_local_wad_sha1sum, net_client_wait_data.wad_sha1sum,
             sizeof(sha1_digest_t)) == 0;
  correct_deh =
      memcmp(net_local_deh_sha1sum, net_client_wait_data.deh_sha1sum,
             sizeof(sha1_digest_t)) == 0;
  same_freedoom = net_client_wait_data.is_freedoom == net_local_is_freedoom;

  if (correct_wad && correct_deh && same_freedoom)
    {
      return;
    }

  if (!correct_wad)
    {
      printf("Warning: WAD SHA1 does not match server:\n");
      print_sha1_digest("Local", net_local_wad_sha1sum);
      print_sha1_digest("Server", net_client_wait_data.wad_sha1sum);
    }

  if (!same_freedoom)
    {
      printf("Warning: Mixing Freedoom with non-Freedoom\n");
      printf("Local: %u  Server: %i\n", net_local_is_freedoom,
             net_client_wait_data.is_freedoom);
    }

  if (!correct_deh)
    {
      printf("Warning: Dehacked SHA1 does not match server:\n");
      print_sha1_digest("Local", net_local_deh_sha1sum);
      print_sha1_digest("Server", net_client_wait_data.deh_sha1sum);
    }

  l_window = txt_new_window("WARNING!");

  cont_button = txt_new_window_action(KEY_ENTER, "Continue");
  txt_signal_connect(cont_button, "pressed", close_window, l_window);

  txt_set_window_action(l_window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(l_window, TXT_HORIZ_CENTER, cont_button);
  txt_set_window_action(l_window, TXT_HORIZ_RIGHT, NULL);

  if (!same_freedoom)
    {
      /* If Freedoom and Doom IWADs are mixed, the WAD directory
       * will be wrong, but this is not necessarily a problem.
       * Display a different message to the WAD directory message.
       */

      if (net_local_is_freedoom)
        {
          txt_add_widget(
              l_window,
              txt_new_label(
                  "You are using the Freedoom IWAD to play with players\n"
                  "using an official Doom IWAD.  Make sure that you are\n"
                  "playing the same levels as other players.\n"));
        }
      else
        {
          txt_add_widget(
              l_window,
              txt_new_label(
                  "You are using an official IWAD to play with players\n"
                  "using the Freedoom IWAD.  Make sure that you are\n"
                  "playing the same levels as other players.\n"));
        }
    }
  else if (!correct_wad)
    {
      txt_add_widget(
        l_window,
        txt_new_label(
        "Your WAD directory does not match other players in the game.\n"
        "Check that you have loaded the exact same WAD files as other\n"
        "players.\n")
      );
    }

  if (!correct_deh)
    {
      txt_add_widget(
          l_window,
          txt_new_label(
              "Your dehacked signature does not match other players in the\n"
              "game.  Check that you have loaded the same dehacked patches\n"
              "as other players.\n"));
    }

  txt_add_widget(
      l_window,
      txt_new_label("If you continue, this may cause your game to desync."));

  had_warning = true;
}

static void parse_command_line_args(void)
{
  int i;

  /* @arg <n>
   * @category net
   *
   * Autostart the netgame when n nodes (clients) have joined the server.
   */

  i = m_check_parm_with_args("-nodes", 1);
  if (i > 0)
    {
      expected_nodes = atoi(myargv[i + 1]);
    }
}

static void check_auto_latch(void)
{
  int nodes;

  if (net_client_received_wait_data && net_client_wait_data.is_controller &&
      expected_nodes > 0)
    {
      nodes =
          net_client_wait_data.num_players + net_client_wait_data.num_drones;

      if (nodes >= expected_nodes)
        {
          start_game(NULL, NULL);
          expected_nodes = 0;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void net_wait_for_launch(void)
{
  if (!txt_init())
    {
      fprintf(stderr, "Failed to initialize GUI\n");
      exit(-1);
    }

  /* Romero's "funky blue" color */

  txt_set_colour(TXT_COLOR_BLUE, 0x04, 0x14, 0x40);

  parse_command_line_args();
  open_wait_dialog();
  had_warning = false;

  while (net_waiting_for_launch)
    {
      update_gui();
      check_auto_latch();
      check_sha1_sums();
      check_master_status();

      txt_dispatch_events();
      txt_draw_desktop();

      net_cl_run();
      net_sv_run();

      if (!net_client_connected)
        {
          i_error("Lost connection to server");
        }

      txt_sleep(100);
    }

  txt_shutdown();
}
