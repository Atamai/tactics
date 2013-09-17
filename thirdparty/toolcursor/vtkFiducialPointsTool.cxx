/*=========================================================================

  Program:   ToolCursor
  Module:    vtkFiducialPointsTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFiducialPointsTool.h"
#include "vtkObjectFactory.h"

#include "vtkCommand.h"
#include "vtkGeometricCursorShapes.h"
#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkProperty.h"
#include "vtkActorCollection.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkFiducialPointsTool);

//----------------------------------------------------------------------------
vtkFiducialPointsTool::vtkFiducialPointsTool()
{
  this->Transform = vtkTransform::New();
  this->PointSet = vtkPolyData::New();

  vtkGeometricCursorShapes *shapes = vtkGeometricCursorShapes::New();

  this->Glyph3D = vtkGlyph3D::New();
  this->Glyph3D->SetColorModeToColorByScalar();
  this->Glyph3D->SetScaleFactor(0.7);
  this->Glyph3D->SetInput(this->PointSet);
  this->Glyph3D->SetSource(vtkPolyData::SafeDownCast(
    shapes->GetShapeData("Sphere")));

  this->Mapper = vtkDataSetMapper::New();
  this->Mapper->SetInputConnection(this->Glyph3D->GetOutputPort());

  this->Actor = vtkActor::New();
  this->Actor->PickableOff();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->GetProperty()->SetColor(1,0,0);

  shapes->Delete();
}

//----------------------------------------------------------------------------
vtkFiducialPointsTool::~vtkFiducialPointsTool()
{
  this->Transform->Delete();
  this->PointSet->Delete();
  this->Glyph3D->Delete();
  this->Actor->Delete();
  this->Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::SetPoints(vtkPoints *points)
{
  if (1) //points != this->PointSet->GetPoints())
    {
    this->PointSet->SetPoints(points);
    int n = points->GetNumberOfPoints();
    vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
    colors->SetNumberOfComponents(3);
    unsigned char red[3] = { 255, 0, 0 };
    unsigned char yellow[3] = { 255, 255, 0 };
    for (int i = 0; i < n; i++)
      {
      unsigned char *color = red;
      if (i == n-1) { color = yellow; }
      colors->InsertNextTupleValue(color);
      }
    //this->PointSet->GetPointData()->SetScalars(colors);
    this->Glyph3D->Modified();
    colors->Delete();

    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkPoints *vtkFiducialPointsTool::GetPoints()
{
  return this->PointSet->GetPoints();
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::SetMarker(vtkPolyData *data)
{
  if (data != this->Glyph3D->GetSource())
    {
    this->Glyph3D->SetSource(data);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkPolyData *vtkFiducialPointsTool::GetMarker()
{
  return this->Glyph3D->GetSource();
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::StartAction()
{
  this->Superclass::StartAction();

  this->Transform->Identity();
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::StopAction()
{
  this->Superclass::StopAction();

  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkFiducialPointsTool::DoAction()
{
  this->Superclass::DoAction();

  // Get the initial point.
  double p0[3];
  this->GetStartPosition(p0);

  // Get the depth.
  double x, y, z;
  this->WorldToDisplay(p0, x, y, z);

  // Get the display position.
  double p[3];
  this->GetDisplayPosition(x, y);
  this->DisplayToWorld(x, y, z, p);

  // Get the vector.
  double v[3];
  v[0] = p[0] - p0[0];
  v[1] = p[1] - p0[1];
  v[2] = p[2] - p0[2];
}
