/*=========================================================================
  Program: Cerebra
  Module:  vtkViewPane.h

  Copyright (c) 2011-2013 David Adair
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

#ifndef VTKVIEWPANE_H
#define VTKVIEWPANE_H

#include "vtkViewObject.h"

class vtkToolCursor;
class vtkProp;
class vtkRenderer;

//! Wrapper class for vtkRenderer.
/*!
 *  Provides a common interface for the other VTK layout classes
 *  (vtkViewRect and vtkViewFrame) and the developer using the classes to
 *  add multiple vtkRenderers to a render scene.
 */
class vtkViewPane : public vtkViewObject
{
 public:
  //! Standard VTK macro to allow for traversals of VTK class hierarchies.
  vtkTypeMacro(vtkViewPane, vtkViewObject);

  //! Retrieves the internal vtkRenderer.
  /*!
   *  Simple getter method that retrieves the object's internal
   *  vtkRenderer member.
  */
  vtkRenderer *GetRenderer() { return this->Renderer; }

  //! Updates the object's viewport.
  void UpdateViewport(const double view[4]);

  //! Returns the vtkToolCursor for this pane.
  vtkToolCursor *GetToolCursor() { return this->ToolCursor; }

  //! Determines if the pane contains the provided coordinates.
  bool Contains(double x, double y);
  
  //! Determines whether the cursor shape changes or not.  
  void SetCursorTracking(bool en);
  bool GetCursorTracking() { return this->CursorTracking; }

  void SetBorderEnabled(bool en) { this->BorderEnabled = en; }
  bool GetBorderEnabled() { return this->BorderEnabled; }

  void AddBorder();

 protected:
  vtkViewPane();
  ~vtkViewPane();

  //! Internal vtkRenderer to connect to the VTK rendering pipeline.
  vtkRenderer *Renderer;

  //! Internal vtkToolCursor to control interaction of the render scene.
  vtkToolCursor *ToolCursor;
  
  //! Enable or disable cursor tracking
  bool CursorTracking;

  bool BorderEnabled;

 private:
  vtkViewPane(const vtkViewPane&); // Not implemented.
  void operator=(const vtkViewPane&); // Not implemented.
};

#endif /* end of include guard: VTKVIEWPANE_H */
