# Interpreters

This `apps/` directory is set aside to hold interpreters that may be
incorporated into NuttX.

## Ficl

This is DIY port of Ficl (the _Forth Inspired Command Language_). See
http://ficl.sourceforge.net/. It is a _DIY_ port because the Ficl source is not
in that directory, only an environment and instructions that will let you build
Ficl under NuttX. The rest is up to you.

## Lua

Fetch and build a Lua interpreter. Versions 5.2 through 5.4 are supported. The
`lua` command will be added to NSH. Lua can run a script for a given path,
execute a string of code, or open a readline compatible REPL on the NSH console.
The `<lua.h>` and `<lauxlib.h>` headers are available to start a new embedded
interpreter or extend Lua with C modules. See the `luamod_hello` example for how
to include a built-in module.

A math library is required to build. Enable the `LIBM` config or use a
toolchain provided math library.

The following configs are recommended for a full featured Lua interpreter:
- `LIBC_FLOATINGPOINT`
- `SYSTEM_READLINE`

## Mini Basic

The Mini Basic implementation at `apps/interpreters` derives from version `1.0`
by Malcolm McLean, Leeds University, and was released under the Creative Commons
Attibution license. I am not legal expert, but this license appears to be
compatible with the NuttX BSD license see:
https://creativecommons.org/licenses/. I, however, cannot take responsibility
for any actions that you might take based on my understanding. Please use your
own legal judgement.
