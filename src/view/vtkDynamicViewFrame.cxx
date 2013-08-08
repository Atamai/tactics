/*=========================================================================
  Program: Cerebra
  Module:  vtkDynamicViewFrame.cxx

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

#include <assert.h>
#include <queue>

#include "vtkViewRect.h"
#include "vtkRenderer.h"
#include "vtkViewPane.h"
#include "vtkViewFrame.h"
#include "vtkViewObjectCollection.h"
#include "vtkDynamicViewFrame.h"
#include "vtkGViewFrame.h"
#include "vtkHViewFrame.h"
#include "vtkObjectFactory.h"
#include "vtkVViewFrame.h"

vtkStandardNewMacro(vtkDynamicViewFrame);

vtkDynamicViewFrame::vtkDynamicViewFrame()
{
  this->Frame = vtkGViewFrame::New();
  this->Rect = NULL;
}

vtkDynamicViewFrame::~vtkDynamicViewFrame()
{
  this->Frame->Delete();
}

void vtkDynamicViewFrame::SetOrientation(vtkDynamicViewFrame::Orientation o)
{
  vtkViewFrame *frame = NULL;
  switch (o)
  {
  case GRID:
    frame = vtkGViewFrame::New();
    frame->DeepCopy(this->Frame);
    frame->FillDummies();
    break;
  case HORIZONTAL:
    frame = vtkHViewFrame::New();
    frame->DeepCopy(this->Frame);
    break;
  case VERTICAL:
    frame = vtkVViewFrame::New();
    frame->DeepCopy(this->Frame);
    break;
  default:
    cerr << "ERROR!" << std::endl;
  }
  this->Frame = frame;
  this->Frame->SetViewRect(this->Rect);
}

void vtkDynamicViewFrame::AddChild(vtkViewObject *object)
{
  this->Frame->AddChild(object);
}

void vtkDynamicViewFrame::SetViewRect(vtkViewRect *rect)
{
  assert(rect != NULL);
  this->Rect = rect;
  this->Frame->SetViewRect(rect);
}

void vtkDynamicViewFrame::UpdateChildView(int index)
{
  this->Frame->SetViewport(this->View);
  this->Frame->UpdateChildView(index);
}

vtkViewFrame* vtkDynamicViewFrame::GetUnderlyingFrame()
{
  return this->Frame;
}

vtkViewObjectCollection *vtkDynamicViewFrame::GetChildren()
{
  return this->Frame->GetChildren();
}
