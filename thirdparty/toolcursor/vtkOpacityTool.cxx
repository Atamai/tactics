/*=========================================================================

  Program:   ToolCursor
  Module:    vtkOpacityTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpacityTool.h"
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
#include "vtkMath.h"

vtkStandardNewMacro(vtkOpacityTool);

//----------------------------------------------------------------------------
vtkOpacityTool::vtkOpacityTool()
{
  this->StartOpacity = 1.0;

  this->CurrentImageProperty = 0;
}

//----------------------------------------------------------------------------
vtkOpacityTool::~vtkOpacityTool()
{
  if (this->CurrentImageProperty)
    {
    this->CurrentImageProperty->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkOpacityTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkOpacityTool::StartAction()
{
  this->Superclass::StartAction();

  this->SetCurrentImageToNthImage(-1);

  if (this->CurrentImageProperty)
    {
    vtkImageProperty *property = this->CurrentImageProperty;
    this->StartOpacity = property->GetOpacity();
    }
}

//----------------------------------------------------------------------------
void vtkOpacityTool::StopAction()
{
  this->Superclass::StopAction();
  this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
}

//----------------------------------------------------------------------------
void vtkOpacityTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();

  // Get the display position.
  double x, y, x0, y0;
  this->GetStartDisplayPosition(x0, y0);
  this->GetDisplayPosition(x, y);

  int *size = cursor->GetRenderer()->GetSize();

  double opacity = this->StartOpacity;
  if (opacity < 0.0) { opacity = 0.0; }
  if (opacity > 1.0) { opacity = 1.0; }

  // use a sinusoidal scale to allow more precision near 0.0 and 1.0
  double delta = (opacity - 0.5);
  double theta = atan2(delta, sqrt(1.0 - delta*delta));
  theta += (x - x0)*2.0/(size[1] + 1);
  if (theta < -0.5*vtkMath::DoublePi())
    {
    opacity = 0.0;
    }
  else if (theta > 0.5*vtkMath::DoublePi())
    {
    opacity = 1.0;
    }
  else
    {
    opacity = 0.5 + sin(theta);
    }

  if (this->CurrentImageProperty)
    {
    this->CurrentImageProperty->SetOpacity(opacity);
    }
}

//----------------------------------------------------------------------------
void vtkOpacityTool::SetCurrentImageToNthImage(int i)
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
        if ( (imageProp = vtkImageSlice::SafeDownCast(tryProp)) != 0 &&
            tryProp->GetPickable() )
          {
          if (j == i) { break; }
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
