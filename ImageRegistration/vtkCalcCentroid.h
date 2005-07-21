/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCalcCentroid.h,v $
  Language:  C++
  Date:      $Date: 2005/07/21 20:37:31 $
  Version:   $Revision: 1.6 $
  Thanks:    Thanks to Yves who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCalcCentroid - compute centre of gravity of vtkImageData
// .SECTION Description
// vtkCalcCentroid computes the centre of gravity of vtkImageData.


#ifndef __vtkCalcCentroid_h
#define __vtkCalcCentroid_h

#ifndef vtkFloatingPointType
#define vtkFloatingPointType vtkFloatingPointType
typedef float vtkFloatingPointType;
#endif

#include "vtkObject.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkCalcCentroid : public vtkObject
{
public:
  vtkTypeMacro(vtkCalcCentroid, vtkObject);
  static vtkCalcCentroid *New();
  
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetObjectMacro(Input, vtkImageData);
  vtkGetObjectMacro(Input, vtkImageData);

  float *GetCentroid();
  float *GetCovarianceMatrix();



protected:
  vtkCalcCentroid();
  ~vtkCalcCentroid();
  void ComputeCentroid();
  void ComputeCovarianceMatrix();
  vtkCalcCentroid(const vtkCalcCentroid&) {};
  void operator=(const vtkCalcCentroid&) {};


  float Centroid[3];
  float CovarianceMatrix[9];
  vtkImageData *Input;
};

#endif


