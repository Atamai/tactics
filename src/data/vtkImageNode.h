/*=========================================================================
  Program: Cerebra
  Module:  vtkImageNode.h

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

#ifndef __vtkImageNode_h
#define __vtkImageNode_h

#include "vtkDataNode.h"

#include <vtkImageProperty.h>
#include <string>

class vtkImageData;
class vtkDICOMMetaData;

//! A data node for images.
/*!
 *  The vtkImageNode is a node for image data.  In addition to the
 *  spatial information provided by the vtkDataNode (i.e. the Matrix
 *  and FrameOfReference), it also provides image-specific information.
 */
class vtkImageNode : public vtkDataNode
{
public:
  static vtkImageNode *New();
  vtkTypeMacro(vtkImageNode, vtkDataNode);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //! Get the image as a data object.
  virtual vtkDataObject *GetDataObject() override;

  //! Get the image from the node.
  vtkImageData *GetImage()
    {
    return this->Image;
    }

  //! Copy the image into the node, using a shallow copy.
  /*!
   *  When we set the data that is stored in the node, we want
   *  to make sure that the data we get is not at the end of a
   *  VTK pipeline, or else it will be overwritten the next time
   *  that the pipeline updates.  So, what we do is copy the data,
   *  but we do so via a shallow copy to save memory.
   */
  void ShallowCopyImage(vtkImageData *image);

  //! Get the meta data from the node.
  vtkDICOMMetaData *GetMetaData()
    {
    return this->MetaData;
    }

  //! Set the meta data into the node.
  void SetMetaData(vtkDICOMMetaData *mataData);

  //! Set the file URL for the image
  void SetFileURL(const char *url);

  //! Get the file URL for the image
  std::string GetFileURL() const;

protected:
  vtkImageNode();
  ~vtkImageNode();

  vtkImageData *Image;
  vtkDICOMMetaData *MetaData;

  std::string FileURL;

private:
  vtkImageNode(const vtkImageNode&);  // Not implemented.
  void operator=(const vtkImageNode&);  // Not implemented.
};

#endif /* __vtkImageNode_h */
