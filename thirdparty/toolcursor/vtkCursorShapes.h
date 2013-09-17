/*=========================================================================

  Program:   ToolCursor
  Module:    vtkCursorShapes.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCursorShapes - A base class for cursor shape sets.
// .SECTION Description
// This is the base class for vtkToolCursor cursor shapes.  Each
// subclass will define a set of 3D cursor shapes that can be used.

#ifndef __vtkCursorShapes_h
#define __vtkCursorShapes_h

#include "vtkObject.h"

class vtkToolCursorShapeArray;
class vtkDataSet;

// Flags for shapes.  The first few flags give hints about how
// the cursor should be oriented.
#define VTK_TOOL_ORIENT   0x0F
#define VTK_TOOL_FLATX    0x01  // cursor is mainly in YZ plane
#define VTK_TOOL_FLATY    0x02  // cursor is mainly in XZ plane
#define VTK_TOOL_RADIALX  0x03  // point "x" away from camera axis
#define VTK_TOOL_RADIALY  0x04  // point "y" away from camera axis
#define VTK_TOOL_IMAGEXY  0x05  // Align cursor with image axes
#define VTK_TOOL_RGB      0x10  // cursor uses RGB scalars

class VTK_EXPORT vtkCursorShapes : public vtkObject
{
public:
  // Description:
  // Instantiate the object.
  static vtkCursorShapes *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkCursorShapes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of shapes.
  int GetNumberOfShapes() { return this->NumberOfShapes; };

  // Description:
  // Add a cursor shape.  The id for that shape will be returned.  Once
  // added, a shape cannot be changed or removed.
  int AddShape(const char *name, vtkDataSet *shape, int flags);

  // Description:
  // Get the name of the shape at the specified index.  Returns null if
  // the index is out of range.
  const char *GetShapeName(int i);

  // Description:
  // Get the index of the shape with the specified name.  Returns -1 if the
  // name is not recognized.
  int GetShapeIndex(const char *name);

  // Description:
  // Get the specified shape.  Returns null if the index is out of range or
  // or if the name is not recognized.
  vtkDataSet *GetShapeData(int i);
  vtkDataSet *GetShapeData(const char *name) {
    return this->GetShapeData(this->GetShapeIndex(name)); };

  // Description:
  // Get the flags for this shape.  Returns zero if the index is out of range
  // or if the name is not recognized.
  int GetShapeFlags(int i);
  int GetShapeFlags(const char *name) {
    return this->GetShapeFlags(this->GetShapeIndex(name)); };

protected:
  vtkCursorShapes();
  ~vtkCursorShapes();

  int NumberOfShapes;
  vtkToolCursorShapeArray *Shapes;

private:
  vtkCursorShapes(const vtkCursorShapes&);  //Not implemented
  void operator=(const vtkCursorShapes&);  //Not implemented
};

#endif
