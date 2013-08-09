/*=========================================================================
  Program: Cerebra
  Module:  vtkDataManagerDefaultKeys.h

  Copyright (c) 2011-2013 Qian Lu
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

#include "vtkDataManager.h"

extern vtkDataManager::UniqueKey TemplateImageKey; //use to storage atas imagedata.

extern vtkDataManager::UniqueKey PrimaryImageKey; //use to storage first loaded T1-weighted imagedata, and modified during image processing steps.
extern vtkDataManager::UniqueKey PrimaryHeadKey; //use to storage first loaded T1-weighted imagedata, but never modified.

extern vtkDataManager::UniqueKey SecondaryImageKey; //use to storage second loaded T2 imagedata, and modified during image processing steps.
extern vtkDataManager::UniqueKey SecondaryHeadKey; //use to storage second loaded T2 imagedata, but never modified.

extern vtkDataManager::UniqueKey MaskImageKey; //use to storage mask image, and modified during image processing steps.
extern vtkDataManager::UniqueKey MaskHeadKey; //use to storage mask image for redoing current steps.
extern vtkDataManager::UniqueKey MaskSurfaceKey; //use to storage mask mesh(vtkPolyData)

extern vtkDataManager::UniqueKey DataImageKey; //use to storage the imagedata for viewing.
extern vtkDataManager::UniqueKey OverlayImageKey; //use to storage the overlay(mask) imagedata for viewing.
extern vtkDataManager::UniqueKey LesionOverlayKey; //use to storage the over-overlay(mask) imagedata of current working lesion.
