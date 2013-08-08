/*=========================================================================
  Program: Cerebra
  Module:  vtkGViewFrame.cxx

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
#include "vtkGViewFrame.h"
#include "vtkObjectFactory.h"
#include "vtkObject.h"
#include "vtkViewObjectCollection.h"
#include "vtkDummyViewPane.h"
#include "vtkViewPane.h"
#include "vtkViewRect.h"
#include "vtkRenderWindow.h"

vtkStandardNewMacro(vtkGViewFrame);

vtkGViewFrame::vtkGViewFrame()
{
}

vtkGViewFrame::~vtkGViewFrame()
{
}

void vtkGViewFrame::UpdateChildView(int index)
{
  vtkViewObject *child = this->Children->GetViewObject(index);

  double view[4];
  this->GetViewport(view);

  double width = view[2] - view[0];
  double height = view[3] - view[1];

  double columns = static_cast<double>(this->GridColumns);
  double rows = static_cast<double>(this->GridRows);

  int position[2];
  this->GetGridPositionFromIndex(index, position);

  position[1] = rows - position[1] - 1;

  double childView[4] = {
    view[0] + position[0] / columns * width,
    view[1] + position[1] / rows * height,
    view[0] + (position[0] + 1.0) / columns * width,
    view[1] + (position[1] + 1.0) / rows * height
  };

  child->SetViewport(childView);
}

void vtkGViewFrame::GetGridPositionFromIndex(int index, int position[2]) const
{
  position[0] = index % this->GridColumns;
  position[1] = index / this->GridRows;
}

void vtkGViewFrame::AddChild(vtkViewObject *child)
{
  assert(child != NULL);

  if (this->Dummies > 0)
    {
     for (int i = 0; i < this->Children->GetNumberOfItems(); i++)
       {
       vtkViewObject *obj = this->Children->GetViewObject(i);
       vtkViewPane *objPane = vtkViewPane::SafeDownCast(obj);
       if (objPane == NULL)
         {
         continue;
         }
       vtkDummyViewPane *dummy = vtkDummyViewPane::SafeDownCast(objPane);
       if (dummy)
         {
         this->Children->ReplaceItem(i, child);
         vtkViewPane *pane = vtkViewPane::SafeDownCast(child);
         if (pane)
           {
           this->Rect->GetRenderWindow()->RemoveRenderer(dummy->GetRenderer());
           this->Rect->GetRenderWindow()->AddRenderer(pane->GetRenderer());
           }
         this->Dummies--;
         return;
         }
       }
    }

  // Add to the end of the collection.
  this->Children->AddItem(child);
  this->UpdateGridDimensions();
  this->FillDummies();
}
