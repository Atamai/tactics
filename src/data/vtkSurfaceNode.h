/*=========================================================================
  Program: Cerebra
  Module:  vtkSurfaceNode.h

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

#ifndef __vtkSurfaceNode_h
#define __vtkSurfaceNode_h

#include "vtkDataNode.h"

class vtkPolyData;

//! A data node for surfaces.
/*!
 *  The vtkSurfaceNode is a node for surface data.  In addition to the
 *  spatial information provided by the vtkDataNode (i.e. the Matrix
 *  and FrameOfReference), it also provides surface-specific information.
 */
class vtkSurfaceNode : public vtkDataNode
{
public:
  static vtkSurfaceNode *New();
  vtkTypeMacro(vtkSurfaceNode, vtkDataNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //! Get the surface as a data object.
  virtual vtkDataObject *GetDataObject() override;

  //! Get the surface as a vtkPolyData.
  vtkPolyData *GetSurface()
    {
    return this->Surface;
    }

  //! Copy a surface into the node, using a shallow copy.
  /*!
   *  When we set the data that is stored in the node, we want
   *  to make sure that the data we get is not at the end of a
   *  VTK pipeline, or else it will be overwritten the next time
   *  that the pipeline updates.  So, what we do is copy the data,
   *  but we do so via a shallow copy to save memory.
   */
  void ShallowCopySurface(vtkPolyData *surface);

protected:
  vtkSurfaceNode();
  ~vtkSurfaceNode();

  vtkPolyData *Surface;

private:
  vtkSurfaceNode(const vtkSurfaceNode&);  // Not implemented.
  void operator=(const vtkSurfaceNode&);  // Not implemented.
};

#endif /* __vtkSurfaceNode_h */
