/****************************************************************************
 * apps/examples/tinygl/tinygl_main.c
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

/* The gear drawing code is based on the classic "gears" demo by Brian Paul,
 * which is in the public domain, as shipped with TinyGL in
 * Raw_Demos/gears.c.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/video/fb.h>

#include <GL/gl.h>
#include <zbuffer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#if TGL_FEATURE_RENDER_BITS == 32
#  define TINYGL_FB_FMT  FB_FMT_RGB32
#  define TINYGL_ZB_MODE ZB_MODE_RGBA
#else
#  define TINYGL_FB_FMT  FB_FMT_RGB16_565
#  define TINYGL_ZB_MODE ZB_MODE_5R6G5B
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tinygl_fb_s
{
  int fd;
  FAR uint8_t *fbmem;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static GLfloat g_view_rotx = 20.0f;
static GLfloat g_view_roty = 30.0f;
static GLfloat g_angle;
static GLint g_gear1;
static GLint g_gear2;
static GLint g_gear3;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gear
 *
 * Description:
 *   Draw a gear wheel into the current display list.
 *
 * Input Parameters:
 *   inner_radius - radius of the hole at the center
 *   outer_radius - radius at the center of the teeth
 *   width        - width of the gear
 *   teeth        - number of teeth
 *   tooth_depth  - depth of a tooth
 *
 ****************************************************************************/

static void gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
                 GLint teeth, GLfloat tooth_depth)
{
  GLfloat r0 = inner_radius;
  GLfloat r1 = outer_radius - tooth_depth / 2.0f;
  GLfloat r2 = outer_radius + tooth_depth / 2.0f;
  GLfloat da = 2.0f * M_PI / teeth / 4.0f;
  GLfloat angle;
  GLfloat u;
  GLfloat v;
  GLfloat len;
  GLint i;

  glNormal3f(0.0f, 0.0f, 1.0f);

  /* Front face */

  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 width * 0.5f);
    }

  glEnd();

  /* Front sides of the teeth */

  glBegin(GL_QUADS);
  for (i = 0; i < teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
      glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da),
                 width * 0.5f);
      glVertex3f(r2 * cosf(angle + 2 * da), r2 * sinf(angle + 2 * da),
                 width * 0.5f);
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 width * 0.5f);
    }

  glEnd();

  glNormal3f(0.0f, 0.0f, -1.0f);

  /* Back face */

  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 -width * 0.5f);
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
    }

  glEnd();

  /* Back sides of the teeth */

  glBegin(GL_QUADS);
  for (i = 0; i < teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 -width * 0.5f);
      glVertex3f(r2 * cosf(angle + 2 * da), r2 * sinf(angle + 2 * da),
                 -width * 0.5f);
      glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da),
                 -width * 0.5f);
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
    }

  glEnd();

  /* Outward faces of the teeth */

  glBegin(GL_QUAD_STRIP);
  for (i = 0; i < teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
      glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
      u = r2 * cosf(angle + da) - r1 * cosf(angle);
      v = r2 * sinf(angle + da) - r1 * sinf(angle);
      len = sqrtf(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0f);
      glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da),
                 width * 0.5f);
      glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da),
                 -width * 0.5f);
      glNormal3f(cosf(angle), sinf(angle), 0.0f);
      glVertex3f(r2 * cosf(angle + 2 * da), r2 * sinf(angle + 2 * da),
                 width * 0.5f);
      glVertex3f(r2 * cosf(angle + 2 * da), r2 * sinf(angle + 2 * da),
                 -width * 0.5f);
      u = r1 * cosf(angle + 3 * da) - r2 * cosf(angle + 2 * da);
      v = r1 * sinf(angle + 3 * da) - r2 * sinf(angle + 2 * da);
      glNormal3f(v, -u, 0.0f);
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 width * 0.5f);
      glVertex3f(r1 * cosf(angle + 3 * da), r1 * sinf(angle + 3 * da),
                 -width * 0.5f);
      glNormal3f(cosf(angle), sinf(angle), 0.0f);
    }

  glVertex3f(r1, 0.0f, width * 0.5f);
  glVertex3f(r1, 0.0f, -width * 0.5f);
  glEnd();

  /* Inside radius cylinder */

  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++)
    {
      angle = i * 2.0f * M_PI / teeth;
      glNormal3f(-cosf(angle), -sinf(angle), 0.0f);
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
      glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
    }

  glEnd();
}

