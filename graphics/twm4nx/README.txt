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
