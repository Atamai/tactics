/*=========================================================================

  Program:   ToolCursor
  Module:    vtkImageTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageTool - Superclass for tools that act on the active image.
// .SECTION Description
// This class finds the active image in the renderer and sets it as the
// current image for the tool when StartAction is called.  This is for
// actions that should always be applied to the image, even if there are
// other pickable actors in the scene.

#ifndef __vtkImageTool_h
#define __vtkImageTool_h

#include "vtkTool.h"

class vtkImageProperty;
class vtkImageMapper3D;
class vtkMatrix4x4;

class VTK_EXPORT vtkImageTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkImageTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkImageTool, vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

protected:
  vtkImageTool();
  ~vtkImageTool();

  // Description:
  // Find the image to perform the interaction on.  If an ImageStack is
  // present, then the active layer in the stack will be used.  Otherwise,
  // the last visible image in the renderer will be used.
  void FindCurrentImage();

  // Description:
  // Set the current image by setting the mapper, property, and matrix.
  void SetCurrentImage(
    vtkImageMapper3D *mapper, vtkImageProperty *property, vtkMatrix4x4 *matrix);

  vtkImageProperty *CurrentImageProperty;
  vtkImageMapper3D *CurrentImageMapper;
  vtkMatrix4x4 *CurrentImageMatrix;

private:
  vtkImageTool(const vtkImageTool&);  //Not implemented
  void operator=(const vtkImageTool&);  //Not implemented
};

#endif
