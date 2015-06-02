/*=========================================================================
  Program: Cerebra
  Module:  vtkVViewFrame.h

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

#ifndef VTKVVIEWFRAME_H
#define VTKVVIEWFRAME_H

#include "vtkViewFrame.h"

class vtkViewPane;
class vtkViewObjectCollection;

//! Provides a convenient automatic vertical packing of child vtkViewObject objects.
class vtkVViewFrame : public vtkViewFrame
{
public:
  //! Provides a way to instantiate a vtkVViewFrame.
  static vtkVViewFrame *New();

  //! Standard VTK macro to allow for traversals of VTK class hierarchies.
  vtkTypeMacro(vtkVViewFrame, vtkViewFrame);

  //! Updates the viewport of child at index to conform to a vertical packing.
  /*!
   *  \param index Index of the child in the collection of vtkViewObjects.
  */
  void UpdateChildView(int index);

protected:
  vtkVViewFrame();
  ~vtkVViewFrame();

private:
  vtkVViewFrame(const vtkVViewFrame&); // Not implemented.
  void operator=(const vtkVViewFrame&); // Not implemented.
};

#endif /* end of include guard: VTKVVIEWFRAME_H */
