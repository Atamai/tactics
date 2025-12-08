/*=========================================================================
  Program: Cerebra
  Module:  vtkImageNode.cxx

  Copyright (c) 2011-2013 Qian Lu, David Gobbi
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

#include "vtkImageNode.h"

#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkDICOMMetaData.h>

// New macro for every VTK object
vtkStandardNewMacro(vtkImageNode);

//--------------------------------------------------------------------------

// Constructor
vtkImageNode::vtkImageNode() : FileURL()
{
  this->Image = vtkImageData::New();
  this->MetaData = NULL;
}

// Destructor
vtkImageNode::~vtkImageNode()
{
  this->Image->Delete();
  if (this->MetaData)
    {
    this->MetaData->Delete();
    }
}

// Get generic data object
vtkDataObject *vtkImageNode::GetDataObject()
{
  return this->Image;
}

// Set the image
void vtkImageNode::ShallowCopyImage(vtkImageData *image)
{
  // This copies just the parts we want, and nothing else.
  this->Image->CopyStructure(image);
  this->Image->GetPointData()->ShallowCopy(image->GetPointData());
  this->Image->Modified();
  this->Modified();
}

// Set mata data
void vtkImageNode::SetMetaData(vtkDICOMMetaData *metaData)
{
  if (this->MetaData != metaData)
    {
    if (this->MetaData)
      {
      this->MetaData->Delete();
      }
    this->MetaData = metaData;
    if (metaData)
      {
      metaData->Register(this);
      }
    }
}

// The required PrintSelf method
void vtkImageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Image: " << this->Image << "\n";
  os << indent << "MetaData: " << this->MetaData << "\n";
}

// Be able to set the file path for the image
void vtkImageNode::SetFileURL(const char *url)
{
  this->FileURL = url;
}

// Get the file path for the image
std::string vtkImageNode::GetFileURL() const
{
  return std::string(this->FileURL);
}
