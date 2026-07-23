
/****************************************************************************
 * apps/examples/microwindows/mwdemo_main.c
 *
 * NuttX entry point for mwdemo (Microwindows Win32 API demo)
 ****************************************************************************/

#include <nuttx/config.h>
#include "windows.h"

extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                          LPSTR lpCmdLine, int nShowCmd);

int main(int argc, FAR char *argv[])
{
  int ret;

  invoke_WinMain_Start(argc, (char **)argv);

  ret = WinMain(0, NULL, (LPSTR) (argc > 0 ? argv[0] : ""), 0);

  invoke_WinMain_End();

  return ret;
}
