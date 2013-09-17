/*=========================================================================

  Program:   ToolCursor
  Module:    vtkFocalPlaneTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFocalPlaneTool - Move the camera focal plane in and out.
// .SECTION Description
// This class moves the focal point of the camera away from or towards the
// viewer, in order to adjust the slice plane for the images.

#ifndef __vtkFocalPlaneTool_h
#define __vtkFocalPlaneTool_h

#include "vtkTool.h"

class vtkTransform;

class VTK_EXPORT vtkFocalPlaneTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkFocalPlaneTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkFocalPlaneTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkFocalPlaneTool();
  ~vtkFocalPlaneTool();

  double StartDistance;

private:
  vtkFocalPlaneTool(const vtkFocalPlaneTool&);  //Not implemented
  void operator=(const vtkFocalPlaneTool&);  //Not implemented
};

#endif
