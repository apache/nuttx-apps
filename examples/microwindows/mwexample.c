#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <nuttx/compiler.h>
#include "device.h"
#include "genfont.h"

#define TEXTBOX_H    60
#define RECT_SIZE    20
#define MAX_TEXT     256

static PSD psd;
static MWCOORD screen_w, screen_h;
static MWCOORD textbox_y;
static PMWCOREFONT cfont;
static char textbuf[MAX_TEXT];
static int textlen = 0;

static void draw_textbox(void)
{
    GdSetForegroundColor(psd, MWRGB(255, 255, 255));
    GdFillRect(psd, 0, textbox_y, screen_w, TEXTBOX_H);
    GdSetForegroundColor(psd, MWRGB(0, 0, 0));
    GdFillRect(psd, 0, textbox_y, screen_w, 2);
    GdFillRect(psd, 0, textbox_y + TEXTBOX_H - 2, screen_w, 2);

    if (textlen > 0) {
        GdSetForegroundColor(psd, MWRGB(0, 0, 0));
        GdSetBackgroundColor(psd, MWRGB(255, 255, 255));
        GdText(psd, (PMWFONT)cfont, 8, textbox_y + 8, textbuf, textlen, MWTF_ASCII | MWTF_TOP);
    }

    MWCOORD tw = 0, th = 0, tb = 0;
    GdGetTextSize((PMWFONT)cfont, textbuf, textlen, &tw, &th, &tb, MWTF_ASCII);
    GdSetForegroundColor(psd, MWRGB(0, 0, 0));
    GdFillRect(psd, 8 + tw, textbox_y + 8, 2, th > 0 ? th : 16);
}

static void draw_mouse_rect(MWCOORD x, MWCOORD y)
{
    int rx = x - RECT_SIZE / 2;
    int ry = y - RECT_SIZE / 2;
    if (rx < 0) rx = 0;
    if (ry < 0) ry = 0;
    if (rx + RECT_SIZE > screen_w) rx = screen_w - RECT_SIZE;
    if (ry + RECT_SIZE > textbox_y) ry = textbox_y - RECT_SIZE;
    GdSetForegroundColor(psd, MWRGB(255, 0, 0));
    GdFillRect(psd, rx, ry, RECT_SIZE, RECT_SIZE);
}

int main(int argc, FAR char *argv[])
{
    psd = GdOpenScreen();
    if (!psd) {
        printf("GdOpenScreen failed\n");
        return 1;
    }

    screen_w = psd->xres;
    screen_h = psd->yres;
    textbox_y = screen_h - TEXTBOX_H;

    cfont = psd->builtin_fonts;

    int mousefd = GdOpenMouse();
    if (mousefd < 0) {
        printf("GdOpenMouse failed, continuing without mouse\n");
        mousefd = -1;
    }

    int kbd_fd = GdOpenKeyboard();
    if (kbd_fd < 0) {
        printf("GdOpenKeyboard failed, continuing without keyboard\n");
        kbd_fd = -1;
    }

    GdSetForegroundColor(psd, MWRGB(0, 0, 0));
    GdFillRect(psd, 0, 0, screen_w, screen_h);

    draw_textbox();

    printf("Mouse+keyboard test. Click to draw, type to input, ESC to quit.\n");

    fd_set fds;
    struct timeval timeout;
    MWCOORD mx, my;
    int buttons, last_buttons = 0;
    int maxfd = 0;
    if (mousefd > maxfd) maxfd = mousefd;
    if (kbd_fd > maxfd) maxfd = kbd_fd;

    while (1) {
        FD_ZERO(&fds);
        if (mousefd >= 0) FD_SET(mousefd, &fds);
        if (kbd_fd >= 0) FD_SET(kbd_fd, &fds);

        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int ret = select(maxfd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0) {
            break;
        }
        if (ret == 0) {
            continue;
        }

        if (mousefd >= 0 && FD_ISSET(mousefd, &fds)) {
            if (GdReadMouse(&mx, &my, &buttons) > 0) {
                if (buttons && !last_buttons) {
                    draw_mouse_rect(mx, my);
                }
                last_buttons = buttons;
            }
        }

        if (kbd_fd >= 0 && FD_ISSET(kbd_fd, &fds)) {
            MWKEY key;
            MWKEYMOD mods;
            MWSCANCODE scancode;
            int kret = GdReadKeyboard(&key, &mods, &scancode);
            if (kret == KBD_KEYPRESS) {
                if (key == MWKEY_ENTER) {
                    break;
                }

                if (key == MWKEY_BACKSPACE) {
                    if (textlen > 0) {
                        textlen--;
                        textbuf[textlen] = '\0';
                        draw_textbox();
                    }
                } else if (key >= 32 && key < 127) {
                    if (textlen < MAX_TEXT - 1) {
                        textbuf[textlen++] = key;
                        textbuf[textlen] = '\0';
                        draw_textbox();
                    }
                }
            }
        }
    }

    if (kbd_fd >= 0) GdCloseKeyboard();
    if (mousefd >= 0) GdCloseMouse();
    GdCloseScreen(psd);
    return 0;
}