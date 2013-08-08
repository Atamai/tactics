/*=========================================================================
  Program: Cerebra
  Module:  vtkImageViewPane.h

  Copyright (c) 2011-2013 David Adair
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

#ifndef VTKIMAGEVIEWPANE_H
#define VTKIMAGEVIEWPANE_H

#include "vtkViewPane.h"

#include <vector>
#include <utility>

class vtkImageData;
class vtkImageProperty;
class vtkImageSlice;
class vtkImageStack;
class vtkMatrix4x4;

//! Allows easy adding and removing of vtkImageData to an image stack.
class vtkImageViewPane : public vtkViewPane
{
 public:
  //! Provides a method to instantiate a vtkImageViewPane.
  static vtkImageViewPane *New();

  //! Standard VTK macro to enable object hierarchy traversal.
  vtkTypeMacro(vtkImageViewPane, vtkViewPane);

  //! Allows a user to add an image to the image stack.
  /*!
   *  \param data The image data to add.
   *  \param matrix The transformation matrix of the image data.
   *  \param property The image properties to add to the image.
   *  \return Returns a unique integer for the image.
   */
  int AddImage(vtkImageData *data, vtkMatrix4x4 *matrix, vtkImageProperty *property);

  //! Allows a user to remove an image from the image stack.
  /*!
   *  \param id The unique ID of the image to remove from the stack.
   */
  void RemoveImage(int id);

  //! Allows a user to retrieve the image slice from the image stack.
  /*!
   *  \param id The unique ID for that imagine (see return of AddImage(..))
   */
  vtkImageSlice *GetImageSlice(int id);
  void ClearImageStack();

 protected:
  vtkImageViewPane();
  ~vtkImageViewPane();

 private:
  //! Underlying vtkImageStack for the render scene.
  vtkImageStack *ImageStack;

  // Table of images in the image stack.
  // Position in the vector signifies layer in the stack.
  // [n].first == unique ID
  // [n].second == image slice
  std::vector< std::pair<int, vtkImageSlice *> > Table;

  int IDProvider;
};

#endif /* end of include guard: VTKIMAGEVIEWPANE_H */
