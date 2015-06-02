#include "vtkFrameRegistration.h"

// VTK header files
#include "vtkTransformPolyDataFilter.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkTransform.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkImageReslice.h"
#include "vtkImageShiftScale.h"
#include "vtkCommand.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAmoebaMinimizer.h"

// Image metric header files
#include "vtkFrameFiducialMetric.h"
#include "vtkPolyData.h"
#include "vtkAbstractImageInterpolator.h"

// A helper class for the optimizer
struct vtkFrameRegistrationInfo
{
  vtkLinearTransform *Transform;
  vtkObject *Optimizer;
  vtkAlgorithm *Metric;
  vtkMatrix4x4 *InitialMatrix;

  int TransformType;
  int OptimizerType;
  int MetricType;

  double Center[3];

  int NumberOfEvaluations;
};

//----------------------------------------------------------------------------
vtkFrameRegistration* vtkFrameRegistration::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkFrameRegistration");

  if (ret)
    {
    return (vtkFrameRegistration*)ret;
    }

  // If the factory was unable to create the object, then create it here.
  return new vtkFrameRegistration;
}

//----------------------------------------------------------------------------
vtkFrameRegistration::vtkFrameRegistration()
{
  this->OptimizerType = vtkFrameRegistration::Amoeba;
  this->MetricType = vtkFrameRegistration::FrameFiducialMetric;
  this->InterpolatorType = vtkFrameRegistration::Gaussian;
  this->TransformType = vtkFrameRegistration::Rigid;
  this->InitializerType = vtkFrameRegistration::Centered;

  this->Transform = vtkTransform::New();
  this->Metric = NULL;
  this->Optimizer = NULL;
  this->Interpolator = NULL;

  this->RegistrationInfo = new vtkFrameRegistrationInfo;
  this->RegistrationInfo->Transform = NULL;
  this->RegistrationInfo->Optimizer = NULL;
  this->RegistrationInfo->Metric = NULL;
  this->RegistrationInfo->InitialMatrix = NULL;
  this->RegistrationInfo->TransformType = 0;
  this->RegistrationInfo->OptimizerType = 0;
  this->RegistrationInfo->MetricType = 0;
  this->RegistrationInfo->NumberOfEvaluations = 0;

  this->InitialTransformMatrix = vtkMatrix4x4::New();

  this->Value = 0.0;

  this->MetricTolerance = 1e-5;
  this->TransformTolerance = 1.0;
  this->MaximumNumberOfIterations = 500;

  this->PolyDataTransformFilter = vtkTransformPolyDataFilter::New();

  // we have the image inputs
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkFrameRegistration::~vtkFrameRegistration()
{
  // delete vtk objects
  if (this->Optimizer)
    {
    this->Optimizer->Delete();
    }
  if (this->Metric)
    {
    this->Metric->Delete();
    }
  if (this->Interpolator)
    {
    this->Interpolator->Delete();
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    }

  if (this->RegistrationInfo)
    {
    delete this->RegistrationInfo;
    }

  if (this->InitialTransformMatrix)
    {
    this->InitialTransformMatrix->Delete();
    }
  if (this->PolyDataTransformFilter)
    {
    this->PolyDataTransformFilter->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkFrameRegistration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OptimizerType: " << this->OptimizerType << "\n";
  os << indent << "MetricType: " << this->MetricType << "\n";
  os << indent << "InterpolatorType: " << this->InterpolatorType << "\n";
  os << indent << "TransformType: " << this->TransformType << "\n";
  os << indent << "InitializerType: " << this->InitializerType << "\n";
  os << indent << "MetricTolerance: " << this->MetricTolerance << "\n";
  os << indent << "TransformTolerance: " << this->TransformTolerance << "\n";
  os << indent << "MaximumNumberOfIterations: "
     << this->MaximumNumberOfIterations << "\n";
  os << indent << "Value: " << this->Value << "\n";
  os << indent << "NumberOfEvaluations: "
     << this->RegistrationInfo->NumberOfEvaluations << "\n";
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::GetNumberOfEvaluations()
{
  return this->RegistrationInfo->NumberOfEvaluations;
}

//----------------------------------------------------------------------------
void vtkFrameRegistration::SetTargetFrame(vtkPolyData *input)
{
  // Ask the superclass to connect the input.
  this->SetNthInputConnection(1, 0, (input ? input->GetProducerPort() : 0));
}

//----------------------------------------------------------------------------
vtkPolyData* vtkFrameRegistration::GetTargetFrame()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return NULL;
    }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
void vtkFrameRegistration::SetSourceImage(vtkImageData *input)
{
  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, 0, (input ? input->GetProducerPort() : 0));
}

//----------------------------------------------------------------------------
vtkImageData* vtkFrameRegistration::GetSourceImage()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//--------------------------------------------------------------------------
namespace {

void vtkTransformVersorRotation(
  vtkTransform *transform, double rx, double ry, double rz)
{
  // compute quaternion from rotation parameters
  double qs = rx*rx + ry*ry + rz*rz;
  double qc = 1.0 - qs;
  while (qc < 0)
    {
    // for rotations past 180 degrees, wrap around
    qs -= 1.0;
    qc = 1.0 - qs;
    rx = -rx;
    ry = -ry;
    rz = -rz;
    }
  qs = sqrt(qs);
  qc = sqrt(qc);
  double theta = atan2(qs,qc)*180/vtkMath::DoublePi();
  if (qs > 0)
    {
    rx /= qs;
    ry /= qs;
    rz /= qs;
    }

  if (qs > 0)
    {
    transform->RotateWXYZ(theta, rx, ry, rz);
    }
}

void vtkSetTransformParameters(vtkFrameRegistrationInfo *registrationInfo)
{
  vtkAmoebaMinimizer* optimizer =
    vtkAmoebaMinimizer::SafeDownCast(registrationInfo->Optimizer);
  vtkTransform* transform =
    vtkTransform::SafeDownCast(registrationInfo->Transform);
  vtkMatrix4x4 *initialMatrix = registrationInfo->InitialMatrix;
  int transformType = registrationInfo->TransformType;

  double tx = optimizer->GetParameterValue(0);
  double ty = optimizer->GetParameterValue(1);
  double tz = optimizer->GetParameterValue(2);

  double rx = 0.0;
  double ry = 0.0;
  double rz = 0.0;

  if (transformType > vtkFrameRegistration::Translation)
    {
    rx = optimizer->GetParameterValue(3);
    ry = optimizer->GetParameterValue(4);
    rz = optimizer->GetParameterValue(5);
    }

  double sx = 1.0;
  double sy = 1.0;
  double sz = 1.0;

  if (transformType > vtkFrameRegistration::Rigid)
    {
    sx = optimizer->GetParameterValue(6);
    sy = sx;
    sz = sx;
    }

  if (transformType > vtkFrameRegistration::Similarity)
    {
    sx = sz*optimizer->GetParameterValue(7);
    sy = sz*optimizer->GetParameterValue(8);
    }

  bool scaledAtSource =
    (transformType == vtkFrameRegistration::ScaleSourceAxes);

  double qx = 0.0;
  double qy = 0.0;
  double qz = 0.0;

  if (transformType >= vtkFrameRegistration::Affine)
    {
    qx = optimizer->GetParameterValue(9);
    qy = optimizer->GetParameterValue(10);
    qz = optimizer->GetParameterValue(11);
    }

  double *center = registrationInfo->Center;

  transform->Identity();
  transform->PostMultiply();
  transform->Translate(-center[0], -center[1], -center[2]);
  if (scaledAtSource)
    {
    vtkTransformVersorRotation(transform, -qx, -qy, -qz);
    transform->Scale(sx, sy, sz);
    vtkTransformVersorRotation(transform, qx, qy, qz);
    transform->Concatenate(initialMatrix);
    vtkTransformVersorRotation(transform, rx, ry, rz);
    }
  else
    {
    vtkTransformVersorRotation(transform, rx, ry, rz);
    transform->Concatenate(initialMatrix);
    vtkTransformVersorRotation(transform, -qx, -qy, -qz);
    transform->Scale(sx, sy, sz);
    vtkTransformVersorRotation(transform, qx, qy, qz);
    }
  transform->Translate(center[0], center[1], center[2]);
  transform->Translate(tx,ty,tz);

  //std::cout << "**IN TRANSFORM PARAMETERS**" << std::endl;
  //transform->Print(std::cout);
}

//--------------------------------------------------------------------------
void vtkEvaluateFunction(void * arg)
{
  vtkFrameRegistrationInfo *registrationInfo =
    static_cast<vtkFrameRegistrationInfo*>(arg);

  double val = 0.0;

  vtkAmoebaMinimizer* optimizer =
    vtkAmoebaMinimizer::SafeDownCast(registrationInfo->Optimizer);
  vtkFrameFiducialMetric *ffMetric =
    vtkFrameFiducialMetric::SafeDownCast(registrationInfo->Metric);

  vtkSetTransformParameters(registrationInfo);

  registrationInfo->Metric->Update();

  val = - ffMetric->GetMetricValue();

  optimizer->SetFunctionValue(val);

  registrationInfo->NumberOfEvaluations++;
}

} // end anonymous namespace

//--------------------------------------------------------------------------
void vtkFrameRegistration::ComputeImageRange(
  vtkImageData *data, double range[2])
{
  data->GetScalarRange(range);

  //range[0] = 0.0;
  if (range[0] >= range[1])
    {
    range[1] = range[0] + 1.0;
    }
}

//--------------------------------------------------------------------------
void vtkFrameRegistration::Initialize(vtkMatrix4x4 *matrix)
{
  // update our inputs
  this->Update();

  vtkPolyData *targetFrame = this->GetTargetFrame();
  vtkImageData *sourceImage = this->GetSourceImage();

  if (targetFrame == NULL || sourceImage == NULL)
    {
    vtkErrorMacro("Initialize: Input image/frame are not set");
    return;
    }

  // get the source image center
  double bounds[6];
  double center[3];
  double size[3];
  sourceImage->GetBounds(bounds);
  center[0] = 0.5*(bounds[0] + bounds[1]);
  center[1] = 0.5*(bounds[2] + bounds[3]);
  center[2] = 0.5*(bounds[4] + bounds[5]);
  size[0] = (bounds[1] - bounds[0]);
  size[1] = (bounds[3] - bounds[2]);
  size[2] = (bounds[5] - bounds[4]);

  vtkTransform *transform =
    vtkTransform::SafeDownCast(this->Transform);
  vtkMatrix4x4 *initialMatrix = this->InitialTransformMatrix;

  // create an initial transform
  initialMatrix->Identity();
  transform->Identity();

  // the initial translation
  double tx = 0.0;
  double ty = 0.0;
  double tz = 0.0;

  // initialize from the supplied matrix
  if (matrix)
    {
    // move the translation into tx, ty, tz variables
    tx = matrix->Element[0][3];
    ty = matrix->Element[1][3];
    tz = matrix->Element[2][3];

    // move rotation/scale/shear into the InitialTransformMatrix
    initialMatrix->DeepCopy(matrix);
    initialMatrix->Element[0][3] = 0.0;
    initialMatrix->Element[1][3] = 0.0;
    initialMatrix->Element[2][3] = 0.0;

    // adjust the translation for the transform centering
    double scenter[4];
    scenter[0] = center[0];
    scenter[1] = center[1];
    scenter[2] = center[2];
    scenter[3] = 1.0;

    initialMatrix->MultiplyPoint(scenter, scenter);

    tx -= center[0] - scenter[0];
    ty -= center[1] - scenter[1];
    tz -= center[2] - scenter[2];
    }

  if (this->InitializerType == vtkFrameRegistration::Centered)
    {
    // set an initial translation from one image center to the other image center
    double tbounds[6];
    double tcenter[3];
    targetFrame->GetBounds(tbounds);
    tcenter[0] = 0.5*(tbounds[0] + tbounds[1]);
    tcenter[1] = 0.5*(tbounds[2] + tbounds[3]);
    tcenter[2] = 0.5*(tbounds[4] + tbounds[5]);

    tx = tcenter[0] - center[0];
    ty = tcenter[1] - center[1];
    tz = tcenter[2] - center[2];
    }

  switch (this->InterpolatorType)
    {
    case vtkFrameRegistration::Nearest:
      break;
    case vtkFrameRegistration::Linear:
      break;
    case vtkFrameRegistration::Cubic:
      break;
    case vtkFrameRegistration::Gaussian:
      break;
    }

  if (this->Metric)
    {
    this->Metric->RemoveAllInputs();
    this->Metric->Delete();
    }

  this->PolyDataTransformFilter->SetInput(targetFrame);
  //this->PolyDataTransformFilter->SetTransform(this->Transform);
  this->PolyDataTransformFilter->SetTransform(this->Transform->GetInverse());

  this->Metric = vtkFrameFiducialMetric::New();
  this->Metric->SetInputConnection(0, sourceImage->GetProducerPort());
  this->Metric->SetInputConnection(1, this->PolyDataTransformFilter->GetOutputPort());

  if (this->Optimizer != NULL)
    {
    this->Optimizer->Delete();
    }

  vtkAmoebaMinimizer *optimizer = vtkAmoebaMinimizer::New();
  this->Optimizer = optimizer;
  optimizer->SetTolerance(this->MetricTolerance);
  optimizer->SetParameterTolerance(this->TransformTolerance);
  optimizer->SetMaxIterations(this->MaximumNumberOfIterations);

  this->RegistrationInfo->Transform = this->Transform;
  this->RegistrationInfo->Optimizer = this->Optimizer;
  this->RegistrationInfo->Metric = this->Metric;
  this->RegistrationInfo->InitialMatrix = this->InitialTransformMatrix;

  this->RegistrationInfo->TransformType = this->TransformType;
  this->RegistrationInfo->OptimizerType = this->OptimizerType;
  this->RegistrationInfo->MetricType = this->MetricType;

  this->RegistrationInfo->NumberOfEvaluations = 0;

  this->RegistrationInfo->Center[0] = center[0];
  this->RegistrationInfo->Center[1] = center[1];
  this->RegistrationInfo->Center[2] = center[2];

  // use golden ratio for amoeba
  optimizer->SetExpansionRatio(1.618);
  optimizer->SetContractionRatio(0.618);
  optimizer->SetFunction(&vtkEvaluateFunction,
                         (void*)(this->RegistrationInfo));

  // compute minimum spacing of source image
  double spacing[3];
  sourceImage->GetSpacing(spacing);
  double minspacing = spacing[0];
  minspacing = ((minspacing < spacing[1]) ? minspacing : spacing[1]);
  minspacing = ((minspacing < spacing[2]) ? minspacing : spacing[2]);

  // compute a radius of gyration
  double r = 1.0;
  int dims = 0;
  for (int ii = 0; ii < 3; ii++)
    {
    if (size[ii] > 1e-6)
      {
      r *= size[ii];
      dims++;
      }
    }
  r = 0.41*pow(r, 1.0/dims);

  // compute parameter scales
  double tscale = this->TransformTolerance*10;
  tscale = ((tscale >= minspacing) ? tscale : minspacing);
  double rscale = tscale/r;
  double sscale = tscale/r;
  if (rscale > 0.5)
    {
    rscale = 0.5;
    }
  if (sscale > 0.1)
    {
    sscale = 0.1;
    }

  double tarray[3], rarray[3];

  tarray[0] = tx;
  tarray[1] = ty;
  tarray[2] = tz;

  rarray[0] = 0.0;
  rarray[1] = 0.0;
  rarray[2] = 0.0;

  this->ConfigureOptimizer(tarray, rarray, tscale, rscale);

  if (this->TransformType == vtkFrameRegistration::Affine)
    {
    // extra rotation parameters, scaled at 25%
    optimizer->SetParameterValue(9, 0);
    optimizer->SetParameterScale(9, rscale*0.25);
    optimizer->SetParameterValue(10, 0);
    optimizer->SetParameterScale(10, rscale*0.25);
    optimizer->SetParameterValue(11, 0);
    optimizer->SetParameterScale(11, rscale*0.25);
    }

  // build the initial transform from the parameters
  vtkSetTransformParameters(this->RegistrationInfo);

  this->Modified();
}

//--------------------------------------------------------------------------
int vtkFrameRegistration::ExecuteRegistration()
{
  int converged = 0;
  // reset Abort flag
  this->AbortExecute = 0;
  this->Progress = 0.0;

  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  vtkAmoebaMinimizer *optimizer =
    vtkAmoebaMinimizer::SafeDownCast(this->Optimizer);

  int count = 3;
  for (int icount = 0; icount < count; icount++)
    {
    converged = 0;

    double t[3], r[3];
    t[0] = optimizer->GetParameterValue(0);
    t[1] = optimizer->GetParameterValue(1);
    t[2] = optimizer->GetParameterValue(2);

    r[0] = optimizer->GetParameterValue(3);
    r[1] = optimizer->GetParameterValue(4);
    r[2] = optimizer->GetParameterValue(5);

    double tscale = 1.0;

    switch (icount)
      {
      case 0: tscale = 20.0;
        break;
      case 1: tscale = 10.0;
        break;
      case 2: tscale = 5.0;
        break;
      default: tscale = 1.0;
        break;
      }

    double rscale = 0.1;

    this->ConfigureOptimizer(t, r, tscale, rscale);

    if (optimizer)
      {
      int n = this->MaximumNumberOfIterations;
      if (n <= 0)
        {
        n = VTK_INT_MAX;
        }
      for (int i = 0; i < n && !converged; i++)
        {
        this->UpdateProgress(i*1.0/n);
        if (this->AbortExecute)
          {
          break;
          }
        converged = !optimizer->Iterate();
        vtkSetTransformParameters(this->RegistrationInfo);
        this->Value = optimizer->GetFunctionValue();
        }

      if (converged && !this->AbortExecute)
        {
        this->UpdateProgress(1.0);
        std::cout << "Iterations: " << optimizer->GetIterations() << std::endl;
        std::cout << "icount: " << icount << std::endl;
        }
      }
    }

  this->ExecuteTime.Modified();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  return converged;
}

//--------------------------------------------------------------------------
int vtkFrameRegistration::Iterate()
{
  vtkAmoebaMinimizer *optimizer =
    vtkAmoebaMinimizer::SafeDownCast(this->Optimizer);

  if (optimizer)
    {
    int result = optimizer->Iterate();
    if (optimizer->GetIterations() >= this->MaximumNumberOfIterations)
      {
      result = 0;
      }
    vtkSetTransformParameters(this->RegistrationInfo);
    this->Value = optimizer->GetFunctionValue();
    return result;
    }

  return 0;
}

//--------------------------------------------------------------------------
int vtkFrameRegistration::UpdateRegistration()
{
  this->Update();
  return this->ExecuteRegistration();
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::FillInputPortInformation(int port,
                                                   vtkInformation* info)
{
  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt[6];

  // source image
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // target image
  inInfo = inputVector[1]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}
//----------------------------------------------------------------------------
int vtkFrameRegistration::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkFrameRegistration::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // generate the data oject
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return 1;
    }
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // propagate update extent
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void vtkFrameRegistration::ConfigureOptimizer(const double t[3], const double r[3],
                                              double tscale, double rscale)
{
  vtkAmoebaMinimizer *optimizer = vtkAmoebaMinimizer::SafeDownCast(this->Optimizer);

  if (!optimizer)
    {
    optimizer = vtkAmoebaMinimizer::New();
    }

  optimizer->Initialize();

  // translation parameters
  optimizer->SetParameterValue(0, t[0]);
  optimizer->SetParameterScale(0, tscale);
  optimizer->SetParameterValue(1, t[1]);
  optimizer->SetParameterScale(1, tscale);
  optimizer->SetParameterValue(2, t[2]);
  optimizer->SetParameterScale(2, tscale);

  // rotation parameters
  if (this->TransformType > vtkFrameRegistration::Translation)
    {
    optimizer->SetParameterValue(3, r[0]);
    optimizer->SetParameterScale(3, rscale);
    optimizer->SetParameterValue(4, r[1]);
    optimizer->SetParameterScale(4, rscale);
    optimizer->SetParameterValue(5, r[2]);
    optimizer->SetParameterScale(5, rscale);
    }
}
