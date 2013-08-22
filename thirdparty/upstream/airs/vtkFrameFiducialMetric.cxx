#include "vtkFrameFiducialMetric.h"

#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

#include "vtkImageGaussianInterpolator.h"

vtkStandardNewMacro(vtkFrameFiducialMetric);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkFrameFiducialMetric::vtkFrameFiducialMetric()
{
  this->MetricValue = 0.0;
  this->Interpolator = vtkImageGaussianInterpolator::New();

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkFrameFiducialMetric::~vtkFrameFiducialMetric()
{
  this->Interpolator->Delete();
}

//----------------------------------------------------------------------------
void vtkFrameFiducialMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MetricValue: " << this->MetricValue << "\n";
}

//----------------------------------------------------------------------------
int vtkFrameFiducialMetric::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameFiducialMetric::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameFiducialMetric::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt0[6];
  vtkInformation *inInfo0 = inputVector[0]->GetInformationObject(0);
  inInfo0->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt0);
  inInfo0->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt0, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameFiducialMetric::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo0 = inputVector[0]->GetInformationObject(0);
  vtkInformation *inInfo1 = inputVector[1]->GetInformationObject(0);

  vtkImageData *inData0 = vtkImageData::SafeDownCast(
    inInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *inData1 = vtkPolyData::SafeDownCast(
    inInfo1->Get(vtkDataObject::DATA_OBJECT()));

  //this->Interpolator->SetBlurFactors(10.0,10.0,10.0);
  this->Interpolator->SetBlurFactors(2.0,2.0,5.0);
  this->Interpolator->Initialize(inData0);

  vtkPoints *points = inData1->GetPoints();
  vtkCellArray *lines = inData1->GetLines();

  vtkIdType location = 0;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  vtkIdType numberOfCells = lines->GetNumberOfCells();
  double totalSum = 0.0;

  // For i N-Frames
  for (vtkIdType i = 0; i < numberOfCells; i++)
    {
    lines->GetCell(location, npts, pts);
    location += npts + 1;
    double p1[3];
    double p2[3];
    points->GetPoint(pts[0], p2);

    // For j points per frame
    for (vtkIdType j = 1; j < npts; j++)
      {
      double sum = 0.0;
      p1[0] = p2[0];
      p1[1] = p2[1];
      p1[2] = p2[2];
      points->GetPoint(pts[j], p2);
      vtkIdType steps = 121;

      // For k steps per line between points
      for (vtkIdType k = 0; k < steps; k++)
        {
        double f = static_cast<double>(k)/static_cast<double>(steps - 1);
        double r = 1.0 - f;
        double p[3];
        p[0] = r*p1[0] + f*p2[0];
        p[1] = r*p1[1] + f*p2[1];
        p[2] = r*p1[2] + f*p2[2];

        double value;
        if (this->Interpolator->Interpolate(p, &value))
          {
          sum += std::log(value);
          }
        else
          {
          }
        }
      totalSum += sum;
      }
    }

  // output values
  this->MetricValue = totalSum;// / (121.0*3.0*4.0);

  return 1;
}
