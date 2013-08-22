/*=========================================================================
  Program: Cerebra
  Module:  vtkGViewFrame.h

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

#ifndef VTKGVIEWFRAME_H
#define VTKGVIEWFRAME_H

#include "vtkViewFrame.h"

class vtkViewPane;
class vtkViewObjectCollection;

//! Provides a method for defining view layout based on grid coordinates.
/*!
 *  Allows a user to define a collection of vtkViewObjects based on a grid
 *  coordinate system. However, a vtkViewObject may not span more than a single
 *  cell or column in the grid.
*/
class vtkGViewFrame : public vtkViewFrame
{
public:
  //! Provides a way to instantiate a vtkGViewFrame.
  static vtkGViewFrame *New();

  //! Standard VTK macro to allow for traversals of VTK class hierarchies.
  vtkTypeMacro(vtkGViewFrame, vtkViewFrame);

  //! Reimplementation of vtkViewFrame::AddChild().
  /*!
   *  Provides grid-specific functionality when adding children to the
   *  collection.
   *  \param child The vtkViewObject to add to the frame.
  */
  void AddChild(vtkViewObject *child);

  //! Updates the viewport of child at index to conform to a grid-based packing.
  /*!
   *  \param index Index of the child in the collection of vtkViewObjects.
  */
  void UpdateChildView(int index);

protected:
  vtkGViewFrame();
  ~vtkGViewFrame();

private:
  vtkGViewFrame(const vtkGViewFrame&); // Not implemented.
  void operator=(const vtkGViewFrame&); // Not implemented.

  //! Helper method to determine the 2-dimensional coordinate of the index.
  /*!
   *  \param index The index in the Children member.
   *  \param position The x,y coordinate pair to calculate and return.
  */
  void GetGridPositionFromIndex(int index, int position[2]) const;
};

#endif /* end of include guard: VTKGVIEWFRAME_H */
