/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/twm4nx_cursor.cxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <nuttx/nx/nxcursor.h>

#include "graphics/twm4nx/twm4nx_cursor.hxx"

#ifdef CONFIG_NX_SWCURSOR

/////////////////////////////////////////////////////////////////////////////
// Pulbic Data
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
#ifdef CONFIG_TWM4NX_CURSOR_LARGE
#  include "cursor-arrow1-30x30.h"
#  include "cursor-grab-25x30.h"
#  include "cursor-resize-30x30.h"
#  include "cursor-wait-23x30.h"
#else
#  include "cursor-arrow1-16x16.h"
#  include "cursor-grab-14x16.h"
#  include "cursor-resize-16x16.h"
#  include "cursor-wait-13x16.h"
#endif
}

#endif // CONFIG_NX_SWCURSOR
