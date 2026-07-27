/****************************************************************************
 * apps/examples/microwindows/mwdemo_main.c
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
#include <time.h>
#include <stdlib.h>
#include <uni_std.h>
#include <windows.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

extern MWIMAGEHDR image_microwin;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

extern int RegisterAppClass(HINSTANCE hInstance);
extern HWND CreateAppWindow(void);

#define APPCLASS    "test"
#define APPCHILD    "test2"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  MSG msg;
  HWND hwnd;
  RECT rc;

  invoke_WinMain_Start(argc, argv);

  srandom(time(NULL));
  RegisterAppClass(0);
  GetWindowRect(GetDesktopWindow(), &rc);

  /* create penguin window */

  CreateWindowEx(0L, APPCHILD, "",
                 WS_BORDER | WS_VISIBLE,
                 rc.right - 130 - 1, rc.bottom - 153 - 1,
                 130, 153,
                 GetDesktopWindow(), (HMENU)1000, NULL, NULL);

  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();
  CreateAppWindow();

  /* set background wallpaper */

  MwSetDesktopWallpaper(&image_microwin);
  hwnd = CreateAppWindow();
  GetWindowRect(hwnd, &rc);
  OffsetRect(&rc, 50, 50);
  MoveWindow(hwnd, rc.left, rc.top,
             rc.bottom - rc.top,
             rc.right - rc.left, TRUE);

  while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  invoke_WinMain_End();

  return 0;
}
