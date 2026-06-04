/****************************************************************************
 * apps/examples/uGLy_demo/tri.c
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <GLES/gl.h>  /* use OpenGL ES 1.x */

#include "tex64x64xRGBA32.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FIX_INTEGER_PART 16

#define int_to_fix(v)    ((int32_t)((v) << FIX_INTEGER_PART))
#define fix_div(v1, v2)  ((GLfixed)((((int64_t)(v1)) * \
                                     (1 << FIX_INTEGER_PART)) / (v2)))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Provided by the uGLy platform backend (see uGLy/include/internal.h) */

typedef void (*KeyCallback)(int charkey);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void initWindow(KeyCallback callback);
void swapBuffers(void);
int tri_run(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static GLfixed g_view_rotx = 0;
static GLfixed g_view_roty = 0;
static GLfixed g_view_rotz = 0;

static GLuint g_texture_id;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void draw(void)
{
  /*      0
   *      |-- 2
   *      |  /|
   *      | / |
   *      |/--|
   *      1   3
   */

  static const GLfixed verts[4][3] =
  {
    {
      -int_to_fix(1), int_to_fix(1), int_to_fix(0)
    },
    {
      -int_to_fix(1), -int_to_fix(1), int_to_fix(0)
    },
    {
      int_to_fix(1), int_to_fix(1), int_to_fix(0)
    },
    {
      int_to_fix(1), -int_to_fix(1), int_to_fix(0)
    },
  };

  static const GLfixed normals[12] =
  {
    int_to_fix(0), int_to_fix(0), int_to_fix(1),
    int_to_fix(0), int_to_fix(0), int_to_fix(1),
    int_to_fix(0), int_to_fix(0), int_to_fix(1),
    int_to_fix(0), int_to_fix(0), int_to_fix(1),
  };

  static const GLfixed colors[4][4] =
  {
    {
      int_to_fix(1), int_to_fix(1), int_to_fix(1), int_to_fix(1)
    },
    {
      int_to_fix(1), int_to_fix(1), int_to_fix(1), int_to_fix(1)
    },
    {
      int_to_fix(1), int_to_fix(1), int_to_fix(1), int_to_fix(1)
    },
    {
      int_to_fix(1), int_to_fix(1), int_to_fix(1), int_to_fix(1)
    },
  };

  static const GLfixed tex_coords[8] =
  {
    int_to_fix(0), int_to_fix(1),
    int_to_fix(0), int_to_fix(0),
    int_to_fix(1), int_to_fix(1),
    int_to_fix(1), int_to_fix(0),
  };

#ifndef DISABLE_DEPTH_BUFFER
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
  glClear(GL_COLOR_BUFFER_BIT);
#endif

  glPushMatrix();
  glRotatex(g_view_rotx, int_to_fix(1), 0, 0);
  glRotatex(g_view_roty, 0, int_to_fix(1), 0);
  glRotatex(g_view_rotz, 0, 0, int_to_fix(1));

  glEnable(GL_TEXTURE_2D);

#ifndef DISABLE_DEPTH_BUFFER
  glEnable(GL_DEPTH_TEST);
#endif

  glTexCoordPointer(2, GL_FIXED, 0, tex_coords);
  glVertexPointer(3, GL_FIXED, 0, verts);
  glColorPointer(4, GL_FIXED, 0, colors);
  glNormalPointer(GL_FIXED, 0, normals);

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  /* Draw triangles */

  glBindTexture(GL_TEXTURE_2D, g_texture_id);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glPopMatrix();
}

/* New window size or exposure */

static void reshape(int width, int height)
{
  const GLfixed ar = fix_div(int_to_fix(width), int_to_fix(height));

  glViewport(0, 0, (GLint)width, (GLint)height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustumx(-ar, ar, -int_to_fix(1), int_to_fix(1), int_to_fix(5),
             int_to_fix(60));

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatex(0, 0, -int_to_fix(10));
}

static void init(void)
{
  static const GLfixed ambient[4] =
  {
    fix_div(int_to_fix(1), int_to_fix(5)),
    fix_div(int_to_fix(1), int_to_fix(5)),
    fix_div(int_to_fix(1), int_to_fix(5)),
    int_to_fix(1)
  };

  static const GLfixed pos[4] =
  {
    int_to_fix(0), int_to_fix(0), int_to_fix(1), 0
  };

  const GLfixed grey = fix_div(int_to_fix(4), int_to_fix(10));
  const GLfixed full_alpha = int_to_fix(1);

  glLightxv(GL_LIGHT0, GL_POSITION, pos);

  glLightModelxv(GL_LIGHT_MODEL_AMBIENT, ambient);

  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);

  glClearColorx(grey, grey, grey, full_alpha);

  glGenTextures(1, &g_texture_id);

  glBindTexture(GL_TEXTURE_2D, g_texture_id);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGB,
               64,
               64,
               0,
               GL_RGB,
               GL_UNSIGNED_BYTE,
               &tex[0]);
}

static void special_key(int special)
{
  switch (special)
    {
      case 'a':
        g_view_roty += int_to_fix(5);
        break;

      case 'd':
        g_view_roty -= int_to_fix(5);
        break;

      case 'w':
        g_view_rotx += int_to_fix(5);
        break;

      case 's':
        g_view_rotx -= int_to_fix(5);
        break;

      case 'z':
        g_view_rotz -= int_to_fix(5);
        break;

      case 'x':
        g_view_rotz += int_to_fix(5);
        break;

      case '0':
        g_view_rotx = 0;
        g_view_roty = 0;
        g_view_rotz = 0;
        break;

      default:
        break;
    }
}

static void main_loop(void)
{
  reshape(CONFIG_GRAPHICS_UGLY_XRES, CONFIG_GRAPHICS_UGLY_YRES);

  while (1)
    {
      draw();
      g_view_rotx += fix_div(int_to_fix(2), int_to_fix(10));
      g_view_roty += fix_div(int_to_fix(3), int_to_fix(10));

      swapBuffers();
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tri_run(void)
{
  initWindow(special_key);

  init();

  main_loop();

  return 0;
}
