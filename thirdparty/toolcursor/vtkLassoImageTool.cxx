/*=========================================================================

  Program:   ToolCursor
  Module:    vtkLassoImageTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLassoImageTool.h"
#include "vtkObjectFactory.h"

#include "vtkFollowerPlane.h"
#include "vtkROIContourData.h"
#include "vtkROIContourDataToPolyData.h"
#include "vtkGeometricCursorShapes.h"
#include "vtkToolCursor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkPlane.h"
#include "vtkMath.h"
#include "vtkGlyph3D.h"
#include "vtkCellLocator.h"
#include "vtkImageData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkImageMapper3D.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkImageSlice.h"

#include "vtkVolumePicker.h"

vtkStandardNewMacro(vtkLassoImageTool);

//----------------------------------------------------------------------------
vtkLassoImageTool::vtkLassoImageTool()
{
  this->ROIMatrix = 0;
  this->ROISelectionPlane = vtkFollowerPlane::New();
  this->ROISelectionPlane->InvertFollowMatrixOn();

  vtkMatrixToLinearTransform *matrixTransform =
    vtkMatrixToLinearTransform::New();
  this->OffsetTransform = vtkTransform::New();
  this->OffsetTransform->SetInput(matrixTransform);
  this->OffsetTransform->PostMultiply();
  matrixTransform->Delete();

  vtkGeometricCursorShapes *shapes = vtkGeometricCursorShapes::New();

  this->ROIData = vtkROIContourData::New();

  this->ROIDataToPointSet = vtkROIContourDataToPolyData::New();
  this->ROIDataToPointSet->SetInput(this->ROIData);
  this->ROIDataToPointSet->SetSelectionPlane(this->ROISelectionPlane);
  this->ROIDataToPolyData = vtkROIContourDataToPolyData::New();
  this->ROIDataToPolyData->SetInput(this->ROIData);
  this->ROIDataToPolyData->SetSelectionPlane(this->ROISelectionPlane);
  this->ROIDataToPolyData->SubdivisionOn();

  this->Glyph3D = vtkGlyph3D::New();
  this->Glyph3D->SetColorModeToColorByScalar();
  this->Glyph3D->SetScaleModeToDataScalingOff();
  this->Glyph3D->SetScaleFactor(0.2);
  this->Glyph3D->SetInputConnection(this->ROIDataToPointSet->GetOutputPort());
  this->Glyph3D->SetSource(vtkPolyData::SafeDownCast(
    shapes->GetShapeData("Sphere")));

  this->GlyphMapper = vtkDataSetMapper::New();
  this->GlyphMapper->SetInputConnection(this->Glyph3D->GetOutputPort());

  this->GlyphActor = vtkActor::New();
  this->GlyphActor->PickableOff();
  this->GlyphActor->SetMapper(this->GlyphMapper);
  this->GlyphActor->GetProperty()->SetColor(1,0,0);
  this->GlyphActor->GetProperty()->LightingOff();
  this->GlyphActor->SetUserTransform(this->OffsetTransform);

  this->ContourMapper = vtkDataSetMapper::New();
  this->ContourMapper->SetInputConnection(this->ROIDataToPolyData->GetOutputPort());

  this->ContourActor = vtkActor::New();
  this->ContourActor->PickableOff();
  this->ContourActor->SetMapper(this->ContourMapper);
  this->ContourActor->GetProperty()->SetColor(1,0,0);
  this->ContourActor->GetProperty()->LightingOff();
  //this->ContourActor->GetProperty()->SetLineStipplePattern(0xe0e0e0e0);
  this->ContourActor->GetProperty()->SetLineStipplePattern(0xfcfcfcfc);
  this->ContourActor->SetUserTransform(this->OffsetTransform);

  this->CellLocator = vtkCellLocator::New();

  shapes->Delete();

  this->CurrentPointId = -1;
  this->CurrentContourId = -1;
  this->InitialPointPosition[0] = 0;
  this->InitialPointPosition[1] = 1;
  this->InitialPointPosition[2] = 2;
}

//----------------------------------------------------------------------------
vtkLassoImageTool::~vtkLassoImageTool()
{
  if (this->ROIData)
    {
    this->ROIData->Delete();
    }
  if (this->ROIMatrix)
    {
    this->ROIMatrix->Delete();
    }

  this->OffsetTransform->Delete();
  this->CellLocator->Delete();
  this->ROISelectionPlane->Delete();
  this->ROIDataToPointSet->Delete();
  this->ROIDataToPolyData->Delete();
  this->Glyph3D->Delete();
  this->GlyphActor->Delete();
  this->GlyphMapper->Delete();
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::SetROIMatrix(vtkMatrix4x4 *matrix)
{
  if (matrix != this->ROIMatrix)
    {
    if (this->ROIMatrix)
      {
      this->ROIMatrix->Delete();
      }
    this->ROIMatrix = matrix;
    if (this->ROIMatrix)
      {
      this->ROIMatrix->Register(this);
      }
    this->ROISelectionPlane->SetFollowMatrix(this->ROIMatrix);
    static_cast<vtkMatrixToLinearTransform *>(
      this->OffsetTransform->GetInput())->SetInput(this->ROIMatrix);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::SetROIContourData(vtkROIContourData *data)
{
  if (data != this->ROIData)
    {
    if (this->ROIData)
      {
      this->ROIData->Delete();
      }
    this->ROIData = data;
    if (this->ROIData)
      {
      this->ROIData->Register(this);
      }
    this->ROIDataToPointSet->SetInput(this->ROIData);
    this->ROIDataToPolyData->SetInput(this->ROIData);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::SetMarker(vtkPolyData *data)
{
  if (data != this->Glyph3D->GetSource())
    {
    this->Glyph3D->SetSource(data);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkPolyData *vtkLassoImageTool::GetMarker()
{
  return this->Glyph3D->GetSource();
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::StartAction()
{
  this->Superclass::StartAction();

  vtkToolCursor *cursor = this->GetToolCursor();

  // Get position, convert to ROI data coords
  double position[4];
  cursor->GetPosition(position);
  position[3] = 1.0;
  if (this->ROIMatrix)
    {
    double invmatrix[16];
    vtkMatrix4x4::Invert(*this->ROIMatrix->Element, invmatrix);
    vtkMatrix4x4::MultiplyPoint(invmatrix, position, position);
    }
  this->ROISelectionPlane->ProjectPoint(position, position);

  double sliceTol = 0.5;
  vtkImageMapper3D *imageMapper = this->CurrentImageMapper;
  if (imageMapper)
    {
    this->ROISelectionPlane->SetFollowPlane(imageMapper->GetSlicePlane());
    vtkImageData *data = imageMapper->GetInput();
    if (data)
      {
      double spacing[3];
      data->GetSpacing(spacing);
      // Assume contours are always drawn in the original scan orientation
      if (0.5*spacing[2] < sliceTol)
        {
        sliceTol = 0.5*spacing[2];
        }
      }
    this->ROIDataToPointSet->SetSelectionPlaneTolerance(sliceTol);
    this->ROIDataToPolyData->SetSelectionPlaneTolerance(sliceTol);
    }

  // Tolerance for point selection
  double tol = 3;

  // Get the spline data and create a locator
  this->ROIDataToPolyData->Update();
  vtkPolyData *contourData =
    vtkPolyData::SafeDownCast(this->ROIDataToPolyData->GetOutput());
  vtkCellLocator *cellLocator = 0;
  if (contourData->GetNumberOfCells() > 0)
    {
    cellLocator = this->CellLocator;
    cellLocator->SetDataSet(contourData);
    cellLocator->BuildLocator();
    }

  // Get the contour data
  vtkROIContourData *data = this->ROIData;
  int numContours = data->GetNumberOfContours();
  this->CurrentPointId = -1;
  this->CurrentContourId = -1;
  double tol2 = tol*tol;
  for (int ic = 0; ic < numContours; ic++)
    {
    vtkPoints *points = data->GetContourPoints(ic);
    vtkIdType n = points->GetNumberOfPoints();

    // Check if the contour is in the current slice
    if (n > 0)
      {
      double p[3];
      points->GetPoint(0, p);
      double d = fabs(this->ROISelectionPlane->EvaluateFunction(p));
      if (d >= sliceTol)
        {
        continue;
        }
      }

    // Check if mouse is over a point
    for (vtkIdType i = 0; i < n; i++)
      {
      double p[3];
      points->GetPoint(i, p);
      double d2 = vtkMath::Distance2BetweenPoints(position, p);
      if (d2 <= tol2)
        {
        tol2 = d2;
        this->CurrentPointId = i;
        this->CurrentContourId = ic;
        this->InitialPointPosition[0] = p[0];
        this->InitialPointPosition[1] = p[1];
        this->InitialPointPosition[2] = p[2];
        break;
        }
      }
    }

  if (this->CurrentPointId == 0 &&
      data->GetContourPoints(this->CurrentContourId)->GetNumberOfPoints() > 2)
    {
    // Clicked on first point: close the contour
    this->ROIData->SetContourType(
      this->CurrentContourId, vtkROIContourData::CLOSED_PLANAR);
    }
  else if (this->CurrentPointId < 0)
    {
    vtkIdType cellId;
    int subId;
    double d2;
    double p[3];
    if (cellLocator &&
        (cellLocator->FindClosestPointWithinRadius(
         position, tol, p, cellId, subId, d2)))
      {
      vtkIntArray *contourIds = vtkIntArray::SafeDownCast(
        contourData->GetCellData()->GetArray("Labels"));
      vtkIntArray *contourSubIds = vtkIntArray::SafeDownCast(
        contourData->GetPointData()->GetArray("SubIds"));

      // Get the cell for this contour
      vtkIdType npts, *pts;
      contourData->GetCellType(cellId);
      contourData->GetCellPoints(cellId, npts, pts);

      // Get the pointId for insertion, and get the contourId
      subId = contourSubIds->GetValue(pts[subId + 1]);
      int contourId = contourIds->GetValue(cellId);
      vtkPoints *points = data->GetContourPoints(contourId);

      // Insert the point at the correct position
      int m = static_cast<int>(points->InsertNextPoint(p));
      for (int i = m; i > subId + 1; --i)
        {
        double ptmp[3];
        points->GetPoint(i-1, ptmp);
        points->SetPoint(i, ptmp);
        }
      points->SetPoint(subId + 1, p);

      this->CurrentContourId = contourId;
      this->CurrentPointId = subId + 1;
      this->InitialPointPosition[0] = p[0];
      this->InitialPointPosition[1] = p[1];
      this->InitialPointPosition[2] = p[2];
      }

    if (this->CurrentContourId < 0)
      {
      // Check if there is an open contour
      for (int i = 0; i < numContours; i++)
        {
        vtkPoints *points = data->GetContourPoints(i);
        vtkIdType n = points->GetNumberOfPoints();

        // Check if the contour is in the current slice
        if (n > 0)
          {
          double q[3];
          points->GetPoint(0, q);
          double d = fabs(this->ROISelectionPlane->EvaluateFunction(q));
          if (d >= sliceTol)
            {
            continue;
            }
          }

        if (data->GetContourType(i) == vtkROIContourData::OPEN_PLANAR)
          {
          this->CurrentContourId = i;
          }
        }
      }

    if (this->CurrentContourId < 0)
      {
      // Create a new contour
      vtkPoints *points = vtkPoints::New(VTK_DOUBLE);
      points->InsertNextPoint(position);
      data->SetNumberOfContours(numContours+1);
      data->SetContourPoints(numContours, points);
      data->SetContourType(numContours, vtkROIContourData::OPEN_PLANAR);
      }
    else if (this->CurrentPointId < 0)
      {
      // Add a new point to the currently unclosed contour
      vtkPoints *points = data->GetContourPoints(this->CurrentContourId);
      this->CurrentPointId = points->InsertNextPoint(position);
      this->InitialPointPosition[0] = position[0];
      this->InitialPointPosition[1] = position[1];
      this->InitialPointPosition[2] = position[2];
      }

    this->ROIData->Modified();
    }

  // Generate an offset to avoid Z buffer problems
  this->OffsetTransform->Identity();
  this->OffsetTransform->Translate(this->ROISelectionPlane->GetNormal());
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::AddViewPropsToRenderer(vtkRenderer *renderer)
{
  // Search the renderer to find the currently active image
  vtkPropCollection *props = renderer->GetViewProps();
  vtkProp *prop = 0;
  vtkAssemblyPath *path;
  vtkImageMapper3D *mapper = 0;
  vtkCollectionSimpleIterator pit;

  for (props->InitTraversal(pit); (prop = props->GetNextProp(pit)); )
    {
    for (prop->InitPathTraversal(); (path = prop->GetNextPath()); )
      {
      vtkProp *tryProp = path->GetLastNode()->GetViewProp();
      if (tryProp->GetVisibility())
        {
        vtkImageSlice *imageProp = 0;

        if ((imageProp = vtkImageSlice::SafeDownCast(tryProp)) != 0)
          {
          mapper = imageProp->GetMapper();
          }
        }
      }
    }

  double sliceTol = 0.5;

  if (mapper)
    {
    this->ROISelectionPlane->SetFollowPlane(mapper->GetSlicePlane());
    vtkImageData *data = mapper->GetInput();
    if (data)
      {
      double spacing[3];
      data->GetSpacing(spacing);
      // Assume contours are always drawn in the original scan orientation
      if (0.5*spacing[2] < sliceTol)
        {
        sliceTol = 0.5*spacing[2];
        }
      }
    }
  else
    {
    vtkCamera *camera = renderer->GetActiveCamera();

    vtkPlane *plane = vtkPlane::New();
    plane->SetOrigin(camera->GetFocalPoint());
    double normal[3];
    camera->GetDirectionOfProjection(normal);
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
    plane->SetNormal(normal);
    this->ROISelectionPlane->SetFollowPlane(plane);
    plane->Delete();
    }

  this->ROIDataToPointSet->SetSelectionPlaneTolerance(sliceTol);
  this->ROIDataToPolyData->SetSelectionPlaneTolerance(sliceTol);

  // Generate an offset to avoid Z-buffer issues
  this->OffsetTransform->Identity();
  this->OffsetTransform->Translate(this->ROISelectionPlane->GetNormal());

  renderer->AddViewProp(this->GlyphActor);
  renderer->AddViewProp(this->ContourActor);
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::RemoveViewPropsFromRenderer(vtkRenderer *renderer)
{
  renderer->RemoveViewProp(this->GlyphActor);
  renderer->RemoveViewProp(this->ContourActor);
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::StopAction()
{
  this->Superclass::StopAction();
}

//----------------------------------------------------------------------------
void vtkLassoImageTool::DoAction()
{
  this->Superclass::DoAction();

  vtkToolCursor *cursor = this->GetToolCursor();

  // Get the initial point.
  double p0[4];
  this->GetStartPosition(p0);
  p0[3] = 1.0;

  // Get the current position
  double position[4];
  cursor->GetPosition(position);
  position[3] = 1.0;

  // Convert positions from world coords to data coords
  if (this->ROIMatrix)
    {
    double invmatrix[16];
    vtkMatrix4x4::Invert(*this->ROIMatrix->Element, invmatrix);
    vtkMatrix4x4::MultiplyPoint(invmatrix, p0, p0);
    vtkMatrix4x4::MultiplyPoint(invmatrix, position, position);
    }
  this->ROISelectionPlane->ProjectPoint(p0, p0);
  this->ROISelectionPlane->ProjectPoint(position, position);

  double dx = position[0] - p0[0];
  double dy = position[1] - p0[1];
  double dz = position[2] - p0[2];

  vtkPoints *points = 0;
  if (this->CurrentContourId >= 0)
    {
    points = this->ROIData->GetContourPoints(this->CurrentContourId);
    }

  if (points && this->CurrentPointId >= 0)
    {
    double p[3];
    p[0] = this->InitialPointPosition[0] + dx;
    p[1] = this->InitialPointPosition[1] + dy;
    p[2] = this->InitialPointPosition[2] + dz;
    points->SetPoint(this->CurrentPointId, p);

    this->ROIData->Modified();
    }
}
