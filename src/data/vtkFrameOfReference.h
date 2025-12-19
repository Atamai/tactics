/*=========================================================================
  Program: Cerebra
  Module:  vtkFrameOfReference.h

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

#ifndef __vtkFrameOfReference_h
#define __vtkFrameOfReference_h

#include <vtkObject.h>

class vtkMatrix4x4;

//! A frame of reference defines a coordinate system.
/*!
 *  When two data sets are loaded from two different files,
 *  it is generally unknown if they share a coordinate system.
 *  What we want to do, then, is create a unique FrameOfReference
 *  object for each data set that is loaded.  The fact that each
 *  data object has its own frame of reference indicates that we
 *  have to register the data sets to each other in order to
 *  establish the the coordinate transformation between their
 *  frames of reference.  This class provides a means to identify
 *  which frame of reference a data set uses.
 */
class vtkFrameOfReference : public vtkObject
{
public:
  static vtkFrameOfReference *New();
  vtkTypeMacro(vtkFrameOfReference, vtkObjectBase);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //! Set the relationship to another frame of reference.
  /*!
   *  This sets the coordianate transformation from this frame of
   *  reference, to another frame of reference.
   */
  void SetRelationship(vtkFrameOfReference *other, vtkMatrix4x4 *mat);

  //! Get the relationship with another frame of reference.
  /*!
   *  This returns the coordinate transformation from this frame
   *  of reference to another frame of reference, or returns NULL
   *  if no relationship exists between the frames of reference.
   */
  vtkMatrix4x4 *GetRelationship(vtkFrameOfReference *other);

protected:
  vtkFrameOfReference();
  ~vtkFrameOfReference();

  class Container;
  class Element;

  Container *Contents;

private:
  vtkFrameOfReference(const vtkFrameOfReference&);  // Not implemented.
  void operator=(const vtkFrameOfReference&);  // Not implemented.
};

#endif /* __vtkFrameOfReference_h */
