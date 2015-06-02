/*=========================================================================
  Program: Cerebra
  Module:  cbMRIRegistration.cxx

  Copyright (c) 2011-2013 Qian Lu, David Adair
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of the Calgary Image Processing and Analysis Centre
    (CIPAC), the University of Calgary, nor the names of any authors nor
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

/*=========================================================================
 Copyright (c) 2005 Atamai, Inc.
 All rights reserved.

 Use, modification and redistribution of the software, in source or
 binary forms, are permitted provided that the following terms and
 conditions are met:

 1) Redistribution of the source code, in verbatim or modified
 form, must retain the above copyright notice, this license,
 the following disclaimer, and any notices that refer to this
 license and/or the following disclaimer.

 2) Redistribution in binary form must include the above copyright
 notice, a copy of this license and the following disclaimer
 in the documentation or with other materials provided with the
 distribution.

 3) Modified copies of the source code must be clearly marked as such,
 and must not be misrepresented as verbatim copies of the source code.

 THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
 WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
 MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
 OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
 THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGES.

 =========================================================================*/

#include "cbMRIRegistration.h"

#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkImageResize.h>
#include <vtkImageReslice.h>
#include <vtkImageSincInterpolator.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkImageSlice.h>
#include <vtkImageStack.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageProperty.h>
#include <vtkTimerLog.h>
#include <vtkErrorCode.h>

#include <vtkImageRegistration.h>
#include <vtkProgressAccumulator.h>

//----------------------------------------------------------------------------
cbMRIRegistration::cbMRIRegistration()
{
  this->m_sourceImage = NULL;
  this->m_targetImage = NULL;
  this->m_sourceMatrix = NULL;
  this->m_targetMatrix = NULL;
  this->m_renderWindow = NULL;
  this->m_progressAccumulate = NULL;
  this->m_modifySourceMatrix = true;
  this->m_registrationMethod = MUTUAL_INFORMATION;
}

