/****************************************************************************
 * apps/examples/microwindows/mwdemo.c
 *
 * Copyright (c) 1999, 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 *
 * Demo program for Microwindows
 *
 *  GB: 10-14-2004: Modified to store degrees data on each window,
 *                  for new Timers features.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <uni_std.h>
#define MWINCLUDECOLORS
#include <windows.h>
#include <device.h>
#include <graph3d.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  int startdegrees;
  int enddegrees;
} demoWndData;

typedef demoWndData *pdemoWndData;

/****************************************************************************
 * Private Data
 ****************************************************************************/

extern MWIMAGEHDR image_penguin;
PMWIMAGEHDR g_image = &image_penguin;

#define APPCLASS    "test"
#define APPCHILD    "test2"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp);

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int MwUserInit(int ac, char **av)
{
  /* test user init procedure - do nothing */

  return 0;
}

int RegisterAppClass(HINSTANCE hInstance)
{
  WNDCLASS wc;

  /* register builtin controls and dialog classes */

  MwInitializeDialogs(hInstance);

  wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = (WNDPROC) WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = sizeof(LONG_PTR);
  wc.hInstance = 0;
  wc.hIcon = 0;                 /* LoadIcon(GetHInstance(), MAKEINTRESOURCE(
                                 * 1)); */
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = APPCLASS;
  RegisterClass(&wc);

  wc.lpfnWndProc = (WNDPROC) ChildWndProc;
  wc.lpszClassName = APPCHILD;
  return RegisterClass(&wc);
}

