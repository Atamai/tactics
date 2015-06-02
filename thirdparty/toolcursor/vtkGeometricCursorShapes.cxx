/*=========================================================================

  Program:   ToolCursor
  Module:    vtkGeometricCursorShapes.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeometricCursorShapes.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkGeometricCursorShapes);

//----------------------------------------------------------------------------
vtkGeometricCursorShapes::vtkGeometricCursorShapes()
{
  this->MakeShapes();
}

//----------------------------------------------------------------------------
vtkGeometricCursorShapes::~vtkGeometricCursorShapes()
{
}

//----------------------------------------------------------------------------
void vtkGeometricCursorShapes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkGeometricCursorShapes::MakeShapes()
{
  vtkDataSet *data;

  data = this->MakeCrossShape(0);
  this->AddShape("Cross", data, 0);
  data->Delete();

  data = this->MakeCrossShape(1);
  this->AddShape("SplitCross", data, 0);
  data->Delete();

  data = this->MakeSphereShape(0);
  this->AddShape("Sphere", data, 0);
  data->Delete();

  data = this->MakeSphereShape(1);
  this->AddShape("SplitSphere", data, 0);
  data->Delete();

  data = this->MakeConeShape(0);
  this->AddShape("Cone", data, 0);
  data->Delete();

  data = this->MakeConeShape(1);
  this->AddShape("DualCone", data, 0);
  data->Delete();
}

//----------------------------------------------------------------------------
vtkDataSet *vtkGeometricCursorShapes::MakeCrossShape(int dual)
{
  double radius = 10.0;
  double inner = 3.5;
  double thickness = 2.0;

  double xmin = inner;
  double xmax = radius;
  double ymin = -thickness*0.5;
  double ymax = +thickness*0.5;
  double zmin = 0;
  double zmax = thickness*0.5;

  vtkIntArray *scalars = vtkIntArray::New();
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *strips = vtkCellArray::New();
  vtkIdType nPoints = 0;

  int colorIndex = 0;

  for (int i = 0; i < 2; i++)
    {
    for (int j = 0; j < 4; j++)
      {
      double z = zmax;
      for (int k = 0; k < 2; k++)
        {
        points->InsertNextPoint(xmin, ymin, z);
        points->InsertNextPoint(xmin, ymax, z);
        points->InsertNextPoint(xmax, ymax, z);
        points->InsertNextPoint(xmax, ymin, z);
        scalars->InsertNextTupleValue(&colorIndex);
        scalars->InsertNextTupleValue(&colorIndex);
        scalars->InsertNextTupleValue(&colorIndex);
        scalars->InsertNextTupleValue(&colorIndex);
        z = zmin;
        }

      static vtkIdType rectIds[5][4] = { { 1, 0, 2, 3 },
                                         { 4, 0, 5, 1 },
                                         { 5, 1, 6, 2 },
                                         { 6, 2, 7, 3 },
                                         { 7, 3, 4, 0 } };
      vtkIdType pointIds[4];
      for (int ii = 0; ii < 5; ii++)
        {
        for (int jj = 0; jj < 4; jj++)
          {
          pointIds[jj] = rectIds[ii][jj]+nPoints;
          }
        strips->InsertNextCell(4, pointIds);
        }
      nPoints += 8;

      // do a rotation of 90 degrees for next piece
      double tmp1 = ymin;
      double tmp2 = ymax;
      ymin = -xmax;
      ymax = -xmin;
      xmin = tmp1;
      xmax = tmp2;
      }

    // do the other side
    zmin = -zmin;
    zmax = -zmax;
    xmin = -xmin;
    xmax = -xmax;

    if (dual)
      {
      colorIndex = 1;
      }
    }

  vtkPolyData *data = vtkPolyData::New();
  data->SetPoints(points);
  points->Delete();
  data->SetStrips(strips);
  strips->Delete();
  data->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  return data;
}

//----------------------------------------------------------------------------
vtkDataSet *vtkGeometricCursorShapes::MakeSphereShape(int dual)
{
  double pi = vtkMath::DoublePi();
  double radius = 5.0;
  int resolution = 9;

  vtkIdType *pointIds = new vtkIdType[4*(resolution+1)];

  vtkIntArray *scalars = vtkIntArray::New();
  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *strips = vtkCellArray::New();
  vtkIdType nPoints = 0;

  int colorIndex = 0;

  for (int i = 0; i < 2; i++)
    {
    // The sign (i.e. for top or bottom) is stored in s
    double s = 1 - 2*i;

    // The unit position vector of the point is stored in v
    double v[3];
    v[0] = 0;
    v[1] = 0;
    v[2] = s;

    points->InsertNextPoint(radius*v[0], radius*v[1], radius*v[2]);
    normals->InsertNextTupleValue(v);
    scalars->InsertNextTupleValue(&colorIndex);

    int n = (resolution + 1)/2;
    int m = 2*resolution;

    for (int j = 1; j <= n; j++)
      {
      double phi = pi*j/resolution;
      double r = sin(phi);
      v[2] = cos(phi)*s;
      if (2*j >= resolution)
        {
        v[2] = 0;
        }

      for (int k = 0; k < m; k++)
        {
        double theta = pi*k/resolution;
        v[0] = r*cos(theta);
        v[1] = r*sin(theta)*s;
        points->InsertNextPoint(radius*v[0], radius*v[1], radius*v[2]);
        normals->InsertNextTupleValue(v);
        scalars->InsertNextTupleValue(&colorIndex);
        }
      }

    // Make the fan for the top
    pointIds[0] = nPoints++;
    for (int ii = 0; ii < (m-1); ii++)
      {
      pointIds[1] = nPoints + ii;
      pointIds[2] = nPoints + ii + 1;
      strips->InsertNextCell(3, pointIds);
      }
    pointIds[1] = nPoints + m - 1;
    pointIds[2] = nPoints;
    strips->InsertNextCell(3, pointIds);

    // Make the strips for the rest
    for (int jj = 1; jj < n; jj++)
      {
      for (int kk = 0; kk < m; kk++)
        {
        pointIds[2*kk] = nPoints + kk;
        pointIds[2*kk+1] = nPoints + kk + m;
        }
      pointIds[2*m] = nPoints;
      pointIds[2*m+1] = nPoints + m;
      strips->InsertNextCell(2*(m+1), pointIds);
      nPoints += m;
      }

    nPoints += m;

    if (dual)
      {
      colorIndex = 1;
      }
    }

  delete [] pointIds;

  vtkPolyData *data = vtkPolyData::New();
  data->SetPoints(points);
  points->Delete();
  data->SetStrips(strips);
  strips->Delete();
  data->GetPointData()->SetScalars(scalars);
  scalars->Delete();
  data->GetPointData()->SetNormals(normals);
  normals->Delete();

  return data;
}

//----------------------------------------------------------------------------
vtkDataSet *vtkGeometricCursorShapes::MakeConeShape(int dual)
{
  double pi = vtkMath::DoublePi();
  double radius = 8.0;
  double height = 15.0;
  int resolution = 20;

  vtkIdType *pointIds = new vtkIdType[2*(resolution+1)];

  vtkIntArray *scalars = vtkIntArray::New();
  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *strips = vtkCellArray::New();
  vtkIdType nPoints = 0;

  int sides = (dual ? 2 : 1);

  for (int colorIndex = 0; colorIndex < sides; colorIndex++)
    {
    // The sign (i.e. for top or bottom) is stored in s
    double s = 1 - 2*colorIndex;

    // The length of the side of the cone
    double l = sqrt(radius*radius + height*height);
    double f1 = radius/l;
    double f2 = height/l;

    // The unit normal vector
    double v[3];

    // The point of the cone
    for (int i = 0; i < 2; i++)
      {
      double r = radius*i;
      double z = height*i;
      double offset = 0.5*(1 - i);

      for (int j = 0; j < resolution; j++)
        {
        double theta = 2*pi*(j + offset)/resolution;
        double ct = cos(theta);
        double st = sin(theta);
        v[0] = f2*ct;
        v[1] = f2*st*s;
        v[2] = -f1*s;
        points->InsertNextPoint(r*ct, r*st*s, z*s);
        normals->InsertNextTupleValue(v);
        scalars->InsertNextTupleValue(&colorIndex);
        }
      }

    // The base of the cone
    v[0] = 0;
    v[1] = 0;
    v[2] = s;
    points->InsertNextPoint(0, 0, height*s);
    normals->InsertNextTupleValue(v);
    scalars->InsertNextTupleValue(&colorIndex);

    for (int k = 0; k < resolution; k++)
      {
      double theta = 2*pi*k/resolution;
      points->InsertNextPoint(radius*cos(theta), radius*sin(theta)*s, height*s);
      normals->InsertNextTupleValue(v);
      scalars->InsertNextTupleValue(&colorIndex);
      }

    // Make the fan for the top
    for (int ii = 0; ii < (resolution-1); ii++)
      {
      pointIds[0] = nPoints + ii;
      pointIds[1] = nPoints + ii + resolution + 1;
      pointIds[2] = nPoints + ii + resolution;
      strips->InsertNextCell(3, pointIds);
      }
    pointIds[0] = nPoints + 2*resolution - 1;
    pointIds[1] = nPoints + resolution - 1;
    pointIds[2] = nPoints + resolution;
    strips->InsertNextCell(3, pointIds);
    nPoints += 2*resolution;

    // Make the fan for the base
    pointIds[0] = nPoints++;
    for (int jj = 0; jj < (resolution-1); jj++)
      {
      pointIds[1] = nPoints + jj;
      pointIds[2] = nPoints + jj + 1;
      strips->InsertNextCell(3, pointIds);
      }
    pointIds[1] = nPoints + resolution - 1;
    pointIds[2] = nPoints;
    strips->InsertNextCell(3, pointIds);
    nPoints += resolution;
    }

  delete [] pointIds;

  vtkPolyData *data = vtkPolyData::New();
  data->SetPoints(points);
  points->Delete();
  data->SetStrips(strips);
  strips->Delete();
  data->GetPointData()->SetScalars(scalars);
  scalars->Delete();
  data->GetPointData()->SetNormals(normals);
  normals->Delete();

  return data;
}

