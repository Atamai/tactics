/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAutoCrop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAutoCrop.h"

#include "vtkCellData.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTypeTraits.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkImageAutoCrop);

//----------------------------------------------------------------------------
vtkImageAutoCrop::vtkImageAutoCrop()
{
  this->CropExtent[0] = 0;
  this->CropExtent[1] = 0;
  this->CropExtent[2] = 0;
  this->CropExtent[3] = 0;
  this->CropExtent[4] = 0;
  this->CropExtent[5] = 0;
  
  this->SetNumberOfOutputPorts(0);
  
  this->Threshold = 0.0;
}

//----------------------------------------------------------------------------
void vtkImageAutoCrop::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CropExtent: "
     << this->CropExtent[0] << ", " << this->CropExtent[1] << ", "
     << this->CropExtent[2] << ", " << this->CropExtent[3] << ", "
     << this->CropExtent[4] << ", " << this->CropExtent[5] << "\n";
  
  os  << indent << "Threshold: " << this->GetThreshold() << "\n";
}

//----------------------------------------------------------------------------
void vtkImageAutoCrop::SetThreshold(double threshold)
{
  this->Threshold = threshold;
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int vtkImageAutoCrop::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{  
  return 1;
}

//----------------------------------------------------------------------------
// Always set the update extent to the whole extent of the input
int vtkImageAutoCrop::RequestUpdateExtent(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
  
  return 1;
}

//----------------------------------------------------------------------------
// This function template implements the filter for any type of data.
// The last two arguments help the vtkTemplateMacro calls below
// instantiate the proper input and output types.
template <class T>
void vtkImageAutoCropExecute(vtkImageAutoCrop* self,
                             vtkImageData* inData,
                             T *inPtr, int inExt[6], int cropExtent[6])
{
  // initialize the crop extent to the full extent
  cropExtent[0] = inExt[0];
  cropExtent[1] = inExt[1];
  cropExtent[2] = inExt[2];
  cropExtent[3] = inExt[3];
  cropExtent[4] = inExt[4];
  cropExtent[5] = inExt[5];
  
  vtkIdType incr[3];
  inData->GetIncrements(incr);
  
  double dthresh = self->GetThreshold();
  if (dthresh < vtkTypeTraits<T>::Min())
    {
    dthresh = vtkTypeTraits<T>::Min();
    }
  if (dthresh > vtkTypeTraits<T>::Max())
    {
    dthresh = vtkTypeTraits<T>::Max();
    }
  T threshold = static_cast<T>(dthresh);
  
  for (int dim = 0; dim < 3; dim++)
    {
    int pixelDim = dim;
    int rowDim = (dim + 1)%3;
    int sliceDim = (dim + 2)%3;
    
    vtkIdType pixelInc = incr[pixelDim];
    vtkIdType rowInc = incr[rowDim];
    vtkIdType sliceInc = incr[sliceDim];
    
    int pixelsPerRow = cropExtent[2*pixelDim + 1] - cropExtent[2*pixelDim] + 1;
    int rowsPerSlice = cropExtent[2*rowDim + 1] - cropExtent[2*rowDim] + 1;
    int totalSlices = cropExtent[2*sliceDim + 1] - cropExtent[2*sliceDim] + 1;
    
    T *slicePtr = inPtr +
    (cropExtent[2*pixelDim] - inExt[2*pixelDim])*pixelInc +
    (cropExtent[2*rowDim] - inExt[2*rowDim])*rowInc +
    (cropExtent[2*sliceDim] - inExt[2*sliceDim])*sliceInc;
    
    T maxValue = vtkTypeTraits<T>::Min();
    
    for (int slice = 0; slice < totalSlices; slice++)
      {
      T *rowPtr = slicePtr; 
      for (int row = 0; row < rowsPerSlice; row++)
        {
        T *pixelPtr = rowPtr;
        for (int pixel = 0; pixel < pixelsPerRow; pixel++)
          {
          // compute max
          maxValue = (maxValue > *pixelPtr ? maxValue : *pixelPtr);
          pixelPtr += pixelInc;
          }
        rowPtr += rowInc;
        }
      if (maxValue > threshold)
        {
        cropExtent[2*sliceDim] += slice;
        break;
        }
      slicePtr += sliceInc;
      }
    
    // recompute total slices for new extent
    totalSlices = cropExtent[2*sliceDim + 1] - cropExtent[2*sliceDim] + 1;
    
    // compute the pointer to the _last_ slice
    slicePtr = inPtr +
    (cropExtent[2*pixelDim] - inExt[2*pixelDim])*pixelInc +
    (cropExtent[2*rowDim] - inExt[2*rowDim])*rowInc +
    (cropExtent[2*sliceDim+1] - inExt[2*sliceDim])*sliceInc;
    
    maxValue = vtkTypeTraits<T>::Min();
    
    for (int slice = 0; slice < totalSlices; slice++)
      {
      T *rowPtr = slicePtr; 
      for (int row = 0; row < rowsPerSlice; row++)
        {
        T *pixelPtr = rowPtr;
        for (int pixel = 0; pixel < pixelsPerRow; pixel++)
          {
          // compute max
          maxValue = (maxValue > *pixelPtr ? maxValue : *pixelPtr);
          pixelPtr += pixelInc;
          }
        rowPtr += rowInc;
        }
      if (maxValue > threshold)
        {
        cropExtent[2*sliceDim+1] -= slice;
        break;
        }
      slicePtr -= sliceInc;
      }
    }
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkImageAutoCrop::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  int *inExt = inData->GetExtent();
  void *inPtr = inData->GetScalarPointerForExtent(inExt);

  switch(inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageAutoCropExecute(
      this, inData, static_cast<VTK_TT*>(inPtr), inExt, this->CropExtent));
    default:
    vtkErrorMacro("RequestData: Unknown input ScalarType");
    return 0;
    }

  return 1;
}