/****************************************************************************
 * Name: gears_init
 ****************************************************************************/

static void gears_init(int width, int height)
{
  static GLfloat pos[4] =
  {
    5.0f, 5.0f, 10.0f, 0.0f
  };

  static GLfloat red[4] =
  {
    0.8f, 0.1f, 0.0f, 1.0f
  };

  static GLfloat green[4] =
  {
    0.0f, 0.8f, 0.2f, 1.0f
  };

  static GLfloat blue[4] =
  {
    0.2f, 0.2f, 1.0f, 1.0f
  };

  static GLfloat white[4] =
  {
    1.0f, 1.0f, 1.0f, 1.0f
  };

  static GLfloat shininess = 5.0f;
  GLfloat h = (GLfloat)height / (GLfloat)width;

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -45.0f);

  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
  glLightfv(GL_LIGHT0, GL_SPECULAR, white);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  /* Build one display list per gear */

  g_gear1 = glGenLists(1);
  glNewList(g_gear1, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, red);
  glMaterialfv(GL_FRONT, GL_SPECULAR, white);
  glMaterialfv(GL_FRONT, GL_SHININESS, &shininess);
  glColor3fv(red);
  gear(1.0f, 4.0f, 1.0f, 20, 0.7f);
  glEndList();

  g_gear2 = glGenLists(1);
  glNewList(g_gear2, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, green);
  glMaterialfv(GL_FRONT, GL_SPECULAR, white);
  glColor3fv(green);
  gear(0.5f, 2.0f, 2.0f, 10, 0.7f);
  glEndList();

  g_gear3 = glGenLists(1);
  glNewList(g_gear3, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, blue);
  glMaterialfv(GL_FRONT, GL_SPECULAR, white);
  glColor3fv(blue);
  gear(1.3f, 2.0f, 0.5f, 10, 0.7f);
  glEndList();
}

/****************************************************************************
 * Name: gears_draw
 ****************************************************************************/

static void gears_draw(void)
{
  g_angle += 2.0f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glRotatef(g_view_rotx, 1.0f, 0.0f, 0.0f);
  glRotatef(g_view_roty, 0.0f, 1.0f, 0.0f);

  glPushMatrix();
  glTranslatef(-3.0f, -2.0f, 0.0f);
  glRotatef(g_angle, 0.0f, 0.0f, 1.0f);
  glCallList(g_gear1);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(3.1f, -2.0f, 0.0f);
  glRotatef(-2.0f * g_angle - 9.0f, 0.0f, 0.0f, 1.0f);
  glCallList(g_gear2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-3.1f, 4.2f, 0.0f);
  glRotatef(-2.0f * g_angle - 25.0f, 0.0f, 0.0f, 1.0f);
  glCallList(g_gear3);
  glPopMatrix();

  glPopMatrix();
}

/****************************************************************************
 * Name: tinygl_fb_open
 ****************************************************************************/

