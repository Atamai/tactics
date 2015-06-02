/*=========================================================================

  Program:   ToolCursor
  Module:    vtkGeometricCursorShapes.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeometricCursorShapes - Basic geometrical cursor shapes.
// .SECTION Description
// This class is a collection of geometric cursor shapes for use with the
// vtkToolCursor.  All shapes come in two varieties, one with a single
// color and another with a two-tone color scheme, where the colors are
// separated  by a split at the Z plane.  The names of the shapes are
// "Cross", "SplitCross", "Sphere", "SplitSphere", "Cone", and "DualCone".

#ifndef __vtkGeometricCursorShapes_h
#define __vtkGeometricCursorShapes_h

#include "vtkCursorShapes.h"

class vtkPolyData;

class VTK_EXPORT vtkGeometricCursorShapes : public vtkCursorShapes
{
public:
  // Description:
  // Instantiate the object.
  static vtkGeometricCursorShapes *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkGeometricCursorShapes,vtkCursorShapes);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGeometricCursorShapes();
  ~vtkGeometricCursorShapes();

  void MakeShapes();
  static vtkDataSet *MakeCrossShape(int splitCross);
  static vtkDataSet *MakeSphereShape(int splitSphere);
  static vtkDataSet *MakeConeShape(int doubleCone);

private:
  vtkGeometricCursorShapes(const vtkGeometricCursorShapes&);  //Not implemented
  void operator=(const vtkGeometricCursorShapes&);  //Not implemented
};

#endif
