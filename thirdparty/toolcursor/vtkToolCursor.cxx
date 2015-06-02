/*=========================================================================

  Program:   ToolCursor
  Module:    vtkToolCursor.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkToolCursor.h"
#include "vtkObjectFactory.h"

#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
#include "vtkImageActor.h"
#include "vtkImageStack.h"
#include "vtkImageMapper3D.h"
#include "vtkProp3DCollection.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"
#include "vtkAssemblyPath.h"
#include "vtkProperty.h"
#include "vtkDataSetMapper.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkLookupTable.h"
#include "vtkDataSetCollection.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkVolumePicker.h"
#include "vtkCommand.h"
#include "vtkVolumeOutlineSource.h"
#include "vtkClipClosedSurface.h"
#include "vtkOutlineSource.h"
#include "vtkCutter.h"
#include "vtkTriangleFilter.h"
#include "vtkTubeFilter.h"

#include "vtkCursorShapes.h"
#include "vtkActionCursorShapes.h"
#include "vtkGeometricCursorShapes.h"
#include "vtkTool.h"
#include "vtkPushPlaneTool.h"
#include "vtkPanCameraTool.h"
#include "vtkRotateCameraTool.h"
#include "vtkSpinCameraTool.h"
#include "vtkZoomCameraTool.h"
#include "vtkFollowerPlane.h"

vtkStandardNewMacro(vtkToolCursor);

//----------------------------------------------------------------------------
class vtkToolCursorRenderCommand : public vtkCommand
{
public:
  static vtkToolCursorRenderCommand *New(vtkToolCursor *cursor) {
    return new vtkToolCursorRenderCommand(cursor); };

  virtual void Execute(vtkObject *, unsigned long, void *) {
    this->Cursor->OnRender(); };

protected:
  vtkToolCursorRenderCommand(vtkToolCursor *cursor) {
    this->Cursor = cursor; };

  vtkToolCursor* Cursor;

private:
  static vtkToolCursorRenderCommand *New(); // Not implemented.
  vtkToolCursorRenderCommand(); // Not implemented.
  vtkToolCursorRenderCommand(const vtkToolCursorRenderCommand&);  // Not implemented.
  void operator=(const vtkToolCursorRenderCommand&);  // Not implemented.
};

//----------------------------------------------------------------------------
vtkToolCursor::vtkToolCursor()
{
  this->DisplayPosition[0] = 0.0;
  this->DisplayPosition[1] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Vector[0] = 0.0;
  this->Vector[1] = 1.0;
  this->Vector[2] = 0.0;

  this->Renderer = 0;

  this->GuideVisibility = 0;
  this->PointNormalAtCamera = 1;
  this->ActionButtons = 0;
  this->Modifier = 0;
  this->Mode = 0;
  this->PickFlags = 0;
  this->Shape = 0;
  this->Action = 0;
  this->ActionButton = 0;
  this->Scale = 1.0;

  this->Actor = vtkActor::New();
  this->Matrix = vtkMatrix4x4::New();
  this->Mapper = vtkDataSetMapper::New();
  this->Mapper->StaticOn();
  this->LookupTable = vtkLookupTable::New();
  this->Mapper->SetLookupTable(this->LookupTable);
  this->Mapper->UseLookupTableScalarRangeOn();
  this->Shapes = vtkCursorShapes::New();
  this->Actions = vtkCollection::New();
  this->ShapeBindings = vtkIntArray::New();
  this->ShapeBindings->SetName("ShapeBindings");
  this->ShapeBindings->SetNumberOfComponents(4);
  this->ActionBindings = vtkIntArray::New();
  this->ActionBindings->SetName("ActionBindings");
  this->ActionBindings->SetNumberOfComponents(4);
  this->Picker = vtkVolumePicker::New();

  this->LookupTable->SetRampToLinear();
  this->LookupTable->SetTableRange(0,255);
  this->LookupTable->SetNumberOfTableValues(256);
  this->LookupTable->SetSaturationRange(0,0);
  this->LookupTable->SetValueRange(0,1);
  this->LookupTable->Build();
  this->LookupTable->SetTableValue(0, 1.0, 0.0, 0.0, 1.0);
  this->LookupTable->SetTableValue(1, 0.0, 1.0, 0.0, 1.0);

  this->Actor->PickableOff();
  this->Actor->VisibilityOff();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetUserMatrix(this->Matrix);

  vtkProperty *property = this->Actor->GetProperty();
  property->BackfaceCullingOn();

  // Insert a null action, so that actions start at 1
  vtkTool *action = vtkTool::New();
  this->Actions->AddItem(action);
  action->Delete();

  // Insert a blank shape, so that shapes start at 1
  // (shape 0 will display the system cursor)
  vtkDataSet *data = vtkPolyData::New();
  this->Shapes->AddShape("", data, 0);
  data->Delete();

  this->RenderCommand = vtkToolCursorRenderCommand::New(this);

  // Volume cropping actor items
  this->VolumeCroppingSource = vtkVolumeOutlineSource::New();
  this->VolumeCroppingSource->GenerateScalarsOn();
  this->VolumeCroppingSource->GenerateFacesOn();
  this->VolumeCroppingSource->SetColor(1, 0, 0);
  this->VolumeCroppingSource->SetActivePlaneColor(0, 1, 0);

  this->ClipOutlineFilter = vtkClipClosedSurface::New();
  this->ClipOutlineFilter->SetScalarModeToColors();
  this->ClipOutlineFilter->GenerateFacesOff();
  this->ClipOutlineFilter->GenerateOutlineOn();
  this->ClipOutlineFilter->SetBaseColor(1, 0, 0);
  this->ClipOutlineFilter->SetActivePlaneColor(0, 1, 0);
  this->ClipOutlineFilter->SetInputConnection(
    this->VolumeCroppingSource->GetOutputPort());

  this->VolumeCroppingMapper = vtkDataSetMapper::New();
  this->VolumeCroppingMapper->SetInputConnection(
    this->ClipOutlineFilter->GetOutputPort());

  this->VolumeCroppingActor = vtkActor::New();
  this->VolumeCroppingActor->SetMapper(this->VolumeCroppingMapper);
  this->VolumeCroppingActor->SetPickable(0);
  this->VolumeCroppingActor->SetVisibility(0);
  this->VolumeCroppingActor->GetProperty()->BackfaceCullingOn();

  // Plane guide actor items
  this->SliceOutlineSource = vtkVolumeOutlineSource::New();
  this->SliceOutlineSource->GenerateFacesOn();
  this->SliceOutlineSource->GenerateScalarsOn();

  this->SliceOutlineCutter = vtkCutter::New();
  this->SliceOutlineCutter->SetInputConnection(
    this->SliceOutlineSource->GetOutputPort());

  this->SliceOutlineTriangleFilter = vtkTriangleFilter::New();
  this->SliceOutlineTriangleFilter->PassVertsOff();
  this->SliceOutlineTriangleFilter->SetInputConnection(
    this->SliceOutlineCutter->GetOutputPort());

  this->SliceOutlineTube = vtkTubeFilter::New();
  this->SliceOutlineTube->SetRadius(0.5);
  this->SliceOutlineTube->SetNumberOfSides(8);
  this->SliceOutlineTube->SetInputConnection(
    this->SliceOutlineTriangleFilter->GetOutputPort());

  this->SliceOutlineMapper = vtkDataSetMapper::New();
  this->SliceOutlineMapper->SetInputConnection(
    this->SliceOutlineTube->GetOutputPort());

  this->SliceOutlineActor = vtkActor::New();
  this->SliceOutlineActor->SetMapper(this->SliceOutlineMapper);
  this->SliceOutlineActor->SetPickable(0);
  this->SliceOutlineActor->SetVisibility(0);
  this->SliceOutlineActor->GetProperty()->BackfaceCullingOn();
  this->SliceOutlineActor->GetProperty()->SetAmbient(1.0);
  this->SliceOutlineActor->GetProperty()->SetDiffuse(0.0);
  this->SliceOutlineActor->GetProperty()->SetColor(0,1,0);

  // For debugging triangularization: show poly outlines
  //this->ClipOutlineFilter->GenerateFacesOn();
  //this->VolumeCroppingActor->GetProperty()->SetRepresentationToWireframe();
}

//----------------------------------------------------------------------------
vtkToolCursor::~vtkToolCursor()
{
  this->SetRenderer(0);

  if (this->VolumeCroppingActor) { this->VolumeCroppingActor->Delete(); }
  if (this->VolumeCroppingMapper) { this->VolumeCroppingMapper->Delete(); }
  if (this->ClipOutlineFilter) { this->ClipOutlineFilter->Delete(); }
  if (this->VolumeCroppingSource) { this->VolumeCroppingSource->Delete(); }

  if (this->SliceOutlineActor) { this->SliceOutlineActor->Delete(); }
  if (this->SliceOutlineMapper) { this->SliceOutlineMapper->Delete(); }
  if (this->SliceOutlineTube) { this->SliceOutlineTube->Delete(); }
  if (this->SliceOutlineCutter) { this->SliceOutlineCutter->Delete(); }
  if (this->SliceOutlineSource) { this->SliceOutlineSource->Delete(); }

  if (this->RenderCommand) { this->RenderCommand->Delete(); }
  if (this->Matrix) { this->Matrix->Delete(); }
  if (this->Shapes) { this->Shapes->Delete(); }
  if (this->Actions) { this->Actions->Delete(); }
  if (this->ShapeBindings) { this->ShapeBindings->Delete(); }
  if (this->ActionBindings) { this->ActionBindings->Delete(); }
  if (this->Mapper) { this->Mapper->Delete(); }
  if (this->LookupTable) { this->LookupTable->Delete(); }
  if (this->Actor) { this->Actor->Delete(); }
  if (this->Picker) { this->Picker->Delete(); }
}

//----------------------------------------------------------------------------
void vtkToolCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkToolCursor::BindDefaultActions()
{
  vtkCursorShapes *actionShapes = vtkActionCursorShapes::New();
  vtkCursorShapes *geometricShapes = vtkGeometricCursorShapes::New();
  vtkTool *pushAction = vtkPushPlaneTool::New();
  vtkTool *rotateAction = vtkRotateCameraTool::New();
  vtkTool *spinAction = vtkSpinCameraTool::New();
  vtkTool *panAction = vtkPanCameraTool::New();
  vtkTool *zoomAction = vtkZoomCameraTool::New();

  int action, shape, mode, modifier, pickInfo;

  // ============ All Bindings for Mode 0 =============
  mode = 0;

  // In terms of the "PickInfo" flags, it is necessary to start at the
  // default bindings and work up to more specific collections of
  // PickInfo flags.

  // ------------ Default Bindings -------------
  pickInfo = 0;

  // Binding for "Rotate" cursor and action, when user clicks background
  modifier = 0;
  shape = this->AddShape(actionShapes, "Rotate");
  action = this->AddAction(rotateAction);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  // Binding for "Mover" cursor and action
  modifier = VTK_TOOL_SHIFT;
  shape = this->AddShape(actionShapes, "Move");
  action = this->AddAction(panAction);
  this->BindShape(shape, mode, pickInfo, modifier);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  // Also bind to middle button
  modifier = 0;
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B3);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B3);

  // Binding for "Zoom" cursor and action
  modifier = VTK_TOOL_CONTROL;
  shape = this->AddShape(actionShapes, "Zoom");
  action = this->AddAction(zoomAction);
  this->BindShape(shape, mode, pickInfo, modifier);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  // Also bind to right button
  modifier = 0;
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B2);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B2);

  // Also bind to the mouse wheel
  modifier = 0;
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_WHEEL_BWD);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_WHEEL_BWD);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_WHEEL_FWD);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_WHEEL_FWD);

  // ------------ Default Prop3D Bindings-------------
  pickInfo = VTK_TOOL_PROP3D;

  // Binding for "Cone" cursor
  modifier = 0;
  shape = this->AddShape(geometricShapes, "Cone");
  this->BindShape(shape, mode, pickInfo, modifier);

  // Binding for "Rotator" cursor and action
  modifier = 0;
  shape = this->AddShape(actionShapes, "Rotate");
  action = this->AddAction(rotateAction);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  // ------------ Bindings for Volumes with Clip/Crop Planes -------------
  pickInfo = (VTK_TOOL_VOLUME |
              VTK_TOOL_CLIP_PLANE |
              VTK_TOOL_CROP_PLANE);

  // Binding for "SplitCross" cursor
  modifier = 0;
  shape = this->AddShape(geometricShapes, "SplitCross");
  this->BindShape(shape, mode, pickInfo, modifier);

  // Binding for "PushPlane" with "Pusher" cursor
  modifier = 0;
  shape = this->AddShape(actionShapes, "Push");
  action = this->AddAction(pushAction);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  // ------------ Bindings for ImageActor -------------
  pickInfo = VTK_TOOL_IMAGE_ACTOR;

  // Binding for "SplitCross" cursor
  modifier = 0;
  shape = this->AddShape(geometricShapes, "SplitCross");
  this->BindShape(shape, mode, pickInfo, modifier);

  // Binding for "PushPlane" with "Pusher" cursor
  modifier = 0;
  shape = this->AddShape(actionShapes, "Push");
  action = this->AddAction(pushAction);
  this->BindShape(shape, mode, pickInfo, modifier | VTK_TOOL_B1);
  this->BindAction(action, mode, pickInfo, modifier | VTK_TOOL_B1);

  actionShapes->Delete();
  geometricShapes->Delete();
  rotateAction->Delete();
  pushAction->Delete();
  spinAction->Delete();
  panAction->Delete();
  zoomAction->Delete();
}

//----------------------------------------------------------------------------
void vtkToolCursor::BindShape(int shape, int mode,
                                 int pickFlags, int modifier)
{
  this->AddBinding(this->ShapeBindings, shape, mode, pickFlags, modifier);
}

//----------------------------------------------------------------------------
void vtkToolCursor::BindAction(int action, int mode,
                                  int pickFlags, int modifier)
{
  this->AddBinding(this->ActionBindings, action, mode, pickFlags, modifier);
}

//----------------------------------------------------------------------------
int vtkToolCursor::FindShape(int mode, int pickFlags, int modifier)
{
  // Search the shape bindings and return the first match

  int i = this->ResolveBinding(this->ShapeBindings, 0,
                               mode, pickFlags, modifier);
  if (i < 0)
    {
    return 0;
    }

  int tuple[4];
  this->ShapeBindings->GetTupleValue(i, tuple);

  return tuple[3];
}

//----------------------------------------------------------------------------
int vtkToolCursor::FindAction(int mode, int pickFlags, int modifier)
{
  // Search the action bindings and return the first match

  int i = this->ResolveBinding(this->ActionBindings, 0,
                               mode, pickFlags, modifier);
  if (i < 0)
    {
    return 0;
    }

  int tuple[4];
  this->ActionBindings->GetTupleValue(i, tuple);

  return tuple[3];
}

//----------------------------------------------------------------------------
int vtkToolCursor::FindActionButtons(int mode, int pickFlags, int modifier)
{
  // Find all matching actions and return a bitmask of the mouse buttons
  // for those actions.

  int modifierMask = 0;
  modifier |= (VTK_TOOL_BUTTON_MASK | VTK_TOOL_WHEEL_MASK);

  int tuple[4];
  int j = 0;
  for (;;)
    {
    int i = this->ResolveBinding(this->ActionBindings, j,
                                 mode, pickFlags, modifier);
    if (i < 0) { break; }

    this->ActionBindings->GetTupleValue(i, tuple);
    modifierMask |= tuple[2];

    j = i + 1;
    }

  return (modifierMask & (VTK_TOOL_BUTTON_MASK | VTK_TOOL_WHEEL_MASK));
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetRenderer(vtkRenderer *renderer)
{
  if (renderer == this->Renderer)
    {
    return;
    }

  if (this->Renderer)
    {
    this->Renderer->RemoveObserver(this->RenderCommand);
    this->Renderer->RemoveActor(this->Actor);
    this->Renderer->RemoveActor(this->VolumeCroppingActor);
    this->Renderer->RemoveActor(this->SliceOutlineActor);
    this->Renderer->Delete();
    this->Renderer = 0;
    }

  if (renderer)
    {
    this->Renderer = renderer;
    this->Renderer->Register(this);
    this->Renderer->AddActor(this->Actor);
    this->Renderer->AddActor(this->VolumeCroppingActor);
    this->Renderer->AddActor(this->SliceOutlineActor);
    this->Renderer->AddObserver(vtkCommand::StartEvent,
                                this->RenderCommand, -1);
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetColor(int i, double r, double b, double g)
{
  if (i >= 0 && i <= 255)
    {
    double rgba[4];
    this->LookupTable->GetTableValue(i, rgba);
    if (rgba[0] != r || rgba[1] != g || rgba[2] != b)
      {
      this->LookupTable->SetTableValue(i, r, g, b, 1.0);
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkToolCursor::GetColor(int i, double rgb[3])
{
  if (i < 0) { i = 0; }
  if (i > 255) { i = 255; }

  double rgba[4];
  this->LookupTable->GetTableValue(i, rgba);

  rgb[0] = rgba[0];
  rgb[1] = rgba[1];
  rgb[2] = rgba[2];
}

//----------------------------------------------------------------------------
void vtkToolCursor::UpdatePropsForPick(vtkPicker *picker,
                                          vtkRenderer *renderer)
{
  // Go through all Prop3Ds that might be picked and update their data.
  // This is necessary if any data has changed since the last render.

  vtkPropCollection *props;
  if ( picker->GetPickFromList() )
    {
    props = picker->GetPickList();
    }
  else
    {
    props = renderer->GetViewProps();
    }

  vtkProp *prop;
  vtkCollectionSimpleIterator pit;
  props->InitTraversal(pit);
  while ( (prop = props->GetNextProp(pit)) )
    {
    vtkAssemblyPath *path;
    prop->InitPathTraversal();
    while ( (path = prop->GetNextPath()) )
      {
      if (!prop->GetPickable() || !prop->GetVisibility())
        {
        break;
        }

      vtkProp *anyProp = path->GetLastNode()->GetViewProp();
      vtkActor *actor;
      vtkVolume *volume;
      vtkImageActor *imageActor;

      if ( (actor = vtkActor::SafeDownCast(anyProp)) )
        {
        vtkDataSet *data = actor->GetMapper()->GetInput();
        if (data)
          {
          data->Update();
          }
        }
      else if ( (volume = vtkVolume::SafeDownCast(anyProp)) )
        {
        vtkDataSet *data = volume->GetMapper()->GetDataSetInput();
        if (data)
          {
          data->UpdateInformation();
          data->SetUpdateExtentToWholeExtent();
          data->Update();
          }
        }
      else if ( (imageActor = vtkImageActor::SafeDownCast(anyProp)) )
        {
        vtkImageData *data = imageActor->GetInput();
        if (data)
          {
          data->UpdateInformation();
          int extent[6], wextent[6], dextent[6];
          data->GetExtent(extent);
          data->GetWholeExtent(wextent);
          imageActor->GetDisplayExtent(dextent);
          if (dextent[0] == -1)
            {
            for (int i = 0; i < 6; i++) { extent[i] = wextent[i]; }
            if (extent[5] < extent[4])
              {
              extent[5] = extent[4];
              }
            }
          else
            {
            for (int i = 0; i < 3; i++)
              {
              int l = 2*i;
              int h = l+1;
              // Clip the display extent with the whole extent
              if (dextent[l] > wextent[l]) { dextent[l] = wextent[l]; }
              if (dextent[h] < wextent[h]) { dextent[h] = wextent[h]; }
              // Expand the extent to include the display extent
              if (extent[l] > dextent[l]) { extent[l] = dextent[l]; }
              if (extent[h] < dextent[h]) { extent[h] = dextent[h]; }
              }
            }
          data->SetUpdateExtent(extent);
          data->PropagateUpdateExtent();
          data->UpdateData();
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkToolCursor::ComputePickFlags(vtkVolumePicker *picker)
{
  const double planeTol = 1e-6;
  const double normalTol = 1e-15;

  int pickFlags = 0;

  vtkProp3D *prop = picker->GetProp3D();
  vtkAbstractMapper3D *mapper = picker->GetMapper();

  if (!prop)
    {
    // No prop, nothing to do
    return 0;
    }

  if (mapper && picker->GetClippingPlaneId() >= 0)
    {
    // Make sure that our Position lies on the plane and that our Normal
    // is perpendicular to the plane.

    vtkPlane *plane = mapper->GetClippingPlanes()->GetItem(
      picker->GetClippingPlaneId());

    double u[3];
    vtkMath::Cross(plane->GetNormal(), picker->GetPickNormal(), u);

    double dcheck = vtkMath::Dot(plane->GetNormal(), picker->GetPickNormal());

    if (fabs(plane->EvaluateFunction(picker->GetPickPosition())) < planeTol &&
        (u[0]*u[0] + u[1]*u[1] + u[2]*u[2]) < normalTol &&
        dcheck < 0)
      {
      pickFlags = (pickFlags | VTK_TOOL_CLIP_PLANE);
      }
    }

  if (mapper && mapper->IsA("vtkVolumeMapper") &&
      picker->GetCroppingPlaneId() >= 0)
    {
    // Also ensure that our Position lies on the cropping plane
    int planeId = picker->GetCroppingPlaneId();
    vtkVolumeMapper *volumeMapper = static_cast<vtkVolumeMapper *>(mapper);

    double bounds[6];
    volumeMapper->GetCroppingRegionPlanes(bounds);

    double mapperPos[3];
    picker->GetMapperPosition(mapperPos);

    double planeNormal[3], mapperNormal[3], u[3];
    planeNormal[0] = planeNormal[1] = planeNormal[2] = 0.0;
    planeNormal[planeId>>1] = 1 - 2*(planeId & 1);
    picker->GetMapperNormal(mapperNormal);
    vtkMath::Cross(planeNormal, mapperNormal, u);
    double dcheck = vtkMath::Dot(planeNormal, mapperNormal);

    if (fabs(mapperPos[planeId>>1] - bounds[planeId]) < planeTol &&
        (u[0]*u[0] + u[1]*u[1] + u[2]*u[2]) < normalTol &&
        dcheck < 0)
      {
      pickFlags = (pickFlags | VTK_TOOL_CROP_PLANE);
      }
    }

  if (prop->IsA("vtkImageActor"))
    {
    pickFlags = (pickFlags | VTK_TOOL_IMAGE_ACTOR);
    }
  else if (prop->IsA("vtkImageStack"))
    {
    mapper = static_cast<vtkImageStack *>(prop)->GetMapper();
    pickFlags = (pickFlags | VTK_TOOL_IMAGE_ACTOR);
    }
  else if (prop->IsA("vtkImageSlice"))
    {
    mapper = static_cast<vtkImageSlice *>(prop)->GetMapper();
    pickFlags = (pickFlags | VTK_TOOL_IMAGE_ACTOR);
    }
  else if (mapper && mapper->IsA("vtkImageMapper3D"))
    {
    pickFlags = (pickFlags | VTK_TOOL_IMAGE_ACTOR);
    }
  else if (mapper && mapper->IsA("vtkMapper"))
    {
    pickFlags = (pickFlags | VTK_TOOL_ACTOR);
    }
  else if (mapper && mapper->IsA("vtkAbstractVolumeMapper"))
    {
    pickFlags = (pickFlags | VTK_TOOL_VOLUME);
    }

  if (mapper && mapper->IsA("vtkImageMapper3D"))
    {
    // check if pick point is close to edge of slice plane
    // by intersecting the slice plane with each of the bounding planes
    // (to get the slice plane edge) and then computing the
    // distantance of the pick point to that edge.
    double mbounds[6];
    double mat[3][3];
    double vec[3];
    double mpoint[3];

    mapper->GetBounds(mbounds);
    picker->GetMapperPosition(mpoint);
    picker->GetMapperNormal(mat[0]);
    vec[0] = vtkMath::Dot(mat[0], mpoint);

    // find the closest edge that is within tolerance
    const double mtol = 7.0; // 7 mm
    for (int jj = 0; jj < 6; jj++)
      {
      double dd = fabs(mpoint[jj/2] - mbounds[jj]);
      if (dd < mtol)
        {
        mat[1][0] = mat[1][1] = mat[1][2] = 0.0;
        mat[1][jj/2] = 1.0;
        vec[1] = mbounds[jj];
        vtkMath::Cross(mat[0], mat[1], mat[2]);
        // make sure the planes aren't parallel to each other
        if (vtkMath::Norm(mat[2]) > 0.001)
          {
          vec[2] = vtkMath::Dot(mat[2], mpoint);
          double point[3];
          vtkMath::LinearSolve3x3(mat, vec, point);
          if (vtkMath::Distance2BetweenPoints(mpoint, point) < mtol*mtol)
            {
            pickFlags = (pickFlags | VTK_TOOL_PLANE_EDGE);
            }
          }
        }
      }
    }

  return pickFlags;
}

//----------------------------------------------------------------------------
void vtkToolCursor::ComputePosition()
{
  if (!this->Renderer)
    {
    return;
    }

  int x = this->DisplayPosition[0];
  int y = this->DisplayPosition[1];

  // Update the props that might be picked.  This is necessary
  // if there hasn't been a Render since the last change.
  this->UpdatePropsForPick(this->Picker, this->Renderer);

  // Do the pick!
  vtkVolumePicker *picker = this->Picker;
  picker->Pick(x, y, 0, this->Renderer);
  picker->GetPickPosition(this->Position);
  picker->GetPickNormal(this->Normal);

  // Allow the action to constrain the cursor
  if (this->Action)
    {
    vtkTool *actionObject =
      static_cast<vtkTool *>(
        this->Actions->GetItemAsObject(this->Action));

    if (actionObject)
      {
      actionObject->ConstrainCursor(this->Position, this->Normal);
      }
    }

  // Direct the normal towards the camera if PointNormalAtCamera is On
  if (this->PointNormalAtCamera &&
      vtkMath::Dot(this->Renderer->GetActiveCamera()
                   ->GetDirectionOfProjection(),
                   this->Normal) > 0)
    {
    this->Normal[0] = -this->Normal[0];
    this->Normal[1] = -this->Normal[1];
    this->Normal[2] = -this->Normal[2];
    }

  // Check to see if the PickFlags have changed
  int pickFlags = this->ComputePickFlags(this->Picker);

  if ((this->Modifier & (VTK_TOOL_BUTTON_MASK | VTK_TOOL_WHEEL_MASK)) == 0 &&
      !this->Action)
    {
    if (pickFlags != this->PickFlags)
      {
      // Compute the cursor shape from the state.
      this->SetShape(this->FindShape(this->Mode, pickFlags, this->Modifier));

      // See if we need focus for potential button actions
      this->ActionButtons = this->FindActionButtons(this->Mode, pickFlags,
                                                    this->Modifier);
      }
    this->PickFlags = pickFlags;

    // Check to see if guide visibility should be changed
    this->CheckGuideVisibility();
    }

  // Compute an "up" vector for the cursor.
  this->ComputeVectorFromNormal(this->Position, this->Normal, this->Vector,
                                this->Renderer,
                                this->Shapes->GetShapeFlags(this->Shape));

  // Compute the pose matrix for the cursor
  this->ComputeMatrix(this->Position, this->Normal, this->Vector,
                      this->Matrix);

  // Scale for the cursor to always be the same number of pixels across.
  double scale = this->ComputeScale(this->Position, this->Renderer);

  this->Actor->SetScale(scale*this->Scale);
}

//----------------------------------------------------------------------------
void vtkToolCursor::Modified()
{
  this->Mapper->Modified();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::OnRender()
{
  // Compute the position when the Renderer renders, since it needs to
  // update all of the props in the scene.
  this->ComputePosition();
  // Don't show cursor if nothing is underneath of it.
  int visibility = (this->IsInViewport != 0 && this->Shape != 0);
  this->Actor->SetVisibility(visibility);
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetGuideVisibility(int v)
{
  if (this->GuideVisibility != v)
    {
    this->GuideVisibility = v;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkToolCursor::CheckGuideVisibility()
{
  vtkVolumeMapper *mapper =
    vtkVolumeMapper::SafeDownCast(this->Picker->GetMapper());
  vtkProp3D *prop = this->Picker->GetProp3D();

  if (mapper &&
      ((((this->PickFlags & VTK_TOOL_CROP_PLANE) &&
          this->Picker->GetCroppingPlaneId() >= 0) ||
          this->Picker->GetPickCroppingPlanes()) ||
       (((this->PickFlags & VTK_TOOL_CLIP_PLANE) &&
          this->Picker->GetClippingPlaneId() >= 0) ||
          this->Picker->GetPickClippingPlanes())))
    {
    int clippingPlaneId = -1;
    if (this->PickFlags & VTK_TOOL_CLIP_PLANE)
      {
      clippingPlaneId = this->Picker->GetClippingPlaneId();
      }

    int croppingPlaneId = -1;
    if (this->PickFlags & VTK_TOOL_CROP_PLANE)
      {
      croppingPlaneId = this->Picker->GetCroppingPlaneId();
      }

    vtkPlaneCollection *mapperPlanes = mapper->GetClippingPlanes();
    vtkPlaneCollection *clippingPlanes = 0;
    if (mapperPlanes)
      {
      clippingPlanes = vtkPlaneCollection::New();
      int n = mapperPlanes->GetNumberOfItems();
      for (int i = 0; i < n; i++)
        {
        vtkFollowerPlane *plane = vtkFollowerPlane::New();
        plane->SetFollowPlane(mapperPlanes->GetItem(i));
        plane->SetFollowMatrix(prop->GetMatrix());
        plane->InvertFollowMatrixOn();
        clippingPlanes->AddItem(plane);
        plane->Delete();
        }
      }

    this->VolumeCroppingActor->SetUserMatrix(prop->GetMatrix());
    this->VolumeCroppingActor->SetVisibility(this->GuideVisibility);
    this->VolumeCroppingSource->SetVolumeMapper(mapper);
    this->VolumeCroppingSource->SetActivePlaneId(croppingPlaneId);
    this->ClipOutlineFilter->SetClippingPlanes(clippingPlanes);
    this->ClipOutlineFilter->SetActivePlaneId(clippingPlaneId);

    if (clippingPlanes)
      {
      clippingPlanes->Delete();
      }
    }
  else
    {
    this->VolumeCroppingActor->SetUserMatrix(0);
    this->VolumeCroppingActor->SetVisibility(0);
    this->VolumeCroppingSource->SetVolumeMapper(0);
    this->VolumeCroppingSource->SetActivePlaneId(-1);
    this->ClipOutlineFilter->SetActivePlaneId(-1);
    this->ClipOutlineFilter->SetClippingPlanes(0);
    }

  vtkImageMapper3D *imageMapper =
    vtkImageMapper3D::SafeDownCast(this->Picker->GetMapper());
  vtkImageStack *imageStack = vtkImageStack::SafeDownCast(prop);
  if (imageStack)
    {
    prop = imageStack->GetActiveImage();
    }
  vtkImageSlice *imageSlice = vtkImageSlice::SafeDownCast(prop);

  if (imageMapper == 0 && imageSlice != 0)
    {
    imageMapper = imageSlice->GetMapper();
    }

  if (imageMapper && this->PickFlags & VTK_TOOL_IMAGE_ACTOR)
    {
    vtkFollowerPlane *plane =
      vtkFollowerPlane::SafeDownCast(
        this->SliceOutlineCutter->GetCutFunction());
    if (plane == NULL)
      {
      plane = vtkFollowerPlane::New();
      this->SliceOutlineCutter->SetCutFunction(plane);
      plane->Delete();
      }
    plane->SetFollowPlane(imageMapper->GetSlicePlane());
    plane->SetFollowMatrix(prop->GetMatrix());
    plane->InvertFollowMatrixOn();

    int imageEdgeId = -1;
    if (this->PickFlags & VTK_TOOL_PLANE_EDGE)
      {
      double mbounds[6];
      double mpoint[3];

      imageMapper->GetBounds(mbounds);
      this->Picker->GetMapperPosition(mpoint);

      // find the closest edge that is within tolerance
      double mdist = VTK_DOUBLE_MAX;
      for (int jj = 0; jj < 6; jj++)
        {
        double dd = fabs(mpoint[jj/2] - mbounds[jj]);
        if (dd < mdist)
          {
          mdist = dd;
          imageEdgeId = jj;
          }
        }
      }

    this->SliceOutlineActor->SetUserMatrix(prop->GetMatrix());
    this->SliceOutlineActor->SetVisibility(this->GuideVisibility);
    //this->SliceOutlineSource->SetBounds(imageMapper->GetBounds());
    // a dummy volume mapper
    vtkVolumeRayCastMapper *vmapper = vtkVolumeRayCastMapper::New();
    vmapper->SetInput(imageMapper->GetInput());
    this->SliceOutlineSource->SetActivePlaneId(imageEdgeId);
    this->SliceOutlineSource->SetVolumeMapper(vmapper);
    vmapper->Delete();
    }
  else
    {
    this->SliceOutlineActor->SetUserMatrix(0);
    this->SliceOutlineActor->SetVisibility(0);
    if (this->SliceOutlineSource->GetVolumeMapper())
      {
      this->SliceOutlineSource->GetVolumeMapper()->
        SetInput(static_cast<vtkImageData *>(0));
      }
    this->SliceOutlineSource->SetVolumeMapper(0);
    this->SliceOutlineCutter->SetCutFunction(0);
    }
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetDisplayPosition(double x, double y)
{
  if (this->DisplayPosition[0] == x && this->DisplayPosition[1] == y)
    {
    return;
    }

  this->DisplayPosition[0] = x;
  this->DisplayPosition[1] = y;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::MoveToDisplayPosition(double x, double y)
{
  this->SetDisplayPosition(x, y);

  if (this->Renderer)
    {
    this->SetIsInViewport(this->Renderer->IsInViewport(x, y));
    }

  if (this->Action)
    {
    vtkTool *actionObject =
      static_cast<vtkTool *>(
        this->Actions->GetItemAsObject(this->Action));

    if (actionObject)
      {
      actionObject->DoAction();
      }
   }
}

//----------------------------------------------------------------------------
int vtkToolCursor::GetVisibility()
{
  return this->Actor->GetVisibility();
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetIsInViewport(int inside)
{
  if (this->IsInViewport == inside)
    {
    return;
    }

  this->IsInViewport = inside;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkToolCursor::ButtonBit(int button)
{
  static int buttonmap[6] = { 0, VTK_TOOL_B1, VTK_TOOL_B2, VTK_TOOL_B3,
    VTK_TOOL_WHEEL_BWD, VTK_TOOL_WHEEL_FWD };

  if (button <= 0 || button > 5)
    {
    return 0;
    }

  return buttonmap[button];
}

//----------------------------------------------------------------------------
int vtkToolCursor::PressButton(int button)
{
  int buttonBit = this->ButtonBit(button);
  int started = 0;

  this->SetModifier(this->Modifier | buttonBit);

  if (buttonBit && !this->Action)
    {
    this->SetAction(this->FindAction(this->Mode, this->PickFlags,
                                     this->Modifier));
    if (this->Action)
      {
      this->ActionButton = button;
      started = 1;
      }
    }

  return started;
}

//----------------------------------------------------------------------------
int vtkToolCursor::ReleaseButton(int button)
{
  int buttonBit = this->ButtonBit(button);
  int concluded = 0;

  if (this->Action && button == this->ActionButton)
    {
    this->SetAction(0);
    this->ActionButton = 0;
    concluded = 1;
    }

  // Force SetModifier to see a change
  this->Modifier = (this->Modifier | buttonBit);
  this->SetModifier(this->Modifier & ~buttonBit);

  return concluded;
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetModifier(int modifier)
{
  if (this->Modifier == modifier)
    {
    return;
    }

  if ( ((this->Modifier & (VTK_TOOL_BUTTON_MASK | VTK_TOOL_WHEEL_MASK)) == 0 ||
        (modifier & (VTK_TOOL_BUTTON_MASK | VTK_TOOL_WHEEL_MASK)) == 0) &&
       !this->Action)
    {
    // Compute the cursor shape from the state.
    this->SetShape(this->FindShape(this->Mode, this->PickFlags, modifier));

    // See if we need focus for potential button actions
    this->ActionButtons = this->FindActionButtons(this->Mode, this->PickFlags,
                                                  modifier);
    }

  this->Modifier = modifier;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetAction(int action)
{
  if (action == this->Action)
    {
    return;
    }

  if (this->Action)
    {
    vtkTool *actionObject =
      static_cast<vtkTool *>(
        this->Actions->GetItemAsObject(this->Action));

    if (actionObject)
      {
      actionObject->StopAction();
      actionObject->SetToolCursor(0);
      }
    }

  this->Action = action;
  this->Modified();

  if (action)
    {
    vtkTool *actionObject =
      static_cast<vtkTool *>(
        this->Actions->GetItemAsObject(this->Action));

    if (actionObject)
      {
      actionObject->SetToolCursor(this);
      actionObject->StartAction();
      }
    }
}

//----------------------------------------------------------------------------
int vtkToolCursor::AddShape(vtkCursorShapes *shapes,
                               const char *name)
{
  int i = shapes->GetShapeIndex(name);
  if (i < 0)
    {
    vtkErrorMacro("The specified shape \"" << name << "\" is not in "
                   << shapes->GetClassName());
    return -1;
    }

  this->Shapes->AddShape(name, shapes->GetShapeData(i),
                         shapes->GetShapeFlags(i));

  return (this->Shapes->GetNumberOfShapes() - 1);
}

//----------------------------------------------------------------------------
int vtkToolCursor::AddAction(vtkTool *action)
{
  this->Actions->AddItem(action);

  return (this->Actions->GetNumberOfItems() - 1);
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetMode(int mode)
{
  if (this->Mode == mode)
    {
    return;
    }

  this->Mode = mode;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::SetShape(int shape)
{
  vtkDataSet *data = this->Shapes->GetShapeData(shape);

  if (data)
    {
    this->Mapper->SetInput(data);
    this->Shape = shape;
    }
}

//----------------------------------------------------------------------------
double vtkToolCursor::ComputeScale(const double position[3],
                                      vtkRenderer *renderer)
{
  // Find the cursor scale factor such that 1 data unit length
  // equals 1 screen pixel at the cursor's distance from the camera.
  // Start by computing the height of the window at the cursor position.
  double worldHeight = 1.0;
  vtkCamera *camera = renderer->GetActiveCamera();
  if (camera->GetParallelProjection())
    {
    worldHeight = 2*camera->GetParallelScale();
    }
  else
    {
    vtkMatrix4x4 *matrix = camera->GetViewTransformMatrix();
    // Get a 3x3 matrix with the camera orientation
    double cvz[3];
    cvz[0] = matrix->GetElement(2, 0);
    cvz[1] = matrix->GetElement(2, 1);
    cvz[2] = matrix->GetElement(2, 2);

    double cameraPosition[3];
    camera->GetPosition(cameraPosition);

    double v[3];
    v[0] = cameraPosition[0] - position[0];
    v[1] = cameraPosition[1] - position[1];
    v[2] = cameraPosition[2] - position[2];

    worldHeight = 2*(vtkMath::Dot(v,cvz)
                     * tan(0.5*camera->GetViewAngle()/57.296));
    }

  // Compare world height to window height.
  int windowHeight = renderer->GetSize()[1];
  double scale = 1.0;
  if (windowHeight > 0)
    {
    scale = worldHeight/windowHeight;
    }

  return scale;
}

//----------------------------------------------------------------------------
void vtkToolCursor::ComputeMatrix(const double p[3], const double n[3],
                                     const double v[3], vtkMatrix4x4 *matrix)
{
  double u[3];
  vtkMath::Cross(v, n, u);

  for (int j = 0; j < 3; j++)
    {
    matrix->SetElement(j, 0, u[j]);
    matrix->SetElement(j, 1, v[j]);
    matrix->SetElement(j, 2, n[j]);
    matrix->SetElement(j, 3, p[j]);
    }
  matrix->Modified();
}

//----------------------------------------------------------------------------
void vtkToolCursor::ComputeVectorFromNormal(const double position[3],
                                               const double normal[3],
                                               double vector[3],
                                               vtkRenderer *renderer,
                                               int cursorFlags)
{
  // Mask out all but the relevant flags
  cursorFlags = (cursorFlags & VTK_TOOL_ORIENT);

  // Get the camera orientation
  vtkCamera *camera = renderer->GetActiveCamera();
  vtkMatrix4x4 *matrix = camera->GetViewTransformMatrix();

  // Get a 3x3 matrix with the camera orientation
  double mat[3][3];
  for (int i = 0; i < 3; i++)
    {
    mat[i][0] = matrix->GetElement(i, 0);
    mat[i][1] = matrix->GetElement(i, 1);
    mat[i][2] = matrix->GetElement(i, 2);
    }

  if (cursorFlags == VTK_TOOL_RADIALX ||
      cursorFlags == VTK_TOOL_RADIALY)
    {
    // Make "y" point away from camera axis
    double f[3];
    camera->GetFocalPoint(f);

    double v[3];
    v[0] = position[0] - f[0];
    v[1] = position[1] - f[1];
    v[2] = position[2] - f[2];

    // Make "x" perpendicular to new "y"
    vtkMath::Cross(v, mat[2], mat[0]);
    vtkMath::Normalize(mat[0]);

    // Orthogonalize "y" wrt to camera axis
    vtkMath::Cross(mat[2], mat[0], mat[1]);
    vtkMath::Normalize(mat[1]);
    }

  // These ints say how we want to create the vector
  int direction = 1; // We want to align the cursor y vector with...
  int primary = 1;   // the camera's y vector if possible...
  int secondary = 2; // or with the camera's z vector otherwise.

  // If the data is "flat" and the flat dimension is not the z dimension,
  // then point the flat side at the camera.
  if (cursorFlags == VTK_TOOL_FLATX)
    {
    direction = 0;
    primary = 2;
    secondary = 1;
    }
  else if (cursorFlags == VTK_TOOL_FLATY)
    {
    direction = 1;
    primary = 2;
    secondary = 1;
    }
  else if (cursorFlags == VTK_TOOL_RADIALX)
    {
    direction = 0;
    primary = 1;
    secondary = 0;
    }
  else if (cursorFlags == VTK_TOOL_RADIALY)
    {
    direction = 1;
    primary = 1;
    secondary = 0;
    }

  // Get primary direction from camera, orthogonalize to the normal.
  double u[3];
  u[0] = mat[primary][0];
  u[1] = mat[primary][1];
  u[2] = mat[primary][2];

  // dot will be 1.0 if primary and normal are the same
  double dot = vtkMath::Dot(normal, u);

  if (dot > 0.999)
    {
    // blend the vector with the secondary for stability
    u[0] += mat[secondary][0] * (dot - 0.999);
    u[1] += mat[secondary][1] * (dot - 0.999);
    u[2] += mat[secondary][2] * (dot - 0.999);
    }

  vtkMath::Cross(normal, u, u);
  if (direction == 1)
    {
    vtkMath::Cross(u, normal, u);
    }

  double norm = vtkMath::Norm(u);
  vector[0] = u[0]/norm;
  vector[1] = u[1]/norm;
  vector[2] = u[2]/norm;
}

//----------------------------------------------------------------------------
// A couple useful diagnostic routines
/*
static void PrintModifier(int modifier);
static void PrintFlags(int flags);
*/

