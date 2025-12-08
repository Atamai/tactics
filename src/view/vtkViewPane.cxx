/*=========================================================================
  Program: Cerebra
  Module:  vtkViewPane.cxx

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

#include "vtkFocalPlaneTool.h"
#include "vtkToolCursor.h"
#include "vtkPanCameraTool.h"
#include "vtkZoomCameraTool.h"
#include "vtkRotateCameraTool.h"

#include "vtkPolyDataMapper2D.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkActor2D.h"
#include "vtkPoints.h"
#include "vtkCellData.h"
#include "vtkProperty2D.h"

#include "vtkSmartPointer.h"
#include "vtkProp.h"
#include "vtkViewPane.h"
#include "vtkRenderer.h"

#include <assert.h>

vtkViewPane::vtkViewPane()
{
  this->Renderer = vtkRenderer::New();
  this->ToolCursor = vtkToolCursor::New();
  this->ToolCursor->SetRenderer(this->Renderer);
  this->ToolCursor->SetScale(1.0);

  /*
  vtkSmartPointer<vtkPanCameraTool> panTool =
    vtkSmartPointer<vtkPanCameraTool>::New();
  int panId = this->ToolCursor->AddAction(panTool);

  vtkSmartPointer<vtkZoomCameraTool> zoomTool =
    vtkSmartPointer<vtkZoomCameraTool>::New();
  int zoomId = this->ToolCursor->AddAction(zoomTool);

  vtkSmartPointer<vtkRotateCameraTool> rotateTool =
    vtkSmartPointer<vtkRotateCameraTool>::New();
  int rotateId = this->ToolCursor->AddAction(rotateTool);

  vtkSmartPointer<vtkFocalPlaneTool> sliceTool =
    vtkSmartPointer<vtkFocalPlaneTool>::New();
  int sliceId = this->ToolCursor->AddAction(sliceTool);

  // Bind all the tools
  this->ToolCursor->BindAction(panId, 0, 0, VTK_TOOL_SHIFT | VTK_TOOL_B2);
  this->ToolCursor->BindAction(zoomId, 0, 0, VTK_TOOL_B2);
  this->ToolCursor->BindAction(sliceId, 0, 0, VTK_TOOL_B1);
  //this->ToolCursor->BindAction(rotateId, 0, 0, VTK_TOOL_B1);
  */
  
  // Set cursor tracking unavailable as default
  this->CursorTracking = false;
  this->BorderEnabled = false;
}

vtkViewPane::~vtkViewPane()
{
  this->Renderer->Delete();
  this->ToolCursor->Delete();
}

void vtkViewPane::UpdateViewport(const double view[4])
{
  double ncView[4];
  for (int i = 0; i < 4; i++)
    {
    ncView[i] = view[i];
    }
  this->Renderer->SetViewport(ncView);
}

bool vtkViewPane::Contains(double x, double y)
{
  if (x >= this->View[0] && x <= this->View[2]
      && y >= this->View[1] && y <= this->View[3])
    {
    return true;
    }
  return false;
}

void vtkViewPane::SetCursorTracking(bool en)
{
  this->CursorTracking = en;
}

void vtkViewPane::AddBorder()
{
  double ncView[4];
  this->Renderer->GetViewport(ncView);

  int borderWidth = 2;
  int width = ncView[2] - ncView[0];
  int height = ncView[3] - ncView[1];

  width = height = 1800;
  
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(8);
  points->SetPoint(0,0,0,0);
  points->SetPoint(1,borderWidth,borderWidth,0);
  points->SetPoint(2,width,0,0);
  points->SetPoint(3,width-borderWidth,borderWidth,0);
  points->SetPoint(4,width,height,0);
  points->SetPoint(5,width-borderWidth,height-borderWidth,0);
  points->SetPoint(6,0,height,0);
  points->SetPoint(7,borderWidth,height-borderWidth,0);

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(2);

  cells->InsertNextCell(4);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(5);
  cells->InsertCellPoint(4);

  cells->InsertNextCell(4);
  cells->InsertCellPoint(4);
  cells->InsertCellPoint(5);
  cells->InsertCellPoint(7);
  cells->InsertCellPoint(6);

  cells->InsertNextCell(4);
  cells->InsertCellPoint(6);
  cells->InsertCellPoint(7);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(0);

  vtkSmartPointer<vtkUnsignedCharArray> scalars = vtkSmartPointer<vtkUnsignedCharArray>::New();
  scalars->SetNumberOfTuples(4);
  scalars->SetTuple1(0,190);
  scalars->SetTuple1(1,190);
  scalars->SetTuple1(2,64);
  scalars->SetTuple1(3,64);

  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  data->SetPoints(points);
  data->SetPolys(cells);
  data->GetCellData()->SetScalars(scalars);

  vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  mapper->SetInputData(data);

  vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
  actor->SetMapper(mapper);
  actor->SetPosition(0,0);
  actor->GetProperty()->SetColor(0.1,0.1,0.1);
  actor->SetVisibility(1);
  this->Renderer->AddActor2D(actor);
}
