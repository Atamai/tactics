/*=========================================================================

  Program:   ToolCursor
  Module:    vtkResliceMath.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkResliceMath.h"
#include "vtkObjectFactory.h"

#include "vtkImageReslice.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkResliceMath);

//----------------------------------------------------------------------------
void vtkResliceMath::SetReslicePlane(
  vtkImageReslice *reslice, const double plane[4])
{
  // Create a reslice matrix from the plane
  double matrix[16], invMatrix[16];
  vtkResliceMath::ConvertPlaneToResliceAxes(plane, matrix);
  vtkMatrix4x4::Invert(matrix, invMatrix);

  // Update the data
  vtkImageData *input = static_cast<vtkImageData *>(reslice->GetInput());
  input->Update();
  double spacing[3], origin[3];
  input->GetSpacing(spacing);
  input->GetOrigin(origin);
  int extent[6];
  input->GetWholeExtent(extent);

  // Compute center
  double center[4], radius[3];
  for (int i = 0; i < 3; i++)
    {
    center[i] = 0.5*(extent[2*i] + extent[2*i+1]);
    center[i] = center[i]*spacing[i] + origin[i];
    radius[i] = 0.5*(extent[2*i+1] - extent[2*i]);
    radius[i] *= spacing[i];
    }

  // Transform the center
  center[3] = 1.0;
  vtkMatrix4x4::MultiplyPoint(invMatrix, center, center);

  // Compute output spacing from input spacing
  spacing[0] = fabs(spacing[0]);
  spacing[1] = fabs(spacing[1]);
  spacing[2] = fabs(spacing[2]);
  double s[2], r[2];
  for (int j = 0; j < 2; j++)
    {
    double xc = matrix[4*j + 0];
    double yc = matrix[4*j + 1];
    double zc = matrix[4*j + 2];
    s[j] = (xc*xc*spacing[0] +
            yc*yc*spacing[1] +
            zc*zc*spacing[2])/sqrt(xc*xc + yc*yc + zc*zc);
    r[j] = (xc*xc*radius[0] +
            yc*yc*radius[1] +
            zc*zc*radius[2])/sqrt(xc*xc + yc*yc + zc*zc);
    }
  spacing[0] = s[0];
  spacing[1] = s[1];
  spacing[2] = 1.0;
  radius[0] = r[0];
  radius[1] = r[0];
  radius[2] = 1.0;

  origin[0] = center[0] - radius[0];
  origin[1] = center[1] - radius[1];
  origin[2] = 0.0;

  extent[0] = 0;
  extent[1] = vtkMath::Ceil(2*radius[0]/spacing[0]);
  extent[2] = 0;
  extent[3] = vtkMath::Ceil(2*radius[1]/spacing[1]);
  extent[4] = 0;
  extent[5] = 0;

  vtkMatrix4x4 *axes = reslice->GetResliceAxes();
  if (axes == 0)
    {
    axes = vtkMatrix4x4::New();
    reslice->SetResliceAxes(axes);
    axes->Delete();
    }

  axes->DeepCopy(matrix);
  reslice->SetOutputOrigin(origin);
  reslice->SetOutputSpacing(spacing);
  reslice->SetOutputExtent(extent);
}


//----------------------------------------------------------------------------
void vtkResliceMath::ConvertPlaneToResliceAxes(
  const double plane[4], double matrix[16])
{
  // We want to find the smallest possible rotation that rotates
  // either the xy, xz, or yz plane to match the given plane.

  // Find the largest component of the normal
  int maxi = 0;
  double maxv = 0.0;
  for (int i = 0; i < 3; i++)
    {
    double tmp = plane[i]*plane[i];
    if (tmp > maxv)
      {
      maxi = i;
      maxv = tmp;
      }
    }

  // Create the axis corresponding to that component
  double axis[3];
  axis[0] = 0.0;
  axis[1] = 0.0;
  axis[2] = 0.0;
  axis[maxi] = ((plane[maxi] < 0.0) ? -1.0 : 1.0);

  // Create two orthogonal axes
  double saxis[3], taxis[3];
  taxis[0] = 0.0;
  taxis[1] = 1.0;
  taxis[2] = 0.0;
  if (maxi == 1)
    {
    taxis[1] = 0.0;
    taxis[2] = 1.0;
    }
  vtkMath::Cross(taxis, axis, saxis);

  // Compute the rotation angle between the axis and the plane
  double vec[3];
  vtkMath::Cross(axis, plane, vec);
  double costheta = vtkMath::Dot(axis, plane);
  double sintheta = vtkMath::Norm(vec);
  double theta = atan2(sintheta, costheta);
  if (sintheta != 0)
    {
    vec[0] /= sintheta;
    vec[1] /= sintheta;
    vec[2] /= sintheta;
    }
  // create a quaternion
  costheta = cos(0.5*theta);
  sintheta = sin(0.5*theta);
  double quat[4];
  quat[0] = costheta;
  quat[1] = vec[0]*sintheta;
  quat[2] = vec[1]*sintheta;
  quat[3] = vec[2]*sintheta;
  // convert to matrix
  double mat[3][3];
  vtkMath::QuaternionToMatrix3x3(quat, mat);

  // Use matrix to rotate the other two axes
  double v1[3], v2[3];
  vtkMath::Multiply3x3(mat, saxis, v1);
  vtkMath::Multiply3x3(mat, taxis, v2);

  // Create a slice-to-data transform matrix
  matrix[0] = v1[0];
  matrix[1] = v2[0];
  matrix[2] = plane[0];
  matrix[3] = -plane[0]*plane[3];

  matrix[4] = v1[1];
  matrix[5] = v2[1];
  matrix[6] = plane[1];
  matrix[7] = -plane[1]*plane[3];

  matrix[8] = v1[2];
  matrix[9] = v2[2];
  matrix[10] = plane[2];
  matrix[11] = -plane[2]*plane[3];

  matrix[12] = 0.0;
  matrix[13] = 0.0;
  matrix[14] = 0.0;
  matrix[15] = 1.0;
}
