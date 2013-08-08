/*=========================================================================
  Program: Cerebra
  Module:  vtkViewFrame.cxx

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

#include <math.h>
#include <assert.h>

#include "vtkViewRect.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkDummyViewPane.h"
#include "vtkViewFrame.h"
#include "vtkViewObjectCollection.h"

vtkViewFrame::vtkViewFrame() : CurrentSquare(1), GridColumns(1), GridRows(1)
{
  this->Children = vtkViewObjectCollection::New();
  this->Rect = NULL;
  this->Dummies = 0;
}

vtkViewFrame::~vtkViewFrame()
{
  this->Children->Delete();
}

void vtkViewFrame::AddChild(vtkViewObject *child)
{
  assert(child != NULL);
  this->Children->AddItem(child);
  this->UpdateGridDimensions();
}

void vtkViewFrame::DeepCopy(vtkViewFrame* other)
{
  this->Rect = other->GetViewRect();
  for (int i = 0; i < other->GetChildren()->GetNumberOfItems(); i++)
    {
    vtkViewObject *obj = other->GetChildren()->GetViewObject(i);
    vtkDummyViewPane *dummy = vtkDummyViewPane::SafeDownCast(obj);
    if (!dummy)
      {
      this->Children->AddItem(obj);
      }
    else
      {
      this->Rect->GetRenderWindow()->RemoveRenderer(dummy->GetRenderer());
      }
    }
  this->CurrentSquare = other->GetCurrentSquare();
  this->GridRows = other->GetGridRows();
  this->GridColumns = other->GetGridColumns();
  this->Dummies = 0;
}

void vtkViewFrame::UpdateGridDimensions() {
  if (this->Children->GetNumberOfItems() - this->Dummies > this->CurrentSquare)
    {
    this->CurrentSquare = this->CurrentSquare + 2 * sqrt(this->CurrentSquare) + 1;
    this->GridRows = sqrt(this->CurrentSquare);
    this->GridColumns = this->GridRows;
    }
}

void vtkViewFrame::FillDummies()
{
  for (int i = this->Children->GetNumberOfItems(); i < this->CurrentSquare; i++)
    {
    vtkDummyViewPane *dummy = vtkDummyViewPane::New();
    this->Children->AddItem(dummy);
    this->Dummies++;
    }
}

void vtkViewFrame::ComputeRangeForIndex(int index, double range[2])
{
  double a = 0.0;
  double b = 0.0;
  double total = 0.0;

  vtkViewObjectCollection *children = this->GetChildren();
  int n = children->GetNumberOfItems();
  for (int i = 0; i < n; i++)
    {
    vtkViewObject *child = children->GetViewObject(i);
    double stretch = child->GetStretchFactor();
    total += stretch;
    if (i < index)
      {
      a = total;
      }
    else if (i == index)
      {
      b = total;
      }
    }

  // Return the start and end of the range for this item relative
  // to the total of all stretch factors.
  range[0] = a/total;
  range[1] = b/total;
}
