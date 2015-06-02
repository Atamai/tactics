/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAutoCrop.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageAutoCrop - Automatically crop an image.
// .SECTION Description
// vtkImageAutoCrop will automatically crop an image by reducing the extent
// so that there are no black borders around the image.  Optionally, you
// can just get the crop extent without actually cropping the data.
#ifndef __vtkImageAutoCrop_h
#define __vtkImageAutoCrop_h

#include "vtkImageAlgorithm.h"

class vtkImageAutoCrop : public vtkImageAlgorithm
{
public:
  static vtkImageAutoCrop *New();
  vtkTypeMacro(vtkImageAutoCrop,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the computed crop extent for the input image.  You
  // must call Update on this filter before you get the crop
  // extent.
  vtkGetVector6Macro(CropExtent, int);
  
  void SetThreshold(double threshold);
  double GetThreshold() { return this->Threshold; }

protected:
  vtkImageAutoCrop();
  ~vtkImageAutoCrop() {};

  int CropExtent[6];
  double Threshold;
  
  virtual int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *);
  
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkImageAutoCrop(const vtkImageAutoCrop&);  // Not implemented.
  void operator=(const vtkImageAutoCrop&);  // Not implemented.
};



#endif



