/*=========================================================================

  Program:   ToolCursor
  Module:    vtkFollowerPlane.h

  Copyright (c) 2011 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFollowerPlane.h"
#include "vtkMatrix4x4.h"
#include "vtkHomogeneousTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkFollowerPlane);
vtkCxxSetObjectMacro(vtkFollowerPlane,FollowPlane,vtkPlane);
vtkCxxSetObjectMacro(vtkFollowerPlane,FollowMatrix,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkFollowerPlane,FollowTransform,vtkHomogeneousTransform);

//----------------------------------------------------------------------------
vtkFollowerPlane::vtkFollowerPlane()
{
  this->FollowPlane = 0;
  this->FollowMatrix = 0;
  this->FollowTransform = 0;
  this->InvertFollowMatrix = 0;
  this->OffsetAlongNormal = 0.0;
}

//----------------------------------------------------------------------------
vtkFollowerPlane::~vtkFollowerPlane()
{
  if (this->FollowPlane)
    {
    this->FollowPlane->Delete();
    }
  if (this->FollowMatrix)
    {
    this->FollowMatrix->Delete();
    }
  if (this->FollowTransform)
    {
    this->FollowTransform->Delete();
    }
}

//----------------------------------------------------------------------------
double vtkFollowerPlane::EvaluateFunction(double x[3])
{
  this->Update();

  return ( this->Normal[0]*(x[0] - this->Origin[0]) + 
           this->Normal[1]*(x[1] - this->Origin[1]) + 
           this->Normal[2]*(x[2] - this->Origin[2]) );
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::EvaluateGradient(double vtkNotUsed(x)[3], double n[3])
{
  this->Update();

  for (int i=0; i<3; i++)
    {
    n[i] = this->Normal[i];
    }
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::SetInvertFollowMatrix(int val)
{
  if (this->InvertFollowMatrix != (val != 0))
    {
    this->InvertFollowMatrix = (val != 0);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::Update()
{
  if (this->GetMTime() > this->UpdateTime.GetMTime() && this->FollowPlane)
    {
    double normal[4], point[4];
    this->FollowPlane->GetNormal(normal);
    this->FollowPlane->GetOrigin(point);
    normal[3] = -vtkMath::Dot(normal, point);
    point[3] = 1.0;
    vtkMatrix4x4 *matrix = this->FollowMatrix;
    if (this->FollowTransform)
      {
      matrix = this->FollowTransform->GetMatrix();
      }

    if (matrix)
      {
      double elements[16], transpose[16];
      double *backward = elements;
      double *forward = *matrix->Element;
      vtkMatrix4x4::Invert(forward, backward);
      if (this->InvertFollowMatrix)
        {
        forward = elements;
        backward = *matrix->Element;
        }
      // use transpose of inverse to transform the normal
      vtkMatrix4x4::Transpose(backward, transpose);
      vtkMatrix4x4::MultiplyPoint(transpose, normal, normal);
      vtkMatrix4x4::MultiplyPoint(forward, point, point);
      point[0] /= point[3]; point[1] /= point[3]; point[2] /= point[3];
      vtkMath::Normalize(normal);
      }
    else
      {
      double d = this->OffsetAlongNormal;
      point[0] += d*normal[0];
      point[1] += d*normal[1];
      point[2] += d*normal[2];
      normal[0] = -normal[0];
      normal[1] = -normal[1];
      normal[2] = -normal[2];
      }

    this->Normal[0] = normal[0];
    this->Normal[1] = normal[1];
    this->Normal[2] = normal[2];

    this->Origin[0] = point[0];
    this->Origin[1] = point[1];
    this->Origin[2] = point[2];

    this->UpdateTime.Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkFollowerPlane::GetNormal()
{
  this->Update();
  return this->Normal;
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::GetNormal(double normal[3])
{
  this->Update();
  normal[0] = this->Normal[0];
  normal[1] = this->Normal[1];
  normal[2] = this->Normal[2];
}

//----------------------------------------------------------------------------
double *vtkFollowerPlane::GetOrigin()
{
  this->Update();
  return this->Origin;
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::GetOrigin(double origin[3])
{
  this->Update();
  origin[0] = this->Origin[0];
  origin[1] = this->Origin[1];
  origin[2] = this->Origin[2];
}

//----------------------------------------------------------------------------
unsigned long vtkFollowerPlane::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if (this->FollowPlane)
    {
    unsigned long mtime2 = this->FollowPlane->GetMTime();
    mtime = (mtime > mtime2 ? mtime : mtime2);
    }
  if (this->FollowMatrix)
    {
    unsigned long mtime2 = this->FollowMatrix->GetMTime();
    mtime = (mtime > mtime2 ? mtime : mtime2);
    }
  if (this->FollowTransform)
    {
    unsigned long mtime2 = this->FollowTransform->GetMTime();
    mtime = (mtime > mtime2 ? mtime : mtime2);
    }

  return mtime;
}

//----------------------------------------------------------------------------
void vtkFollowerPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FollowPlane: " << this->FollowPlane << "\n";
  os << indent << "FollowMatrix: " << this->FollowMatrix << "\n";
  os << indent << "FollowTransform: " << this->FollowTransform << "\n";
  os << indent << "InvertFollowMatrix: "
     << (this->InvertFollowMatrix ? "On\n" : "Off\n");
  os << indent << "OffsetAlongNormal: " << this->OffsetAlongNormal << "\n";
}
