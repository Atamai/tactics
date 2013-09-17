/*=========================================================================

  Program:   ToolCursor
  Module:    vtkRotateCameraTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRotateCameraTool - Controls camera rotation.
// .SECTION Description
// This class controls camera rotation interaction.

#ifndef __vtkRotateCameraTool_h
#define __vtkRotateCameraTool_h

#include "vtkTool.h"

class vtkTransform;

class VTK_EXPORT vtkRotateCameraTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkRotateCameraTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkRotateCameraTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

  // Description:
  // Constrain the position or orientation of the cursor.
  virtual void ConstrainCursor(double position[3], double normal[3]);

protected:
  vtkRotateCameraTool();
  ~vtkRotateCameraTool();

  int IsStickyPossible(const double position[3]);

  void GetCenterOfRotation(double center[3]) {
    center[0] = this->CenterOfRotation[0];
    center[1] = this->CenterOfRotation[1];
    center[2] = this->CenterOfRotation[2]; };

  void GetStickyPosition(double pos[3]) {
    pos[0] = this->StickyPosition[0];
    pos[1] = this->StickyPosition[1];
    pos[2] = this->StickyPosition[2]; };

  double CenterOfRotation[3];
  double StickyPosition[3];
  double Radius;
  double MinimumRadius;
  int Sticky;
  double StickyDirection[2];

  double StartCameraPosition[3];
  double StartCameraViewUp[3];

  vtkTransform *Transform;

private:
  vtkRotateCameraTool(const vtkRotateCameraTool&);  //Not implemented
  void operator=(const vtkRotateCameraTool&);  //Not implemented
};

#endif
