/*=========================================================================
  Program: Cerebra
  Module:  vtkDataNode.cxx

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

#include "vtkDataNode.h"

#include <vtkObjectFactory.h>
#include <vtkMatrix4x4.h>

//--------------------------------------------------------------------------

// Constructor
vtkDataNode::vtkDataNode()
{
  this->Matrix = vtkMatrix4x4::New();
  this->FrameOfReference = 0;
}

// Destructor
vtkDataNode::~vtkDataNode()
{
  // the matrix will never be null
  this->Matrix->Delete();
  if (this->FrameOfReference)
    {
    this->FrameOfReference->Delete();
    }
}

// Set a new matrix
void vtkDataNode::SetMatrix(vtkMatrix4x4 *matrix)
{
  this->Matrix->DeepCopy(matrix);
  this->Modified();
}

// Set the frame of reference
void vtkDataNode::SetFrameOfReference(vtkFrameOfReference *ref)
{
  if (this->FrameOfReference != ref)
    {
    if (this->FrameOfReference)
      {
      this->FrameOfReference->Delete();
      }
    if (ref)
      {
      ref->Register(this);
      }
    this->FrameOfReference = ref;
    this->Modified();
    }
}

// The required PrintSelf method
void vtkDataNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Matrix: " << this->Matrix << "\n";
  os << indent << "FrameOfReference: " << this->FrameOfReference << "\n";
}
