/*=========================================================================

  Program:   ToolCursor
  Module:    vtkImageToROIContourData.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageToROIContourData - Generate ROIContourData from an image
// .SECTION Description
// This filter will contour an image at a specified isovalue to generate
// slice-by-slice ROI contours that will be stored in a vtkROIContourData.

#ifndef __vtkImageToROIContourData_h
#define __vtkImageToROIContourData_h

#include "vtkAlgorithm.h"

class vtkROIContourData;
class vtkImageData;
class vtkPolyData;

class VTK_EXPORT vtkImageToROIContourData : public vtkAlgorithm
{
public:
  static vtkImageToROIContourData *New();
  vtkTypeMacro(vtkImageToROIContourData,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The isovalue for which to generate the contours.
  vtkSetMacro(Value, double);
  vtkGetMacro(Value, double);

  // Description:
  // The input to this filter must be a vtkImageData.
  void SetInput(vtkDataObject *d);
  vtkDataObject *GetInput();

  // Description:
  // Get the output data object.
  vtkROIContourData* GetOutput();
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkImageToROIContourData();
  ~vtkImageToROIContourData();

  virtual int ComputePipelineMTime(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, int requestFromOutputPort,
    unsigned long* mtime);

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  double Value;

private:
  vtkImageToROIContourData(const vtkImageToROIContourData&);  // Not implemented.
  void operator=(const vtkImageToROIContourData&);  // Not implemented.

  void MarchingSquares(
    vtkImageData *input, vtkPolyData *output, int extent[6], double value);

};

#endif
