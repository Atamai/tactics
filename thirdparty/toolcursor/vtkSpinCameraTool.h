/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSpinCameraTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpinCameraTool - Spin the camera with the cursor.
// .SECTION Description
// This is an interaction class for spinning the cursor around the
// camera's view direction.

#ifndef __vtkSpinCameraTool_h
#define __vtkSpinCameraTool_h

#include "vtkTool.h"

class vtkTransform;

class VTK_EXPORT vtkSpinCameraTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkSpinCameraTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkSpinCameraTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkSpinCameraTool();
  ~vtkSpinCameraTool();

  double StartCameraViewUp[3];

  vtkTransform *Transform;

private:
  vtkSpinCameraTool(const vtkSpinCameraTool&);  //Not implemented
  void operator=(const vtkSpinCameraTool&);  //Not implemented
};

#endif
