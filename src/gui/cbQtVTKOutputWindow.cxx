/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cbQtVTKOutputWindow.cxx

  Copyright (c) 2016 David Gobbi
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of the Calgary Image Processing and Analysis Centre
    (CIPAC), the University of Calgary, nor the names of any authors nor
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/
#include "cbQtVTKOutputWindow.h"

#include <QtGlobal>

cbQtVTKOutputWindow *cbQtVTKOutputWindow::New()
{
  return new cbQtVTKOutputWindow;
}

cbQtVTKOutputWindow::cbQtVTKOutputWindow()
{
}

cbQtVTKOutputWindow::~cbQtVTKOutputWindow()
{
}

void cbQtVTKOutputWindow::Initialize()
{
}

void cbQtVTKOutputWindow::Install()
{
  cbQtVTKOutputWindow *win = cbQtVTKOutputWindow::New();
  vtkOutputWindow::SetInstance(win);
  win->Delete();
}

void cbQtVTKOutputWindow::DisplayText(const char* text)
{
  if (text)
    {
#if QT_VERSION < 0x050500
#if QT_NO_DEBUG_OUTPUT
    std::cerr << text << std::endl;
#else
    qDebug("%s", text);
#endif
#else
    qInfo("%s", text);
#endif
    }
}

void cbQtVTKOutputWindow::DisplayErrorText(const char* text)
{
  if (text)
    {
    qCritical("%s", text);
    }
}

void cbQtVTKOutputWindow::DisplayWarningText(const char* text)
{
  if (text)
    {
    qWarning("%s", text);
    }
}

void cbQtVTKOutputWindow::DisplayGenericWarningText(const char* text)
{
  if (text)
    {
    qWarning("%s", text);
    }
}

void cbQtVTKOutputWindow::DisplayDebugText(const char* text)
{
  if (text)
    {
    qDebug("%s", text);
    }
}

void cbQtVTKOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
