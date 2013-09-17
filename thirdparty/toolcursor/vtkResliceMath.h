/*=========================================================================

  Program:   ToolCursor
  Module:    vtkResliceMath.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkResliceMath - Helper math functions
// .SECTION Description
// This class provides some math functions that are useful for the
// vtkToolCursor classes.

#ifndef __vtkResliceMath_h
#define __vtkResliceMath_h

#include "vtkObject.h"

class vtkImageReslice;

class VTK_EXPORT vtkResliceMath : public vtkObject
{
public:
  // Description:
  // Instantiate the object.
  static vtkResliceMath *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkResliceMath,vtkObject);

  // Description:
  // Given a plane as a 4-vector, set the axes and other information
  // for a vtkImageReslice filter so that the slice will be extracted.
  static void SetReslicePlane(
    vtkImageReslice *reslice, const double plane[4]);

  // Description:
  // Given a plane as a 4-vector, generate a 4x4 matrix that  can be
  // used for slicing an image at that plane.
  static void ConvertPlaneToResliceAxes(
    const double plane[4], double matrix[16]);

protected:
  vtkResliceMath() {};
  ~vtkResliceMath() {};

private:
  vtkResliceMath(const vtkResliceMath&);  //Not implemented
  void operator=(const vtkResliceMath&);  //Not implemented
};

#endif
