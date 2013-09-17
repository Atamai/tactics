/*=========================================================================

  Program:   ToolCursor
  Module:    vtkPushPlaneTool.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPushPlaneTool - Controls plane-pushing action.
// .SECTION Description
// This class controls the "push" interaction for vtkImageSlice slices,
// vtkVolumeMapper cropping planes, or clipping planes on all types of
// mappers.

#ifndef __vtkPushPlaneTool_h
#define __vtkPushPlaneTool_h

#include "vtkTool.h"

class vtkImageActor;
class vtkVolumeMapper;
class vtkImageMapper3D;
class vtkAbstractMapper3D;
class vtkLODProp3D;
class vtkTransform;

class VTK_EXPORT vtkPushPlaneTool : public vtkTool
{
public:
  // Description:
  // Instantiate the object.
  static vtkPushPlaneTool *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkPushPlaneTool,vtkTool);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allow the tool to perform rotation.
  vtkSetMacro(AllowRotation, int);
  vtkGetMacro(AllowRotation, int);
  vtkBooleanMacro(AllowRotation, int);

  // Description:
  // Allow the tool to perform slicing.
  vtkSetMacro(AllowSlicing, int);
  vtkGetMacro(AllowSlicing, int);
  vtkBooleanMacro(AllowSlicing, int);

  // Description:
  // The maximum rotation to allow, from the start of the action.
  vtkSetMacro(MaximumRotationDegree, double);
  vtkGetMacro(MaximumRotationDegree, double);

  // Description:
  // These are the methods that are called when the action takes place.
  virtual void StartAction();
  virtual void StopAction();
  virtual void DoAction();

  // Description:
  // This method allows the action to constrain the cursor position.
  virtual void ConstrainCursor(double position[3], double normal[3]);

protected:
  vtkPushPlaneTool();
  ~vtkPushPlaneTool();

  vtkTransform *Transform;
  vtkImageActor *ImageActor;
  vtkVolumeMapper *VolumeMapper;
  vtkImageMapper3D *ImageMapper;
  vtkAbstractMapper3D *Mapper;
  vtkLODProp3D *LODProp3D;
  int PlaneId;
  int EdgeId;
  int PerpendicularPlane;
  double StartNormal[3];
  double StartOrigin[3];
  double Origin[3];
  double Normal[3];
  double DistanceLimits[2];
  bool IsOffOfPlane;
  int AllowRotation;
  int AllowSlicing;
  double MaximumRotationDegree;

  int IsPlaneValid() { return (this->PlaneId >= 0); };

  void GetPropInformation();
  void GetPlaneOriginAndNormal(double origin[3], double normal[3]);

  void SetStartOrigin(const double origin[3]) {
    this->StartOrigin[0] = origin[0];
    this->StartOrigin[1] = origin[1];
    this->StartOrigin[2] = origin[2]; };

  void GetStartOrigin(double origin[3]) {
    origin[0] = this->StartOrigin[0];
    origin[1] = this->StartOrigin[1];
    origin[2] = this->StartOrigin[2]; };

  void SetOrigin(const double origin[3]);

  void GetOrigin(double origin[3]) {
    origin[0] = this->Origin[0];
    origin[1] = this->Origin[1];
    origin[2] = this->Origin[2]; };

  void SetStartNormal(const double normal[3]) {
    this->StartNormal[0] = normal[0];
    this->StartNormal[1] = normal[1];
    this->StartNormal[2] = normal[2]; };

  void GetStartNormal(double normal[3]) {
    normal[0] = this->StartNormal[0];
    normal[1] = this->StartNormal[1];
    normal[2] = this->StartNormal[2]; };

  void SetNormal(const double normal[3]);

  void GetNormal(double normal[3]) {
    normal[0] = this->Normal[0];
    normal[1] = this->Normal[1];
    normal[2] = this->Normal[2]; };

private:
  vtkPushPlaneTool(const vtkPushPlaneTool&);  //Not implemented
  void operator=(const vtkPushPlaneTool&);  //Not implemented
};

#endif
