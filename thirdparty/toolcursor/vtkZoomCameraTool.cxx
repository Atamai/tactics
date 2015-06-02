/*=========================================================================

  Program:   ToolCursor
  Module:    vtkZoomCameraTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkZoomCameraTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkZoomCameraTool);

//----------------------------------------------------------------------------
vtkZoomCameraTool::vtkZoomCameraTool()
{
  this->ZoomByDolly = 1;
  this->RadialInteraction = 0;

  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkZoomCameraTool::~vtkZoomCameraTool()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkZoomCameraTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ZoomByDolly: " << (this->ZoomByDolly ? "On\n" : "Off\n");
  os << indent << "RadialInteraction: "
     << (this->RadialInteraction ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkZoomCameraTool::StartAction()
{
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  camera->GetPosition(this->StartCameraPosition);
  camera->GetClippingRange(this->StartClippingRange);
  this->StartParallelScale = camera->GetParallelScale();
  this->StartViewAngle = camera->GetViewAngle();

  this->ZoomFactor = 1.0;

  this->Transform->Identity();

  // code for handling the mouse wheel interaction
  if ((cursor->GetModifier() & VTK_TOOL_WHEEL_MASK) != 0)
    {
    if ((cursor->GetModifier() & VTK_TOOL_WHEEL_BWD) != 0)
      {
      this->ZoomFactor = 1.1;
      }
    else if ((cursor->GetModifier() & VTK_TOOL_WHEEL_FWD) != 0)
      {
      this->ZoomFactor = 1.0/1.1;
      }
    if (camera->GetParallelProjection())
      {
      camera->SetParallelScale(this->StartParallelScale/this->ZoomFactor);
      }
    else
      {
      if (this->ZoomByDolly)
        {
        double p1[3], p2[3];
        camera->GetPosition(p1);
        camera->Dolly(this->ZoomFactor);
        camera->GetPosition(p2);
        // find the distance moved
        double d = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
        d = (this->ZoomFactor < 1 ? d : -d);
        // adjust the clipping range by this distance
        double tol = cursor->GetRenderer()->GetNearClippingPlaneTolerance();
        double d1, d2;
        camera->GetClippingRange(d1, d2);
        if (d1 < 2*d2*tol)
          { // too close to camera, reset the near range
          cursor->GetRenderer()->ResetCameraClippingRange();
          double d3; // dummy variable
          camera->GetClippingRange(d1, d3);
          }
        else
          {
          d1 += d;
          }
        d2 += d;
        if (d1 < d2*tol) { d1 = d2*tol; }
        camera->SetClippingRange(d1, d2);
        }
      else // Zoom by changing the height of the scene
        {
        double h = 2*tan(0.5*vtkMath::RadiansFromDegrees(this->StartViewAngle));
        h /= this->ZoomFactor;
        double viewAngle = 2*vtkMath::DegreesFromRadians(atan(0.5*h));
        camera->SetViewAngle(viewAngle);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkZoomCameraTool::StopAction()
{
  this->Superclass::StopAction();
}

//----------------------------------------------------------------------------
void vtkZoomCameraTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();

  // Get the camera's z axis
  double cvz[3];
  for (int i = 0; i < 3; i++)
    {
    cvz[i] = viewMatrix->GetElement(2, i);
    }

  if (this->RadialInteraction)
    {
    double f[3];
    camera->GetFocalPoint(f);

    double c[3];
    c[0] = this->StartCameraPosition[0];
    c[1] = this->StartCameraPosition[1];
    c[2] = this->StartCameraPosition[2];

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

    // Find positions relative to camera position.
    double u[3];
    u[0] = p0[0] - f[0];
    u[1] = p0[1] - f[1];
    u[2] = p0[2] - f[2];

    // Distance from focal plane
    double df = vtkMath::Dot(u, cvz);

    // Point about which magnification occurs
    double g[3];
    g[0] = f[0] + df*cvz[0];
    g[1] = f[1] + df*cvz[1];
    g[2] = f[2] + df*cvz[2];

    // Distance from center for the two points
    double r1 = sqrt(vtkMath::Distance2BetweenPoints(g, p0));
    double r2 = sqrt(vtkMath::Distance2BetweenPoints(g, p));

    // Get the camera position
    camera->GetPosition(p);
    double dp = sqrt(vtkMath::Distance2BetweenPoints(p,g));

    // Get viewport height at the current depth
    double height = 1;
    if (camera->GetParallelProjection())
      {
      height = camera->GetParallelScale();
      }
    else
      {
      double angle = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
      height = 2*dp*sin(angle/2);
      }

    // Constrain the values when they are close to the center, in order to
    // avoid magifications of zero or infinity
    double halfpi = 0.5*vtkMath::DoublePi();
    double r0 = 0.1*height;
    if (r1 < r0)
      {
      r1 = r0*(1.0 - sin((1.0 - r1/r0)*halfpi)/halfpi);
      }
    if (r2 < r0)
      {
      r2 = r0*(1.0 - sin((1.0 - r2/r0)*halfpi)/halfpi);
      }

    // Compute magnification and corresponding camera motion
    double mag = r2/r1;
    double delta = dp - dp/mag;

    this->Transform->PostMultiply();
    this->Transform->Translate(-delta*cvz[0], -delta*cvz[1], -delta*cvz[2]);

    this->ZoomFactor *= mag;
    }
  else
    {
    // This is called if RadialInteraction is off (it's much simpler):
    // moving the mouse by half the viewport height will zoom by 10
    vtkRenderer *renderer = cursor->GetRenderer();
    int *size = renderer->GetSize();
    double x, y, x0, y0;
    this->GetDisplayPosition(x, y);
    this->GetStartDisplayPosition(x0, y0);
    double dyFactor = (y - y0)/(0.5*(size[1] + 1));
    this->ZoomFactor = pow(10.0, dyFactor);

    double f[3], p[3];
    camera->GetFocalPoint(f);
    double *p0 = this->StartCameraPosition;

    p[0] = p0[0]/this->ZoomFactor + (1.0 - 1.0/this->ZoomFactor)*f[0]; 
    p[1] = p0[1]/this->ZoomFactor + (1.0 - 1.0/this->ZoomFactor)*f[1]; 
    p[2] = p0[2]/this->ZoomFactor + (1.0 - 1.0/this->ZoomFactor)*f[2]; 

    this->Transform->Identity();
    this->Transform->Translate(p[0] - p0[0], p[1] - p0[1], p[2] - p0[2]);
    }

  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(this->StartParallelScale/this->ZoomFactor);
    }
  else
    {
    if (this->ZoomByDolly)
      {
      double cameraPos[3];
      this->Transform->TransformPoint(this->StartCameraPosition, cameraPos);

      camera->SetPosition(cameraPos);

      double v[3];
      v[0] = cameraPos[0] - this->StartCameraPosition[0];
      v[1] = cameraPos[1] - this->StartCameraPosition[1];
      v[2] = cameraPos[2] - this->StartCameraPosition[2];

      double dist = vtkMath::Dot(v, cvz);

      double d1 = this->StartClippingRange[0];
      double d2 = this->StartClippingRange[1];

      double tol = cursor->GetRenderer()->GetNearClippingPlaneTolerance();

      // was near clipping plane close to tolerance?
      if (d1 < 2*d2*tol)
        {
        // need to recompute the near clipping range
        cursor->GetRenderer()->ResetCameraClippingRange();
        camera->GetClippingRange(d1, d2);
        d2 = this->StartClippingRange[1] + dist;
        }
      else
        {
        // no problems, just adjust clipping range by dolly distance
        d1 += dist;
        d2 += dist;
        }

      // ensure that front plane is not too close to camera
      if (d1 < d2*tol)
        {
        d1 = d2*tol;
        }

      camera->SetClippingRange(d1, d2);
      }
    else
      {
      // Zoom by changing the view angle

      double h = 2*tan(0.5*vtkMath::RadiansFromDegrees(this->StartViewAngle));
      h /= this->ZoomFactor;
      double viewAngle = 2*vtkMath::DegreesFromRadians(atan(0.5*h));

      camera->SetViewAngle(viewAngle);
      }
    }
}

//----------------------------------------------------------------------------
void vtkZoomCameraTool::ConstrainCursor(double vtkNotUsed(position)[3],
                                        double normal[3])
{
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();
  vtkMatrix4x4 *matrix = camera->GetViewTransformMatrix();

  // Force the normal to point towards camera
  normal[0] = matrix->GetElement(2, 0);
  normal[1] = matrix->GetElement(2, 1);
  normal[2] = matrix->GetElement(2, 2);

  // Force the position to the near clipping plane
}

