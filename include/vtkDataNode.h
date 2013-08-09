/*=========================================================================
  Program: Cerebra
  Module:  vtkDataNode.h

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

#ifndef __vtkDataNode_h
#define __vtkDataNode_h

#include "vtkFrameOfReference.h"

#include <vtkObject.h>

class vtkDataObject;

//! A data node class, for storing data in the vtkDataManager.
/*!
 *  The vtkDataNode stores a vtkDataObject as well as information
 *  about the spatial coordinates of the data.  Every data node
 *  has a vtkFrameOfReference, and a vtkMatrix4x4 that provides
 *  the coordinate transformation from data coordinates to the
 *  frame-of-reference coordinates.
 *
 *  In general, the "Matrix" provided by the data node should be
 *  used as the VTK actor matrix when the data is displayed.
 *  Data objects should only be displayed in the same scene if
 *  they share their frame of reference.
 */  
class vtkDataNode : public vtkObject
{
public:
  vtkTypeMacro(vtkDataNode, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  //! Get the data object for the node.
  virtual vtkDataObject *GetDataObject() = 0;

  //! Get the coordinate transform matrix for the data.
  /*!
   *  This matrix provides the coordinate transformation from
   *  data coordinates to frame-of-reference coordinates.
   */
  vtkMatrix4x4 *GetMatrix()
    {
    return this->Matrix;
    }

  //! Copy the elements of the given matrix into the internal matrix.
  void SetMatrix(vtkMatrix4x4 *obj);

  //! Get the frame of reference for the data.
  /*!
   *  Every data node has a frame of reference.
   */
  vtkFrameOfReference *GetFrameOfReference()
    {
    return this->FrameOfReference;
    }

  //! Set the frame of reference for the data.
  void SetFrameOfReference(vtkFrameOfReference *ref);

protected:
  vtkDataNode();
  ~vtkDataNode();

  vtkMatrix4x4 *Matrix;
  vtkFrameOfReference *FrameOfReference;

private:
  vtkDataNode(const vtkDataNode&);  // Not implemented.
  void operator=(const vtkDataNode&);  // Not implemented.
};

#endif /* __vtkDataNode_h */
