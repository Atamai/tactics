/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2012-2015 David Gobbi
  All rights reserved.
  See Copyright.txt or http://dgobbi.github.io/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDICOMCTRectifier - Prepare a CT for 3D processing
// .SECTION Description
// This class will identify gantry-tilted CT images and resample them
// into a rectangular volume.  This is often a necessary step prior to
// volume rendering or other forms of 3D rendering.

#ifndef vtkDICOMCTRectifier_h
#define vtkDICOMCTRectifier_h

#include "vtkDICOMAlgorithm.h"
#include "vtkDICOMModule.h" // For export macro

class vtkMatrix4x4;

//----------------------------------------------------------------------------
class VTKDICOM_EXPORT vtkDICOMCTRectifier : public vtkDICOMAlgorithm
{
public:
  // Description:
  // Static method for construction.
  static vtkDICOMCTRectifier *New();
  vtkTypeMacro(vtkDICOMCTRectifier, vtkDICOMAlgorithm);

  // Description:
  // Print information about this object.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Reverse the default operation.
  // When this option is set, the filter takes a rectangular volume
  // as input, and produces a volume whose geometry matches the
  // VolumeMatrix.
  void SetReverse(int v);
  void ReverseOn() { this->SetReverse(1); }
  void ReverseOff() { this->SetReverse(0); }
  int GetReverse() { return this->Reverse; }

  // Description:
  // Set the matrix that describes the CT volume geometry.
  // This should be set to the PatientMatrix that comes from the
  // vtkDICOMReader.
  void SetVolumeMatrix(vtkMatrix4x4 *matrix);
  vtkMatrix4x4 *GetVolumeMatrix() { return this->VolumeMatrix; }

  // Description:
  // Get the matrix that describes the rectified geometry.
  // This matrix is generated when any of these methods is called:
  // Update(), UpdateInformation(), or UpdateMatrix().
  vtkMatrix4x4 *GetRectifiedMatrix() { return this->RectifiedMatrix; }

  // Description:
  // Update the RectifiedMatrix without updating the output data.
  // The input data must be set before this is called.
  void UpdateMatrix();

  // Description:
  // A static method to measure the tilt from a matrix.
  // This can be used to see whether it is necessary to rectify the
  // volume.  It returns the Gantry Detector Tilt angle, in degrees,
  // as computed from the shear in the volume matrix.
  static double GetGantryDetectorTilt(vtkMatrix4x4 *volumeMatrix);

protected:
  vtkDICOMCTRectifier();
  ~vtkDICOMCTRectifier();

  // Description:
  // Compute the rectified matrix from the given volume matrix.
  // The extent, spacing, and origin of the image must also be given,
  // and they will be adjusted as necessary.
  void ComputeMatrix(
    const double matrix[16], const int extent[6], double spacing[3],
    double origin[3]);

  virtual int RequestInformation(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual void ThreadedRequestData(
    vtkInformation *request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector, vtkImageData ***inData,
    vtkImageData **outData, int ext[6], int id);

  vtkMatrix4x4 *VolumeMatrix;
  vtkMatrix4x4 *RectifiedMatrix;
  vtkMatrix4x4 *Matrix;
  int Reverse;

private:
  vtkDICOMCTRectifier(const vtkDICOMCTRectifier&);  // Not implemented.
  void operator=(const vtkDICOMCTRectifier&);  // Not implemented.
};

#endif // vtkDICOMCTRectifier_h
