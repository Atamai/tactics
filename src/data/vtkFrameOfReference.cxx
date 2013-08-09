/*=========================================================================
  Program: Cerebra
  Module:  vtkFrameOfReference.cxx

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

#include "vtkFrameOfReference.h"

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

#include <vector>
#include <utility>

// New macro for every VTK object
vtkStandardNewMacro(vtkFrameOfReference);

//--------------------------------------------------------------------------

// An Element is just a std::pair of smart pointers
// (smart pointers provide automatic reference counting)
class vtkFrameOfReference::Element :
  public std::pair<vtkSmartPointer<vtkFrameOfReference>,
                   vtkSmartPointer<vtkMatrix4x4> >
{
public:
  Element(vtkFrameOfReference *ref, vtkMatrix4x4 *mat) :
    std::pair<vtkSmartPointer<vtkFrameOfReference>,
              vtkSmartPointer<vtkMatrix4x4> >(ref, mat) {};
};

// A Container is just a vector
class vtkFrameOfReference::Container :
  public std::vector<vtkFrameOfReference::Element>
{
};

//--------------------------------------------------------------------------

// Constructor
vtkFrameOfReference::vtkFrameOfReference()
{
  this->Contents = new Container;
}

// Destructor
vtkFrameOfReference::~vtkFrameOfReference()
{
  delete this->Contents;
}

// Add a relationship
void vtkFrameOfReference::SetRelationship(
  vtkFrameOfReference *other, vtkMatrix4x4 *mat)
{
  Container::iterator iter = this->Contents->begin();
  Container::iterator iend = this->Contents->end();

  // if there is already an entry, then replace it
  for (; iter != iend; ++iter)
    {
    if (iter->first == other)
      {
      if (mat)
        {
        iter->second = mat;
        }
      break;
      }
    }

  // otherwise, add a new entry to the end of the vector
  if (iter == iend && mat)
    {
    this->Contents->push_back(Element(other, mat));
    }

  // or, if mat is NULL, remove the element
  if (iter != iend && !mat)
    {
    this->Contents->erase(iter);
    }
}

// Get a relationship
vtkMatrix4x4 *vtkFrameOfReference::GetRelationship(
  vtkFrameOfReference *other)
{
  Container::iterator iter = this->Contents->begin();
  Container::iterator iend = this->Contents->end();

  // look for the entry
  for (; iter != iend; ++iter)
    {
    if (iter->first == other)
      {
      return iter->second;
      }
    }

  return 0;
}

// The required PrintSelf method
void vtkFrameOfReference::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  Container::iterator iter = this->Contents->begin();
  Container::iterator iend = this->Contents->end();

  // print all relationships
  for (; iter != iend; ++iter)
    {
    os << indent << "Relationships:\n";
    os << indent << "    "
       << iter->first.GetPointer() << " "
       << iter->second.GetPointer() << "\n";
    }
}
