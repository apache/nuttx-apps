/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_widget.h
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
 ****************************************************************************/

#ifndef TXT_WIDGET_H
#define TXT_WIDGET_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TXT_UNCAST_ARG_NAME(name) uncast_##name
#define TXT_UNCAST_ARG(name) void *TXT_UNCAST_ARG_NAME(name)
#define TXT_CAST_ARG(type, name) type *name = (type *)uncast_##name

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Forward definitions */

typedef struct txt_table_s txt_table_t;
typedef struct txt_strut_s txt_strut_t;
typedef struct txt_spincontrol_s txt_spincontrol_t;
typedef struct txt_separator_s txt_separator_t;
typedef struct txt_scrollpane_s txt_scrollpane_t;
typedef struct txt_radiobutton_s txt_radiobutton_t;
typedef struct txt_inputbox_s txt_inputbox_t;
typedef struct txt_dropdown_list_s txt_dropdown_list_t;
typedef struct txt_checkbox_s txt_checkbox_t;
typedef struct txt_button_s txt_button_t;

typedef enum
{
  TXT_VERT_TOP,
  TXT_VERT_CENTER,
  TXT_VERT_BOTTOM,
} txt_vert_align_t;

typedef enum
{
  TXT_HORIZ_LEFT,
  TXT_HORIZ_CENTER,
  TXT_HORIZ_RIGHT,
} txt_horiz_align_t;

/* A GUI widget.
 *
 * A widget is an individual component of a GUI.  Various different widget
 * types exist.
 *
 * Widgets may emit signals.  The types of signal emitted by a widget
 * depend on the type of the widget.  It is possible to be notified
 * when a signal occurs using the @ref txt_signal_connect function.
 */

typedef struct txt_widget_s txt_widget_t;

typedef struct txt_widget_class_s txt_widget_class_t;
typedef struct txt_callback_table_s txt_callback_table_t;

typedef void (*txt_widget_size_calc_f)(TXT_UNCAST_ARG(widget));
typedef void (*txt_widget_drawer_f)(TXT_UNCAST_ARG(widget));
typedef void (*txt_widget_destroy_f)(TXT_UNCAST_ARG(widget));
typedef int (*txt_widget_keypress_f)(TXT_UNCAST_ARG(widget), int key);
typedef void (*txt_widget_signal_f)(TXT_UNCAST_ARG(widget), void *user_data);
typedef void (*txt_mouse_press_f)(TXT_UNCAST_ARG(widget), int x, int y,
                                  int b);
typedef void (*txt_widget_layout_f)(TXT_UNCAST_ARG(widget));
typedef int (*txt_widget_selectable_f)(TXT_UNCAST_ARG(widget));
typedef void (*txt_widget_focus_f)(TXT_UNCAST_ARG(widget), int focused);

struct txt_widget_class_s
{
  txt_widget_selectable_f selectable;
  txt_widget_size_calc_f size_calc;
  txt_widget_drawer_f drawer;
  txt_widget_keypress_f key_press;
  txt_widget_destroy_f destructor;
  txt_mouse_press_f mouse_press;
  txt_widget_layout_f layout;
  txt_widget_focus_f focus_change;
};

struct txt_widget_s
{
  txt_widget_class_t *widget_class;
  txt_callback_table_t *callback_table;
  int visible;
  txt_horiz_align_t align;
  int focused;

  /* These are set automatically when the window is drawn and should
   * not be set manually.
   */

  int x;
  int y;
  unsigned int w;
  unsigned int h;

  /* Pointer up to parent widget that contains this widget. */

  txt_widget_t *parent;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void txt_init_widget(TXT_UNCAST_ARG(widget),
                     txt_widget_class_t *widget_class);
void txt_calc_widget_size(TXT_UNCAST_ARG(widget));
void txt_draw_widget(TXT_UNCAST_ARG(widget));
void txt_emit_signal(TXT_UNCAST_ARG(widget), const char *signal_name);
int txt_widget_key_press(TXT_UNCAST_ARG(widget), int key);
void txt_widget_mouse_press(TXT_UNCAST_ARG(widget), int x, int y, int b);
void txt_destroy_widget(TXT_UNCAST_ARG(widget));
void txt_layout_widget(TXT_UNCAST_ARG(widget));
int txt_always_selectable(TXT_UNCAST_ARG(widget));
int txt_never_selectable(TXT_UNCAST_ARG(widget));
void txt_set_widget_focus(TXT_UNCAST_ARG(widget), int focused);

/**
 * Set a callback function to be invoked when a signal occurs.
 *
 * @param widget       The widget to watch.
 * @param signal_name  The signal to watch.
 * @param func         The callback function to invoke.
 * @param user_data    User-specified pointer to pass to the callback
 * function.
 */

void txt_signal_connect(TXT_UNCAST_ARG(widget), const char *signal_name,
                        txt_widget_signal_f func, void *user_data);

/**
 * Set the policy for how a widget should be aligned within a table.
 * By default, widgets are aligned to the left of the column.
 *
 * @param widget       The widget.
 * @param horiz_align  The alignment to use.
 */

void txt_set_widget_align(TXT_UNCAST_ARG(widget),
                        txt_horiz_align_t horiz_align);

/**
 * Query whether a widget is selectable with the cursor.
 *
 * @param widget       The widget.
 * @return             Non-zero if the widget is selectable.
 */

int txt_selectable_widget(TXT_UNCAST_ARG(widget));

/**
 * Query whether the mouse is hovering over the specified widget.
 *
 * @param widget       The widget.
 * @return             Non-zero if the mouse cursor is over the widget.
 */

int txt_hovering_over_widget(TXT_UNCAST_ARG(widget));

/**
 * Set the background to draw the specified widget, depending on
 * whether it is selected and the mouse is hovering over it.
 *
 * @param widget       The widget.
 */

void txt_set_widget_bg(TXT_UNCAST_ARG(widget));

#endif /* TXT_WIDGET_H */