//----------------------------------------------------------------------------
void vtkToolCursor::AddBinding(vtkIntArray *array, int item, int mode,
                                  int pickFlags, int modifier)
{
  // The bindings are sorted by mode and by the number of modifier bits.
  // Exact matches replace the previous entry.

  //cerr << "AddBinding " << array->GetName() << " ";
  //PrintFlags(pickFlags);
  //cerr << " ";
  //PrintModifier(modifier);
  //cerr << " " << item << "\n";

  int i = vtkToolCursor::ResolveBinding(array, 0,
                                           mode, pickFlags, modifier);
  int n = array->GetNumberOfTuples();

  int tuple[4];
  tuple[0] = 0;
  tuple[1] = 0;
  tuple[2] = 0;
  tuple[3] = 0;

  if (i >= 0)
    {
    // If it's an exact match, then replace
    array->GetTupleValue(i, tuple);
    if (tuple[0] == mode && tuple[1] == pickFlags && tuple[2] == modifier)
      {
      tuple[3] = item;
      array->SetTupleValue(i, tuple);
      return;
      }
    }
  else
    {
    // If it wasn't resolved, then we append to the end of the list
    i = n;
    }

  //cerr << "inserting ";
  //cerr << item << " (" << i << " of " << (n+1) << "): " << mode << " ";
  //PrintFlags(pickFlags);
  //cerr << " ";
  //PrintModifier(modifier);
  //cerr << "\n";

  // Extend array by one value.  Actually, this is just a dummy tuple,
  // it's just the easiest way of increasing the size of the array.
  array->InsertNextTupleValue(tuple);

  //cerr << "n = " << n << " i = " << i << "\n";
  // Shuffle values up by one
  for (int j = n; j > i; j--)
    {
    array->GetTupleValue(j-1, tuple);
    array->SetTupleValue(j, tuple);
    }

  // Set the tuple at the desired index
  tuple[0] = mode;
  tuple[1] = pickFlags;
  tuple[2] = modifier;
  tuple[3] = item;

  array->SetTupleValue(i, tuple);
}

