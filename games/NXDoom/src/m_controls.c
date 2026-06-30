/****************************************************************************
 * apps/games/NXDoom/src/m_controls.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 1993-2008 Raven Software
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "doomkeys.h"
#include "doomtype.h"

#include "m_config.h"
#include "m_misc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Keyboard controls */

int key_right = KEY_RIGHTARROW;
int key_left = KEY_LEFTARROW;

int key_up = KEY_UPARROW;
int key_down = KEY_DOWNARROW;
int key_strafeleft = 0x2c; /* Comma, ',' */
int key_straferight = '.';
int key_fire = KEY_RCTRL;
int key_use = ' ';
int key_strafe = KEY_RALT;
int key_speed = KEY_RSHIFT;

/* Heretic keyboard controls */

int key_flyup = KEY_PGUP;
int key_flydown = KEY_INS;
int key_flycenter = KEY_HOME;

int key_lookup = KEY_PGDN;
int key_lookdown = KEY_DEL;
int key_lookcenter = KEY_END;

int key_invleft = '[';
int key_invright = ']';
int key_useartifact = KEY_ENTER;

int key_arti_quartz = 0;
int key_arti_urn = 0;
int key_arti_bomb = 0;
int key_arti_tome = 127;
int key_arti_ring = 0;
int key_arti_chaosdevice = 0;
int key_arti_shadowsphere = 0;
int key_arti_wings = 0;
int key_arti_torch = 0;
int key_arti_morph = 0;

/* Hexen key controls */

int key_jump = '/';

int key_arti_all = KEY_BACKSPACE;
int key_arti_health = '\\';
int key_arti_poisonbag = '0';
int key_arti_blastradius = '9';
int key_arti_teleport = '8';
int key_arti_teleportother = '7';
int key_arti_egg = '6';
int key_arti_invulnerability = '5';

/* Strife key controls
 *
 * haleyjd 09/01/10
 *
 * Note: Strife also uses key_invleft, key_invright, key_jump, key_lookup,
 * and key_lookdown, but with different default values.
 */

int key_usehealth = 'h';
int key_invquery = 'q';
int key_mission = 'w';
int key_invpop = 'z';
int key_invkey = 'k';
int key_invhome = KEY_HOME;
int key_invend = KEY_END;
int key_invuse = KEY_ENTER;
int key_invdrop = KEY_BACKSPACE;

/* Mouse controls */

int mousebfire = 0;
int mousebstrafe = 1;
int mousebforward = 2;
int mousebspeed = 3;

int mousebjump = -1;

int mousebstrafeleft = -1;
int mousebstraferight = -1;
int mousebturnleft = -1;
int mousebturnright = -1;
int mousebbackward = -1;
int mousebuse = -1;

int mousebprevweapon = -1;
int mousebnextweapon = -1;
int mousebinvleft = -1;
int mousebinvright = -1;
int mousebuseartifact = -1;

int key_message_refresh = KEY_ENTER;
int key_pause = KEY_PAUSE;
int key_demo_quit = 'q';
int key_spy = KEY_F12;

/* Multiplayer chat keys: */

int key_multi_msg = 't';
int key_multi_msgplayer[8];

/* Weapon selection keys: */

int key_weapon1 = '1';
int key_weapon2 = '2';
int key_weapon3 = '3';
int key_weapon4 = '4';
int key_weapon5 = '5';
int key_weapon6 = '6';
int key_weapon7 = '7';
int key_weapon8 = '8';
int key_prevweapon = 0;
int key_nextweapon = 0;

/* Map control keys: */

int key_map_north = KEY_UPARROW;
int key_map_south = KEY_DOWNARROW;
int key_map_east = KEY_RIGHTARROW;
int key_map_west = KEY_LEFTARROW;
int key_map_zoomin = '=';
int key_map_zoomout = '-';
int key_map_toggle = KEY_TAB;
int key_map_maxzoom = '0';
int key_map_follow = 'f';
int key_map_grid = 'g';
int key_map_mark = 'm';
int key_map_clearmark = 'c';

/* menu keys: */

int key_menu_activate = KEY_ESCAPE;
int key_menu_up = KEY_UPARROW;
int key_menu_down = KEY_DOWNARROW;
int key_menu_left = KEY_LEFTARROW;
int key_menu_right = KEY_RIGHTARROW;
int key_menu_back = KEY_BACKSPACE;
int key_menu_forward = KEY_ENTER;
int key_menu_confirm = 'y';
int key_menu_abort = 'n';

