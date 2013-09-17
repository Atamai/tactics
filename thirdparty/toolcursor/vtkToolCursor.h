/*=========================================================================

  Program:   ToolCursor
  Module:    vtkToolCursor.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkToolCursor - Cursor for picking and manipulating Prop3Ds
// .SECTION Description
// This class assists with picking and with providing interaction with
// objects in a 3D scene.  It allows the picking to be customized for
// different actors in the scene.

#ifndef __vtkToolCursor_h
#define __vtkToolCursor_h

#include "vtkObject.h"

class vtkActor;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkMatrix4x4;
class vtkLookupTable;
class vtkPolyData;
class vtkDataSet;
class vtkDataSetMapper;
class vtkDataSetCollection;
class vtkCollection;
class vtkPicker;
class vtkVolumePicker;
class vtkIntArray;
class vtkCommand;

class vtkTool;
class vtkCursorShapes;
class vtkVolumeOutlineSource;
class vtkClipClosedSurface;
class vtkOutlineSource;
class vtkCutter;
class vtkTriangleFilter;
class vtkTubeFilter;

// Modifier keys and mouse buttons.
#define VTK_TOOL_SHIFT        0x0001
#define VTK_TOOL_CAPS         0x0002
#define VTK_TOOL_CONTROL      0x0004
#define VTK_TOOL_META         0x0008
#define VTK_TOOL_ALT          0x0010
#define VTK_TOOL_BUTTON_MASK  0x0700
#define VTK_TOOL_B1           0x0100
#define VTK_TOOL_B2           0x0200
#define VTK_TOOL_B3           0x0400
#define VTK_TOOL_WHEEL_MASK   0x1800
#define VTK_TOOL_WHEEL_BWD    0x0800
#define VTK_TOOL_WHEEL_FWD    0x1000

// Pick flags, these describe what is under the cursor.
#define VTK_TOOL_PROP3D       0x0F00
#define VTK_TOOL_ACTOR        0x0100
#define VTK_TOOL_VOLUME       0x0200
#define VTK_TOOL_IMAGE_ACTOR  0x0400
#define VTK_TOOL_PLANE        0x3000
#define VTK_TOOL_CLIP_PLANE   0x1000
#define VTK_TOOL_CROP_PLANE   0x2000
#define VTK_TOOL_PLANE_EDGE   0x4000

class VTK_EXPORT vtkToolCursor : public vtkObject
{
public:
  // Description:
  // Instantiate the object.
  static vtkToolCursor *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkToolCursor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the default bindings.
  virtual void BindDefaultActions();

  // Description:
  // Set the Renderer for this cursor to be active in.
  void SetRenderer(vtkRenderer *renderer);
  vtkRenderer *GetRenderer() { return this->Renderer; };

  // Description:
  // Set the current (x,y) display location for the cursor.
  void SetDisplayPosition(double x, double y);
  vtkGetVector2Macro(DisplayPosition, double);

  // Description:
  // Compute the cursor position based on the display position.
  virtual void ComputePosition();

  // Description:
  // Get the cursor position in world coordinates.
  vtkGetVector3Macro(Position, double);

  // Description:
  // Get the cursor normal in world coordinates.
  vtkGetVector3Macro(Normal, double);

  // Description:
  // Get the cursor vertical direction in world coordinates.
  // This is the ViewUp of the camera, tilted to be made perpendicular
  // to the Normal.
  vtkGetVector3Macro(Vector, double);

  // Description:
  // Get the cursor's pose matrix.  This matrix is composed from
  // the Position, Normal, and CursorVertical.
  vtkMatrix4x4 *GetMatrix() { return this->Matrix; };

  // Description:
  // Get the picker.  The picker is a vtkVolumePicker.  Every time that
  // ComputePosition is called, a pick is done and all of the picker
  // information is updated.
  vtkVolumePicker *GetPicker() { return this->Picker; };

  // Description:
  // Get the pick flags.  The flags provide information about what was
  // under the cursor the last time that a pick was done.  The flags are
  // used to determine what kinds of actions the cursor can take.
  int GetPickFlags() { return this->PickFlags; };

  // Description:
  // Set the current mode for the cursor.  This allows you to easily
  // switch between different sets of action bindings for the cursor.
  // Each mode can have its own set of bindings.  This method can be
  // overridden in subclasses if you want to do anything special when
  // the mode changes.
  virtual void SetMode(int mode);
  int GetMode() { return this->Mode; };

  // Description:
  // Set the scale factor for the cursor, usually to make it larger.
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);

  // Description:
  // Set the colors to use for the cursor.  The first color is used for
  // the part of the cursor on the "near" side, and the second color is
  // used for the part of the cursor on the "far".  Other colors are
  // specific to particular cursor shaped.  If a cursor shape uses
  // RGBA color instead of indexed color, then these colors will have
  // no effect.
  void SetColor(int i, double r, double g, double b);
  void SetColor(int i, const double rgb[3]) {
    this->SetColor(i, rgb[0], rgb[1], rgb[2]); };
  double *GetColor(int i) {
    this->GetColor(i, this->Color); return this->Color; };
  void GetColor(int i, double rgb[3]);

  // Description:
  // Set the visibility of the outline around the object under the
  // cursor.
  void SetGuideVisibility(int v);
  void GuideVisibilityOn() { this->SetGuideVisibility(1); }
  void GuideVisibilityOff() { this->SetGuideVisibility(0); }
  int GetGuideVisibility() { return this->GuideVisibility; }

  // Description:
  // Set whether surface normals should always point towards the camera,
  // or whether they should point away if the cursor is on the backface
  // of the surface.  The default is "On".
  vtkSetMacro(PointNormalAtCamera, int);
  vtkBooleanMacro(PointNormalAtCamera, int);
  vtkGetMacro(PointNormalAtCamera, int);

  // Description:
  // Move the cursor to a specific position, usually in response to the
  // mouse motion.  This is not a passive method like SetDisplayPosition().
  // Depending on the Action ivar, the motion will go to the appropriate
  // interaction method.
  virtual void MoveToDisplayPosition(double x, double y);

  // Description:
  // Set or get the modifier bitfield.  This is how the GUI toolkit
  // communicates changes in the modifier keys or the mouse buttons.
  // You must supply a mask that contains the modifier bits that you
  // want to change.
  void SetModifierBits(int modifier, int mask);

  // Description:
  // Call this method to indicate that a mouse button has been pressed.
  // It will return 1 if the button press initiated an action, and 0 if not.
  int PressButton(int button);

  // Description:
  // Call this method to indicate that a mouse button has been released.
  // It will return 1 if the button release concluded an action, and 0 if not.
  int ReleaseButton(int button);

  // Description:
  // Add an action.  The id for the action will be returned.  Once an
  // action is added, it cannot be removed.
  int AddAction(vtkTool *action);

  // Description:
  // Add a cursor shape.  The id for the shape will be returned.  Once a
  // shape has been added, it cannot be removed.
  int AddShape(vtkCursorShapes *shapes, const char *name);

  // Description:
  // Bind the conditions under which the specified action will take place.
  // The "mode" is the cursor mode that the binding will apply to.
  // The "pickFlags" is the kind of object that must be under the cursor.
  // The "modifier" is the combination of modifier keys and the mouse
  // buttons that must be held down for the action to occur.
  void BindAction(int action, int mode, int pickFlags, int modifier);

  // Description:
  // Bind the conditions under which the specified shape will be used.
  // The "mode" is the cursor mode that the binding will apply to.
  // The "pickFlags" is the kind of object that must be under the cursor.
  // The "modifier" is the combination of modifier keys and mouse
  // buttons that must be held down for the shape to be shown.
  void BindShape(int shape, int mode, int pickFlags, int modifier);

  // Description:
  // Set whether the mouse is in the renderer's viewport.  This controls
  // cursor visibility.  It is automatically set by SetDisplayPosition()
  // and MoveToDisplayPosition(), but it must be set manually when the
  // mouse moves out of the window.
  virtual void SetIsInViewport(int inside);
  int GetIsInViewport() { return this->IsInViewport; };

  // Description:
  // Get whether the cursor is currently visible.  If not, then the
  // surface cursor should be made visible.
  int GetVisibility();

  // Description:
  // If GetActionButtons() is nonzero, then it gives the mouse bit
  // flags for the buttons that have actions defined for the current
  // mode, position, and modifiers.
  int GetActionButtons() { return this->ActionButtons; };

  // Description:
  // This is an internal method that is automatically called at the
  // begining of every render.
  virtual void OnRender();

  // Description:
  // We override this method to modify the actor, otherwise the
  // RenderWindow won't know that it needs to render.
  virtual void Modified();

  // Description:
  // Get the button modifier bitmask corresponding to the mouse button.
  static int ButtonBit(int button);

  // Description:
  // Get the current modifier bits.
  int GetModifier() { return this->Modifier; };

protected:
  vtkToolCursor();
  ~vtkToolCursor();

  double OpacityThreshold;

  double DisplayPosition[2];
  double Position[3];
  double Normal[3];
  double Vector[3];

  double Color[3];

  int GuideVisibility;
  int PointNormalAtCamera;
  int ActionButtons;
  int Mode;
  int PickFlags;
  int Shape;
  int Action;
  int ActionButton;
  int Modifier;
  int IsInViewport;
  double Scale;
  vtkMatrix4x4 *Matrix;
  vtkCursorShapes *Shapes;
  vtkIntArray *ShapeBindings;
  vtkCollection *Actions;
  vtkIntArray *ActionBindings;
  vtkDataSetMapper *Mapper;
  vtkLookupTable *LookupTable;
  vtkActor *Actor;
  vtkVolumePicker *Picker;
  vtkRenderer *Renderer;
  vtkCommand *RenderCommand;

  vtkActor *VolumeCroppingActor;
  vtkDataSetMapper *VolumeCroppingMapper;
  vtkVolumeOutlineSource *VolumeCroppingSource;
  vtkClipClosedSurface *ClipOutlineFilter;

  vtkActor *SliceOutlineActor;
  vtkDataSetMapper *SliceOutlineMapper;
  vtkVolumeOutlineSource *SliceOutlineSource;
  vtkCutter *SliceOutlineCutter;
  vtkTriangleFilter *SliceOutlineTriangleFilter;
  vtkTubeFilter *SliceOutlineTube;

  // Description:
  // Set the modifier bits.  This is protected because SetModifierBits()
  // is the method that should be used to set the modifier.
  virtual void SetModifier(int modifier);

  // Description:
  // Set the current action.  This is protected because the action depends
  // on the current state.
  virtual void SetAction(int action);
  int GetAction() { return this->Action; };

  // Description:
  // Set the cursor shape.  This is protected because the shape depends on
  // the state.
  void SetShape(int shape);
  int GetShape() { return this->Shape; };

  void CheckGuideVisibility();
  int FindShape(int mode, int pickFlags, int modifier);
  int FindAction(int mode, int pickFlags, int modifier);
  int FindActionButtons(int mode, int pickFlags, int modifier);
  static void AddBinding(vtkIntArray *array, int item, int mode,
                         int pickFlags, int modifier);
  static int ResolveBinding(vtkIntArray *array, int start,
                            int mode, int pickFlags, int modifier);
  static int ComputePickFlags(vtkVolumePicker *picker);
  static double ComputeScale(const double position[3], vtkRenderer *renderer);
  static void ComputeMatrix(const double position[3], const double normal[3],
                            const double vector[3], vtkMatrix4x4 *matrix);
  static void ComputeVectorFromNormal(const double position[3],
                                      const double normal[3], double vector[3],
                                      vtkRenderer *renderer, int cursorFlags);

  static void UpdatePropsForPick(vtkPicker *picker, vtkRenderer *renderer);

private:
  vtkToolCursor(const vtkToolCursor&);  //Not implemented
  void operator=(const vtkToolCursor&);  //Not implemented
};

inline void vtkToolCursor::SetModifierBits(int modifier, int mask)
{
  this->SetModifier((this->GetModifier() & ~mask) | (modifier & mask));
}

#endif
