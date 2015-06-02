/*=========================================================================

  Program:   ToolCursor
  Module:    vtkCursorShapes.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCursorShapes.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkDataSet.h"
#include "vtkSmartPointer.h"

#include <string>
#include <vector>

vtkStandardNewMacro(vtkCursorShapes);

// A simple container for cursor data
class vtkToolCursorShape
{
public:
  vtkToolCursorShape(const char *name,
                        vtkDataSet *data,
                        int flags) : Name(name), Data(data), Flags(flags) {};

  std::string Name;
  vtkSmartPointer<vtkDataSet> Data;
  int Flags;
};

// A vector of the above, with a VTK-like interface
class vtkToolCursorShapeArray
{
private:
  typedef std::vector<vtkToolCursorShape> VectorType;
  VectorType Vector;

public:
  static vtkToolCursorShapeArray *New() {
    return new vtkToolCursorShapeArray; };

  void Delete() {
    delete this; };

  void AddItem(const char *name, vtkDataSet *shape, int flags) {
    this->Vector.push_back(vtkToolCursorShape(name, shape, flags)); };

  int GetIndex(const char *name) {
    if (name) {
      for (VectorType::size_type i = 0; i < this->Vector.size(); i++) {
        if (this->Vector[i].Name.compare(name) == 0) {
          return static_cast<int>(i); } } } return -1; };

  const char *GetName(int i) {
    VectorType::size_type j = static_cast<VectorType::size_type>(i);
    if (i < 0 || j >= this->Vector.size()) { return 0; }
    else { return this->Vector[j].Name.c_str(); } };

  vtkDataSet *GetData(int i) {
    VectorType::size_type j = static_cast<VectorType::size_type>(i);
    if (i < 0 || j >= this->Vector.size()) { return 0; }
    else {return this->Vector[j].Data; } };

  int GetFlags(int i) {
    VectorType::size_type j = static_cast<VectorType::size_type>(i);
    if (i < 0 || j >= this->Vector.size()) { return 0; }
    else {return this->Vector[j].Flags; } };
};

//----------------------------------------------------------------------------
vtkCursorShapes::vtkCursorShapes()
{
  this->NumberOfShapes = 0;
  this->Shapes = vtkToolCursorShapeArray::New();
}

//----------------------------------------------------------------------------
vtkCursorShapes::~vtkCursorShapes()
{
  this->Shapes->Delete();
}

//----------------------------------------------------------------------------
void vtkCursorShapes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfShapes: " << this->NumberOfShapes << "\n";
}

//----------------------------------------------------------------------------
int vtkCursorShapes::AddShape(const char *name, vtkDataSet *data,
                                     int flags)
{
  this->Shapes->AddItem(name, data, flags);

  this->NumberOfShapes++;

  return (this->NumberOfShapes - 1);
}

//----------------------------------------------------------------------------
int vtkCursorShapes::GetShapeIndex(const char *name)
{
  return this->Shapes->GetIndex(name);
}

//----------------------------------------------------------------------------
const char *vtkCursorShapes::GetShapeName(int i)
{
  return this->Shapes->GetName(i);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkCursorShapes::GetShapeData(int i)
{
  return this->Shapes->GetData(i);
}

//----------------------------------------------------------------------------
int vtkCursorShapes::GetShapeFlags(int i)
{
  return this->Shapes->GetFlags(i);
}