int key_menu_help = KEY_F1;
int key_menu_save = KEY_F2;
int key_menu_load = KEY_F3;
int key_menu_volume = KEY_F4;
int key_menu_detail = KEY_F5;
int key_menu_qsave = KEY_F6;
int key_menu_endgame = KEY_F7;
int key_menu_messages = KEY_F8;
int key_menu_qload = KEY_F9;
int key_menu_quit = KEY_F10;
int key_menu_gamma = KEY_F11;

int key_menu_incscreen = KEY_EQUALS;
int key_menu_decscreen = KEY_MINUS;
int key_menu_screenshot = 0;

/* Joystick controls */

int joybfire = 0;
int joybstrafe = 1;
int joybuse = 3;
int joybspeed = 2;

int joybstrafeleft = -1;
int joybstraferight = -1;

int joybjump = -1;

int joybprevweapon = -1;
int joybnextweapon = -1;

int joybmenu = -1;
int joybautomap = -1;

int joybuseartifact = -1;
int joybinvleft = -1;
int joybinvright = -1;

int joybflyup = -1;
int joybflydown = -1;
int joybflycenter = -1;

/* Control whether if a mouse button is double clicked, it acts like
 * "use" has been pressed
 */

int dclick_use = 1;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Bind all of the common controls used by Doom and all other games. */

void m_bind_base_controls(void)
{
  m_bind_int_variable("key_right", &key_right);
  m_bind_int_variable("key_left", &key_left);
  m_bind_int_variable("key_up", &key_up);
  m_bind_int_variable("key_down", &key_down);
  m_bind_int_variable("key_strafeleft", &key_strafeleft);
  m_bind_int_variable("key_straferight", &key_straferight);
  m_bind_int_variable("key_fire", &key_fire);
  m_bind_int_variable("key_use", &key_use);
  m_bind_int_variable("key_strafe", &key_strafe);
  m_bind_int_variable("key_speed", &key_speed);

  m_bind_int_variable("mouseb_fire", &mousebfire);
  m_bind_int_variable("mouseb_strafe", &mousebstrafe);
  m_bind_int_variable("mouseb_forward", &mousebforward);
  m_bind_int_variable("mouseb_speed", &mousebspeed);

  m_bind_int_variable("joyb_fire", &joybfire);
  m_bind_int_variable("joyb_strafe", &joybstrafe);
  m_bind_int_variable("joyb_use", &joybuse);
  m_bind_int_variable("joyb_speed", &joybspeed);

  m_bind_int_variable("joyb_menu_activate", &joybmenu);
  m_bind_int_variable("joyb_toggle_automap", &joybautomap);

  /* Extra controls that are not in the Vanilla versions: */

  m_bind_int_variable("joyb_strafeleft", &joybstrafeleft);
  m_bind_int_variable("joyb_straferight", &joybstraferight);
  m_bind_int_variable("mouseb_strafeleft", &mousebstrafeleft);
  m_bind_int_variable("mouseb_straferight", &mousebstraferight);
  m_bind_int_variable("mouseb_turnleft", &mousebturnleft);
  m_bind_int_variable("mouseb_turnright", &mousebturnright);
  m_bind_int_variable("mouseb_use", &mousebuse);
  m_bind_int_variable("mouseb_backward", &mousebbackward);
  m_bind_int_variable("dclick_use", &dclick_use);
  m_bind_int_variable("key_pause", &key_pause);
  m_bind_int_variable("key_message_refresh", &key_message_refresh);
}

