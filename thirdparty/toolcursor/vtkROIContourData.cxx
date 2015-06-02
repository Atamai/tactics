/*=========================================================================

  Program:   ToolCursor
  Module:    vtkROIContourData.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkROIContourData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"

#include <stddef.h>
#include <vector>

vtkStandardNewMacro(vtkROIContourData);

//----------------------------------------------------------------------------
struct vtkROIContourElement
{
  vtkROIContourElement() {
    this->GeometricType = vtkROIContourData::CLOSED_PLANAR; }

  vtkSmartPointer<vtkPoints> Points;
  int GeometricType;
};

//----------------------------------------------------------------------------
class vtkROIContourVector : public std::vector<vtkROIContourElement>
{
};

//----------------------------------------------------------------------------
vtkROIContourData::vtkROIContourData()
{
  this->Contours = new vtkROIContourVector;
  this->NumberOfContours = 0;
}

//----------------------------------------------------------------------------
vtkROIContourData::~vtkROIContourData()
{
  delete this->Contours;
}

//----------------------------------------------------------------------------
void vtkROIContourData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfContours: " << this->NumberOfContours << "\n";
}

//----------------------------------------------------------------------------
void vtkROIContourData::SetNumberOfContours(int n)
{
  if (n != this->NumberOfContours && n >= 0)
    {
    this->NumberOfContours = n;
    this->Contours->resize(static_cast<size_t>(n));
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkROIContourData::SetContourPoints(int i, vtkPoints *points)
{
  if (i < 0 || i >= this->NumberOfContours)
    {
    vtkErrorMacro("index " << i << " is too large");
    }
  else
    {
    vtkROIContourElement *contour = &(*this->Contours)[static_cast<size_t>(i)];
    if (contour->Points != points)
      {
      contour->Points = points;
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
vtkPoints *vtkROIContourData::GetContourPoints(int i)
{
  vtkPoints *points = 0;

  if (i < 0 || i >= this->NumberOfContours)
    {
    vtkErrorMacro("index " << i << " is too large");
    }
  else
    {
    vtkROIContourElement *contour = &(*this->Contours)[static_cast<size_t>(i)];
    points = contour->Points;
    }

  return points;
}

//----------------------------------------------------------------------------
void vtkROIContourData::SetContourType(int i, int t)
{
  if (i < 0 || i >= this->NumberOfContours)
    {
    vtkErrorMacro("index " << i << " is too large");
    }
  else if (t < vtkROIContourData::POINT || t > vtkROIContourData::CLOSED_PLANAR)
    {
    vtkErrorMacro("unrecognized contour type " << t);
    }
  else
    {
    vtkROIContourElement *contour = &(*this->Contours)[static_cast<size_t>(i)];
    if (contour->GeometricType != t)
      {
      contour->GeometricType = t;
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkROIContourData::GetContourType(int i)
{
  int t = 0;

  if (i < 0 || i >= this->NumberOfContours)
    {
    vtkErrorMacro("index " << i << " is too large");
    }
  else
    {
    vtkROIContourElement *contour = &(*this->Contours)[static_cast<size_t>(i)];
    t = contour->GeometricType;
    }

  return t;
}

//----------------------------------------------------------------------------
void vtkROIContourData::RemoveContour(int i)
{
  if (i < 0 || i >= this->NumberOfContours)
    {
    vtkErrorMacro("index " << i << " is too large");
    }
  else
    {
    this->Contours->erase(this->Contours->begin() + i);
    }
}

//----------------------------------------------------------------------------
void vtkROIContourData::Initialize()
{
  this->Contours->clear();
  this->NumberOfContours = 0;
}

//----------------------------------------------------------------------------
void vtkROIContourData::DeepCopy(vtkDataObject *o)
{
  vtkROIContourData *src = vtkROIContourData::SafeDownCast(o);

  if (src && src != this)
    {
    int n = src->GetNumberOfContours();
    this->NumberOfContours = n;
    this->Contours->resize(static_cast<size_t>(n));
    vtkROIContourElement *contour = 0;
    for (int i = 0; i < n; i++)
      {
      contour = &(*this->Contours)[static_cast<size_t>(i)];
      vtkPoints *points = src->GetContourPoints(i);
      if (points)
        {
        vtkPoints *newpoints = vtkPoints::New();
        newpoints->DeepCopy(points);
        points = newpoints;
        }
      contour->Points = points;
      contour->GeometricType = src->GetContourType(i);
      }

    this->Modified();
    }

  this->Superclass::DeepCopy(o);
}

//----------------------------------------------------------------------------
void vtkROIContourData::ShallowCopy(vtkDataObject *o)
{
  vtkROIContourData *src = vtkROIContourData::SafeDownCast(o);

  if (src && src != this)
    {
    int n = src->GetNumberOfContours();
    this->NumberOfContours = n;
    this->Contours->resize(static_cast<size_t>(n));
    vtkROIContourElement *contour = 0;
    for (int i = 0; i < n; i++)
      {
      contour = &(*this->Contours)[static_cast<size_t>(i)];
      contour->Points = src->GetContourPoints(i);
      contour->GeometricType = src->GetContourType(i);
      }

    this->Modified();
    }

  this->Superclass::DeepCopy(o);
}
