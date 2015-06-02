/*=========================================================================

  Program:   ToolCursor
  Module:    vtkTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkVolumePicker.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkTool);

//----------------------------------------------------------------------------
vtkTool::vtkTool()
{
  this->ToolCursor = 0;
}

//----------------------------------------------------------------------------
vtkTool::~vtkTool()
{
  // ToolCursor is not reference counted and therefore not deleted.
  this->ToolCursor = 0;
}

//----------------------------------------------------------------------------
void vtkTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkTool::SetToolCursor(vtkToolCursor *cursor)
{
  if (cursor != this->ToolCursor)
    {
    this->ToolCursor = cursor;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTool::StartAction()
{
  this->ToolCursor->GetPosition(this->StartPosition);
  this->ToolCursor->GetDisplayPosition(this->StartDisplayPosition);
  this->DisplayPosition[0] = this->StartDisplayPosition[0];
  this->DisplayPosition[1] = this->StartDisplayPosition[1];
  this->LastDisplayPosition[0] = this->DisplayPosition[0];
  this->LastDisplayPosition[1] = this->DisplayPosition[1];
}

//----------------------------------------------------------------------------
void vtkTool::StopAction()
{
}

//----------------------------------------------------------------------------
void vtkTool::DoAction()
{
  this->LastDisplayPosition[0] = this->DisplayPosition[0];
  this->LastDisplayPosition[1] = this->DisplayPosition[1];
  this->ToolCursor->GetDisplayPosition(this->DisplayPosition);
}

//----------------------------------------------------------------------------
void vtkTool::ConstrainCursor(double *, double *)
{
}

//----------------------------------------------------------------------------
void vtkTool::WorldToDisplay(const double world[3],
                                            double &x, double &y, double &z)
{
  vtkRenderer *renderer = this->ToolCursor->GetRenderer();
  double hcoord[4];
  hcoord[0] = world[0];
  hcoord[1] = world[1];
  hcoord[2] = world[2];
  hcoord[3] = 1.0;

  // Use the horrendous viewport interterface for conversions.
  renderer->SetWorldPoint(hcoord);
  renderer->WorldToDisplay();
  renderer->GetDisplayPoint(hcoord);
  x = hcoord[0];
  y = hcoord[1];
  z = hcoord[2];
}

//----------------------------------------------------------------------------
void vtkTool::DisplayToWorld(double x, double y, double z,
                                            double world[3])
{
  vtkRenderer *renderer = this->ToolCursor->GetRenderer();
  double hcoord[4];

  // Use the viewport interterface for conversions.
  renderer->SetDisplayPoint(x, y, z);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(hcoord);
  world[0] = hcoord[0]/hcoord[3];
  world[1] = hcoord[1]/hcoord[3];
  world[2] = hcoord[2]/hcoord[3];
}

//----------------------------------------------------------------------------
void vtkTool::GetViewRay(double x, double y, double z,
                                        double p[3], double v[3])
{
  double p1[3], p2[3];
  this->DisplayToWorld(x, y, 0.0, p1);
  this->DisplayToWorld(x, y, z, p);
  this->DisplayToWorld(x, y, 1.0, p2);

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  vtkMath::Normalize(v);
}

