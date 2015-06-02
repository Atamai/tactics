/*=========================================================================

  Program:   ToolCursor
  Module:    vtkROIContourDataToPolyData.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkROIContourDataToPolyData - Convert ROI contours to polydata
// .SECTION Description
// This filter will convert a set of contours that define an ROI into a
// vtkPolyData consisting of verts and lines.  The resulting polydata will
// have integer cell scalars called "Labels" to identify of the contour that
// they originated from, and will have integer point scalars called "SubIds"
// to indicate which line segment they correspond to.

#ifndef __vtkROIContourDataToPolyData_h
#define __vtkROIContourDataToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class vtkROIContourData;
class vtkPlane;
class vtkSpline;
class vtkPoints;
class vtkCellArray;
class vtkDoubleArray;
class vtkIntArray;

class VTK_EXPORT vtkROIContourDataToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkROIContourDataToPolyData *New();
  vtkTypeMacro(vtkROIContourDataToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the plane to use for selecting the contours.  Only contours within
  // a certain distance of this plane will be included in the output.  If the
  // plane is set to NULL, then the output will contain all the contours.
  void SetSelectionPlane(vtkPlane *plane);
  vtkPlane *GetSelectionPlane() { return this->SelectionPlane; }

  // Description:
  // Set the tolerance for how far the contours can be from the selection plane
  // and still be included in the output.
  vtkSetMacro(SelectionPlaneTolerance, double);
  vtkGetMacro(SelectionPlaneTolerance, double);

  // Description:
  // Smooth the contour by subdividing it with a spline.
  vtkSetMacro(Subdivision, int);
  vtkBooleanMacro(Subdivision, int);
  vtkGetMacro(Subdivision, int);

  // Description:
  // The target segment length for subdivision.
  vtkSetMacro(SubdivisionTarget, double);
  vtkGetMacro(SubdivisionTarget, double);

  // Description:
  // Specify an instance of vtkSpline to use to perform the subdivision.
  virtual void SetSpline(vtkSpline *spline);
  vtkSpline *GetSpline() { return this->Spline; }

protected:
  vtkROIContourDataToPolyData();
  ~vtkROIContourDataToPolyData();

  virtual int ComputePipelineMTime(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, int requestFromOutputPort,
    unsigned long* mtime);

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void ComputeSpline(
    vtkPoints *points, bool closed, double &tmax, double &dmax);

  bool GenerateSpline(
    vtkPoints *contourPoints, bool closed,
    vtkPoints *points, vtkCellArray *lines, vtkIntArray *subIds);

  bool CatmullRomSpline(
    vtkPoints *contourPoints, bool closed,
    vtkPoints *points, vtkCellArray *lines, vtkIntArray *subIds);

  vtkPlane *SelectionPlane;
  double SelectionPlaneTolerance;
  int Subdivision;
  double SubdivisionTarget;
  vtkSpline *Spline;

  vtkSpline *SplineX;
  vtkSpline *SplineY;
  vtkSpline *SplineZ;
  vtkDoubleArray *KnotPositions;

private:
  vtkROIContourDataToPolyData(const vtkROIContourDataToPolyData&);  // Not implemented.
  void operator=(const vtkROIContourDataToPolyData&);  // Not implemented.
};

#endif
