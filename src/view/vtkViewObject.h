/*=========================================================================
  Program: Cerebra
  Module:  vtkViewObject.h

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

#ifndef VTKVIEWOBJECT_H
#define VTKVIEWOBJECT_H

#include <vtkObject.h>

class vtkViewObjectCollection;

//! Base class for all view-layout oriented classes.
/*!
 *  This class acts a base class for all view-layout oriented classes,
 *  such as vtkViewRect, vtkViewFrame, and vtkViewPane. It provides a
 *  general interface to each object's viewport.
*/
class vtkViewObject : public vtkObject
{
public:
 //! Standard VTK macro that allows traversal of object inheritence
 //hierarchies.
 vtkTypeMacro(vtkViewObject, vtkObject);

 //! Simple const getter method for the object's viewport.
 /*!
  *  \param out The output copy of the object's viewport.
 */
 void GetViewport(double out[4]) const;

 //! Simple setter method for the object's viewport.
 /*!
  *  \param newView The view to copy into the object's viewport.
 */
 void SetViewport(const double newView[4]);

 //! Set the stretch factor relative to siblings.
 /*!
  *  The default value is one.  An item with a stretch factor of
  *  two will be twice the size of an item with a stretch factor
  *  of one.  The stretch factor is ignored by the grid layout.
  *
  *  \param stretch The stretch factor.
  */
 void SetStretchFactor(double stretch) {
   this->StretchFactor = stretch; }

 //! Get the stretch factor relative to siblings.
 double GetStretchFactor() const {
   return this->StretchFactor; }

protected:
 vtkViewObject();
 ~vtkViewObject();

 //! Protected member that contains the object's viewport boundaries.
 double View[4];

 //! Protected member that contains the object's stretch factor.
 double StretchFactor;

private:
 vtkViewObject(const vtkViewObject&); // Not implemented.
 void operator=(const vtkViewObject&); // Not implemented.
};

#endif /* end of include guard: VTKVIEWOBJECT_H */
