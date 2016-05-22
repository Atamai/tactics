/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cbQtVTKOutputWindow.h

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

#ifndef cbQtVTKOutputWindow_h
#define cbQtVTKOutputWindow_h

#include "vtkOutputWindow.h"

//! Route VTK warning, error, and debug text to Qt.
/*!
 *  The purpose of this class is to cause VTK's debug, error, and warning
 *  macro output to be routed to the Qt message handler.
 */
class cbQtVTKOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(cbQtVTKOutputWindow, vtkOutputWindow);
  static cbQtVTKOutputWindow* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  virtual void DisplayDebugText(const char*);
  static void Install();

protected:
  cbQtVTKOutputWindow();
  virtual ~cbQtVTKOutputWindow();
  void Initialize();

private:
  cbQtVTKOutputWindow(const cbQtVTKOutputWindow&);  // Not implemented.
  void operator=(const cbQtVTKOutputWindow&);  // Not implemented.
};

#endif
