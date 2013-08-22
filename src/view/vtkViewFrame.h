/*=========================================================================
  Program: Cerebra
  Module:  vtkViewFrame.h

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

#ifndef VTKVIEWFRAME_H
#define VTKVIEWFRAME_H

#include "vtkObjectFactory.h"
#include "vtkViewObject.h"
#include "vtkViewPane.h"
#include <vector>

class vtkViewRect;
class vtkViewObjectCollection;

//! Pure abstract class for the different frame orientation classes.
class vtkViewFrame : public vtkViewObject
{
 public:
  //! Standard VTK macro to allow for traversals of VTK class hierarchies.
  vtkTypeMacro(vtkViewFrame, vtkViewObject);

  //! General method for adding a child vtkViewObject to the frame.
  /*!
   *  Overriden in vtkGViewFrame to provide grid-based adding of children.
   *  \param child The child vtkViewObject to add to the frame.
   *  \sa vtkGViewFrame
  */
  virtual void AddChild(vtkViewObject *child);

  //! Simple getter for retrieving the collection of children on this frame.
  vtkViewObjectCollection *GetChildren() { return this->Children; }

  //! Simple set method to give the frame access to the vtkViewRect.
  void SetViewRect(vtkViewRect *rect) { this->Rect = rect; }

  //! Returns the current size of the grid.
  int GetCurrentSquare() const { return this->CurrentSquare; }

  //! Returns the current number of rows in the grid.
  int GetGridRows() const { return this->GridRows; }

  //! Returns the current number of columns in the grid.
  int GetGridColumns() const { return this->GridColumns; }

  //! Returns the parent vtkViewRect.
  vtkViewRect *GetViewRect() { return this->Rect; }

  //! Determines if the grid dimensions need to be updated, and updates them.
  void UpdateGridDimensions();

  //! Fills the collection with dummy panes if gridsize > panes.
  void FillDummies();

 protected:
  vtkViewFrame();
  ~vtkViewFrame();

  //! Virtual method for updating the viewport of each child based on the index of the child in the collection.
  /*!
   *  Implemented in the inherited classes (vtkHViewFrame, vtkVViewFrame,
   *  and vtkGViewFrame) to provide specific functionality for that frame type.
   *  \param index The index of the child in the collection.
  */
  virtual void UpdateChildView(int index) = 0;

  //! Protected function to compute view fractions for given item.
  void ComputeRangeForIndex(int index, double range[2]);

  //! Performs a deep copy of the other frame's Children collection.
  void DeepCopy(vtkViewFrame *other);

  //! Collection of children for this vtkViewFrame.
  vtkViewObjectCollection *Children;

  //! Pointer to the outer vtkViewRect. Required for adding to the render window
  vtkViewRect *Rect;

  //! Maintains knowledge of the current grid size
  int CurrentSquare;

  //! Maintains knowledge of the current grid columns
  int GridColumns;

  //! Maintains knowledge of the current grid rows
  int GridRows;

  //! Maintains knowledge of the current number of dummy panes
  int Dummies;

 private:
  vtkViewFrame(const vtkViewFrame&); // Not implemented.
  void operator=(const vtkViewFrame&); // Not implemented.

  //! vtkViewRect is classified as a friend to give access to the UpdateChildView method.
  friend class vtkViewRect;

  //! vtkDynamicViewFrame is classified as a friend because it is a facade, and not derived from vtkViewFrame.
  friend class vtkDynamicViewFrame;
};

#endif /* end of include guard: VTKVIEWFRAME_H */
