/*=========================================================================

  Program:   ToolCursor
  Module:    vtkImageTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkRenderer.h"
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkCommand.h"
#include "vtkLODProp3D.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkImageTool);

//----------------------------------------------------------------------------
vtkImageTool::vtkImageTool()
{
  this->CurrentImageProperty = 0;
  this->CurrentImageMapper = 0;
  this->CurrentImageMatrix = 0;
}

//----------------------------------------------------------------------------
vtkImageTool::~vtkImageTool()
{
  this->SetCurrentImage(0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkImageTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageTool::StartAction()
{
  this->Superclass::StartAction();
  this->FindCurrentImage();
}

//----------------------------------------------------------------------------
void vtkImageTool::StopAction()
{
  this->Superclass::StopAction();
  this->SetCurrentImage(0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkImageTool::DoAction()
{
  this->Superclass::DoAction();
}

//----------------------------------------------------------------------------
void vtkImageTool::FindCurrentImage()
{
  // Search the renderer to find the currently active image
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkPropCollection *props = cursor->GetRenderer()->GetViewProps();
  vtkProp *prop = 0;
  vtkAssemblyPath *path;
  vtkImageProperty *property = 0;
  vtkImageMapper3D *mapper = 0;
  vtkMatrix4x4 *matrix = 0;
  vtkCollectionSimpleIterator pit;

  for (props->InitTraversal(pit); (prop = props->GetNextProp(pit)); )
    {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath()); )
      {
      vtkProp *tryProp = path->GetLastNode()->GetViewProp();
      if (tryProp->GetVisibility())
        {
        vtkImageSlice *imageProp = 0;
        vtkLODProp3D *lodProp = 0;

        if ((imageProp = vtkImageSlice::SafeDownCast(tryProp)) != 0)
          {
          mapper = imageProp->GetMapper();
          property = imageProp->GetProperty();
          matrix = path->GetLastNode()->GetMatrix();
          }
        else if ((lodProp = vtkLODProp3D::SafeDownCast(tryProp)) != 0)
          {
          int lodId = lodProp->GetPickLODID();
          vtkAbstractMapper3D *tryMapper = lodProp->GetLODMapper(lodId);
          if ( (mapper = vtkImageMapper3D::SafeDownCast(tryMapper)) != 0)
            {
            lodProp->GetLODProperty(lodId, &property);
            matrix = path->GetLastNode()->GetMatrix();
            }
          }
        }
      }
    }

  this->SetCurrentImage(mapper, property, matrix);
}

//----------------------------------------------------------------------------
void vtkImageTool::SetCurrentImage(
  vtkImageMapper3D *mapper, vtkImageProperty *property, vtkMatrix4x4 *matrix)
{
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

  if (mapper != this->CurrentImageMapper)
    {
    if (this->CurrentImageMapper)
      {
      this->CurrentImageMapper->Delete();
      }

    this->CurrentImageMapper = mapper;

    if (this->CurrentImageMapper)
      {
      this->CurrentImageMapper->Register(this);
      }
    }

  if (matrix != this->CurrentImageMatrix)
    {
    if (this->CurrentImageMatrix)
      {
      this->CurrentImageMatrix->Delete();
      }

    this->CurrentImageMatrix = matrix;

    if (this->CurrentImageMatrix)
      {
      this->CurrentImageMatrix->Register(this);
      }
    }
}
