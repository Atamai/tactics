/*=========================================================================

  Program:   ToolCursor
  Module:    vtkROIContourDataToPolyData.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkROIContourDataToPolyData.h"

#include "vtkROIContourData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPlane.h"
#include "vtkKochanekSpline.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkROIContourDataToPolyData);
vtkCxxSetObjectMacro(vtkROIContourDataToPolyData,SelectionPlane,vtkPlane);
vtkCxxSetObjectMacro(vtkROIContourDataToPolyData,Spline,vtkSpline);

//----------------------------------------------------------------------------
vtkROIContourDataToPolyData::vtkROIContourDataToPolyData()
{
  this->SelectionPlane = NULL;
  this->SelectionPlaneTolerance = 0.5;
  this->Subdivision = 0;
  this->SubdivisionTarget = 1.0;
  this->Spline = 0;

  this->SplineX = 0;
  this->SplineY = 0;
  this->SplineZ = 0;
  this->KnotPositions = 0;
}

//----------------------------------------------------------------------------
vtkROIContourDataToPolyData::~vtkROIContourDataToPolyData()
{
  if (this->SelectionPlane)
    {
    this->SelectionPlane->Delete();
    }
  if (this->Spline)
    {
    this->Spline->Delete();
    }
  if (this->SplineX)
    {
    this->SplineX->Delete();
    this->SplineY->Delete();
    this->SplineZ->Delete();
    }
  if (this->KnotPositions)
    {
    this->KnotPositions->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkROIContourDataToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SelectionPlane: " << this->SelectionPlane << "\n";
  os << indent << "SelectionPlaneTolerance: "
     << this->SelectionPlaneTolerance << "\n";
  os << indent << "Subdivision: " << (this->Subdivision ? "On\n" : "Off\n");
  os << indent << "SubdivisionTarget: " << this->SubdivisionTarget << "\n";
  os << indent << "Spline: " << this->Spline << "\n";
}

//----------------------------------------------------------------------------
int vtkROIContourDataToPolyData::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkROIContourData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkROIContourDataToPolyData::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  unsigned long* mtime)
{
  unsigned long mTime = this->GetMTime();

  vtkPlane *plane = this->SelectionPlane;

  if (plane)
    {
    unsigned long planeMTime = plane->GetMTime();
    if (planeMTime > mTime)
      {
      mTime = planeMTime;
      }
    }

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
// Compute SplineX, SplineY, and SplineZ for the given points, using the
// accumulated distance between points as the parameter for the spline.
// Set "closed" to one if the splines should form a closed loop.
// The reference parameter tmax will be set to the maximum parameter
// value for the spline, and dmax will be set to the full length of
// the contour, computed by summing the lengths of the line segments.
void vtkROIContourDataToPolyData::ComputeSpline(
  vtkPoints *points, bool closed, double &tmax, double &dmax)
{
  if (this->SplineX && !this->SplineX->IsA(this->Spline->GetClassName()))
    {
    this->SplineX->Delete();
    this->SplineY->Delete();
    this->SplineZ->Delete();

    this->SplineX = 0;
    this->SplineY = 0;
    this->SplineZ = 0;
    }
  if (this->SplineX == 0)
    {
    this->SplineX = this->Spline->NewInstance();
    this->SplineX->DeepCopy(this->Spline);
    this->SplineY = this->Spline->NewInstance();
    this->SplineY->DeepCopy(this->Spline);
    this->SplineZ = this->Spline->NewInstance();
    this->SplineZ->DeepCopy(this->Spline);
    }
  if (this->KnotPositions == 0)
    {
    this->KnotPositions = vtkDoubleArray::New();
    }

  vtkSpline *xspline = this->SplineX;
  vtkSpline *yspline = this->SplineY;
  vtkSpline *zspline = this->SplineZ;
  vtkDoubleArray *knots = this->KnotPositions;

  // initialize the spline
  xspline->RemoveAllPoints();
  yspline->RemoveAllPoints();
  zspline->RemoveAllPoints();
  knots->Initialize();

  // set whether splines are closed
  xspline->SetClosed(closed);
  yspline->SetClosed(closed);
  zspline->SetClosed(closed);

  // get the number of points
  vtkIdType n = points->GetNumberOfPoints();
  double p0[3], p[3];

  // factor between real distance and parametric distance
  double f = 1.0;
  // the length of the implicit segment for closed loops
  double lastd = 0;

  // verify that there are enough knots for the spline
  if (n < 2)
    {
    tmax = 0;
    dmax = 0;
    return;
    }

  // get the first and last point
  points->GetPoint(0, p0);

  if (closed)
    {
    // require a tolerance, base it off the desired subdivision
    double tol = this->SubdivisionTarget*1e-3;
    tol *= tol;

    // sometimes that last point (or several last points) are almost exactly
    // on top of the first point, we must ignore such points
    vtkIdType m = n;
    do
      {
      points->GetPoint(--m, p);
      lastd = vtkMath::Distance2BetweenPoints(p0, p);
      }
    while (m > 0 && lastd < tol);
    n = m + 1;

    // set factor to scale the implicit segment to unity
    if (lastd > 0)
      {
      lastd = sqrt(lastd);
      f = 1.0/lastd;
      }
    }

  // verify that there are still enough knots for the spline
  if (n < 2)
    {
    tmax = 0;
    dmax = 0;
    return;
    }

  // add all the points to the spline
  double d = 0.0;
  for (vtkIdType i = 0; i < n; i++)
    {
    points->GetPoint(i, p);
    d += sqrt(vtkMath::Distance2BetweenPoints(p0, p));

    double t = f*d;

    xspline->AddPoint(t, p[0]);
    yspline->AddPoint(t, p[1]);
    zspline->AddPoint(t, p[2]);
    knots->InsertNextValue(t);

    p0[0] = p[0];
    p0[1] = p[1];
    p0[2] = p[2];
    }

  // do the spline precomputations
  xspline->Compute();
  yspline->Compute();
  zspline->Compute();

  // the spline is valid over t = [0, tmax]
  d += lastd;
  tmax = f*d;
  dmax = d;

  // add another knot point for closed splines
  if (closed)
    {
    knots->InsertNextValue(tmax);
    }
}


//----------------------------------------------------------------------------
bool vtkROIContourDataToPolyData::GenerateSpline(
  vtkPoints *contourPoints, bool closed,
  vtkPoints *points, vtkCellArray *lines, vtkIntArray *subIds)
{
  double tmax, dmax;
  this->ComputeSpline(contourPoints, closed, tmax, dmax);

  vtkSpline *xspline = this->SplineX;
  vtkSpline *yspline = this->SplineY;
  vtkSpline *zspline = this->SplineZ;

  vtkDoubleArray *knots = this->KnotPositions;
  vtkIdType m = knots->GetNumberOfTuples();

  if (m < 2)
    {
    return false;
    }

  // Because InsertNextPoint is very slow
  vtkDoubleArray *da = vtkDoubleArray::SafeDownCast(points->GetData());

  vtkIdType id0 = points->GetNumberOfPoints();
  double t0 = 0;
  double f = dmax/(tmax*this->SubdivisionTarget);
  for (vtkIdType j = 1; j < m; j++)
    {
    double t1 = knots->GetValue(j);
    int n = vtkMath::Floor((t1 - t0)*f) + 1;
    vtkIdType id = points->GetNumberOfPoints();
    double *p = da->WritePointer(id*3, n*3);
    for (int i = 0; i < n; i++)
      {
      double t = (t0*(n-i) + t1*i)/n;
      p[0] = xspline->Evaluate(t);
      p[1] = yspline->Evaluate(t);
      p[2] = zspline->Evaluate(t);
      p += 3;
      }
    if (subIds)
      {
      int *iptr = subIds->WritePointer(subIds->GetMaxId()+1, n);
      int k = j-1;
      do { *iptr++ = k; } while (--n);
      }
    t0 = t1;
    }

  if (!closed)
    {
    double p[3];
    p[0] = xspline->Evaluate(tmax);
    p[1] = yspline->Evaluate(tmax);
    p[2] = zspline->Evaluate(tmax);
    points->InsertNextPoint(p);
    if (subIds)
      {
      subIds->InsertNextValue(m-1);
      }
    }

  vtkIdType id1 = points->GetNumberOfPoints();
  lines->SetNumberOfCells(lines->GetNumberOfCells() + 1);
  vtkIdTypeArray *ia = lines->GetData();
  vtkIdType cellSize = id1 - id0 + closed;
  vtkIdType *iptr = ia->WritePointer(ia->GetMaxId()+1, cellSize+1);
  *iptr++ = cellSize;
  for (vtkIdType id = id0; id < id1; id++)
    {
    *iptr++ = id;
    }
  if (closed)
    {
    *iptr++ = id0;
    }

  // Free any memory that was used
  xspline->RemoveAllPoints();
  yspline->RemoveAllPoints();
  zspline->RemoveAllPoints();
  knots->Initialize();

  return true;
}

//----------------------------------------------------------------------------
// If a vtkSpline is not provided, do the spline computations here
// (several times faster than using vtkSpline to make a generic spline)
bool vtkROIContourDataToPolyData::CatmullRomSpline(
  vtkPoints *contourPoints, bool closed,
  vtkPoints *points, vtkCellArray *lines, vtkIntArray *subIds)
{
  vtkIdType m = contourPoints->GetNumberOfPoints();

  if (closed && m > 2)
    {
    // require a tolerance, base it off the desired subdivision
    double tol = this->SubdivisionTarget*1e-3;
    tol *= tol;

    // ignore all end point that are the same as first point
    double p0[3], p[3];
    contourPoints->GetPoint(0, p0);
    double lastd = 0;
    do
      {
      contourPoints->GetPoint(--m, p);
      lastd = vtkMath::Distance2BetweenPoints(p0, p);
      }
    while (m > 0 && lastd < tol);
    m += 1;
    }

  if (m < 2)
    {
    return false;
    }

  // Save the initial size of the point array
  vtkIdType id0 = points->GetNumberOfPoints();

  // For fast writing to point data
  vtkDoubleArray *da = vtkDoubleArray::SafeDownCast(points->GetData());

  double p1[3], p0[3], p2[3];
  contourPoints->GetPoint(m-1, p2);
  contourPoints->GetPoint(0, p0);
  contourPoints->GetPoint(1, p1);
  double d1 = sqrt(vtkMath::Distance2BetweenPoints(p0, p2));
  double d0 = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
  double f = 1.0/(d0 + d1)*closed;
  double dx0 = (p1[0] - p2[0])*f;
  double dy0 = (p1[1] - p2[1])*f;
  double dz0 = (p1[2] - p2[2])*f;

  int m1 = static_cast<int>(m) + closed - 1;

  for (int j = 0; j < m1; j++)
    {
    int jp2 = (j + 2) % m;
    contourPoints->GetPoint(jp2, p2);
    d1 = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
    f = 1.0/(d0 + d1)*(closed | ((j+2) < m));
    double dx1 = (p2[0] - p0[0])*f;
    double dy1 = (p2[1] - p0[1])*f;
    double dz1 = (p2[2] - p0[2])*f;

    int n = vtkMath::Floor(d0/this->SubdivisionTarget) + 1;
    double dt = 1.0/n;
    double t = 0.0;

    double x0 = p0[0];
    double dx = p1[0] - x0;
    double dx2 = d0*dx0;
    double dx3 = dx2 + d0*dx1;
    double cx[4];
    cx[0] = x0;
    cx[1] = dx2;
    cx[2] = dx + dx + dx - dx2 - dx3;
    cx[3] = dx3 - dx - dx;

    double y0 = p0[1];
    double dy = p1[1] - y0;
    double dy2 = d0*dy0;
    double dy3 = dy2 + d0*dy1;
    double cy[4];
    cy[0] = y0;
    cy[1] = dy2;
    cy[2] = dy + dy + dy - dy2 - dy3;
    cy[3] = dy3 - dy - dy;

    double z0 = p0[2];
    double dz = p1[2] - z0;
    double dz2 = d0*dz0;
    double dz3 = dz2 + d0*dz1;
    double cz[4];
    cz[0] = z0;
    cz[1] = dz2;
    cz[2] = dz + dz + dz - dz2 - dz3;
    cz[3] = dz3 - dz - dz;

    vtkIdType id = points->GetNumberOfPoints();
    double *q = da->WritePointer(3*id, 3*n);
    int i = n;
    do
      {
      double t2 = t*t;
      double t3 = t2*t;
      q[0] = cx[0] + t*cx[1] + t2*cx[2] + t3*cx[3];
      q[1] = cy[0] + t*cy[1] + t2*cy[2] + t3*cy[3];
      q[2] = cz[0] + t*cz[1] + t2*cz[2] + t3*cz[3];

      q += 3;
      t += dt;
      }
    while (--i);

    if (subIds)
      {
      id = subIds->GetMaxId() + 1;
      int *iptr = subIds->WritePointer(id, n);
      do { *iptr++ = j; } while (--n);
      }

    p0[0] = p1[0];
    p0[1] = p1[1];
    p0[2] = p1[2];

    p1[0] = p2[0];
    p1[1] = p2[1];
    p1[2] = p2[2];

    dx0 = dx1;
    dy0 = dy1;
    dz0 = dz1;

    d0 = d1;
    }

  if (!closed)
    {
    double q[3];
    contourPoints->GetPoint(m1, q);
    points->InsertNextPoint(q);
    if (subIds)
      {
      subIds->InsertNextValue(m1);
      }
    }

  vtkIdType id1 = points->GetNumberOfPoints();
  lines->SetNumberOfCells(lines->GetNumberOfCells() + 1);
  vtkIdTypeArray *ia = lines->GetData();
  vtkIdType cellSize = id1 - id0 + closed;
  vtkIdType *iptr = ia->WritePointer(ia->GetMaxId()+1, cellSize+1);
  *iptr++ = cellSize;
  for (vtkIdType id = id0; id < id1; id++)
    {
    *iptr++ = id;
    }
  if (closed)
    {
    *iptr++ = id0;
    }

  return true;
}

//----------------------------------------------------------------------------
int vtkROIContourDataToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkROIContourData *input = vtkROIContourData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // For selecting which contours to include in the output
  vtkPlane *plane = this->SelectionPlane;
  double tol = this->SelectionPlaneTolerance;

  // The output points and cells
  vtkPoints *outPoints = vtkPoints::New(VTK_DOUBLE);
  vtkCellArray *lines = 0;
  vtkCellArray *verts = 0;

  // The scalars
  vtkIntArray *contourIds = vtkIntArray::New();
  contourIds->SetName("Labels");
  vtkIntArray *contourSubIds = vtkIntArray::New();
  contourSubIds->SetName("SubIds");

  // Go through all the contours
  int n = input->GetNumberOfContours();
  for (int i = 0; i < n; i++)
    {
    vtkPoints *points = input->GetContourPoints(i);
    int t = input->GetContourType(i);

    if (points)
      {
      vtkIdType m = points->GetNumberOfPoints();
      bool includeContour = true;

      if (plane)
        {
        for (int j = 0; j < m; j++)
          {
          double p[3];
          points->GetPoint(j, p);
          double d = plane->DistanceToPlane(p);
          if (d < -tol || d > tol)
            {
            includeContour = false;
            break;
            }
          }
        }

      // Include this contour in the output
      if (includeContour)
        {
        bool closed = false;
        vtkCellArray *cells = 0;
        if (t == vtkROIContourData::POINT)
          {
          if (!verts)
            {
            verts = vtkCellArray::New();
            }
          cells = verts;
          }
        else
          {
          if (!lines)
            {
            lines = vtkCellArray::New();
            }
          cells = lines;

          if (t == vtkROIContourData::CLOSED_PLANAR)
            {
            closed = true;
            }
          }

        // Cell requires extra point id if contour is closed
        vtkIdType cellSize = m + closed;
        bool success = false;

        if (this->Subdivision && m > 2 && t != vtkROIContourData::POINT)
          {
          // Use a spline to subdivide and smooth contour
          if (this->Spline)
            {
            success = this->GenerateSpline(
              points, closed, outPoints, lines, contourSubIds);
            }
          else
            {
            success = this->CatmullRomSpline(
              points, closed, outPoints, lines, contourSubIds);
            }
          }
        else if (m > 0)
          {
          // Add the contour without subdivision
          cells->SetNumberOfCells(cells->GetNumberOfCells() + 1);
          vtkIdTypeArray *ida = cells->GetData();
          vtkIdType *idptr = ida->WritePointer(ida->GetMaxId()+1, cellSize+1);
          *idptr++ = cellSize;

          vtkIdType firstPointId = outPoints->GetNumberOfPoints();

          vtkDoubleArray *da =
            vtkDoubleArray::SafeDownCast(outPoints->GetData());
          vtkIdType id = firstPointId;
          double *p = da->WritePointer(id*3, m*3);

          for (int j = 0; j < m; j++)
            {
            points->GetPoint(j, p);
            *idptr++ = id++;
            p += 3;
            }
          if (contourSubIds)
            {
            id = contourSubIds->GetMaxId() + 1;
            int *iptr = contourSubIds->WritePointer(id, m);
            for (int j = 0; j < m; j++)
              {
              *iptr++ = j;
              }
            }

          // Close the contour, if necessary
          if (cellSize > m)
            {
            *idptr++ = firstPointId;
            }
          success = true;
          }

        // Add a scalar to allow identification of countour
        if (contourIds && success)
          {
          contourIds->InsertNextValue(i);
          }
        }
      }
    }

  output->SetPoints(outPoints);
  output->SetLines(lines);
  output->SetVerts(verts);
  output->GetCellData()->SetScalars(contourIds);
  output->GetPointData()->AddArray(contourSubIds);

  if (outPoints)
    {
    outPoints->Delete();
    }
  if (lines)
    {
    lines->Delete();
    }
  if (verts)
    {
    verts->Delete();
    }
  if (contourIds)
    {
    contourIds->Delete();
    }
  if (contourSubIds)
    {
    contourSubIds->Delete();
    }

  // assign colors to the output points
  unsigned char color[3] = { 255, 0, 0 };
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  colors->SetName("Colors");
  vtkIdType m = outPoints->GetNumberOfPoints();
  colors->SetNumberOfTuples(m);
  for (vtkIdType j = 0; j < m; j++)
    {
    colors->SetTupleValue(j, color);
    }

  output->GetPointData()->SetScalars(colors);
  colors->Delete();

  return 1;
}
