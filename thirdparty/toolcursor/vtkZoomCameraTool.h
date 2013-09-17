/*=========================================================================

  Program:   ToolCursor
  Module:    vtkZoomCameraTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkZoomCameraTool - Zoom the view by moving the camera in and out.
// .SECTION Description
// This class is used to zoom the view.  The default way of doing the zoom
// is by dollying the camera towards or away from the focal point in order
// to achieve the zoom.  If a parallel projection is used, then the zoom
// will adjust the parallel scale.

#ifndef __vtkZoomCameraTool_h
#define __vtkZoomCameraTool_h

#include "vtkTool.h"

class vtkTransform;

class VTK_EXPORT vtkZoomCameraTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkZoomCameraTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkZoomCameraTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set whether to zoom by dollying the camera or by decreasing the
  // field of view.  This option is ignored if the camera is using
  // a parallel projection.  The default is to zoom by dollying.
  vtkSetMacro(ZoomByDolly, int);
  vtkBooleanMacro(ZoomByDolly, int);
  int GetZoomByDolly(int) { return this->ZoomByDolly; };

  // Description:
  // The default behavior is to zoom the camera the mouse moves vertically
  // on the screen, but if RadialInteraction is On, then moving the mouse
  // away from the center of the window will cause the camera to zoom.
  vtkSetMacro(RadialInteraction, int);
  vtkBooleanMacro(RadialInteraction, int);
  vtkGetMacro(RadialInteraction, int);
  
  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

  // Description:
  // Constrain the position or orientation of the cursor.
  virtual void ConstrainCursor(double position[3], double normal[3]);

protected:
  vtkZoomCameraTool();
  ~vtkZoomCameraTool();

  int ZoomByDolly;
  int RadialInteraction;

  double StartCameraPosition[3];
  double StartClippingRange[2];
  double StartParallelScale;
  double StartViewAngle;

  double ZoomFactor;

  vtkTransform *Transform;

private:
  vtkZoomCameraTool(const vtkZoomCameraTool&);  //Not implemented
  void operator=(const vtkZoomCameraTool&);  //Not implemented
};

#endif
