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

Progress:
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
    properly and the Icon Manager appears properly in the upper right hand
    corner.  The Icon Manager Window can be iconified or de-iconified.
    The Icon Manager window can be grabbed by the toolbar title and moved
    about on the window (the movement is not very smooth on the particular
    hardware that I am working with).
  2019-05-10:  A left click on the background brings up the main menu.  At
    present there are only two options:  "Desktop" which will iconify all
    windows and "Twm4Nx Icon Manager" which will de-iconify and/or raise
    the Icon Manager window to the top of the hierarchy.  That latter option
    is only meaningful when the desktop is very crowded.
  2019-05-13:  Added the NxTerm application.  If enabled via
    CONFIG_TWM4XN_NXTERM, there will now be a "NuttShell" entry in the Main
    Menu.  When pressed, this will bring up an NSH session in a Twm4Nx
    window.
  2019-05-14:  We can now move an icon on the desktop.  Includes logic to
    avoid collisions with other icons and with the background image.  That
    later is an issue.  The background image image widget needs to be
    removed; it can occlude a desktop icon.  We need to paint the image
    directly on the background without the use of a widget.
  2019-05-15:  Resizing now seems to work correctly in Twm4Nx.
  2019-05-20:  Calibration screen is now in place.
  2019-05-21:  A "CONTEMPORARY" theme was added.  Still has a few glitches.

How To:

  Icon Manager
    - At start up, only the Icon Manager window is shown.  The Icon Manager
      is a TWM alternative to more common desktop icons.  Currently Twm4Nx
      supports both desktop icons and the Icon Manager.

      Whenever a new application is started from the Main Menu, its name
      shows up in the Icon Manager.  Selecting the name will either de-
      iconify the window, or just raise it to the top of the display.

  Main Menu:
    - A touch/click at any open location on the background (except the
      image at the center or on another icon) will bring up the Main Menu.
      Options:

      o Desktop.  Iconify all windows and show the desktop
      o Twm4Nx Icom Manager.  De-iconify and/or raise the Icon Manager to
        the top of the display.
      o Calibration.  Perform touchscreen re-calibration.
      o NuttShell.  Start and instance of NSH running in an NxTerm.

    - All windows close after the terminal menu option is selected.

  Window Toolbar
    - Most windows have a toolbar at the top.  It is optional but used
      in most windows.
    - The toolbar contains window title and from zero to 4 buttons:

      o Right side:  A menu button may be presented.  The menu button
        is not used by anything in the current implementation and is
        always suppressed
      o Left side:  The far left is (1)the terminate button (if present).
        If present, it will close window when selected.  Not all windows can
        be closed.  You can't close the Icon Manager or menu windows, for
        example.  Then (2) a resize button.  If presented and is selected,
        then the resize sequence described below it started.  This may
        the be preceded by a minimize button that iconifies the window.

  Moving a Window:
    - Grab the title in the toolbar and move the window to the desired
      position.

  Resizing a Window:
    - A window must have the green resize button with the square or it
      cannot be resized.
    - Press resize button.  A small window should pop-up in the upper
      left hand corner showing the current window size.
    - Touch anywhere in window (not the toolbar) and slide your finger.
      The resize window will show the new size but there will be no other
      update to the display.  It is thought that continuous size updates
      would overwhelm lower end MCUs.  Movements support include:

      o Move toward the right increases the width of the window
      o Move toward the left decreases the width of the window
      o Move toward the bottom increases the height of the window
      o Move toward the top decreases the height of the Window
      o Other moves will affect both the height and width of the window.

    - NOTE:  While resizing, non-critical events from all other windows
      are ignored.

  Themes
    - There are two themes support by the configuration system:
      o CONFIG_TWM4NX_CLASSIC.  Strong bordered windows with dark primary
        colors.  Reminiscent of Windows 98.
      o CONFIG_TWM4NX_CONTEMPORARY.  Border-less windows in pastel shades
        for a more contemporary look.

Issues:

    2019-05-16:
      Twm4Nx is in a very complete state but only at perhaps "alpha" in its
      maturity.  You should expect to see some undocumented problems.  If
      you see such problems and can describe a sequence to actions to
      reproduce the problem, let me know and I will try to resolve the
      problems.

    Here are all known issues and features that are missing:

    TWM Compatibilities Issues:
    1. Resizing works a little differently in Twm4Nx.
    2. Right click should bring up a window list
    3. For TWM-like behavior, a window frame and toolbar should be highlighted
       when the window has focus.
    4. A right click on the toolbar should bring up a window-specific menu.

    Other issues/bugs
    5. Icon drag movement includes logic to avoid collisions with other
       icons and with the background image.  That later is an issue.  We
       need to paint the image directly on the background without the
       use of a widget.
    6. There are a few color artifacts in the toolbar of the CONTEMPORARY
       theme.  These look like borders are being drawn around the toolbar
       widgets (even though the are configured to be borderless).
    7. Most Twm4Nx configuration settings are hard-coded in *_config.hxx header
       files.  These all need to be brought out and made accessible via Kconfig
       files
    8. Things become buggy after perhaps 10 shell windows have been opened.
       Most likely, some resource allocation is failing silently and leaving
       things in a bad state.  The board I am using has 128Mb of SDRAM so I
       can't believe that memory is the limiting factor.  These are, however,
       RAM-backed windows and will use significant amounts of memory.

