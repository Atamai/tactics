/*=========================================================================

  Program:   ToolCursor
  Module:    vtkRotateCameraTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommand.h"
#include "vtkRotateCameraTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkRotateCameraTool);

//----------------------------------------------------------------------------
vtkRotateCameraTool::vtkRotateCameraTool()
{
  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkRotateCameraTool::~vtkRotateCameraTool()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkRotateCameraTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkRotateCameraTool::StartAction()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  camera->GetFocalPoint(this->CenterOfRotation);
  camera->GetPosition(this->StartCameraPosition);
  camera->GetViewUp(this->StartCameraViewUp);

  // The camera distance
  double d = camera->GetDistance();

  // Get viewport height at focal plane
  double height;
  if (camera->GetParallelProjection())
    {
    height = camera->GetParallelScale();
    }
  else
    {
    double angle = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
    height = 2*d*sin(angle/2);
    }

  // Radius of the sphere used for interaction
  this->Radius = 0.5*height;

  // Initialize the transform
  this->Transform->Identity();
}

//----------------------------------------------------------------------------
void vtkRotateCameraTool::StopAction()
{
  this->Superclass::StopAction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkRotateCameraTool::DoAction()
{
  this->Superclass::DoAction();

  // Here are the values that we will be setting in the code: the angle,
  // in radians, and the axis of rotation
  double rotationAngle = 0;
  double rotationAxis[3];
  rotationAxis[0] = 0;
  rotationAxis[1] = 0;
  rotationAxis[2] = 1;

  // Get the camera
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

  // Get the camera's position
  double cameraPos[3];
  camera->GetPosition(cameraPos);

  // Center of rotation (focal point) and rotation sphere radius
  double f[3];
  this->GetCenterOfRotation(f);

  double r = this->Radius;

  // Camera distance, squared, and rotation radius, squared.
  double d2 = vtkMath::Distance2BetweenPoints(cameraPos,f);
  double r2 = r*r;

  // The interaction uses a plane parallel to the focal plane
  // but far enough in front of the focal plane that it defines
  // a circle that contains a certain percentage of the visible
  // sphere area, say 99%.
  double s2max = 0.99*r2/d2*(d2 - r2);

  // And here is the position of the desired interaction plane.
  double fg = sqrt(r2 - s2max);
  double g[3];
  g[0] = f[0] + fg*cvz[0];
  g[1] = f[1] + fg*cvz[1];
  g[2] = f[2] + fg*cvz[2];

  // Get the current display position.
  double x, y;
  this->GetDisplayPosition(x, y);

  // Get the view ray and see where it intersects the sphere of rotation.
  // This involves several steps and solving a quadratic equation.
  double p1[3], p2[3];
  this->DisplayToWorld(x, y, 0.0, p1);
  this->DisplayToWorld(x, y, 1.0, p2);

  // Vector along direction of view-ray line
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Vector to the center of interaction plane.
  double u[3];
  u[0] = p1[0] - g[0];
  u[1] = p1[1] - g[1];
  u[2] = p1[2] - g[2];

  // Find the "t" value for the interaction plane
  double t = -vtkMath::Dot(u,cvz)/vtkMath::Dot(v,cvz);

  // Use "t" to compute the intersection point for the view ray
  double p[3];
  p[0] = p1[0]*(1 - t) + p2[0]*t;
  p[1] = p1[1]*(1 - t) + p2[1]*t;
  p[2] = p1[2]*(1 - t) + p2[2]*t;

  // Do the same for the previous position
  this->GetLastDisplayPosition(x, y);

  this->DisplayToWorld(x, y, 0.0, p1);
  this->DisplayToWorld(x, y, 1.0, p2);

  double pl[3];
  pl[0] = p1[0]*(1 - t) + p2[0]*t;
  pl[1] = p1[1]*(1 - t) + p2[1]*t;
  pl[2] = p1[2]*(1 - t) + p2[2]*t;

  // Compute the motion vector
  double w[3];
  w[0] = pl[0] - p[0];
  w[1] = pl[1] - p[1];
  w[2] = pl[2] - p[2];

  // And the rotation angle corresponding to the vector
  rotationAngle = 2*vtkMath::Norm(w)/r;

  // Rotation axis is perpendicular to view plane normal and motion vector
  vtkMath::Cross(w, cvz, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  // Get ready to apply the camera transformation
  this->Transform->PostMultiply();

  // First, turn the transform into a pure rotation matrix, to avoid
  // accumulation of roundoff errors in the position
  double oldTrans[3];
  this->Transform->GetPosition(oldTrans);
  this->Transform->Translate(-oldTrans[0], -oldTrans[1], -oldTrans[2]);

  // Increment by the new rotation
  this->Transform->RotateWXYZ(vtkMath::DegreesFromRadians(-rotationAngle),
                              rotationAxis);

  // Center the rotation at the focal point
  this->Transform->PreMultiply();
  this->Transform->Translate(-f[0], -f[1], -f[2]);
  this->Transform->PostMultiply();
  this->Transform->Translate(f[0], f[1], f[2]);

  // Rotate the original direction of projection and view-up
  this->Transform->TransformPoint(this->StartCameraPosition, cameraPos);
  this->Transform->TransformVector(this->StartCameraViewUp, cvy);

  camera->SetPosition(cameraPos);
  camera->SetViewUp(cvy);

  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkRotateCameraTool::ConstrainCursor(double position[3],
                                            double normal[3])
{
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  double f[3];
  this->GetCenterOfRotation(f);

  if (camera->GetParallelProjection())
    {
    normal[0] = position[0] - f[0];
    normal[1] = position[1] - f[1];
    normal[2] = position[2] - f[2];
    vtkMath::Normalize(normal);
    }
  else
    {
    double p[3];
    camera->GetPosition(p);

    double u[3];
    u[0] = position[0] - p[0];
    u[1] = position[1] - p[1];
    u[2] = position[2] - p[2];
    vtkMath::Normalize(u);

    double v[3];
    v[0] = f[0] - p[0];
    v[1] = f[1] - p[1];
    v[2] = f[2] - p[2];

    double t = vtkMath::Dot(u, v);

    double q[3];
    q[0] = p[0] + t*u[0];
    q[1] = p[1] + t*u[1];
    q[2] = p[2] + t*u[2];

    normal[0] = q[0] - f[0];
    normal[1] = q[1] - f[1];
    normal[2] = q[2] - f[2];
    vtkMath::Normalize(normal);
    }
}
