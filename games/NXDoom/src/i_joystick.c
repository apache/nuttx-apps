/****************************************************************************
 * apps/games/NXDoom/src/i_joystick.c
 *
 * SPDX-License-Identifer: GPLv2
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
 * DESCRIPTION:
 *   SDL Joystick code.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_event.h"
#include "doomtype.h"
#include "i_joystick.h"
#include "i_system.h"

#include "m_config.h"
#include "m_fixed.h"
#include "m_misc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DIRECTION_DEADZONE (50 * 32768 / 100)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Configuration variables: */

/* Standard default.cfg Joystick enable/disable */

static int usejoystick = 0;

/* Use SDL_gamecontroller interface for the selected device */

static int use_gamepad = 0;

/* SDL_GameControllerType of gamepad */

static int gamepad_type = 0;

/* SDL GUID and index of the joystick to use. */

static char *joystick_guid = "";
static int joystick_index = -1;

/* Which joystick axis to use for horizontal movement, and whether to
 * invert the direction:
 */

static int joystick_x_axis = 0;
static int joystick_x_invert = 0;

/* Which joystick axis to use for vertical movement, and whether to
 * invert the direction:
 */

static int joystick_y_axis = 1;
static int joystick_y_invert = 0;

/* Which joystick axis to use for strafing? */

static int joystick_strafe_axis = -1;
static int joystick_strafe_invert = 0;

/* Which joystick axis to use for looking? */

static int joystick_look_axis = -1;
static int joystick_look_invert = 0;

/* Configurable dead zone for each axis, specified as a percentage of the
 * axis max value.
 */

static int joystick_x_dead_zone = 33;
static int joystick_y_dead_zone = 33;
static int joystick_strafe_dead_zone = 33;
static int joystick_look_dead_zone = 33;

/* Virtual to physical button joystick button mapping. By default this
 * is a straight mapping.
 */

static int joystick_physical_buttons[NUM_VIRTUAL_BUTTONS] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

int use_analog = 0;

int joystick_turn_sensitivity = 10;
int joystick_move_sensitivity = 10;
int joystick_look_sensitivity = 10;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_init_joystick(void)
{
}

void i_update_gamepad(void)
{
}

void i_shutdown_joystick(void)
{
}

void i_update_joystick(void)
{
}

void i_bind_joystick_variables(void)
{
  int i;

  m_bind_int_variable("use_joystick", &usejoystick);
  m_bind_int_variable("use_gamepad", &use_gamepad);
  m_bind_int_variable("gamepad_type", &gamepad_type);
  m_bind_string_variable("joystick_guid", &joystick_guid);
  m_bind_int_variable("joystick_index", &joystick_index);
  m_bind_int_variable("joystick_x_axis", &joystick_x_axis);
  m_bind_int_variable("joystick_y_axis", &joystick_y_axis);
  m_bind_int_variable("joystick_strafe_axis", &joystick_strafe_axis);
  m_bind_int_variable("joystick_x_invert", &joystick_x_invert);
  m_bind_int_variable("joystick_y_invert", &joystick_y_invert);
  m_bind_int_variable("joystick_strafe_invert", &joystick_strafe_invert);
  m_bind_int_variable("joystick_look_axis", &joystick_look_axis);
  m_bind_int_variable("joystick_look_invert", &joystick_look_invert);
  m_bind_int_variable("joystick_x_dead_zone", &joystick_x_dead_zone);
  m_bind_int_variable("joystick_y_dead_zone", &joystick_y_dead_zone);
  m_bind_int_variable("joystick_strafe_dead_zone",
                      &joystick_strafe_dead_zone);
  m_bind_int_variable("joystick_look_dead_zone", &joystick_look_dead_zone);
  m_bind_int_variable("use_analog", &use_analog);
  m_bind_int_variable("joystick_turn_sensitivity",
                      &joystick_turn_sensitivity);
  m_bind_int_variable("joystick_move_sensitivity",
                      &joystick_move_sensitivity);
  m_bind_int_variable("joystick_look_sensitivity",
                      &joystick_look_sensitivity);

  for (i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
      char name[32];
      snprintf(name, sizeof(name), "joystick_physical_button%i", i);
      m_bind_int_variable(name, &joystick_physical_buttons[i]);
    }
}