//----------------------------------------------------------------------------
int vtkToolCursor::ResolveBinding(vtkIntArray *array, int start,
                                     int mode, int pickFlags, int modifier)
{
  // The following rules are used to resolve a binding:
  //
  // 1) the mode must match the binding exactly
  //
  // 2) all bits of each of the PROP3D and PLANE sections of the pick
  //    flags must have a matching bit in the binding, unless that section
  //    of the binding is empty.
  //
  // 3) the modifier bits must include all bits in the binding.

  //cerr << "ResolveBinding " << array->GetName() << " ";
  //PrintFlags(pickFlags);
  //cerr << " ";
  //PrintModifier(modifier);
  //cerr << "\n";

  int propType = (pickFlags & VTK_TOOL_PROP3D);
  int planeType = (pickFlags & VTK_TOOL_PLANE);

  int tuple[4];
  int n = array->GetNumberOfTuples();
  for (int i = start; i < n; i++)
    {
    array->GetTupleValue(i, tuple);

    //cerr << "item " << i << " of " << n << ": " << tuple[0] << " ";
    //PrintFlags(tuple[1]);
    //cerr << " ";
    //PrintModifier(tuple[2]);
    //cerr << "\n";

    if (tuple[0] == mode)
      {
      if (((tuple[1] & VTK_TOOL_PROP3D) == 0 ||
           (propType != 0 && (tuple[1] & propType) == propType)) &&
          ((tuple[1] & VTK_TOOL_PLANE) == 0 ||
           (planeType != 0 && (tuple[1] & planeType) == planeType)))
        {
        if ((tuple[2] & modifier) == tuple[2])
          {
          //cerr << "resolved " << tuple[3] << "\n";
          return i;
          }
        }
      }
    else if (tuple[0] > mode)
      {
      break;
      }
    }

  //cerr << "not resolved\n";
  return -1;
}

