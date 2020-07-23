# Interpreters

This `apps/` directory is set aside to hold interpreters that may be
incorporated into NuttX.

## Ficl

This is DIY port of Ficl (the _Forth Inspired Command Language_). See
http://ficl.sourceforge.net/. It is a _DIY_ port because the Ficl source is not
in that directory, only an environment and instructions that will let you build
Ficl under NuttX. The rest is up to you.

## Mini Basic

The Mini Basic implementation at `apps/interpreters` derives from version `1.0`
by Malcolm McLean, Leeds University, and was released under the Creative Commons
Attibution license. I am not legal expert, but this license appears to be
compatible with the NuttX BSD license see:
https://creativecommons.org/licenses/. I, however, cannot take responsibility
for any actions that you might take based on my understanding. Please use your
own legal judgement.
