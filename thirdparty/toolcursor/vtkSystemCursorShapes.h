/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSystemCursorShapes.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSystemCursorShapes - Vectorized system cursor shapes.
// .SECTION Description
// This class is a collection of cursor shapes for use with the
// vtkToolCursor.  The shapes are vectorized version of system
// cursor shapes that can be magnified or reoriented without looking
// pixellated.  The two shapes available are "Pointer" and "Crosshair".

#ifndef __vtkSystemCursorShapes_h
#define __vtkSystemCursorShapes_h

#include "vtkCursorShapes.h"

class vtkPolyData;

class VTK_EXPORT vtkSystemCursorShapes : public vtkCursorShapes
{
public:
  // Description:
  // Instantiate the object.
  static vtkSystemCursorShapes *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkSystemCursorShapes,vtkCursorShapes);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSystemCursorShapes();
  ~vtkSystemCursorShapes();

  void MakeShapes();
  static vtkDataSet *MakePointerShape();
  static vtkDataSet *MakeCrosshairShape();

private:
  vtkSystemCursorShapes(const vtkSystemCursorShapes&);  //Not implemented
  void operator=(const vtkSystemCursorShapes&);  //Not implemented
};

#endif
