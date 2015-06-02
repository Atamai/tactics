/*=========================================================================
  Program: Cerebra
  Module:  vtkDataManager.h

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

#ifndef __vtkDataManager_h
#define __vtkDataManager_h

#include <vtkObject.h>

class vtkDataNode;
class vtkImageNode;
class vtkSurfaceNode;
class vtkDataNodeCollection;
class vtkImageNodeCollection;

//! Class for storing VTK data, including vtkImageData, for an application
/*!
 *  The vtkDataManager class is meant to be the place where an application
 *  can centrally store any data that might have to be accessed by more
 *  that one component of the application.  Data is held in vtkDataNode
 *  objects, where each data node holds not only a vtkDataObject, but also
 *  some other information that we need, such as the spatial frame of
 *  reference for that data object and the 4x4 transformation matrix that
 *  relates the data to its frame of reference.
 */
class vtkDataManager : public vtkObject
{
public:
  static vtkDataManager *New();
  vtkTypeMacro(vtkDataManager, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  //! The key type for doing lookups in the data manager.
  class Key;

  //! A key type that can be associated with only one dataset.
  class UniqueKey;

  //! Add a data node, provide a key to access it by.
  void AddDataNode(vtkDataNode *node, const UniqueKey& key);

  //! Find the image node with the specified unique key.
  vtkImageNode *FindImageNode(const UniqueKey& key);

  //! Find the image node with the specified unique key.
  vtkSurfaceNode *FindSurfaceNode(const UniqueKey& key);

  //! Find the data node with the specified unique key.
  vtkDataNode *FindDataNode(const UniqueKey& key);

  //! Find all image nodes with the specified key. NOT IMPLEMENTED YET.
  void FindImageNodes(const Key& key, vtkImageNodeCollection *nodes);

  //! Find all data nodes with the specified key. NOT IMPLEMENTED YET.
  void FindDataNodes(const Key& key, vtkDataNodeCollection *nodes);

  //! The key type for doing lookups in the data manager.
  class Key
  {
  public:

    //! Create a new key.
    Key();

    //! Create a key that is a copy of another key.
    Key(const Key& other);

    //! Convert a vtkObject into a key.
    Key(vtkObjectBase *obj);

    //! Destructor.
    ~Key();

    //! Allow key variables to be assigned new values.
    Key &operator=(const Key &other);

    //! Inline equivalence operator.
    bool operator==(const Key &other) const
    {
      return (this->Id == other.Id &&
              this->ObjectId == other.ObjectId);
    }

    //! Inline comparison operator.
    bool operator<(const Key &other) const
    {
      return (this->Id < other.Id ||
              (this->Id == other.Id &&
               this->ObjectId < other.ObjectId));
    }

  private:
    long long Id;
    static volatile long long IdCounter;
    vtkObjectBase *ObjectId;
  };

  //! A key type that can be associated with only one dataset.
  class UniqueKey : public Key
  {
  public:
    //! Create a new UniqueKey.
    UniqueKey() : Key() {}
  };

protected:
  vtkDataManager();
  ~vtkDataManager();

  class Container;
  class Element;

  Container *Contents;

private:
  vtkDataManager(const vtkDataManager&);  // Not implemented.
  void operator=(const vtkDataManager&);  // Not implemented.
};

#endif /* __vtkDataManager_h */
