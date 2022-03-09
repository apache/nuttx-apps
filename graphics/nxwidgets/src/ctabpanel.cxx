/****************************************************************************
 * apps/graphics/nxwidgets/src/ctabpanel.hxx
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/ctabpanel.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * CTabPanel Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

CTabPanel::CTabPanel(CWidgetControl *pWidgetControl, uint8_t numPages,
                     nxgl_coord_t x, nxgl_coord_t y,
                     nxgl_coord_t width, nxgl_coord_t height,
                     nxgl_coord_t buttonHeight,
                     FAR const CWidgetStyle *style
                    ):
  CNxWidget(pWidgetControl, x, y, width, height, 0, style)
{
  m_buttonbar = new CLatchButtonArray(pWidgetControl, x, y,
                                      numPages, 1,
                                      width / numPages,
                                      buttonHeight,
                                      0);
  m_buttonbar->addWidgetEventHandler(this);
  this->addWidget(m_buttonbar);

  for (int i = 0; i < numPages; i++)
    {
      CNxWidget *tabpage = new CNxWidget(pWidgetControl, x, y + buttonHeight,
                                         width, height - buttonHeight, 0);
      tabpage->setBackgroundColor(getBackgroundColor());
      tabpage->setBorderless(true);
      m_tabpages.push_back(tabpage);
      this->addWidget(tabpage);
    }

  // Activate the first page

  showPage(0);
}

void CTabPanel::setPageName(uint8_t index, const CNxString &name)
{
  m_buttonbar->setText(index, 0, name);
}

void CTabPanel::showPage(uint8_t index)
{
  if (!m_buttonbar->isThisButtonStuckDown(index, 0))
    {
      m_buttonbar->stickDown(index, 0);
    }

  for (int i = 0; i < m_tabpages.size(); i++)
    {
      if (i != index)
        {
          m_tabpages.at(i)->hide();
          m_tabpages.at(i)->disable();
        }
    }

  m_tabpages.at(index)->enable();
  m_tabpages.at(index)->show();
}

void CTabPanel::handleActionEvent(const CWidgetEventArgs &e)
{
  if (e.getSource() == m_buttonbar)
    {
      int x = 0;
      int y = 0;

      m_buttonbar->isAnyButtonStuckDown(x, y);
      showPage(x);
      m_widgetEventHandlers->raiseActionEvent();
    }
}

uint8_t CTabPanel::getCurrentPageIndex() const
{
  int x = 0;
  int y = 0;
  m_buttonbar->isAnyButtonStuckDown(x, y);
  return x;
}
