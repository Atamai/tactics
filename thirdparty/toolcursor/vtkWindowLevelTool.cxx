/*=========================================================================

  Program:   ToolCursor
  Module:    vtkWindowLevelTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWindowLevelTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkVolumePicker.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkWindowLevelTool);

//----------------------------------------------------------------------------
vtkWindowLevelTool::vtkWindowLevelTool()
{
  this->StartWindowLevel[0] = 1.0;
  this->StartWindowLevel[1] = 0.5;

  this->CurrentImageProperty = 0;
}

//----------------------------------------------------------------------------
vtkWindowLevelTool::~vtkWindowLevelTool()
{
  if (this->CurrentImageProperty)
    {
    this->CurrentImageProperty->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkWindowLevelTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkWindowLevelTool::StartAction()
{
  this->Superclass::StartAction();

  this->SetCurrentImageToNthImage(-1);

  if (this->CurrentImageProperty)
    {
    vtkImageProperty *property = this->CurrentImageProperty;
    this->StartWindowLevel[0] = property->GetColorWindow();
    this->StartWindowLevel[1] = property->GetColorLevel();
    }
}

//----------------------------------------------------------------------------
void vtkWindowLevelTool::StopAction()
{
  this->Superclass::StopAction();
  this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
}

//----------------------------------------------------------------------------
void vtkWindowLevelTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();

  // Get the display position.
  double x, y, x0, y0;
  this->GetStartDisplayPosition(x0, y0);
  this->GetDisplayPosition(x, y);

  int *size = cursor->GetRenderer()->GetSize();

  double window = this->StartWindowLevel[0];
  double level = this->StartWindowLevel[1];

  level += window*(y - y0)*2.0/(size[1] + 1);
  window *= pow(10.0, (x - x0)*2.0/(size[1] + 1));

  if (this->CurrentImageProperty)
    {
    this->CurrentImageProperty->SetColorWindow(window);
    this->CurrentImageProperty->SetColorLevel(level);
    }
}

//----------------------------------------------------------------------------
void vtkWindowLevelTool::SetCurrentImageToNthImage(int i)
{
  // Get all the information needed for the interaction
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkPropCollection *props = cursor->GetRenderer()->GetViewProps();
  vtkProp *prop = 0;
  vtkAssemblyPath *path;
  vtkImageSlice *imageProp = 0;
  vtkCollectionSimpleIterator pit;

  for (int k = 0; k < 2; k++)
    {
    int j = 0;
    for (props->InitTraversal(pit); (prop = props->GetNextProp(pit)); )
      {
      for (prop->InitPathTraversal(); (path = prop->GetNextPath()); )
        {
        vtkProp *tryProp = path->GetLastNode()->GetViewProp();
        if ( tryProp && tryProp->GetPickable() &&
             (imageProp = vtkImageSlice::SafeDownCast(tryProp)) != 0 )
          {
          if (j == i)
            {
            break;
            }
          imageProp = 0;
          j++;
          }
        }
      if (imageProp)
        {
        break;
        }
      }
    if (i < 0)
      {
      i += j;
      }
    }

  vtkImageProperty *property = 0;
  if (imageProp)
    {
    property = imageProp->GetProperty();
    }

  if (property != this->CurrentImageProperty)
    {
    if (this->CurrentImageProperty)
      {
      this->CurrentImageProperty->Delete();
      }

    this->CurrentImageProperty = property;

    if (this->CurrentImageProperty)
      {
      this->CurrentImageProperty->Register(this);
      }
    }
}
