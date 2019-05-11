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

    1. Update some logic that is only fragmentary for things like resizing.
       Resizing events should be be generated when user pulls to right,
       left, top, bottom, etc.  None of that is implemented.
    2. Right click should bring up a window list (like the icon manager???)
    3. For TWM-like behavior, a window frame and toolbar should be highlighted
       when the window has focus.
    4. A right click on the toolbar should bring up a window-specific menu.
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
  2019-05-10: A left click on the background brings up the main menu.  At
    present there are only two options:  "Desktop" which will iconify all
    windows and "Twm4Nx Icon Manager" which will de-configy and/or raise
    the Icon Manager window to the top of the hierarchy.  That latter option
    is only meaningful when the desktop is very crowded.

    Further progress depends upon getting a some additional applications
    in place in the main menu in place.  NxTerm is needed as is probably
    a clock.  These would provide good illustrations of how to hook in an
    arbitrary application.

    Some known bugs yet-to-fixed.  Surely there are more as will be revealed
    by additional testing:

    1. The is a small artifact in the upper lefthand corner.  I am not sure
       exactly what that is.
    2. The logic to move an icon on the desk top does not work.
    3. There is no calibration screen for touchscreen calibration.
