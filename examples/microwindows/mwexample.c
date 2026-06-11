#include <stdio.h>
#include <unistd.h>
#include <nuttx/compiler.h>
#include "device.h"

int main(int argc, FAR char *argv[])
{
    PSD psd = GdOpenScreen();
    if (!psd) {
        printf("GdOpenScreen failed\n");
        return 1;
    }

    GdSetForegroundColor(psd, MWRGB(255, 0, 0));
    GdFillRect(psd, 20, 20, 20, 20);

    sleep(5);

    GdCloseScreen(psd);
    return 0;
}
