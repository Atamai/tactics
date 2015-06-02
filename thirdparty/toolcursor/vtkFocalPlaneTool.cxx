/*=========================================================================

  Program:   ToolCursor
  Module:    vtkFocalPlaneTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommand.h"
#include "vtkFocalPlaneTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkFocalPlaneTool);

//----------------------------------------------------------------------------
vtkFocalPlaneTool::vtkFocalPlaneTool()
{
}

//----------------------------------------------------------------------------
vtkFocalPlaneTool::~vtkFocalPlaneTool()
{
}

//----------------------------------------------------------------------------
void vtkFocalPlaneTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkFocalPlaneTool::StartAction()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);

  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  this->StartDistance = camera->GetDistance();
}

//----------------------------------------------------------------------------
void vtkFocalPlaneTool::StopAction()
{
  this->Superclass::StopAction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkFocalPlaneTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  // Get the display position.
  double x0, y0, x, y;
  this->GetStartDisplayPosition(x0, y0);
  this->GetDisplayPosition(x, y);

  // Get viewport height at the current depth
  double height = 1;
  if (camera->GetParallelProjection())
    {
    height = camera->GetParallelScale();
    }
  else
    {
    double angle = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
    height = 2*this->StartDistance*sin(angle*0.5);
    }

  // Get the viewport size in pixels
  vtkRenderer *renderer = cursor->GetRenderer();
  int *size = renderer->GetSize();

  // Get the vertical offset
  double dy = y - y0;
  double distance = this->StartDistance + dy*height/size[1];


  double focalPoint[3], position[3];
  camera->GetFocalPoint(focalPoint);
  camera->GetPosition(position);

  double vector[3];
  vector[0] = focalPoint[0] - position[0];
  vector[1] = focalPoint[1] - position[1];
  vector[2] = focalPoint[2] - position[2];

  vtkMath::Normalize(vector);

  focalPoint[0] = position[0] + distance*vector[0];
  focalPoint[1] = position[1] + distance*vector[1];
  focalPoint[2] = position[2] + distance*vector[2];

  camera->SetFocalPoint(focalPoint);

  this->InvokeEvent(vtkCommand::InteractionEvent);
}