/*
static void PrintModifier(int modifier)
{
  cerr << "( ";
  if (modifier & VTK_TOOL_SHIFT) { cerr << "SHIFT "; }
  if (modifier & VTK_TOOL_CAPS) { cerr << "CAPS "; }
  if (modifier & VTK_TOOL_CONTROL) { cerr << "CONTROL "; }
  if (modifier & VTK_TOOL_META) { cerr << "META "; }
  if (modifier & VTK_TOOL_ALT) { cerr << "ALT "; }
  if (modifier & VTK_TOOL_B1) { cerr << "B1 "; }
  if (modifier & VTK_TOOL_B2) { cerr << "B2 "; }
  if (modifier & VTK_TOOL_B3) { cerr << "B3 "; }
  cerr << ")";
}

static void PrintFlags(int flags)
{
  cerr << "( ";
  if ((flags & VTK_TOOL_PROP3D) == VTK_TOOL_PROP3D)
    {
    cerr << "PROP3DS ";
    }
  else
    {
    if (flags & VTK_TOOL_ACTOR) { cerr << "ACTOR "; }
    if (flags & VTK_TOOL_VOLUME) { cerr << "VOLUME "; }
    if (flags & VTK_TOOL_IMAGE_ACTOR) { cerr << "IMAGE_ACTOR "; }
    }
  if ((flags & VTK_TOOL_CLIP_PLANE) && (flags & VTK_TOOL_CROP_PLANE))
    {
    cerr << "PLANES ";
    }
  else
    {
    if (flags & VTK_TOOL_CLIP_PLANE) { cerr << "CLIP_PLANE "; }
    if (flags & VTK_TOOL_CROP_PLANE) { cerr << "CROP_PLANE "; }
    }
  cerr << ")";
}
*/

