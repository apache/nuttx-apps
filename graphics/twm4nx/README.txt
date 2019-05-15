README
======

Twm4Nx is a port of twm, Tab Window Manager (or Tom's Window Manager)
version 1.0.10 to NuttX NX windows server.  No, a port is not the right
word.  It is a re-design of TWM from the inside out to work with the NuttX
NX server.  The name Twm4Nx reflects this legacy.  But Twm4Nx is more a
homage to TWM than a port of TWM.

The original TWM was based on X11 which provides a rich set of features.
TWM provided titlebars, shaped windows, several forms of icon management,
user-defined macro functions, click-to-type and pointer-driven keyboard
focus, graphic contexts, and user-specified key and pointer button bindings,
etc.

Twm4Nx, on the other hand is based on the NuttX NX server which provides
comparatively minimal support.  Additional drawing support comes from
the NuttX NxWidgets library (which necessitated the change to C++).

Twm4Nx is greatly stripped down and targeted on small embedded systems
with minimal resources.  For example, no assumption is made about the
availability of a file system; no .twmrc file is used.  Bitmaps are not
used (other than for fonts).

The TWM license is, I believe compatible with the BSD license used by NuttX.
The origin TWM license required notice of copyrights within each file and
a full copy of the original license which you can find in the COPYING file.
within this directory.

STATUS
======
  2019-04-28:  This port was brutal.  Much TWM logic was removed because it
    depended on X11 features (or just because I could not understand how to
    use it).  The replacement logic is only mostly in place but more
    needs to be done to have a complete system (hence, it is marked
    EXPERIMENTAL).  The kinds of things that need to done are:

    1. Right click should bring up a window list (like the icon manager???)
    2. For TWM-like behavior, a window frame and toolbar should be highlighted
       when the window has focus.
    3. A right click on the toolbar should bring up a window-specific menu.
  2019-05-02:  Some testing progress.  The system comes up, connects to and
    initializes the VNC window.  For some reason, the VNC client breaks the
    connection.  The server is no longer connected so Twm4Nx constipates and
    and eventually hangs.
  2019-05-08:  I abandoned the VNC interface and found that things are much
    better using a direct, hardware framebuffer.  The background comes up
    properly and the Icon Manager appears properly in the upper rightthand
    corner.  The Icon Manager Window can be iconfified or de-inconified.
    The Icon Manager window can be grabbed by the toolbar title and moved
    about on the window (the movement is not very smooth on the particular
    hardware that I am working with).
  2019-05-10:  A left click on the background brings up the main menu.  At
    present there are only two options:  "Desktop" which will iconify all
    windows and "Twm4Nx Icon Manager" which will de-configy and/or raise
    the Icon Manager window to the top of the hierarchy.  That latter option
    is only meaningful when the desktop is very crowded.
  2019-05-13:  Added the NxTerm application.  If enabled via
    CONFIG_TWM4XN_NXTERM, there will now be a "NuttShell" entry in the Main
    Menu.  When pressed, this will bring up an NSH session in a Twm4Nx
    window.
  2019-05-14:  We can now move an icon on the desktop.  Includes logic to
    avoid collisions with other icons and with the background image.  That
    later is an issue.  The background image image widget needs to be
    removed; it can occlude a dektop icon.  We need to paint the image
    directly on the background without the use of a widget.
  2019-05-15:  Resizing now seems to work correctly in Twm4Nx.  It is still
    not usable, however.  When the window size is extended, the newly exposed
    regions must initialized with meaningul data. This is a problem in NX.
    Normally, NX generates a redraw callback whenever the application needs
    to redraw a region the the display.  However, with RAM-backed windows,
    all callbacks are suppressed.  That is okay in all cases except for a
    resize.  The missing callback leaves the new window region improperly
    initialized.

Issues:
    Here are all known issues and features that are missing:

    TWM Compatibilities Issues:
    1. Resizing works a little differently in Twm4Nx.
    2. Right click should bring up a window list
    3. For TWM-like behavior, a window frame and toolbar should be highlighted
       when the window has focus.
    4. A right click on the toolbar should bring up a window-specific menu.

    Other issues/bugs
    5. There is no calibration screen for touchscreen calibration.  I have
       been working with a system where the touchscreen naturally matches
       up with the display and no calibration is required.  But the general
       case requires calibration.
    6. Icon drag movement is fairly smooth, but I think there may be issues
       with the window drag movement.  it is hard to tell because of
       limitations in the touchscreen performance on the system that I am
       working with.
    7. Icon drag movement includes logic to avoid collisions with other
       icons and with the background image.  That later is an issue.  The
       background image image widget needs to be removed; it can occlude a
       dektop icon.  We need to paint the image directly on the background
       without the use of a widget.
    8. More issues with the background image:  It absorbs touchscreen
       presses without doing anything.  It should bring-up the main menu
       menu just as any other region of the background.  This would be easy
       to fix, but just replacing the background image widget is the better
       solution.
    9. The Icon Manager currently used the default window width.  That is
       set half of the display width which is okay for the display I am using,
       but it really needs to set a width that is appropriate for the number
       of columns and the size of a generic name string.

    Enhancement Ideas:
    10.  How about a full-screen mode?  This would just be a shortcut for
         the existing resize logic.
