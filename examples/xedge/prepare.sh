#!/bin/bash
# This script clones the required repos and builds the Xedge resource file

# Clone BAS repo if not already present
if [ ! -d "BAS" ]; then
  git clone https://github.com/RealTimeLogic/BAS.git
fi

# Clone BAS-Resources repo if not already present
if [ ! -d "BAS-Resources" ]; then
  git clone https://github.com/RealTimeLogic/BAS-Resources.git
fi

# Build XedgeZip.c (Xedge resource file) only if it doesn't exist
if [ ! -f "BAS/examples/xedge/XedgeZip.c" ]; then
  echo "Building XedgeZip.c"
  cd BAS-Resources/build/ || exit 1
  printf "n\nl\nn\n" | bash Xedge.sh > /dev/null
  cp XedgeZip.c ../../BAS/examples/xedge/  || exit 1
fi
