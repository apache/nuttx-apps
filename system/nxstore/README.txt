nxstore - an LVGL touchscreen app store for NuttX
=================================================

  nxstore is a graphical front-end for system/nxpkg.  It lists the
  packages available in a nxpkg repository, installs the one you tap via
  nxpkg, launches it, and supervises it with an on-screen "Close"
  control.  It is the touchscreen equivalent of driving the nxpkg CLI by
  hand; both consume the same repository, so the repository/server setup
  (layout, how to serve it, populating it with tools/export_pkg_repo.py,
  pointing the board at it) is documented once, in system/nxpkg's
  README.txt, and is not repeated here.


Dependencies
============

  Kconfig: SYSTEM_NXSTORE depends on GRAPHICS_LVGL and SYSTEM_NXPKG and
  selects NETUTILS_CJSON.  It needs a working framebuffer (/dev/fb0) and
  a touch input device (/dev/input0) - the same graphics/input stack
  examples/lvgldemo uses.  NETUTILS_CJSON is used to parse the
  repository index and package manifests.


On-device lifecycle
===================

  1. Browse   - the app list is built from the repository index nxpkg
                fetched.  Installed packages show a launch action;
                not-yet-installed ones show an install action.
  2. Install  - tapping an uninstalled package runs the nxpkg install
                flow (download -> sha256 verify -> transactional
                install) on a background worker thread, with progress
                shown as a toast.
  3. Launch   - tapping an installed package spawns its payload
                (posix_spawn) and switches to the "running" screen: a
                thin bar across the top NXSTORE_BAR_HEIGHT pixels (see
                include/system/nxstore_chrome.h) carrying the app name
                and a Close button, with the rest of the framebuffer
                left to the running app.
  4. Close    - see "Closing a running app" below.

  Launched apps are given NXSTORE_LAUNCH_STACKSIZE (32 KiB) of stack -
  posix_spawn's default (CONFIG_POSIX_SPAWN_DEFAULT_STACKSIZE, 2 KiB) is
  far too small for a real LVGL/framebuffer app, and overflowing it does
  not fail loudly (it corrupts adjacent heap, surfacing later as a crash
  nowhere near the real cause).


Closing a running app: the cooperative-SIGTERM contract
=======================================================

  This is the one thing an app author MUST get right to be launchable
  from nxstore.

  The Close button sends the running app SIGTERM and waits for it to
  reap itself.  It does NOT (and cannot safely) force-kill the task:

    - On a build with CONFIG_SIG_DEFAULT disabled (the default), NuttX
      has no default action for SIGTERM at all.  Delivery does nothing
      unless the app installs its own handler with sigaction().
    - An earlier nxstore fell back to task_delete() when SIGTERM went
      unreaped.  On real hardware that force-kill landed while the app
      was mid framebuffer/heap access and hung the *entire board* (not
      just the task), requiring a physical power cycle.  That fallback
      was removed: there is no safe way to force-terminate an arbitrary
      task from the outside here.

  So an nxstore-launchable app must cooperate:

    - Install an async-signal-safe SIGTERM handler that only sets a
      volatile sig_atomic_t flag.
    - Poll that flag from its own main loop and exit cleanly (freeing
      the framebuffer, restoring input state, etc.) when it is set.

  Apps whose main loop can never block indefinitely (they run to
  completion on their own, or fail fast at startup) do not strictly need
  a handler, but any app that renders in a long-lived loop does.  See
  these in-tree examples for the exact pattern:

    - games/NXDoom      (i_install_quit_signal / i_poll_quit_signal)
    - examples/calculator
    - games/cgol, games/brickmatch

  If SIGTERM is not reaped within the timeout, nxstore keeps the running
  screen up rather than returning to the app list - switching back while
  the app might still be alive and drawing into the framebuffer would
  just reproduce the original hang, hidden.


Configuration
=============

  SYSTEM_NXSTORE_PROGNAME    program/registration name  (default nxstore)
  SYSTEM_NXSTORE_PRIORITY    task priority              (default 100)
  SYSTEM_NXSTORE_STACKSIZE   nxstore's own stack        (default 8192)

  NXSTORE_LAUNCH_STACKSIZE (the stack given to launched apps) and
  NXSTORE_BAR_HEIGHT (the reserved top bar) are compile-time constants in
  the source / include/system/nxstore_chrome.h rather than Kconfig
  options.
