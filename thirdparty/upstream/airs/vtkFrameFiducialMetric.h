#ifndef __vtkFrameFiducialMetric_h
#define __vtkFrameFiducialMetric_h

#include "vtkImageAlgorithm.h"

class vtkImageGaussianInterpolator;

class VTK_EXPORT vtkFrameFiducialMetric : public vtkImageAlgorithm
{
public:
  static vtkFrameFiducialMetric *New();
  vtkTypeMacro(vtkFrameFiducialMetric,vtkImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This increases as the match between the fiducials and the image increases.
  vtkGetMacro(MetricValue, double);

protected:
  vtkFrameFiducialMetric();
  ~vtkFrameFiducialMetric();

  virtual int RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *vtkNotUsed(outInfo));
  virtual int RequestInformation(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *vtkNotUsed(outInfo));

  virtual int RequestData(vtkInformation *,
			  vtkInformationVector **,
			  vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  double MetricValue;
  vtkImageGaussianInterpolator *Interpolator;

private:
  vtkFrameFiducialMetric(const vtkFrameFiducialMetric&);  // Not implemented.
  void operator=(const vtkFrameFiducialMetric&);  // Not implemented.
};

#endif