HWND CreateAppWindow(void)
{
  HWND hwnd;
  static int nextid = 1;
  int width;
  int height;
  RECT r;
  GetWindowRect(GetDesktopWindow(), &r);
  width = height = r.right / 2;

  hwnd = CreateWindowEx(0L, APPCLASS,
                        "Microwindows Application",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        width, height,
                        NULL, (HMENU)(LONG_PTR)nextid++, NULL, NULL);

  if (hwnd && (nextid & 03) != 2)
    {
      CreateWindowEx(0L, APPCHILD,
                     "",
                     WS_BORDER | WS_CHILD | WS_VISIBLE,
                     4, 4, width / 3 - 6, height / 3,
                     hwnd, (HMENU)2, NULL, NULL);
      CreateWindowEx(0L, APPCHILD,
                     "",
                     WS_BORDER | WS_CHILD | WS_VISIBLE,
                     width / 3, height / 3,
                     width / 3 - 6, height / 3,
                     hwnd, (HMENU)3, NULL, NULL);
      CreateWindowEx(0L, APPCHILD,
                     "",
                     WS_BORDER | WS_CHILD | WS_VISIBLE,
                     width * 3 / 5, height * 3 / 5,
                     width * 2 / 3, height * 2 / 3,
                     hwnd, (HMENU)4, NULL, NULL);
      CreateWindowEx(0L, "EDIT",
                     "OK",
                     WS_BORDER | WS_CHILD | WS_VISIBLE,
                     width * 5 / 8, 10,
                     100, 18,
                     hwnd, (HMENU)5, NULL, NULL);
      CreateWindowEx(0L, "PROGBAR",
                     "OK",
                     WS_BORDER | WS_CHILD | WS_VISIBLE,
                     width * 5 / 8, 32,
                     100, 18,
                     hwnd, (HMENU)6, NULL, NULL);

      HWND hlist = CreateWindowEx(0L, "LISTBOX",
                                  "OK",
                                  WS_HSCROLL | WS_VSCROLL |
                                  WS_BORDER | WS_CHILD |
                                  WS_VISIBLE,
                                  width * 5 / 8, 54,
                                  100, 48,
                                  hwnd, (HMENU)7, NULL, NULL);
      SendMessage(hlist, LB_ADDSTRING, 0,
                  (LPARAM)(LPSTR)"Cherry");
      SendMessage(hlist, LB_ADDSTRING, 0,
                  (LPARAM)(LPSTR)"Apple");
      SendMessage(hlist, LB_ADDSTRING, 0,
                  (LPARAM)(LPSTR)"Orange");

      CreateWindowEx(0L, "BUTTON",
                     "Cancel",
                     BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
                     width * 5 / 8 + 50, 106,
                     50, 14,
                     hwnd, (HMENU)8, NULL, NULL);
    }

  return hwnd;
}

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  RECT rc;
  PAINTSTRUCT ps;

  switch (msg)
    {
    case WM_PAINT:
      BeginPaint(hwnd, &ps);
      GetClientRect(hwnd, &rc);

      HDC hdcMem;
      HBITMAP hbmp, hbmpOrg;

      /* redirect painting to offscreen dc, then use blit function */

      hdcMem = CreateCompatibleDC(ps.hdc);

      /* Note: rc.right, rc.bottom happens to be smaller than image
       * width/height.  We use the image size, so we can stretchblit
       * from the whole image.
       */

      hbmp = CreateCompatibleBitmap(hdcMem, g_image->width,
                                    g_image->height);
      hbmpOrg = SelectObject(hdcMem, hbmp);

      /* draw onto offscreen dc */

      DrawDIB(hdcMem, 0, 0, g_image);

      /* blit offscreen with physical screen */

      StretchBlt(ps.hdc, 0, 0, rc.right * 4 / 5,
                 rc.bottom * 4 / 5, hdcMem,
                 0, 0, g_image->width, g_image->height, MWROP_COPY);
      DeleteObject(SelectObject(hdcMem, hbmpOrg));
      DeleteDC(hdcMem);
      EndPaint(hwnd, &ps);
      break;
    default:
      return DefWindowProc(hwnd, msg, wp, lp);
    }

  return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rc;
  static int countup = 1;
  int id;
  static vec1 gx;
  static vec1 gy;
  static POINT mousept;
  pdemoWndData pData;

  switch (msg)
    {
    case WM_CREATE:
      pData = (pdemoWndData) malloc(sizeof(demoWndData));
      SetWindowLongPtr(hwnd, 0, (LONG_PTR) pData);
      mousept.x = 60;
      mousept.y = 20;
      pData->startdegrees = 0;
      pData->enddegrees = 30;
      SetTimer(hwnd, 1, (50 + random() % 250), NULL);
      break;

    case WM_TIMER:
      pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
      GetClientRect(hwnd, &rc);
      if (countup)
        {
          mousept.y += 20;
          if (mousept.y >= rc.bottom)
            {
              mousept.y -= 20;
              countup = 0;
            }
        }
      else
        {
          mousept.y -= 20;
          if (mousept.y < 20)
            {
              mousept.y += 20;
              countup = 1;
            }
        }

      SendMessage(hwnd, WM_MOUSEMOVE, 0,
                  MAKELONG(mousept.x, mousept.y));
      break;
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
      free(pData);
      SetWindowLongPtr(hwnd, 0, (LONG_PTR)0);
      break;
    case WM_SIZE:
      break;
    case WM_MOVE:
      break;

    case WM_ERASEBKGND:
      if ((GetWindowLong(hwnd, GWL_ID) & 03) == 1)
        {
          return 1;
        }

      return DefWindowProc(hwnd, msg, wp, lp);
    case WM_PAINT:
      hdc = BeginPaint(hwnd, &ps);
      id = (int)GetWindowLong(hwnd, GWL_ID) & 03;
      init3(hdc, id == 1 ? hwnd : NULL);
      switch (id)
        {
        case 0:
          rose(1.0, 7, 13);
          break;
        case 1:
          look3(-2 * gx, -2 * gy, 1.2);
          drawgrid(-8.0, 8.0, 10, -8.0, 8.0, 10);
          break;
        case 2:
          setcolor3(BLACK);
          circle3(1.0);
          break;
        case 3:
          setcolor3(BLUE);
          daisy(1.0, 20);
          break;
        }

      paint3(hdc);
      EndPaint(hwnd, &ps);
      break;

    case WM_MOUSEMOVE:
      pData = (pdemoWndData)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
      if ((GetWindowLong(hwnd, GWL_ID) & 03) == 1)
        {
          POINT pt;

          POINTSTOPOINT(pt, lp);
          GetClientRect(hwnd, &rc);
          gx = (vec1) pt.x / rc.right;
          gy = (vec1) pt.y / rc.bottom;
          InvalidateRect(hwnd, NULL, FALSE);
          mousept.x = pt.x;
          mousept.y = pt.y;
        }
      break;
    case WM_LBUTTONDOWN:
      break;
    case WM_LBUTTONUP:
      break;
    case WM_RBUTTONDOWN:
      break;
    default:
      return DefWindowProc(hwnd, msg, wp, lp);
    }

  return 0;
}
