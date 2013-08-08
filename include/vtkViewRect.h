/*=========================================================================
  Program: Cerebra
  Module:  vtkViewRect.h

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

#ifndef VTKVIEWRECT_H
#define VTKVIEWRECT_H

#include "vtkViewObject.h"
#include <vector>

class vtkCamera;
class vtkToolCursor;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkDynamicViewFrame;

//! Container for other VTK view objects.
/*!
 *  Container for the other VTK view classes (vtkViewFrame and vtkViewPane).
 *  vtkViewRect wraps the vtkRenderWindow class and allows users to add any
 *  number of vtkViewFrames to the view layout of a render window.
 */
class vtkViewRect : public vtkViewObject
{
 public:
  //! Provides a way to instantiate a vtkViewRect.
  static vtkViewRect *New();

  //! Standard VTK macro to allow for traversals of VTK class hierarchies.
  vtkTypeMacro(vtkViewRect, vtkViewObject);

  //! Sets the main vtkViewFrame for the layout.
  /*!
   *  \param frame The frame to use as parent to all other frames.
  */
  void SetMainFrame(vtkDynamicViewFrame *frame);

  //! Starts the internal interactor.
  /*!
   *  Internally, this method updates the hierarchy of vtkRenderers,
   *  forces a render call on the internal vtkRenderWindow.
  */
  void Start();

  //! Forces a render on the internal vtkRenderWindow.
  void Render();

  //! Simple getter method for retrieving the internal vtkRenderWindow.
  vtkRenderWindow *GetRenderWindow() { return this->RenderWindow; }

  //! Returns the interactor for the render window.
  vtkRenderWindowInteractor *GetInteractor() { return this->Interactor; }

  //! Returns the vtkToolCursor associated for the pane at the coordinates.
  /*!
   *  Traverses child tree to discover which child pane contains the specific
   *  coordinates and returns that pane's associated vtkToolCursor.
   *  \return The tool cursor if found, NULL otherwise.
  */
  vtkToolCursor *RequestToolCursor(int x, int y);
  std::vector<vtkRenderer *> RequestRenderers();
  void UpdateAllBorders();

  vtkDynamicViewFrame *GetFrame() { return this->Frame; }

  //! Traversal of vtkViewObject children.
  /*!
   *  This method performs a traversal of all children VTK view-layout
   *  objects (vtkViewFrame, vtkViewPane) and updates all internal
   *  vtkViewPane viewports to be given global render window coordinates.
  */
  void UpdateRenderers();

  //! This functions allows a vtkViewRect to sychronize camera positions.
  void UpdateAllCamerasToMatch(vtkCamera *camera);

  //! Fixed or dynamic mourse cursor tracking. 
  void SetCursorTracking(bool en);
  bool GetCursorTracking() { return this->Tracked; }

 protected:
  vtkViewRect();
  ~vtkViewRect();

  //! Simple method to add a new vtkRenderer to the internal vtkRenderWindow.
  /*!
   *  \param ren The new renderer to add to the render window.
  */
  void AddRenderer(vtkRenderer *ren);

  //! Internal vtkRenderWindow to add all of the vtkRenderers to.
  vtkRenderWindow *RenderWindow;

  //! Internal vtkRenderWindowInteractor for the vtkRenderWindow.
  vtkRenderWindowInteractor *Interactor;

  //! Pointer to the parent frame of all added vtkViewFrames and vtkViewPanes.
  vtkDynamicViewFrame *Frame;
  
  bool Tracked;

 private:
  vtkViewRect(const vtkViewRect&); // Not implemented.
  void operator=(const vtkViewRect&); // Not implemented.

  //! Keypress callback to enable custom keyboard input.
  static void KeyPressCallbackFunction(vtkObject *caller,
                                       long unsigned int eventId,
                                       void *clientData,
                                       void *callData);
};

#endif /* end of include guard: VTKVIEWRECT_H */
