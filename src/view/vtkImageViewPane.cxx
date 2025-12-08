/*=========================================================================
  Program: Cerebra
  Module:  vtkImageViewPane.cxx

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
#include "vtkCamera.h"
#include "vtkImageSliceCollection.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkImageStack.h"
#include "vtkImageViewPane.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

#include <iostream>

vtkStandardNewMacro(vtkImageViewPane);

vtkImageViewPane::vtkImageViewPane() : Table(0)
{
  this->ImageStack = vtkImageStack::New();
  this->Renderer->GetActiveCamera()->SetParallelProjection(1);
  this->Renderer->GetActiveCamera()->SetParallelScale(120);
  this->Renderer->AddViewProp(this->ImageStack);
  this->IDProvider = 0;
}

vtkImageViewPane::~vtkImageViewPane()
{
  this->ImageStack->Delete();
}

// Return ID of image added
int vtkImageViewPane::AddImage(vtkImageData *data,
                               vtkMatrix4x4 *matrix,
                               vtkImageProperty *property)
{
  vtkImageSlice *actor = vtkImageSlice::New();
  vtkImageResliceMapper *mapper = vtkImageResliceMapper::New();

  mapper->SetInputData(data);
  mapper->SliceAtFocalPointOn();
  mapper->SliceFacesCameraOn();
  mapper->JumpToNearestSliceOn();
  mapper->ResampleToScreenPixelsOff();
  mapper->BorderOff();
  mapper->SetBackground(1);

  actor->SetMapper(mapper);
  actor->SetProperty(property);
  actor->SetUserMatrix(matrix);

  std::pair<int, vtkImageSlice *> e;
  e.first = IDProvider;
  e.second = actor;
  this->Table.push_back(e);
  this->ImageStack->AddImage(e.second);

  mapper->Delete();
  actor->Delete();
  return IDProvider++;
}

void vtkImageViewPane::RemoveImage(int id)
{
  for (size_t i = 0; i < this->Table.size(); i++)
    {
    std::pair<int, vtkImageSlice *> e = this->Table[i];
    if (e.first == id)
      {
      this->ImageStack->RemoveImage(e.second);
      this->Table.erase(this->Table.begin() + i);
      return;
      }
    }
  std::cout << "Image with ID " << id << " not found." << std::endl;
}

vtkImageSlice *vtkImageViewPane::GetImageSlice(int id)
{
  for (size_t i = 0; i < this->Table.size(); i++)
    {
    std::pair<int, vtkImageSlice *> e = this->Table[i];
    if (e.first == id)
      {
      return e.second;
      }
    }
  std::cout << "Image with ID " << id << " not found." << std::endl;
  return NULL;
}

void vtkImageViewPane::ClearImageStack()
{
  for (size_t i = 0; i < this->Table.size(); i++)
    {
    std::pair<int, vtkImageSlice *> e = this->Table[i];
    this->ImageStack->RemoveImage(e.second);
    }
  this->Table.clear();
}