void m_bind_heretic_controls(void)
{
  m_bind_int_variable("key_flyup", &key_flyup);
  m_bind_int_variable("key_flydown", &key_flydown);
  m_bind_int_variable("key_flycenter", &key_flycenter);

  m_bind_int_variable("key_lookup", &key_lookup);
  m_bind_int_variable("key_lookdown", &key_lookdown);
  m_bind_int_variable("key_lookcenter", &key_lookcenter);

  m_bind_int_variable("key_invleft", &key_invleft);
  m_bind_int_variable("key_invright", &key_invright);
  m_bind_int_variable("key_useartifact", &key_useartifact);

  m_bind_int_variable("mouseb_invleft", &mousebinvleft);
  m_bind_int_variable("mouseb_invright", &mousebinvright);
  m_bind_int_variable("mouseb_useartifact", &mousebuseartifact);

  m_bind_int_variable("joyb_invleft", &joybinvleft);
  m_bind_int_variable("joyb_invright", &joybinvright);
  m_bind_int_variable("joyb_useartifact", &joybuseartifact);

  m_bind_int_variable("joyb_flyup", &joybflyup);
  m_bind_int_variable("joyb_flydown", &joybflydown);
  m_bind_int_variable("joyb_flycenter", &joybflycenter);

  m_bind_int_variable("key_arti_quartz", &key_arti_quartz);
  m_bind_int_variable("key_arti_urn", &key_arti_urn);
  m_bind_int_variable("key_arti_bomb", &key_arti_bomb);
  m_bind_int_variable("key_arti_tome", &key_arti_tome);
  m_bind_int_variable("key_arti_ring", &key_arti_ring);
  m_bind_int_variable("key_arti_chaosdevice", &key_arti_chaosdevice);
  m_bind_int_variable("key_arti_shadowsphere", &key_arti_shadowsphere);
  m_bind_int_variable("key_arti_wings", &key_arti_wings);
  m_bind_int_variable("key_arti_torch", &key_arti_torch);
  m_bind_int_variable("key_arti_morph", &key_arti_morph);
}

void m_bind_hexen_controls(void)
{
  m_bind_int_variable("key_jump", &key_jump);
  m_bind_int_variable("mouseb_jump", &mousebjump);
  m_bind_int_variable("joyb_jump", &joybjump);

  m_bind_int_variable("key_arti_all", &key_arti_all);
  m_bind_int_variable("key_arti_health", &key_arti_health);
  m_bind_int_variable("key_arti_poisonbag", &key_arti_poisonbag);
  m_bind_int_variable("key_arti_blastradius", &key_arti_blastradius);
  m_bind_int_variable("key_arti_teleport", &key_arti_teleport);
  m_bind_int_variable("key_arti_teleportother", &key_arti_teleportother);
  m_bind_int_variable("key_arti_egg", &key_arti_egg);
  m_bind_int_variable("key_arti_invulnerability", &key_arti_invulnerability);
}

void m_bind_strife_controls(void)
{
  /* These are shared with all games, but have different defaults: */

  key_message_refresh = '/';

  /* These keys are shared with Heretic/Hexen but have different defaults: */

  key_jump = 'a';
  key_lookup = KEY_PGUP;
  key_lookdown = KEY_PGDN;
  key_invleft = KEY_INS;
  key_invright = KEY_DEL;

  m_bind_int_variable("key_jump", &key_jump);
  m_bind_int_variable("key_look_up", &key_lookup);
  m_bind_int_variable("key_look_down", &key_lookdown);
  m_bind_int_variable("key_inv_left", &key_invleft);
  m_bind_int_variable("key_inv_right", &key_invright);

  /* Custom Strife-only Keys: */

  m_bind_int_variable("key_use_health", &key_usehealth);
  m_bind_int_variable("key_invquery", &key_invquery);
  m_bind_int_variable("key_mission", &key_mission);
  m_bind_int_variable("key_inv_pop", &key_invpop);
  m_bind_int_variable("key_inv_key", &key_invkey);
  m_bind_int_variable("key_invHome", &key_invhome);
  m_bind_int_variable("key_inv_end", &key_invend);
  m_bind_int_variable("key_inv_use", &key_invuse);
  m_bind_int_variable("key_inv_drop", &key_invdrop);

  /* Strife also supports jump on mouse and joystick, and in the exact same
   * manner as Hexen!
   */

  m_bind_int_variable("mouseb_jump", &mousebjump);
  m_bind_int_variable("joyb_jump", &joybjump);

  /* Subset of inventory actions common to Heretic/Hexen and Strife. */

  m_bind_int_variable("joyb_invleft", &joybinvleft);
  m_bind_int_variable("joyb_invright", &joybinvright);

  /* This is technically "invuse" in Strife, but let's reuse the value. */

  m_bind_int_variable("joyb_useartifact", &joybuseartifact);
}

