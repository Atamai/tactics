/*=========================================================================

  Program:   ToolCursor
  Module:    vtkPanCameraTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPanCameraTool - Pan the camera in order to move the scene.
// .SECTION Description
// This class is used to pan the camera.  The pan is achieved by moving
// the camera position and focal point together, not by slowly rotating
// the camera around its position.

#ifndef __vtkPanCameraTool_h
#define __vtkPanCameraTool_h

#include "vtkTool.h"

class vtkTransform;

class VTK_EXPORT vtkPanCameraTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkPanCameraTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkPanCameraTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkPanCameraTool();
  ~vtkPanCameraTool();

  double StartCameraFocalPoint[3];
  double StartCameraPosition[3];

  vtkTransform *Transform;

private:
  vtkPanCameraTool(const vtkPanCameraTool&);  //Not implemented
  void operator=(const vtkPanCameraTool&);  //Not implemented
};

#endif
