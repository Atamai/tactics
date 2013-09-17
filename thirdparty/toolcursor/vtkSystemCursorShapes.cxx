/*=========================================================================

  Program:   ToolCursor
  Module:    vtkSystemCursorShapes.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSystemCursorShapes.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkSystemCursorShapes);

//----------------------------------------------------------------------------
vtkSystemCursorShapes::vtkSystemCursorShapes()
{
  this->MakeShapes();
}

//----------------------------------------------------------------------------
vtkSystemCursorShapes::~vtkSystemCursorShapes()
{
}

//----------------------------------------------------------------------------
void vtkSystemCursorShapes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkSystemCursorShapes::MakeShapes()
{
  vtkDataSet *data;

  data = this->MakePointerShape();
  this->AddShape("Pointer", data, VTK_TOOL_RGB);
  data->Delete();

  data = this->MakeCrosshairShape();
  this->AddShape("Crosshair", data, VTK_TOOL_RGB);
  data->Delete();
}

//----------------------------------------------------------------------------
vtkDataSet *vtkSystemCursorShapes::MakePointerShape()
{
  vtkUnsignedCharArray *scalars = vtkUnsignedCharArray::New();
  scalars->SetNumberOfComponents(4);
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *strips = vtkCellArray::New();
  vtkCellArray *lines = vtkCellArray::New();

  static unsigned char black[4] = {  0,   0,   0, 255};
  static unsigned char white[4] = {255, 255, 255, 255};

  static double hotspot[2] = { 0.5, -0.5 };
  static double coords[7][2] = {
    {  1,  -1 },
    {  1, -15 },
    {  4, -12 },
    {  8, -19 },
    { 10, -18 },
    {  7, -11 },
    { 11, -11 },
  };

  static vtkIdType stripIds[] = {
    3, 0, 1, 2,
    3, 0, 2, 5,
    3, 0, 5, 6,
    4, 2, 3, 5, 4,
  };

  static vtkIdType lineIds[] = {
    8, 7, 8, 9, 10, 11, 12, 13, 7,
    8, 14, 15, 16, 17, 18, 19, 20, 14,
  };

  // Add the points three times: white, then black, and black again
  for (int i = 0; i < 7; i++)
    {
    points->InsertNextPoint(coords[i][0] - hotspot[0],
                            coords[i][1] - hotspot[1], 0.0);
    scalars->InsertNextTupleValue(white);
    }

  for (int j = 0; j < 7; j++)
    {
    points->InsertNextPoint(coords[j][0] - hotspot[0],
                            coords[j][1] - hotspot[1], +0.1);
    scalars->InsertNextTupleValue(black);
    }

  for (int k = 0; k < 7; k++)
    {
    points->InsertNextPoint(coords[k][0] - hotspot[0],
                            coords[k][1] - hotspot[1], -0.1);
    scalars->InsertNextTupleValue(black);
    }

  // Make the strips
  strips->InsertNextCell(stripIds[0], &stripIds[1]);
  strips->InsertNextCell(stripIds[4], &stripIds[5]);
  strips->InsertNextCell(stripIds[8], &stripIds[9]);
  strips->InsertNextCell(stripIds[12], &stripIds[13]);

  // Make the lines
  lines->InsertNextCell(lineIds[0], &lineIds[1]);
  lines->InsertNextCell(lineIds[8], &lineIds[9]);

  vtkPolyData *data = vtkPolyData::New();
  data->SetPoints(points);
  points->Delete();
  data->SetStrips(strips);
  strips->Delete();
  data->SetLines(lines);
  lines->Delete();
  data->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  return data;
}

//----------------------------------------------------------------------------
vtkDataSet *vtkSystemCursorShapes::MakeCrosshairShape()
{
  vtkUnsignedCharArray *scalars = vtkUnsignedCharArray::New();
  scalars->SetNumberOfComponents(4);
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *strips = vtkCellArray::New();
  vtkCellArray *lines = vtkCellArray::New();

  static unsigned char black[4] = {  0,   0,   0, 255};
  static unsigned char white[4] = {255, 255, 255, 255};

  const double radius = 8;
  const double inner = 3;

  static double coords[8][2] = {
    { 0, -radius }, { 0, -inner }, { 0, +inner }, { 0, +radius },
    { -radius, 0 }, { -inner, 0 }, { +inner, 0 }, { +radius, 0 },
  };

  static double outCoords[16][2] = {
    { -1, -radius-1 }, { +1, -radius-1 }, { +1, -inner+1 }, { -1, -inner+1 },
    { +1, +radius+1 }, { -1, +radius+1 }, { -1, +inner-1 }, { +1, +inner-1 },
    { -radius-1, +1 }, { -radius-1, -1 }, { -inner+1, -1 }, { -inner+1, +1 },
    { +radius+1, -1 }, { +radius+1, +1 }, { +inner-1, +1 }, { +inner-1, -1 },
  };

  static vtkIdType toplineIds[] = {
    2, 0, 1,
    2, 2, 3,
    2, 4, 5,
    2, 6, 7,
  };

  static vtkIdType botlineIds[] = {
    2, 8, 9,
    2, 10, 11,
    2, 12, 13,
    2, 14, 15,
  };

  static vtkIdType outlineIds[] = {
    5, 16, 17, 18, 19, 16,
    5, 20, 21, 22, 23, 20,
    5, 24, 25, 26, 27, 24,
    5, 28, 29, 30, 31, 28,
  };

  static vtkIdType stripIds[] = {
    4, 16, 17, 19, 18,
    4, 20, 21, 23, 22,
    4, 24, 25, 27, 26,
    4, 28, 29, 31, 30,
  };

  for (int i = 0; i < 8; i++)
    {
    points->InsertNextPoint(coords[i][0]+0.5, coords[i][1]-0.5, +0.1);
    scalars->InsertNextTupleValue(black);
    }

  for (int j = 0; j < 8; j++)
    {
    points->InsertNextPoint(coords[j][0]+0.5, coords[j][1]-0.5, -0.1);
    scalars->InsertNextTupleValue(black);
    }

  for (int k = 0; k < 16; k++)
    {
    points->InsertNextPoint(outCoords[k][0]+0.5, outCoords[k][1]-0.5, 0);
    scalars->InsertNextTupleValue(white);
    }

  // Make the crosshairs
  lines->InsertNextCell(toplineIds[0], &toplineIds[1]);
  lines->InsertNextCell(toplineIds[3], &toplineIds[4]);
  lines->InsertNextCell(toplineIds[6], &toplineIds[7]);
  lines->InsertNextCell(toplineIds[9], &toplineIds[10]);

  // Make the crosshairs
  lines->InsertNextCell(botlineIds[0], &botlineIds[1]);
  lines->InsertNextCell(botlineIds[3], &botlineIds[4]);
  lines->InsertNextCell(botlineIds[6], &botlineIds[7]);
  lines->InsertNextCell(botlineIds[9], &botlineIds[10]);

  // Make the outline
  lines->InsertNextCell(outlineIds[0], &outlineIds[1]);
  lines->InsertNextCell(outlineIds[6], &outlineIds[7]);
  lines->InsertNextCell(outlineIds[12], &outlineIds[13]);
  lines->InsertNextCell(outlineIds[18], &outlineIds[19]);

  // Fill the outline
  strips->InsertNextCell(stripIds[0], &stripIds[1]);
  strips->InsertNextCell(stripIds[5], &stripIds[6]);
  strips->InsertNextCell(stripIds[10], &stripIds[11]);
  strips->InsertNextCell(stripIds[15], &stripIds[16]);

  vtkPolyData *data = vtkPolyData::New();
  data->SetPoints(points);
  points->Delete();
  data->SetLines(lines);
  lines->Delete();
  data->SetStrips(strips);
  strips->Delete();
  data->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  return data;
}
