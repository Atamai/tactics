/*=========================================================================

  Program:   ToolCursor
  Module:    vtkPanCameraTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPanCameraTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkPanCameraTool);

//----------------------------------------------------------------------------
vtkPanCameraTool::vtkPanCameraTool()
{
  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkPanCameraTool::~vtkPanCameraTool()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkPanCameraTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPanCameraTool::StartAction()
{
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  camera->GetFocalPoint(this->StartCameraFocalPoint);
  camera->GetPosition(this->StartCameraPosition);

  this->Transform->Identity();
}

//----------------------------------------------------------------------------
void vtkPanCameraTool::StopAction()
{
  this->Superclass::StopAction();
}

//----------------------------------------------------------------------------
void vtkPanCameraTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  // Get the initial point.
  double p0[3];
  this->GetStartPosition(p0);

  // Get the depth.
  double x, y, z;
  this->WorldToDisplay(p0, x, y, z);

  // Get the display position.
  double p[3];
  this->GetDisplayPosition(x, y);
  this->DisplayToWorld(x, y, z, p);

  // Get the vector.
  double v[3];
  v[0] = p[0] - p0[0];
  v[1] = p[1] - p0[1];
  v[2] = p[2] - p0[2];

  this->Transform->PostMultiply();
  this->Transform->Translate(-v[0], -v[1], -v[2]);

  double cameraPos[3], cameraFocalPoint[3];
  this->Transform->TransformPoint(this->StartCameraPosition, cameraPos);
  this->Transform->TransformPoint(this->StartCameraFocalPoint,
                                  cameraFocalPoint);

  camera->SetPosition(cameraPos);
  camera->SetFocalPoint(cameraFocalPoint);
}

