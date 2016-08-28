/*=========================================================================
  Program: Cerebra
  Module:  cbMRIRegistration.h

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
// .NAME cbMRIRegistration - Registrate an MR image to another.
// .SECTION Description
// cbMRIRegistration class uses vtkImageRegistration class etc. provided by Atamai, Inc.
// This class will register the source image into the target image

#ifndef CBMRIREGISTRATION_H
#define CBMRIREGISTRATION_H

class vtkImageData;
class vtkRenderWindow;
class vtkImageData;
class vtkMatrix4x4;
class vtkProgressAccumulator;
class vtkImageRegistration;
class vtkImageResize;
class vtkImageSincInterpolator;

class cbMRIRegistration
{
public:
  static cbMRIRegistration *New() {
    return new cbMRIRegistration; };
  cbMRIRegistration();
  ~cbMRIRegistration();

  // Description:
  // Input the image uses as the source image and target image.
  // Input the 4x4 matrix used by source image and target image.
  void SetInputSource(vtkImageData *inputSource);
  vtkImageData *GetInputSource();
  void SetInputTarget(vtkImageData *inputTarget);
  vtkImageData *GetInputTarget();
  void SetInputSourceMatrix(vtkMatrix4x4 *inputSourceMatrix);
  vtkMatrix4x4 *GetInputSourceMatrix();
  void SetInputTargetMatrix(vtkMatrix4x4 *inputTargetMatrix);
  vtkMatrix4x4 *GetInputTargetMatrix();

  // Description:
  // Get output modified matrix
  vtkMatrix4x4 *GetModifiedSourceMatrix();
  vtkMatrix4x4 *GetModifiedTargetMatrix();

  // Description:
  // Select the image registration method will be used in here.
  // The DEFAULT method is Mutual Information
  void SetRegistrationMethod(int method);

  // Description:
  // Choose matrix to be modified, either source matrix or target matrix.
  // The DEFAULT is to modify the source matrix.
  void SetModifyMatrixToSource();
  void SetModifyMatrixToTarget();

  // Description:
  // Setup the render window for viewing the image registration procedure
  void SetRenderWindow(vtkRenderWindow *renderwindow);

  // Description:
  // This provide a way to tract the program progress
  void SetProgressAccumulator(vtkProgressAccumulator *progressAccumulate);
  vtkProgressAccumulator *GetProgressAccumulator();

  // Description:
  // Execute the image registration
  int Execute();

  // Description:
  // Initialize the image registration (will be called by Execute)
  int Initialize();

  // Description:
  // Start one registration level (will be called by Execute).
  // The blur factor should be unity or greater than unity.
  int StartLevel(double blurFactor);

  // Description:
  // Iterate the registration (will be called by Execute).
  int Iterate();

  // Description:
  // Finish the registration (will be called by Execute).
  int Finish();

  enum RegistrationMethod {
    MUTUAL_INFORMATION = 1,
    CROSS_CORRELATION = 2,
  };

protected:

private:
  vtkImageData *m_sourceImage;
  vtkImageData *m_targetImage;
  vtkMatrix4x4 *m_sourceMatrix;
  vtkMatrix4x4 *m_targetMatrix;
  vtkRenderWindow *m_renderWindow;
  vtkProgressAccumulator *m_progressAccumulate;
  bool m_modifySourceMatrix;
  int m_registrationMethod;

  vtkImageRegistration *m_registration;
  vtkImageResize *m_sourceBlur;
  vtkImageResize *m_targetBlur;
  vtkImageSincInterpolator *m_sourceBlurKernel;
  vtkImageSincInterpolator *m_targetBlurKernel;
  double m_transformTolerance;
  bool m_registrationInitialized;
};

#endif // CBMRIREGISTRATION_H
