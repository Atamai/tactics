/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSliceImageTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliceImageTool - Move the camera focal plane in and out.
// .SECTION Description
// This class moves the focal point of the camera away from or towards the
// viewer, in order to adjust the slice plane for the images.

#ifndef __vtkSliceImageTool_h
#define __vtkSliceImageTool_h

#include "vtkImageTool.h"

class VTK_EXPORT vtkSliceImageTool : public vtkImageTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkSliceImageTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkSliceImageTool, vtkImageTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn this on to make the interaction jump to the nearest slice,
  // instead of interpolating between slices.  This option is ignored
  // if the view plane is oblique to the original image scan planes.
  // The image that is used is whatever image is on top, or if an image
  // stack is present, then the active layer of the stack is used.
  void SetJumpToNearestSlice(int val);
  void JumpToNearestSliceOff() { this->SetJumpToNearestSlice(0); }
  void JumpToNearestSliceOn() { this->SetJumpToNearestSlice(1); }
  int GetJumpToNearestSlice() { return this->JumpToNearestSlice; }

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

  // Description:
  // This is useful methods for moving back or forth by one slice.
  // The delta is the number of slices to advance by, use negative
  // values to move backwards.
  virtual void AdvanceSlice(int delta);

protected:
  vtkSliceImageTool();
  ~vtkSliceImageTool();

  int JumpToNearestSlice;
  double StartDistance;

private:
  vtkSliceImageTool(const vtkSliceImageTool&);  //Not implemented
  void operator=(const vtkSliceImageTool&);  //Not implemented
};

#endif
