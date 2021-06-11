/****************************************************************************
 * apps/include/graphics/nxwidgets/ctabpanel.hxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CTABPANEL_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CTABPANEL_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/tnxarray.hxx"
#include "graphics/nxwidgets/clatchbuttonarray.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  /**
   * Forward references
   */

  class CWidgetControl;
  class CRect;
  class CStickyButtonArray;

  /**
   * Tab panel, with tabs at the top and a panel at the bottom.
   */

  class CTabPanel : public CNxWidget, public CWidgetEventHandler
  {
  protected:
    TNxArray<CNxWidget*> m_tabpages;
    CLatchButtonArray *m_buttonbar;

    virtual void handleActionEvent(const CWidgetEventArgs &e);

    virtual void drawContents(CGraphicsPort* port) {}
    virtual void drawBorder(CGraphicsPort* port) {}

  public:
    CTabPanel(CWidgetControl *pWidgetControl, uint8_t numPages,
              nxgl_coord_t x, nxgl_coord_t y,
              nxgl_coord_t width, nxgl_coord_t height,
              nxgl_coord_t buttonHeight,
              FAR const CWidgetStyle *style = (FAR const CWidgetStyle *)NULL
             );

    inline CNxWidget &page(uint8_t index) { return *m_tabpages.at(index); }

    void setPageName(uint8_t index, const CNxString &name);

    void showPage(uint8_t index);

    uint8_t getCurrentPageIndex() const;
  };
}

#endif
#endif
