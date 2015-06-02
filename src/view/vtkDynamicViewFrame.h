/*=========================================================================
  Program: Cerebra
  Module:  vtkDynamicViewFrame.h

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

#ifndef VTKDYNAMICVIEWFRAME_H
#define VTKDYNAMICVIEWFRAME_H

#include "vtkViewObject.h"

class vtkViewFrame;
class vtkViewRect;

//! Facade for providing derived vtkViewFrame classes.
/*!
 *  This class provides a simple interface for creating dynamic layouts. When a
 *  user wishes to change the frame orientation using SetOrientation(..), the
 *  facade creates a new vtkViewFrame (either vtkGViewFrame, vtkVViewFrame,
 *  or vtkHViewFrame) and copies necessary data to the new frame, making the
 *  layout seem dynamic.
*/
class vtkDynamicViewFrame : public vtkViewObject
{
 public:
  //! Provides the ability to instantiate a vtkDynamicViewFrame object.
  static vtkDynamicViewFrame *New();

  //! Standard VTK macro to enable object hierarchy traversals.
  vtkTypeMacro(vtkDynamicViewFrame, vtkViewObject);

  //! Internal enumeration of possible layout orientations.
  enum Orientation { GRID, HORIZONTAL, VERTICAL };

  //! Allows a user to change the orientation of the frame.
  /*!
   *  Internally, this method creates a new instance of the desired orientation
   *  frame, and copies over necessary information to the new frame. It then
   *  replaces the old frame with the new orientation.
   *  \param o The orientation enum state to switch to (vtkDynamicViewFrame::GRID, vtkDynamicViewFrame::HORIZONTAL, vtkDynamicViewFrame::VERTICAL).
  */
  void SetOrientation(Orientation o);

  //! Message passing method to the underlying vtkViewFrame.
  void AddChild(vtkViewObject *object);

  //! Message passing method to the underlying vtkViewFrame.
  void SetViewRect(vtkViewRect *rect);

  //! Message passing method to the underlying vtkViewFrame.
  void UpdateChildView(int index);

  //! Returns the underlying vtkViewFrame.
  vtkViewFrame* GetUnderlyingFrame();

  //! Message passing method to the underlying vtkViewFrame.
  vtkViewObjectCollection *GetChildren();

 protected:
  vtkDynamicViewFrame();
  ~vtkDynamicViewFrame();

  //! The underlying frame for the facade.
  vtkViewFrame *Frame;

  //! The root vtkViewObject of the layout tree. Required for [Add|Remove]Renderer(..) in the underlying frame.
  vtkViewRect *Rect;

 private:
  vtkDynamicViewFrame(const vtkDynamicViewFrame&); // Not implemented.
  void operator=(const vtkDynamicViewFrame&); // Not implemented.
};


#endif /* end of include guard: VTKDYNAMICVIEWFRAME_H */
