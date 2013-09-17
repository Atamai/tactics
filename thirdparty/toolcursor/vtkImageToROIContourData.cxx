/*=========================================================================

  Program:   ToolCursor
  Module:    vtkImageToROIContourData.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToROIContourData.h"

#include "vtkROIContourData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkMarchingSquaresLineCases.h"
#include "vtkTemplateAliasMacro.h"

vtkStandardNewMacro(vtkImageToROIContourData);

//----------------------------------------------------------------------------
vtkImageToROIContourData::vtkImageToROIContourData()
{
  this->Value = 0.5;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageToROIContourData::~vtkImageToROIContourData()
{
}

//----------------------------------------------------------------------------
void vtkImageToROIContourData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Value: " << this->Value << "\n";
}

//----------------------------------------------------------------------------
vtkROIContourData* vtkImageToROIContourData::GetOutput()
{
  return vtkROIContourData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkImageToROIContourData::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkImageToROIContourData::GetInput()
{
  return this->GetExecutive()->GetInputData(0, 0);
}

//----------------------------------------------------------------------------
void vtkImageToROIContourData::SetInput(vtkDataObject* input)
{
  vtkAlgorithmOutput *producerPort = 0;

  if (input)
    {
    producerPort = input->GetProducerPort();
    }

  this->SetInputConnection(0, producerPort);
}

//----------------------------------------------------------------------------
int vtkImageToROIContourData::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkROIContourData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToROIContourData::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToROIContourData::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  unsigned long* mtime)
{
  unsigned long mTime = this->GetMTime();

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToROIContourData::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // create data object
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkROIContourData *data = vtkROIContourData::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    if (!data)
      {
      data = vtkROIContourData::New();
      data->SetPipelineInformation(info);
      data->Delete();
      }
    return 1;
    }

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  // tell inputs how to update
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    int extent[6];
    info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
    return 1;
    }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// This code was taken from vtkMarchingSquares and modified so that it
// does not break the contours at the edges of the image.
namespace {
template <class T>
void vtkContourImage(
  T *scalars, vtkDataArray *newScalars, int extent[6], int dir[3],
  int start[2], int end[2], vtkIdType offset[3], double spacing[3],
  double origin[3], double *values, int numValues,
  vtkIncrementalPointLocator *p, vtkCellArray *lines)
{
  int i, j;
  vtkIdType ptIds[2];
  double t, *x1, *x2, x[3], xp, yp;
  double pts[4][3];
  int contNum, jOffset, idx, ii, jj, index, *vert;
  static int CASE_MASK[4] = {1,2,8,4};
  vtkMarchingSquaresLineCases *lineCase, *lineCases;
  static int edges[4][2] = { {0,1}, {1,3}, {2,3}, {0,2} };
  EDGE_LIST  *edge;  // EDGE_LIST is a typedef for int

  if (numValues < 1)
    {
    return;
    }

  // Get minval/maxval contour values
  double minval = values[0];
  double maxval = values[0];
  for (i = 1; i < numValues; i++)
    {
    if (values[i] < minval) { minval = values[i]; }
    if (values[i] > maxval) { maxval = values[i]; }
    }

  lineCases = vtkMarchingSquaresLineCases::GetCases();

  // assign coordinate value to non-varying coordinate direction
  x[dir[2]] = origin[dir[2]] + extent[dir[2]*2]*spacing[dir[2]];

  // Traverse pixel cells, generating line segments using marching squares.
  for (j = extent[start[1]] - 1; j <= extent[end[1]]; j++)
    {
    jOffset = j*offset[1];
    pts[0][dir[1]] = origin[dir[1]] + j*spacing[dir[1]];
    yp = origin[dir[1]] + (j+1)*spacing[dir[1]];

    for (i = extent[start[0]] - 1; i <= extent[end[0]]; i++)
      {
      // get scalar values
      idx = i*offset[0] + jOffset + offset[2];
      double s[4];
      s[0] = VTK_DOUBLE_MIN;
      s[1] = VTK_DOUBLE_MIN;
      s[2] = VTK_DOUBLE_MIN;
      s[3] = VTK_DOUBLE_MIN;
      if (i >= extent[start[0]] && j >= extent[start[1]])
        {
        s[0] = scalars[idx];
        }
      if (i < extent[end[0]] && j >= extent[start[1]])
        {
        s[1] = scalars[idx + offset[0]];
        }
      if (i >= extent[start[0]] && j < extent[end[1]])
        {
        s[2] = scalars[idx + offset[1]];
        }
      if (i < extent[end[0]] && j < extent[end[1]])
        {
        s[3] = scalars[idx + offset[0] + offset[1]];
        }

      if ((s[0] < minval && s[1] < minval &&
           s[2] < minval && s[3] < minval) ||
          (s[0] > maxval && s[1] > maxval &&
           s[2] > maxval && s[3] > maxval))
        {
        // no contours possible
        continue;
        }

      //create pixel points
      pts[0][dir[0]] = origin[dir[0]] + i*spacing[dir[0]];
      xp = origin[dir[0]] + (i+1)*spacing[dir[0]];

      pts[1][dir[0]] = xp;
      pts[1][dir[1]] = pts[0][dir[1]];

      pts[2][dir[0]] = pts[0][dir[0]];
      pts[2][dir[1]] = yp;

      pts[3][dir[0]] = xp;
      pts[3][dir[1]] = yp;

      // Loop over contours in this pixel
      for (contNum = 0; contNum < numValues; contNum++)
        {
        double value = values[contNum];

        // Build the case table
        index = 0;
        for (ii = 0; ii < 4; ii++)
          {
          if (s[ii] >= value)
            {
            index |= CASE_MASK[ii];
            }
          }
        if (index == 0 || index == 15)
          {
          continue; //no lines
          }

        lineCase = lineCases + index;
        edge = lineCase->edges;

        for (; edge[0] > -1; edge += 2)
          {
          // insert line
          for (ii = 0; ii < 2; ii++)
            {
            vert = edges[edge[ii]];
            t = (value - s[vert[0]])/(s[vert[1]] - s[vert[0]]);
            x1 = pts[vert[0]];
            x2 = pts[vert[1]];

            //only need to interpolate two values
            for (jj = 0; jj < 2; jj++)
              {
              x[dir[jj]] = x1[dir[jj]] + t * (x2[dir[jj]] - x1[dir[jj]]);
              }

            if (p->InsertUniquePoint(x, ptIds[ii]) && newScalars)
              {
              newScalars->InsertComponent(ptIds[ii], 0, value);
              }
            }

          if (ptIds[0] != ptIds[1]) //check for degenerate line
            {
            lines->InsertNextCell(2, ptIds);
            }

          }//for each line
        }//for all contours
      }//for i
    }//for j
}
} // end anonymous namespace

//----------------------------------------------------------------------------
void vtkImageToROIContourData::MarchingSquares(
  vtkImageData *input, vtkPolyData *output, int extent[6], double value)
{
  void *inPtr = input->GetScalarPointerForExtent(extent);
  double *spacing = input->GetSpacing();
  double *origin = input->GetOrigin();
  int *inExt = input->GetExtent();

  vtkIdType offset[3];
  offset[0] = input->GetNumberOfScalarComponents();
  offset[1] = offset[0]*(inExt[1] - inExt[0] + 1);
  offset[2] = -(offset[0]*extent[0] + offset[1]*extent[2]);

  double bounds[6];
  bounds[0] = extent[0]*spacing[0] + origin[0];
  bounds[1] = extent[1]*spacing[0] + origin[0];
  bounds[2] = extent[2]*spacing[1] + origin[1];
  bounds[3] = extent[3]*spacing[1] + origin[1];
  bounds[4] = extent[4]*spacing[2] + origin[2];
  bounds[5] = extent[5]*spacing[2] + origin[2];

  double values[1] = { value };
  int numValues = 1;
  int dir[3] = { 0, 1, 2 };
  int start[2] = { 0, 2 };
  int end[2] = { 1, 3 };

  vtkPoints *points = vtkPoints::New(VTK_DOUBLE);
  vtkMergePoints *locator = vtkMergePoints::New();
  locator->InitPointInsertion(points, bounds);
  vtkCellArray *lines = vtkCellArray::New();

  vtkDataArray *newScalars = NULL;

  switch (input->GetScalarType())
    {
    vtkTemplateAliasMacro(
      vtkContourImage(static_cast<VTK_TT*>(inPtr), newScalars,
                      extent, dir, start, end, offset, spacing, origin,
                      values, numValues, locator, lines);
      );
    }

  output->SetPoints(points);
  output->SetLines(lines);

  locator->Delete();
  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------------
namespace {
void vtkReducePoints(vtkPoints *contourPoints)
{
  vtkPoints *points = vtkPoints::New(VTK_DOUBLE);
  vtkIdType m = contourPoints->GetNumberOfPoints();

  if (m <= 4)
    {
    return;
    }

  // Compute the curvature at each point
  double p0[3], p1[3];
  contourPoints->GetPoint(m-1, p1);
  contourPoints->GetPoint(0, p0);

  double dx0 = (p0[0] - p1[0]);
  double dy0 = (p0[1] - p1[1]);
  double dz0 = (p0[2] - p1[2]);
  double d0 = sqrt(dx0*dx0 + dy0*dy0 + dz0*dz0);

  double dskip = d0;

  vtkIdType n = 0;
  for (vtkIdType j = 0; j < m; j++)
    {
    int jp1 = (j + 1) % m;
    contourPoints->GetPoint(jp1, p1);

    double dx1 = (p1[0] - p0[0]);
    double dy1 = (p1[1] - p0[1]);
    double dz1 = (p1[2] - p0[2]);
    double d1 = sqrt(dx1*dx1 + dy1*dy1 + dz1*dz1);

    double f = 2.0/(d0 + d1);
    double f0 = f/d0;
    double f1 = f/d1;
    double ddx = dx1*f1 - dx0*f0;
    double ddy = dy1*f1 - dy0*f0;
    double ddz = dz1*f1 - dz0*f0;

    double curvature = sqrt(ddx*ddx + ddy*ddy + ddz*ddz);

    dskip += d1;
    if (dskip*curvature > 15 || (n + m - j - 1) <= 4)
      {
      points->InsertNextPoint(p0);
      n++;
      dskip = d1;
      }

    p0[0] = p1[0];
    p0[1] = p1[1];
    p0[2] = p1[2];

    dx0 = dx1;
    d0 = d1;
    }

  contourPoints->DeepCopy(points);
  points->Delete();
}
}

//----------------------------------------------------------------------------
int vtkImageToROIContourData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkROIContourData *output = vtkROIContourData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Create filters to do the work
  vtkPolyData *sliceContours = vtkPolyData::New();

  // Go through the input slice by slice
  int extent[6];
  input->GetWholeExtent(extent);
  int zMin = extent[4];
  int zMax = extent[5];

  // The isovalue for the contour
  double value = this->Value;
  vtkIdList *cellIds = vtkIdList::New();
  cellIds->Allocate(2);

  for (int zIdx = zMin; zIdx <= zMax; zIdx++)
    {
    // Process and get output
    extent[4] = zIdx;
    extent[5] = zIdx;
    this->MarchingSquares(input, sliceContours, extent, value);
    sliceContours->BuildCells();
    sliceContours->BuildLinks();
    vtkPoints *slicePoints = sliceContours->GetPoints();
    vtkCellArray *sliceLines = sliceContours->GetLines();

    // Add contours to output
    vtkIdType numCells = sliceLines->GetNumberOfCells();
    int contourId = output->GetNumberOfContours();
    output->SetNumberOfContours(contourId + numCells);
    for (vtkIdType j = 0; j < numCells; j++)
      {
      vtkIdType currentId = j;
      vtkIdType numPts, *ptIds;
      sliceLines->GetCell(3*currentId, numPts, ptIds);
      if (ptIds[0] >= 0)
        {
        vtkPoints *points = vtkPoints::New();
        do
          {
          // Add the current point and mark it as visited
          double p[3];
          slicePoints->GetPoint(ptIds[0], p);
          points->InsertNextPoint(p);
          ptIds[0] = -1;
          // Find next line segment and continue
          sliceContours->GetPointCells(ptIds[1], cellIds);
          vtkIdType n1 = cellIds->GetId(0);
          vtkIdType n2 = cellIds->GetId(1);
          n1 = ((n2 == currentId) ? n1 : n2);
          currentId = n1;
          sliceLines->GetCell(3*currentId, numPts, ptIds);
          }
        while (ptIds[0] >= 0);

        vtkReducePoints(points);

        output->SetNumberOfContours(contourId + 1);
        output->SetContourPoints(contourId, points);
        output->SetContourType(contourId, vtkROIContourData::CLOSED_PLANAR);
        points->Delete();
        contourId++;
        }
      }
    }

  // Free temporary objects
  sliceContours->Delete();
  cellIds->Delete();

  return 1;
}
