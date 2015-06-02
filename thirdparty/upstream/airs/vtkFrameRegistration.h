// .NAME vtkFrameRegistration - Perform linear image registration.
// .SECTION Description
// This class will find the transformation that registers the source
// image to the target image.

#ifndef __vtkFrameRegistration_h
#define __vtkFrameRegistration_h

#include "vtkAlgorithm.h"

class vtkTransformPolyDataFilter;
class vtkImageData;
class vtkLinearTransform;
class vtkMatrix4x4;
class vtkImageReslice;
class vtkImageShiftScale;
class vtkAbstractImageInterpolator;
class vtkPolyData;

struct vtkFrameRegistrationInfo;

class VTK_EXPORT vtkFrameRegistration : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkFrameRegistration, vtkAlgorithm);
  static vtkFrameRegistration *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The image to use as the source image.  The source voxels define
  // the points at which the target image will be interpolated during
  // the registration.
  void SetSourceImageInputConnection(vtkAlgorithmOutput *input) {
    this->SetInputConnection(0, input); }
  vtkAlgorithmOutput *GetSourceImageInputConnection() {
    return this->GetInputConnection(0, 0); }
  void SetSourceImage(vtkImageData *input);
  vtkImageData *GetSourceImage();

  // Description:
  // The image to use as the target image.  The target image will be
  // resampled at the source voxel locations, after these locations
  // have been passed through the source-to-target transform.  For
  // best results, the target image should be the one with smaller
  // voxel spacing, because it can be interpolated with higher accuracy
  // than an image with larger voxel spacing.
  void SetTargetFrameInputConnection(vtkAlgorithmOutput *input) {
    this->SetInputConnection(1, input); }
  vtkAlgorithmOutput *GetTargetFrameInputConnection() {
    return this->GetInputConnection(1, 0); }
  void SetTargetFrame(vtkPolyData *input);
  vtkPolyData *GetTargetFrame();

  // Optimizer types
  enum
  {
    Amoeba,
  };

  // Metric types
  enum
  {
    FrameFiducialMetric,
  };

  // Interpolator types
  enum
  {
    Nearest,
    Linear,
    Cubic,
    Gaussian,
  };

  // Transform types
  enum
  {
    Translation,
    Rigid,
    Similarity,
    ScaleSourceAxes,
    ScaleTargetAxes,
    Affine,
  };

  // Initializer types
  enum
  {
    None,
    Centered,
  };

  // Description:
  // Set the image registration metric.  The default is normalized
  // cross correlation.
  vtkSetMacro(MetricType, int);
  void SetMetricTypeToFrameFiducialMetric() {
    this->SetMetricType(FrameFiducialMetric); }
  vtkGetMacro(MetricType, int);

  // Description:
  // Set the optimizer.  The default is Amoeba (Nelder-Mead Simplex).
  vtkSetMacro(OptimizerType, int);
  void SetOptimizerTypeToAmoeba() {
    this->SetOptimizerType(Amoeba); }
  vtkGetMacro(OptimizerType, int);

  // Description:
  // Set the image interpolator.  The default is Linear.
  vtkSetMacro(InterpolatorType, int);
  void SetInterpolatorTypeToNearest() {
    this->SetInterpolatorType(Nearest); }
  void SetInterpolatorTypeToLinear() {
    this->SetInterpolatorType(Linear); }
  void SetInterpolatorTypeToCubic() {
    this->SetInterpolatorType(Cubic); }
  vtkGetMacro(InterpolatorType, int);

  // Description:
  // Set the transform type.  The default is Rigid.  The Similarity
  // transform type adds a universal scale factor, ScaleSourceAxes
  // allows scaling along all three source image axes, ScaleTargetAxes
  // allows scaling along all three target image axes. 
  vtkSetMacro(TransformType, int);
  void SetTransformTypeToRigid() {
    this->SetTransformType(Rigid); }
  void SetTransformTypeToSimilarity() {
    this->SetTransformType(Similarity); }
  void SetTransformTypeToScaleSourceAxes() {
    this->SetTransformType(ScaleSourceAxes); }
  void SetTransformTypeToScaleTargetAxes() {
    this->SetTransformType(ScaleTargetAxes); }
  void SetTransformTypeToAffine() {
    this->SetTransformType(Affine); }
  vtkGetMacro(TransformType, int);

  // Description:
  // Set the initializer type.  The default is None.  The Centered
  // initializer sets an initial translation that will center the
  // images over each other.
  vtkSetMacro(InitializerType, int);
  void SetInitializerTypeToNone() {
    this->SetInitializerType(None); }
  void SetInitializerTypeToCentered() {
    this->SetInitializerType(Centered); }
  vtkGetMacro(InitializerType, int);

  // Description:
  // Initialize the transform.  This will also initialize the
  // NumberOfEvaluations to zero.  If a TransformInitializer is
  // set, then only the rotation part of this matrix will be used,
  // and the initial translation will be set from the initializer.
  void Initialize(vtkMatrix4x4 *matrix);

  // Description:
  // Set the tolerance that the optimizer will apply to the value returned
  // by the image similarity metric.  The default value is 1e-4.
  vtkSetMacro(MetricTolerance, double);
  vtkGetMacro(MetricTolerance, double);

  // Description:
  // Set the tolerance that the optimizer will apply to the transform
  // parameters.  Provide the value to use for the translation parameters,
  // it will automatically be scaled when applied to the rotation and
  // scale parameters.  The default value is 0.1.
  vtkSetMacro(TransformTolerance, double);
  vtkGetMacro(TransformTolerance, double);

  // Description:
  // Set the maximum number of iterations to perform.  The number of metric
  // evaluations per iteration will depend on the optimizer.
  vtkSetMacro(MaximumNumberOfIterations, int);
  vtkGetMacro(MaximumNumberOfIterations, int);

  // Description:
  // Get the number of times that the metric has been evaluated.
  int GetNumberOfEvaluations();

  // Description:
  // Get the last transform that was produced by the optimizer.
  vtkLinearTransform *GetTransform() { return this->Transform; }

  // Description:
  // Get the value that is being minimized.
  vtkGetMacro(Value, double);

  // Description:
  // Iterate the registration.  Returns zero if the termination condition has
  // been reached.
  int Iterate();

  // Description:
  // Start registration.  The registration will run to completion,
  // according to the optimization parameters that were set.  To
  // see intermediate results, set an observer for ProgressEvents.
  // There will be one ProgressEvent per iteration.  This will return
  // zero if the maximum number of iterations was reached before convergence.
  int UpdateRegistration();

