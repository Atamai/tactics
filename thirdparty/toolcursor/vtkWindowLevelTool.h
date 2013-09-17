/*=========================================================================

  Program:   ToolCursor
  Module:    vtkWindowLevelTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWindowLevelTool - Adjusts the Window/Level of an image.
// .SECTION Description
// This class adjusts the Window/Level of an image property.  The Window
// increases as the mouse moves right, the Level increases as the mouse
// moves up.

#ifndef __vtkWindowLevelTool_h
#define __vtkWindowLevelTool_h

#include "vtkTool.h"

class vtkImageProperty;

class VTK_EXPORT vtkWindowLevelTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkWindowLevelTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkWindowLevelTool, vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkWindowLevelTool();
  ~vtkWindowLevelTool();

  double StartWindowLevel[2];
  vtkImageProperty *CurrentImageProperty;

  void SetCurrentImageToNthImage(int i);

private:
  vtkWindowLevelTool(const vtkWindowLevelTool&);  //Not implemented
  void operator=(const vtkWindowLevelTool&);  //Not implemented
};

#endif
