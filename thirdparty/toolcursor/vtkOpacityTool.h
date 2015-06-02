/*=========================================================================

  Program:   ToolCursor
  Module:    vtkOpacityTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpacityTool - Adjusts the opacity of an image.
// .SECTION Description
// This class adjusts the Opacity of an image property.

#ifndef __vtkOpacityTool_h
#define __vtkOpacityTool_h

#include "vtkTool.h"

class vtkImageProperty;

class VTK_EXPORT vtkOpacityTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkOpacityTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkOpacityTool, vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkOpacityTool();
  ~vtkOpacityTool();

  double StartOpacity;
  vtkImageProperty *CurrentImageProperty;

  void SetCurrentImageToNthImage(int i);

private:
  vtkOpacityTool(const vtkOpacityTool&);  //Not implemented
  void operator=(const vtkOpacityTool&);  //Not implemented
};

#endif