protected:
  vtkFrameRegistration();
  ~vtkFrameRegistration();

  void ComputeImageRange(vtkImageData *data, double range[2]);
  int ExecuteRegistration();

  // Functions overridden from Superclass
  virtual int ProcessRequest(vtkInformation *,
                             vtkInformationVector **,
                             vtkInformationVector *);
  virtual int RequestData(vtkInformation *,
			  vtkInformationVector **,
			  vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *vtkNotUsed(outInfo));
  virtual int RequestInformation(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *vtkNotUsed(outInfo));
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  int                              OptimizerType;
  int                              MetricType;
  int                              InterpolatorType;
  int                              TransformType;
  int                              InitializerType;

  int                              MaximumNumberOfIterations;
  double                           MetricTolerance;
  double                           TransformTolerance;
  double                           Value;

  vtkTimeStamp                     ExecuteTime;

  vtkObject                       *Optimizer;
  vtkAlgorithm                    *Metric;
  vtkAbstractImageInterpolator    *Interpolator;
  vtkLinearTransform              *Transform;

  vtkMatrix4x4                    *InitialTransformMatrix;

  vtkFrameRegistrationInfo        *RegistrationInfo;

private:
  // Copy constructor and assigment operator are purposely not implemented
  vtkFrameRegistration(const vtkFrameRegistration&);
  void operator=(const vtkFrameRegistration&);
  vtkTransformPolyDataFilter *PolyDataTransformFilter;

  void ConfigureOptimizer(const double t[3], const double r[3],
                          double tscale, double rscale);
};

#endif //__vtkFrameRegistration_h
