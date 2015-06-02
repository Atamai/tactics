/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSpinCameraTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSpinCameraTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkSpinCameraTool);

//----------------------------------------------------------------------------
vtkSpinCameraTool::vtkSpinCameraTool()
{
  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkSpinCameraTool::~vtkSpinCameraTool()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkSpinCameraTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkSpinCameraTool::StartAction()
{
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  camera->GetViewUp(this->StartCameraViewUp);

  this->Transform->Identity();
}

//----------------------------------------------------------------------------
void vtkSpinCameraTool::StopAction()
{
  this->Superclass::StopAction();
}

//----------------------------------------------------------------------------
void vtkSpinCameraTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();
  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();

  // Get the camera's x, y, and z axes
  double cvx[3], cvy[3], cvz[3];
  for (int i = 0; i < 3; i++)
    {
    cvx[i] = viewMatrix->GetElement(0, i);
    cvy[i] = viewMatrix->GetElement(1, i);
    cvz[i] = viewMatrix->GetElement(2, i);
    }

  double f[3];
  camera->GetFocalPoint(f);

  // Get the initial point.
  double p0[3];
  this->GetStartPosition(p0);

  // Get the display position.
  double x, y;
  this->GetDisplayPosition(x, y);

  // Get any point along the view ray.
  double p[3];
  this->DisplayToWorld(x, y, 0.5, p);

  // Find positions relative to focal point
  double u[3];
  u[0] = p0[0] - f[0];
  u[1] = p0[1] - f[1];
  u[2] = p0[2] - f[2];

  double v[3];
  v[0] = p[0] - f[0];
  v[1] = p[1] - f[1];
  v[2] = p[2] - f[2];

  // Convert to camera coords, centered on focal point
  double cx0 = vtkMath::Dot(u, cvx);
  double cy0 = vtkMath::Dot(u, cvy);
  double cr0 = sqrt(cx0*cx0 + cy0*cy0);

  double cx = vtkMath::Dot(v, cvx);
  double cy = vtkMath::Dot(v, cvy);
  double cr = sqrt(cx*cx + cy*cy);

  // Find the rotation angle
  double costheta = (cx0*cx + cy0*cy)/(cr0*cr);
  double sintheta = (cx0*cy - cy0*cx)/(cr0*cr);
  double theta = atan2(sintheta, costheta);

  // Increment by the new rotation
  this->Transform->PostMultiply();
  this->Transform->RotateWXYZ(vtkMath::DegreesFromRadians(-theta), cvz);

  // Apply the transform
  double cameraViewUp[3];
  this->Transform->TransformVector(this->StartCameraViewUp, cameraViewUp);
  camera->SetViewUp(cameraViewUp);
}

