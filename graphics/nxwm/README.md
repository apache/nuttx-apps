# Graphics / `nxwm` NxWM

This directory holds a tiny desktop for small embedded devices with a
touchscreen,. NxWM. NxWM is true multiple window manager but only one window is
displayed at a time. This simplification helps performance on LCD based products
(in the same way that a tiled window manager helps) and also makes the best use
of small displays. It is awkward from a human factors point-of-view trying to
manage multiple windows on a small display.

The window manager consists of a task bar with icons representing the running
tasks. If you touch the task's icon, it comes to the top. Each window has a
toolbar with (1) a title, (2) a minimize button, and (3) a stop application
button using the standard icons for these things.

There is always a start window that is available in the task bar. When you touch
the start window icon, it brings up the start window containing icons
representing all of the available applications. If you touch an icon in the
start window, it will be started and added to the task bar.

There is a base class that defines an add-on application and an interface that
supports incorporation of new application. The only application that is provided
is NxTerm. This is an  NSH session running in a window. You should be able to
select the NX icon in the start menu and create as many NSH sessions in windows
as you want. (keybard input still comes through serial).

Note 1: NwWM requires `NuttX-7.19` or above to work with the current
`NxWidgets-1.18` release.
