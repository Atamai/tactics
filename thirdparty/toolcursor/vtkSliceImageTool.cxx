/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSliceImageTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSliceImageTool.h"
#include "vtkObjectFactory.h"

#include "vtkCommand.h"
#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkImageMapper3D.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkSliceImageTool);

//----------------------------------------------------------------------------
vtkSliceImageTool::vtkSliceImageTool()
{
  this->JumpToNearestSlice = 0;
}

//----------------------------------------------------------------------------
vtkSliceImageTool::~vtkSliceImageTool()
{
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::SetJumpToNearestSlice(int val)
{
  if (val != this->JumpToNearestSlice)
    {
    this->JumpToNearestSlice = val;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::StartAction()
{
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  this->StartDistance = camera->GetDistance();

    // code for handling the mouse wheel interaction
  if ((cursor->GetModifier() & VTK_TOOL_WHEEL_MASK) != 0)
    {
    int delta = 0;
    if ((cursor->GetModifier() & VTK_TOOL_WHEEL_BWD) != 0)
      {
      delta = -1;
      }
    else if ((cursor->GetModifier() & VTK_TOOL_WHEEL_FWD) != 0)
      {
      delta = 1;
      }
    this->AdvanceSlice(delta);
    }
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::StopAction()
{
  this->Superclass::StopAction();
  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::DoAction()
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

  vtkImageData *data = 0;

  if (this->JumpToNearestSlice &&
      this->CurrentImageMatrix &&
      this->CurrentImageMapper &&
      (data = vtkImageData::SafeDownCast(this->CurrentImageMapper->GetInput())))
    {
    double point[4];
    point[0] = focalPoint[0];
    point[1] = focalPoint[1];
    point[2] = focalPoint[2];
    point[3] = 1.0;

    double normal[4];
    normal[0] = vector[0];
    normal[1] = vector[1];
    normal[2] = vector[2];
    normal[3] = -vtkMath::Dot(point, normal);

    // convert normal to data coordinates
    double worldToData[16];
    vtkMatrix4x4 *dataToWorld = this->CurrentImageMatrix;
    vtkMatrix4x4::Transpose(*dataToWorld->Element, worldToData);
    vtkMatrix4x4::MultiplyPoint(worldToData, normal, normal);

    // find the slice orientation from the normal
    int k = 0;
    double maxsq = 0;
    double sumsq = 0;
    for (int i = 0; i < 3; i++)
      {
      double tmpsq = normal[i]*normal[i];
      sumsq += tmpsq;
      if (tmpsq > maxsq)
        {
        maxsq = tmpsq;
        k = i;
        }
      }

    // if the slice is not oblique
    if ((1.0 - maxsq/sumsq) < 1e-12)
      {
      // get the point in data coordinates
      vtkMatrix4x4::Invert(*dataToWorld->Element, worldToData);
      vtkMatrix4x4::MultiplyPoint(worldToData, point, point);

      double *spacing = data->GetSpacing();
      double *origin = data->GetOrigin();

      // set the point to lie exactly on a slice
      double z = (point[k] - origin[k])/spacing[k];
      if (z > VTK_INT_MIN && z < VTK_INT_MAX)
        {
        int j = vtkMath::Floor(z + 0.5);
        point[k] = j*spacing[k] + origin[k];
        }

      // convert back to world coordinates
      dataToWorld->MultiplyPoint(point, point);

      if (point[3] != 0)
        {
        focalPoint[0] = point[0]/point[3];
        focalPoint[1] = point[1]/point[3];
        focalPoint[2] = point[2]/point[3];
        }
      }
    }

  camera->SetFocalPoint(focalPoint);
}

//----------------------------------------------------------------------------
void vtkSliceImageTool::AdvanceSlice(int delta)
{
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkCamera *camera = cursor->GetRenderer()->GetActiveCamera();

  // Get the camera information
  double point[4], normal[4], position[3];
  camera->GetFocalPoint(point);
  camera->GetPosition(position);
  normal[0] = point[0] - position[0];
  normal[1] = point[1] - position[1];
  normal[2] = point[2] - position[2];
  vtkMath::Normalize(normal);
  normal[3] = -vtkMath::Dot(point, normal);

  // Convert the normal to data coordinates
  if (this->CurrentImageMatrix)
    {
    double matrix[16];
    vtkMatrix4x4::Transpose(*this->CurrentImageMatrix->Element, matrix);
    vtkMatrix4x4::MultiplyPoint(matrix, normal, normal);
    }

  // Get the image information
  vtkImageData *data = 0;
  if (this->CurrentImageMapper)
    {
    data = vtkImageData::SafeDownCast(this->CurrentImageMapper->GetInput());
    }
  if (data)
    {
    // Compute the data bounds
    double origin[3], spacing[3], bounds[6];
    int extent[6];
    data->GetOrigin(origin);
    data->GetSpacing(spacing);
    data->GetWholeExtent(extent);

    bounds[0] = origin[0] + spacing[0]*extent[0];
    bounds[1] = origin[0] + spacing[0]*extent[1];
    bounds[2] = origin[1] + spacing[1]*extent[2];
    bounds[3] = origin[1] + spacing[1]*extent[3];
    bounds[4] = origin[2] + spacing[2]*extent[4];
    bounds[5] = origin[2] + spacing[2]*extent[5];

    // Compute the distance to each of the 8 corners of the data cube
    int maxc = 0;
    double maxdist = VTK_DOUBLE_MIN;
    for (int c = 0; c < 8; c++)
      {
      double dist = (normal[0]*bounds[c&1] +
                     normal[1]*bounds[2+((c&2)>>1)] +
                     normal[2]*bounds[4+((c&4)>>2)] + normal[3]);
      if (dist > maxdist)
        {
        maxdist = dist;
        maxc = c;
        }
      }
    // Compute the distance to the opposite corner
    int minc = (maxc ^ 7);
    double mindist = (normal[0]*bounds[minc&1] +
                      normal[1]*bounds[2+((minc&2)>>1)] +
                      normal[2]*bounds[4+((minc&4)>>2)] + normal[3]);

    // Compute the spacing to use
    double wx = normal[0]*normal[0];
    double wy = normal[1]*normal[1];
    double wz = normal[2]*normal[2];
    double s = fabs(spacing[0])*wx + fabs(spacing[1])*wy + fabs(spacing[2])*wz;
    // Round to get the nearest slice index
    int n = vtkMath::Floor(maxdist/s + 0.5);
    int nmax = vtkMath::Floor((maxdist - mindist)/s + 0.5);
    n += delta;
    // Apply some limits
    n = (n < 0 ? 0 : n);
    n = (n > nmax ? nmax : n);
    // Adjust the plane
    normal[3] -= n*s - maxdist;
    }

  // Convert the plane back to world coordinates
  if (this->CurrentImageMatrix)
    {
    double matrix[16];
    vtkMatrix4x4::Invert(*this->CurrentImageMatrix->Element, matrix);
    vtkMatrix4x4::Transpose(matrix, matrix);
    vtkMatrix4x4::MultiplyPoint(matrix, normal, normal);
    }

  // project the focal point onto this new plane
  double f = vtkMath::Dot(normal, point) + normal[3];
  point[0] += f*normal[0];
  point[1] += f*normal[1];
  point[2] += f*normal[2];

  camera->SetFocalPoint(point);
}
