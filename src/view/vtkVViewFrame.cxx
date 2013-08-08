/*=========================================================================
  Program: Cerebra
  Module:  vtkVViewFrame.cxx

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

#include "vtkVViewFrame.h"
#include "vtkObjectFactory.h"
#include "vtkObject.h"
#include "vtkViewObjectCollection.h"

vtkStandardNewMacro(vtkVViewFrame);

vtkVViewFrame::vtkVViewFrame()
{
}

vtkVViewFrame::~vtkVViewFrame()
{
}

void vtkVViewFrame::UpdateChildView(int index)
{
  double view[4];
  this->GetViewport(view);
  double height = view[3] - view[1];
  vtkViewObject *child = this->Children->GetViewObject(index);

  double range[2];
  this->ComputeRangeForIndex(index, range);

  // use (1.0 - range) to pack top-to-bottom instead of bottom-to-top
  double childview[4] = {
    view[0],
    view[1] + (1.0 - range[1]) * height,
    view[2],
    view[1] + (1.0 - range[0]) * height,
  };

  child->SetViewport(childview);
}
