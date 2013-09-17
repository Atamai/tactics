/*=========================================================================

  Program:   ToolCursor
  Module:    vtkActionCursorShapes.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkActionCursorShapes - Cursor shapes for interaction.
// .SECTION Description
// This class is a collection of action shapes for use with the
// vtkToolCursor.  The shapes available are "Move", "Rotate",
// "Push", and "Spin".  All cursors are constructed from the
// same 3D arrow shape and have a two-tone color scheme.

#ifndef __vtkActionCursorShapes_h
#define __vtkActionCursorShapes_h

#include "vtkCursorShapes.h"

class vtkPolyData;

class VTK_EXPORT vtkActionCursorShapes : public vtkCursorShapes
{
public:
  // Description:
  // Instantiate the object.
  static vtkActionCursorShapes *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkActionCursorShapes,vtkCursorShapes);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkActionCursorShapes();
  ~vtkActionCursorShapes();

  void MakeShapes();
  static vtkDataSet *MakeMoveShape(vtkPolyData *arrow, int warped);
  static vtkDataSet *MakePushShape(vtkPolyData *arrow);
  static vtkDataSet *MakeSpinShape(vtkPolyData *arrow);
  static vtkDataSet *MakeZoomShape(vtkPolyData *arrow);
  static vtkPolyData *MakeArrow();
  static vtkPolyData *MakeWarpedArrow(vtkPolyData *arrow,
                                      double warpX, double warpY,
                                      double warpZ, double warpScale);

private:
  vtkActionCursorShapes(const vtkActionCursorShapes&);  //Not implemented
  void operator=(const vtkActionCursorShapes&);  //Not implemented
};

#endif