void m_bind_weapon_controls(void)
{
  m_bind_int_variable("key_weapon1", &key_weapon1);
  m_bind_int_variable("key_weapon2", &key_weapon2);
  m_bind_int_variable("key_weapon3", &key_weapon3);
  m_bind_int_variable("key_weapon4", &key_weapon4);
  m_bind_int_variable("key_weapon5", &key_weapon5);
  m_bind_int_variable("key_weapon6", &key_weapon6);
  m_bind_int_variable("key_weapon7", &key_weapon7);
  m_bind_int_variable("key_weapon8", &key_weapon8);

  m_bind_int_variable("key_prevweapon", &key_prevweapon);
  m_bind_int_variable("key_nextweapon", &key_nextweapon);

  m_bind_int_variable("joyb_prevweapon", &joybprevweapon);
  m_bind_int_variable("joyb_nextweapon", &joybnextweapon);

  m_bind_int_variable("mouseb_prevweapon", &mousebprevweapon);
  m_bind_int_variable("mouseb_nextweapon", &mousebnextweapon);
}

void m_bind_map_controls(void)
{
  m_bind_int_variable("key_map_north", &key_map_north);
  m_bind_int_variable("key_map_south", &key_map_south);
  m_bind_int_variable("key_map_east", &key_map_east);
  m_bind_int_variable("key_map_west", &key_map_west);
  m_bind_int_variable("key_map_zoomin", &key_map_zoomin);
  m_bind_int_variable("key_map_zoomout", &key_map_zoomout);
  m_bind_int_variable("key_map_toggle", &key_map_toggle);
  m_bind_int_variable("key_map_maxzoom", &key_map_maxzoom);
  m_bind_int_variable("key_map_follow", &key_map_follow);
  m_bind_int_variable("key_map_grid", &key_map_grid);
  m_bind_int_variable("key_map_mark", &key_map_mark);
  m_bind_int_variable("key_map_clearmark", &key_map_clearmark);
}

void m_bind_menu_controls(void)
{
  m_bind_int_variable("key_menu_activate", &key_menu_activate);
  m_bind_int_variable("key_menu_up", &key_menu_up);
  m_bind_int_variable("key_menu_down", &key_menu_down);
  m_bind_int_variable("key_menu_left", &key_menu_left);
  m_bind_int_variable("key_menu_right", &key_menu_right);
  m_bind_int_variable("key_menu_back", &key_menu_back);
  m_bind_int_variable("key_menu_forward", &key_menu_forward);
  m_bind_int_variable("key_menu_confirm", &key_menu_confirm);
  m_bind_int_variable("key_menu_abort", &key_menu_abort);

  m_bind_int_variable("key_menu_help", &key_menu_help);
  m_bind_int_variable("key_menu_save", &key_menu_save);
  m_bind_int_variable("key_menu_load", &key_menu_load);
  m_bind_int_variable("key_menu_volume", &key_menu_volume);
  m_bind_int_variable("key_menu_detail", &key_menu_detail);
  m_bind_int_variable("key_menu_qsave", &key_menu_qsave);
  m_bind_int_variable("key_menu_endgame", &key_menu_endgame);
  m_bind_int_variable("key_menu_messages", &key_menu_messages);
  m_bind_int_variable("key_menu_qload", &key_menu_qload);
  m_bind_int_variable("key_menu_quit", &key_menu_quit);
  m_bind_int_variable("key_menu_gamma", &key_menu_gamma);

  m_bind_int_variable("key_menu_incscreen", &key_menu_incscreen);
  m_bind_int_variable("key_menu_decscreen", &key_menu_decscreen);
  m_bind_int_variable("key_menu_screenshot", &key_menu_screenshot);
  m_bind_int_variable("key_demo_quit", &key_demo_quit);
  m_bind_int_variable("key_spy", &key_spy);
}

void m_bind_chat_controls(unsigned int num_players)
{
  char name[32];  /* haleyjd: 20 not large enough - Thank you, come again! */
  unsigned int i; /* haleyjd: signedness conflict */

  m_bind_int_variable("key_multi_msg", &key_multi_msg);

  for (i = 0; i < num_players; ++i)
    {
      snprintf(name, sizeof(name), "key_multi_msgplayer%i", i + 1);
      m_bind_int_variable(name, &key_multi_msgplayer[i]);
    }
}

/* Apply custom patches to the default values depending on the
 * platform we are running on.
 */

void m_apply_platform_defaults(void)
{
  /* no-op. Add your platform-specific patches here. */
}
