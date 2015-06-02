/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourToPolygonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourToPolygonFilter - Fill all 2D contours to create polygons
// .SECTION Description
// vtkContourToPolygonFilter will generate triangles to fill all of the 2D
// contours in its input.  The contours may be concave, and may even
// contain holes i.e. a contour may contain an internal contour that
// is wound in the opposite direction to indicate that it is a hole.
// .SECTION Caveats
// The triangulation of is done in O(n) time for simple convex
// inputs, but for non-convex inputs the worst-case time is O(n^2*m^2)
// where n is the number of points and m is the number of holes.
// The best triangulation algorithms, in contrast, are O(n log n).
// The resulting triangles may be quite narrow, the algorithm does
// not attempt to produce high-quality triangles.
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkContourToPolygonFilter_h
#define __vtkContourToPolygonFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkIdList;

class vtkContourToPolygonFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkContourToPolygonFilter *New();
  vtkTypeMacro(vtkContourToPolygonFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generate errors when the triangulation fails.  Usually the
  // triangulation errors are too small to see, but they result in
  // a surface that is not watertight.  This option has no impact
  // on performance.
  vtkSetMacro(TriangulationErrorDisplay, int);
  vtkBooleanMacro(TriangulationErrorDisplay, int);
  vtkGetMacro(TriangulationErrorDisplay, int);

protected:
  vtkContourToPolygonFilter();
  ~vtkContourToPolygonFilter();

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // A robust method for triangulating a polygon.  It cleans up the polygon
  // and then applies the ear-cut method that is implemented in vtkPolygon.
  // A zero return value indicates that triangulation failed.
  static int TriangulatePolygon(
    vtkIdList *polygon, vtkPoints *points, vtkCellArray *triangles,
    vtkCellArray *tmpCellArray, vtkPolygon *tmpPolygon,
    vtkIdList *tmpIdList);

  // Description:
  // Given some closed contour lines, create a triangle mesh that
  // fills those lines.  The input lines must be single-segment lines,
  // not polylines.  The input lines do not have to be in order.
  // Only numLines starting from firstLine will be used.  Specify
  // the normal of the clip plane used to generate the contours, which
  // will be opposite the the normals of the polys that will be produced.
  static int MakePolysFromContours(
    vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
    vtkCellArray *outputPolys, const double normal[3],
    vtkCellArray *tmpCellArray, vtkPolygon *tmpPolygon,
    vtkIdList *tmpIdList);

  int TriangulationErrorDisplay;
  vtkIdList *IdList;
  vtkCellArray *CellArray;
  vtkPolygon *Polygon;

  friend class vtkContourToPolygonFilterToClipClosedSurfaceFriendship;

private:
  vtkContourToPolygonFilter(const vtkContourToPolygonFilter&);  // Not implemented.
  void operator=(const vtkContourToPolygonFilter&);  // Not implemented.
};

#endif