//----------------------------------------------------------------------------
cbMRIRegistration::~cbMRIRegistration()
{
  if (m_sourceImage) {
    this->m_sourceImage->Delete();
  }
  if (m_targetImage) {
    this->m_targetImage->Delete();
  }
  if (m_sourceMatrix) {
    this->m_sourceMatrix->Delete();
  }
  if (m_targetMatrix) {
    this->m_targetMatrix->Delete();
  }
  if (m_renderWindow) {
    this->m_renderWindow->Delete();
  }
  if (m_progressAccumulate) {
    this->m_progressAccumulate->Delete();
  }
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetRenderWindow(vtkRenderWindow *renderwindow)
{
  m_renderWindow = renderwindow;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputSource(vtkImageData *inputSource)
{
  this->m_sourceImage = inputSource;
}

//----------------------------------------------------------------------------
vtkImageData *cbMRIRegistration::GetInputSource()
{
  return this->m_sourceImage;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputTarget(vtkImageData *inputTarget)
{
  this->m_targetImage = inputTarget;
}

//----------------------------------------------------------------------------
vtkImageData *cbMRIRegistration::GetInputTarget()
{
  return this->m_targetImage;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputSourceMatrix(vtkMatrix4x4 *inputSourceMatrix)
{
  this->m_sourceMatrix = inputSourceMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetInputSourceMatrix()
{
  return this->m_sourceMatrix;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputTargetMatrix(vtkMatrix4x4 *inputTargetMatrix)
{
  this->m_targetMatrix = inputTargetMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetInputTargetMatrix()
{
  return this->m_targetMatrix;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetRegistrationMethod(int method)
{
  if (method == 0) {
    this->m_registrationMethod = MUTUAL_INFORMATION;
  }
  else if (method == 1) {
    this->m_registrationMethod = CROSS_CORRELATION;
  }
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetModifyMatrixToSource()
{
  this->m_modifySourceMatrix = true;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetModifyMatrixToTarget()
{
  this->m_modifySourceMatrix = false;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetProgressAccumulator(
  vtkProgressAccumulator *progressAccumulate)
{
  this->m_progressAccumulate = progressAccumulate;
}

//----------------------------------------------------------------------------
vtkProgressAccumulator *cbMRIRegistration::GetProgressAccumulator()
{
  return this->m_progressAccumulate;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::Execute()
{
  if (this->m_sourceImage == NULL || this->m_targetImage == NULL ||
      this->m_sourceMatrix == NULL || this->m_targetMatrix == NULL)
  {
    cout << "Execute: Input source image & matrix and"
            "target image & matrix are not set " << endl;
    return 0;
  }

  // parameters for registration
  int interpolatorType = vtkImageRegistration::Linear;
  double transformTolerance = 0.1; // tolerance on transformation result
  int numberOfBins = 64; // for Mattes' mutual information
  double initialBlurFactor = 4.0;

  // get information about the images
  double targetSpacing[3], sourceSpacing[3];
  this->m_targetImage->GetSpacing(targetSpacing);
  this->m_sourceImage->GetSpacing(sourceSpacing);

  for (int jj = 0; jj < 3; jj++)
  {
    targetSpacing[jj] = fabs(targetSpacing[jj]);
    sourceSpacing[jj] = fabs(sourceSpacing[jj]);
  }

  double minSpacing = sourceSpacing[0];
  if (minSpacing > sourceSpacing[1])
  {
    minSpacing = sourceSpacing[1];
  }
  if (minSpacing > sourceSpacing[2])
  {
    minSpacing = sourceSpacing[2];
  }

  // blur source image with Hamming-windowed sinc
  vtkSmartPointer<vtkImageSincInterpolator> sourceBlurKernel =
  vtkSmartPointer<vtkImageSincInterpolator>::New();
  sourceBlurKernel->SetWindowFunctionToHamming();

  // reduce the source resolution
  vtkSmartPointer<vtkImageResize> sourceBlur =
  vtkSmartPointer<vtkImageResize>::New();
  sourceBlur->SetInput(this->m_sourceImage);
  sourceBlur->SetResizeMethodToOutputSpacing();
  sourceBlur->SetInterpolator(sourceBlurKernel);

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(sourceBlur,0.07f);
  }

  // blur target with Hamming-windowed sinc
  vtkSmartPointer<vtkImageSincInterpolator> targetBlurKernel =
  vtkSmartPointer<vtkImageSincInterpolator>::New();
  targetBlurKernel->SetWindowFunctionToHamming();

  // keep target at full resolution
  vtkSmartPointer<vtkImageResize> targetBlur =
  vtkSmartPointer<vtkImageResize>::New();
  targetBlur->SetInput(this->m_targetImage);
  targetBlur->SetResizeMethodToOutputSpacing();
  targetBlur->SetInterpolator(targetBlurKernel);

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(targetBlur,0.07f);
  }

  // get the initial transformation
  vtkSmartPointer<vtkMatrix4x4> matrix =
  vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->DeepCopy(this->m_targetMatrix);
  matrix->Invert();
  vtkMatrix4x4::Multiply4x4(matrix, this->m_sourceMatrix, matrix);

  // set up the registration
  vtkSmartPointer<vtkImageRegistration> registration =
  vtkSmartPointer<vtkImageRegistration>::New();
  registration->SetTargetImageInputConnection(targetBlur->GetOutputPort());
  registration->SetSourceImageInputConnection(sourceBlur->GetOutputPort());
  registration->SetInitializerTypeToCentered();

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(registration,0.05f);
  }

  if (!m_modifySourceMatrix) {
    registration->SetTransformTypeToScaleTargetAxes();
  }
  else {
    registration->SetTransformTypeToRigid();
  }

  if (this->m_registrationMethod == MUTUAL_INFORMATION) {
    registration->SetMetricTypeToNormalizedMutualInformation();
  }
  else if (this->m_registrationMethod == CROSS_CORRELATION) {
    registration->SetMetricTypeToNormalizedCrossCorrelation();
  }
  registration->SetInterpolatorType(interpolatorType);
  registration->SetJointHistogramSize(numberOfBins,numberOfBins);
  registration->SetMetricTolerance(1e-4);
  registration->SetTransformTolerance(transformTolerance);
  registration->SetMaximumNumberOfIterations(500);

  // make a timer
  vtkSmartPointer<vtkTimerLog> timer =
  vtkSmartPointer<vtkTimerLog>::New();
  double startTime = timer->GetUniversalTime();
  double lastTime = startTime;

  // do the registration
  // the registration starts at low-resolution
  double blurFactor = initialBlurFactor;
  // two stages for each resolution:
  // first without interpolation, and then with interpolation
  int stage = 0;
  // will be set to "true" when registration is initialized
  bool initialized = false;

  for (;;)
  {
    if (stage == 0)
    {
      registration->SetInterpolatorTypeToNearest();
      registration->SetTransformTolerance(minSpacing);
    }
    else
    {
      registration->SetInterpolatorType(interpolatorType);
      registration->SetTransformTolerance(transformTolerance);
    }
    if (blurFactor < 1.1)
    {
      // full resolution: no blurring or resampling
      sourceBlur->InterpolateOff();
      sourceBlur->SetOutputSpacing(sourceSpacing);
      sourceBlur->Update();

      targetBlur->InterpolateOff();
      targetBlur->SetOutputSpacing(targetSpacing);
      targetBlur->Update();
    }
    else
    {
      // reduced resolution: set the blurring
      double spacing[3];
      for (int j = 0; j < 3; j++)
      {
        spacing[j] = blurFactor*minSpacing;
        if (spacing[j] < sourceSpacing[j])
        {
          spacing[j] = sourceSpacing[j];
        }
      }

      sourceBlurKernel->SetBlurFactors(
                                       spacing[0]/sourceSpacing[0],
                                       spacing[1]/sourceSpacing[1],
                                       spacing[2]/sourceSpacing[2]);

      sourceBlur->SetOutputSpacing(spacing);
      sourceBlur->Update();

      targetBlurKernel->SetBlurFactors(
                                       blurFactor*minSpacing/targetSpacing[0],
                                       blurFactor*minSpacing/targetSpacing[1],
                                       blurFactor*minSpacing/targetSpacing[2]);

      targetBlur->Update();
    }

    if (initialized)
    {
      // re-initialize with the matrix from the previous step
      registration->SetInitializerTypeToNone();
      matrix->DeepCopy(registration->GetTransform()->GetMatrix());
    }

    registration->Initialize(matrix);

    initialized = true;

    while (registration->Iterate())
    {
      //registration->UpdateRegistration();
      // will iterate until convergence or failure
      if (m_modifySourceMatrix) {
        vtkMatrix4x4::Multiply4x4(this->m_targetMatrix,
                                  registration->GetTransform()->GetMatrix(),
                                  this->m_sourceMatrix);
        this->m_sourceMatrix->Modified();
      }
      else {
        vtkMatrix4x4::Multiply4x4(
                                  this->m_sourceMatrix,
                                  registration->GetTransform()->GetLinearInverse()->GetMatrix(),
                                  this->m_targetMatrix);
        this->m_targetMatrix->Modified();
      }


      if (m_renderWindow) {
        m_renderWindow->Render();
      }
    }

    double newTime = timer->GetUniversalTime();
    cout << "blur " << blurFactor << " stage " << stage << " took "
    << (newTime - lastTime) << "s and "
    << registration->GetNumberOfEvaluations() << " evaluations" << endl;
    lastTime = newTime;

    // prepare for next iteration
    if (stage == 1)
    {
      blurFactor /= 2.0;
      if (blurFactor < 0.9)
      {
        break;
      }
    }
    stage = (stage + 1) % 2;
  }

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterEndEvent();
  }

  cout << "registration took " << (lastTime - startTime) << "s" << endl;

  return 1;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetModifiedSourceMatrix()
{
  return this->m_sourceMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetModifiedTargetMatrix()
{
  return this->m_targetMatrix;
}
