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
//VTK includes
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
//AIRS includes
#include <vtkImageRegistration.h>
#include <vtkProgressAccumulator.h>

#include <iostream>
//----------------------------------------------------------------------------
cbMRIRegistration::cbMRIRegistration()
{
  m_sourceImage = NULL;
  m_targetImage = NULL;
  m_sourceMatrix = NULL;
  m_targetMatrix = NULL;
  m_renderWindow = NULL;
  m_progressAccumulate = NULL;
  m_modifySourceMatrix = true;
  m_registrationMethod = MUTUAL_INFORMATION;
  m_registration = NULL;
  m_sourceBlur = NULL;
  m_targetBlur = NULL;
  m_sourceBlurKernel = NULL;
  m_targetBlurKernel = NULL;
  m_registrationInitialized = false;
  m_transformTolerance = 0.1;
  m_funcEvals = 0;
}

//----------------------------------------------------------------------------
cbMRIRegistration::~cbMRIRegistration()
{
  if (m_registration) {
    m_registration->Delete();
  }
  if (m_sourceBlur) {
    m_sourceBlur->Delete();
  }
  if (m_sourceBlurKernel) {
    m_sourceBlurKernel->Delete();
  }
  if (m_targetBlur) {
    m_targetBlur->Delete();
  }
  if (m_targetBlurKernel) {
    m_targetBlurKernel->Delete();
  }
  if (m_sourceImage) {
    m_sourceImage->Delete();
  }
  if (m_targetImage) {
    m_targetImage->Delete();
  }
  if (m_sourceMatrix) {
    m_sourceMatrix->Delete();
  }
  if (m_targetMatrix) {
    m_targetMatrix->Delete();
  }
  if (m_renderWindow) {
    m_renderWindow->Delete();
  }
  if (m_progressAccumulate) {
    m_progressAccumulate->Delete();
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
  m_sourceImage = inputSource;
}

//----------------------------------------------------------------------------
vtkImageData *cbMRIRegistration::GetInputSource()
{
  return m_sourceImage;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputTarget(vtkImageData *inputTarget)
{
  m_targetImage = inputTarget;
}

//----------------------------------------------------------------------------
vtkImageData *cbMRIRegistration::GetInputTarget()
{
  return m_targetImage;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputSourceMatrix(vtkMatrix4x4 *inputSourceMatrix)
{
  m_sourceMatrix = inputSourceMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetInputSourceMatrix()
{
  return m_sourceMatrix;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetInputTargetMatrix(vtkMatrix4x4 *inputTargetMatrix)
{
  m_targetMatrix = inputTargetMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetInputTargetMatrix()
{
  return m_targetMatrix;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetRegistrationMethod(int method)
{
  if (method == 0) {
    m_registrationMethod = MUTUAL_INFORMATION;
  }
  else if (method == 1) {
    m_registrationMethod = CROSS_CORRELATION;
  }
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetModifyMatrixToSource()
{
  m_modifySourceMatrix = true;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetModifyMatrixToTarget()
{
  m_modifySourceMatrix = false;
}

//----------------------------------------------------------------------------
void cbMRIRegistration::SetProgressAccumulator(
  vtkProgressAccumulator *progressAccumulate)
{
  m_progressAccumulate = progressAccumulate;
}

//----------------------------------------------------------------------------
vtkProgressAccumulator *cbMRIRegistration::GetProgressAccumulator()
{
  return m_progressAccumulate;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::Execute()
{
  double initialBlurFactor = 4.0;
  double blurFactor = initialBlurFactor;

  this->Initialize();

  // make a timer
  vtkSmartPointer<vtkTimerLog> timer =
  vtkSmartPointer<vtkTimerLog>::New();
  double startTime = timer->GetUniversalTime();
  double lastTime = startTime;

  // do multi-level registration
  for (;;)
  {
    this->StartLevel(blurFactor);

    // iterate until this level is done
    while (this->Iterate()) {}

    double newTime = timer->GetUniversalTime();
    std::cout << "blur " << blurFactor << " took "
         << (newTime - lastTime) << "s and "
         << m_registration->GetNumberOfEvaluations() << " evaluations" << endl;
    lastTime = newTime;

    // prepare for next iteration
    blurFactor /= 2.0;
    if (blurFactor < 0.9)
    {
      break;
    }
  }

  this->Finish();

  std::cout << "registration took " << (lastTime - startTime) << "s" << endl;

  return 1;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::Initialize()
{
  if (m_sourceImage == NULL || m_targetImage == NULL ||
      m_sourceMatrix == NULL || m_targetMatrix == NULL)
  {
    std::cout << "Execute: Input source image & matrix and"
            "target image & matrix are not set " << endl;
    return 0;
  }

  // parameters for registration
  int interpolatorType = vtkImageRegistration::Rigid;
  int numberOfBins = 64; // for Mattes' mutual information

  // get information about the images
  double targetSpacing[3], sourceSpacing[3];
  m_targetImage->GetSpacing(targetSpacing);
  m_sourceImage->GetSpacing(sourceSpacing);

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
  m_sourceBlurKernel = vtkImageSincInterpolator::New();
  m_sourceBlurKernel->SetWindowFunctionToHamming();

  // reduce the source resolution
  m_sourceBlur = vtkImageResize::New();
  m_sourceBlur->SetInputData(m_sourceImage);
  m_sourceBlur->SetResizeMethodToOutputSpacing();
  m_sourceBlur->SetInterpolator(m_sourceBlurKernel);

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(m_sourceBlur,0.07f);
  }

  // blur target with Hamming-windowed sinc
  m_targetBlurKernel = vtkImageSincInterpolator::New();
  m_targetBlurKernel->SetWindowFunctionToHamming();

  // keep target at full resolution
  m_targetBlur = vtkImageResize::New();
  m_targetBlur->SetInputData(m_targetImage);
  m_targetBlur->SetResizeMethodToOutputSpacing();
  m_targetBlur->SetInterpolator(m_targetBlurKernel);

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(m_targetBlur,0.07f);
  }

  // set up the registration
  m_registration = vtkImageRegistration::New();
  m_registration->SetTargetImageInputConnection(m_targetBlur->GetOutputPort());
  m_registration->SetSourceImageInputConnection(m_sourceBlur->GetOutputPort());
  m_registration->SetInitializerTypeToCentered();

  if (m_progressAccumulate) {
    m_progressAccumulate->RegisterFilter(m_registration,0.05f);
  }

  m_registration->SetTransformTypeToRigid();

  if (m_registrationMethod == MUTUAL_INFORMATION) {
    m_registration->SetMetricTypeToNormalizedMutualInformation();
  }
  else if (m_registrationMethod == CROSS_CORRELATION) {
    m_registration->SetMetricTypeToNormalizedCrossCorrelation();
  }
  m_registration->SetInterpolatorType(interpolatorType);
  m_registration->SetJointHistogramSize(numberOfBins,numberOfBins);
  m_registration->SetCostTolerance(1e-4);
  m_registration->SetTransformTolerance(m_transformTolerance);
  m_registration->SetMaximumNumberOfIterations(500);

  m_registrationInitialized = false;
  m_funcEvals = 0;

  return 1;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::StartLevel(double blurFactor)
{
  // get information about the images
  double targetSpacing[3], sourceSpacing[3];
  m_targetImage->GetSpacing(targetSpacing);
  m_sourceImage->GetSpacing(sourceSpacing);

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

  if (blurFactor < 1.1)
  {
    // full resolution: no blurring or resampling
    m_sourceBlur->InterpolateOff();
    m_sourceBlur->SetOutputSpacing(sourceSpacing);
    m_sourceBlur->Update();

    m_targetBlur->InterpolateOff();
    m_targetBlur->SetOutputSpacing(targetSpacing);
    m_targetBlur->Update();

    m_registration->SetTransformTolerance(m_transformTolerance);
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

    m_registration->SetTransformTolerance(m_transformTolerance*blurFactor);

    m_sourceBlurKernel->SetBlurFactors(spacing[0]/sourceSpacing[0],
                                       spacing[1]/sourceSpacing[1],
                                       spacing[2]/sourceSpacing[2]);

    m_sourceBlur->SetOutputSpacing(spacing);
    m_sourceBlur->Update();

    m_targetBlurKernel->SetBlurFactors(blurFactor*minSpacing/targetSpacing[0],
                                       blurFactor*minSpacing/targetSpacing[1],
                                       blurFactor*minSpacing/targetSpacing[2]);

    m_targetBlur->Update();
  }

  // get the initial transformation
  vtkSmartPointer<vtkMatrix4x4> matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();

  if (m_registrationInitialized)
  {
    // re-initialize with the matrix from the previous step
    m_registration->SetInitializerTypeToNone();
    matrix->DeepCopy(m_registration->GetTransform()->GetMatrix());
  }
  else
  {
    matrix->DeepCopy(m_targetMatrix);
    matrix->Invert();
    vtkMatrix4x4::Multiply4x4(matrix, m_sourceMatrix, matrix);
  }

  m_registration->Initialize(matrix);
  m_registrationInitialized = true;
  m_funcEvals = 0; // start fresh

  return 1;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::Iterate()
{
  if (m_registration->Iterate())
  {
    //m_registration->UpdateRegistration();
    // will iterate until convergence or failure
    if (m_modifySourceMatrix) {
      vtkMatrix4x4::Multiply4x4(m_targetMatrix,
                                m_registration->GetTransform()->GetMatrix(),
                                m_sourceMatrix);
      m_sourceMatrix->Modified();
    }
    else {
      vtkMatrix4x4::Multiply4x4(m_sourceMatrix,
                                m_registration->GetTransform()->GetLinearInverse()->GetMatrix(),
                                m_targetMatrix);
      m_targetMatrix->Modified();
    }


    if (m_renderWindow) {
      m_renderWindow->Render();
    }

    m_funcEvals = m_registration->GetNumberOfEvaluations();

    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int cbMRIRegistration::Finish()
{
  m_sourceBlur->Delete();
  m_sourceBlur = NULL;
  m_sourceBlurKernel->Delete();
  m_sourceBlurKernel = NULL;
  m_targetBlur->Delete();
  m_targetBlur = NULL;
  m_targetBlurKernel->Delete();
  m_targetBlurKernel = NULL;
  m_registration->Delete();
  m_registration = NULL;

  if (m_progressAccumulate) {
//    m_progressAccumulate->RegisterEndEvent();
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetModifiedSourceMatrix()
{
  return m_sourceMatrix;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *cbMRIRegistration::GetModifiedTargetMatrix()
{
  return m_targetMatrix;
}
