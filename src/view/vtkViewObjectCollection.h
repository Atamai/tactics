/*=========================================================================
  Program: Cerebra
  Module:  vtkViewObjectCollection.h

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

#ifndef VTKVIEWOBJECTCOLLECTION_H
#define VTKVIEWOBJECTCOLLECTION_H

#include "vtkCollection.h"
#include "vtkViewObject.h"

//! Simple wrapper class for vtkCollection.
/*!
 *  vtkViewObjectCollection is a simple wrapper class for a vtkCollection
 *  object that contains vtkViewObjects. Wrapping vtkCollection provides for
 *  much more readable code.
*/
class vtkViewObjectCollection : public vtkCollection
{
public:
 //! Provides a way to instantiate a vtkViewObjectCollection.
 static vtkViewObjectCollection *New();

 //! Standard VTK macro to allow for traversals of VTK class hierarchies.
 vtkTypeMacro(vtkViewObjectCollection, vtkCollection);

 //! Adds the vtkViewObject to the collection.
 /*!
  *  \param o The object to insert.
 */
 void AddItem(vtkViewObject *o) {
   this->vtkCollection::AddItem(o); }

 //! Retrieves the next item from the collection.
 vtkViewObject *GetNextItem() {
   return static_cast<vtkViewObject *>(this->GetNextItemAsObject()); }

 //! Retrieves the next item from the collection using an iterator.
 /*!
  *  \param c The iterator cookie.
 */
 vtkViewObject *GetNextViewObject(vtkCollectionSimpleIterator &c) {
   return static_cast<vtkViewObject *>(this->GetNextItemAsObject(c)); }

 //! Retrieves the vtkViewObject at index i.
 /*!
  *  \param i The index of the object to retrieve.
 */
 vtkViewObject *GetViewObject(int i) {
   return static_cast<vtkViewObject *>(this->GetItemAsObject(i)); }

protected:
 vtkViewObjectCollection() {};
 ~vtkViewObjectCollection() {};

private:
 //! Hide the AddItem method from the public.
 void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); }

 vtkViewObjectCollection(const vtkViewObjectCollection&);
 void operator=(const vtkViewObjectCollection&);
};

#endif /* end of include guard: VTKVIEWOBJECTCOLLECTION_H */