static int tinygl_fb_open(FAR const char *path, FAR struct tinygl_fb_s *fb)
{
  int ret;

  fb->fd = open(path, O_RDWR);
  if (fb->fd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: open(%s) failed: %d\n", path, ret);
      return ret;
    }

  if (ioctl(fb->fd, FBIOGET_VIDEOINFO,
            (unsigned long)((uintptr_t)&fb->vinfo)) < 0 ||
      ioctl(fb->fd, FBIOGET_PLANEINFO,
            (unsigned long)((uintptr_t)&fb->pinfo)) < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: FBIOGET_VIDEOINFO/PLANEINFO failed: %d\n",
              ret);
      goto errout;
    }

  if (fb->vinfo.fmt != TINYGL_FB_FMT)
    {
      fprintf(stderr, "ERROR: framebuffer format %d, TinyGL needs %d\n",
              fb->vinfo.fmt, TINYGL_FB_FMT);
      ret = -EINVAL;
      goto errout;
    }

  fb->fbmem = mmap(NULL, fb->pinfo.fblen, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_FILE, fb->fd, 0);
  if (fb->fbmem == MAP_FAILED)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: mmap() failed: %d\n", ret);
      goto errout;
    }

  return OK;

errout:
  close(fb->fd);
  return ret;
}

/****************************************************************************
 * Name: tinygl_fb_update
 ****************************************************************************/

static void tinygl_fb_update(FAR struct tinygl_fb_s *fb, FAR ZBuffer *zb)
{
  ZB_copyFrameBuffer(zb, fb->fbmem, fb->pinfo.stride);

#ifdef CONFIG_FB_UPDATE
  struct fb_area_s area;

  area.x = 0;
  area.y = 0;
  area.w = zb->xsize;
  area.h = zb->ysize;
  ioctl(fb->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)&area));
#endif
}

/****************************************************************************
 * Name: now_ms
 ****************************************************************************/

static uint32_t now_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   tinygl [frames] [fbdev]
 *
 *   Render the TinyGL gears demo on a framebuffer.  "frames" is the number
 *   of frames to draw (0, the default, runs forever) and "fbdev" the
 *   framebuffer device (CONFIG_EXAMPLES_TINYGL_DEFAULTFB by default).
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *fbdev = CONFIG_EXAMPLES_TINYGL_DEFAULTFB;
  struct tinygl_fb_s fb;
  FAR ZBuffer *zb;
  uint32_t start;
  uint32_t now;
  int frames = 0;
  int count = 0;
  int total = 0;
  int ret;

  if (argc > 1)
    {
      frames = atoi(argv[1]);
    }

  if (argc > 2)
    {
      fbdev = argv[2];
    }

  ret = tinygl_fb_open(fbdev, &fb);
  if (ret < 0)
    {
      return EXIT_FAILURE;
    }

  printf("tinygl: %s %dx%d, %d bpp, stride %" PRIu32 "\n", fbdev,
         fb.vinfo.xres, fb.vinfo.yres, fb.pinfo.bpp, fb.pinfo.stride);

  /* Let TinyGL render into its own buffer and copy each finished frame to
   * the framebuffer, so that a half drawn frame is never visible.
   */

  zb = ZB_open(fb.vinfo.xres, fb.vinfo.yres, TINYGL_ZB_MODE, NULL);
  if (zb == NULL)
    {
      fprintf(stderr, "ERROR: ZB_open() failed\n");
      ret = EXIT_FAILURE;
      goto errout_with_fb;
    }

  glInit(zb);
  gears_init(zb->xsize, zb->ysize);

  start = now_ms();
  while (frames == 0 || total < frames)
    {
      gears_draw();
      tinygl_fb_update(&fb, zb);

      count++;
      total++;
      now = now_ms();
      if (now - start >= 5000)
        {
          printf("tinygl: %d frames in %" PRIu32 " ms = %d.%d FPS\n",
                 count, now - start, count * 1000 / (int)(now - start),
                 (count * 10000 / (int)(now - start)) % 10);
          count = 0;
          start = now;
        }
    }

  printf("tinygl: rendered %d frames\n", total);

  glDeleteList(g_gear1);
  glDeleteList(g_gear2);
  glDeleteList(g_gear3);
  glClose();
  ZB_close(zb);
  ret = EXIT_SUCCESS;

errout_with_fb:
  munmap(fb.fbmem, fb.pinfo.fblen);
  close(fb.fd);
  return ret;
}
