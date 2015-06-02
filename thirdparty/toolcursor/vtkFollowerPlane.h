/*=========================================================================

  Program:   ToolCursor
  Module:    vtkFollowerPlane.h

  Copyright (c) 2011 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFollowerPlane - a plane that follows another plane
// .SECTION Description
// vtkFollowerPlane takes a plane and a transform (or matrix) and
// automatically updates itself so that its origin and normal are
// always the transform of the followed plane's origin and normal.

#ifndef __vtkFollowerPlane_h
#define __vtkFollowerPlane_h

#include "vtkPlane.h"

class vtkMatrix4x4;
class vtkHomogeneousTransform;

class VTK_EXPORT vtkFollowerPlane : public vtkPlane
{
public:
  static vtkFollowerPlane *New();
  vtkTypeMacro(vtkFollowerPlane,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the plane to follow.
  void SetFollowPlane(vtkPlane *plane);
  vtkPlane *GetFollowPlane() { return this->FollowPlane; }

  // Description:
  // Set a matrix to transform the followed plane.  You can use this
  // or SetFollowTransform, but not both.
  void SetFollowMatrix(vtkMatrix4x4 *matrix);
  vtkMatrix4x4 *GetFollowMatrix() { return this->FollowMatrix; };

  // Description:
  // If this is true, then the inverse of the matrix will be applied
  // to the followed plane.
  void SetInvertFollowMatrix(int val);
  void InvertFollowMatrixOn() { this->SetInvertFollowMatrix(1); }
  void InvertFollowMatrixOff() { this->SetInvertFollowMatrix(0); }
  int GetInvertFollowMatrix() { return this->InvertFollowMatrix; }
  
  // Description:
  // Set a transform to transform the followed plane.  You can use this
  // or SetFollowMatrix, but not both.
  void SetFollowTransform(vtkHomogeneousTransform *matrix);
  vtkHomogeneousTransform *GetFollowTransform() { return this->FollowTransform; };

  // Description:
  // Set an offset along the normal for the new plane.
  vtkSetMacro(OffsetAlongNormal, double);
  vtkGetMacro(OffsetAlongNormal, double);

  // Description:
  // Evaluate plane equation for point x[3].
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z) {
    return this->vtkImplicitFunction::EvaluateFunction(x, y, z); }

  // Description:
  // Evaluate function gradient at point x[3].
  void EvaluateGradient(double x[3], double g[3]);

  // Description:
  // Get the normal.
  void GetNormal(double normal[3]);
  double *GetNormal();

  // Description:
  // Get the origin.
  void GetOrigin(double origin[3]);
  double *GetOrigin();

  // Description:
  // Get the MTime, including the followed plane and matrix or transform.
  unsigned long GetMTime();

protected:
  vtkFollowerPlane();
  ~vtkFollowerPlane();

  void Update();

  vtkPlane *FollowPlane;
  vtkMatrix4x4 *FollowMatrix;
  vtkHomogeneousTransform *FollowTransform;
  int InvertFollowMatrix;
  vtkTimeStamp UpdateTime;

  double OffsetAlongNormal;

private:
  vtkFollowerPlane(const vtkFollowerPlane&);  // Not implemented.
  void operator=(const vtkFollowerPlane&);  // Not implemented.
};

#endif
