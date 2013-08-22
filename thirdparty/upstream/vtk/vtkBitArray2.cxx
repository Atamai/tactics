/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitArray2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBitArray2.h"

#include "vtkBitArrayIterator.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
class vtkBitArrayLookup
{
public:
  vtkBitArrayLookup() : Rebuild(true)
    {
    this->ZeroArray = NULL;
    this->OneArray = NULL;
    }
  ~vtkBitArrayLookup()
    {
    if (this->ZeroArray)
      {
      this->ZeroArray->Delete();
      this->ZeroArray = NULL;
      }
    if (this->OneArray)
      {
      this->OneArray->Delete();
      this->OneArray = NULL;
      }
    }
  vtkIdList* ZeroArray;
  vtkIdList* OneArray;
  bool Rebuild;
};

vtkStandardNewMacro(vtkBitArray2);

//----------------------------------------------------------------------------
// Instantiate object.
vtkBitArray2::vtkBitArray2(vtkIdType numComp)
{
  this->NumberOfComponents = static_cast<int>(numComp < 1 ? 1 : numComp);
  this->Array = NULL;
  this->TupleSize = 3;
  this->Tuple = new double[this->TupleSize]; //used for conversion
  this->SaveUserArray = 0;
  this->Lookup = NULL;
}

//----------------------------------------------------------------------------
vtkBitArray2::~vtkBitArray2()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  delete [] this->Tuple;
  if (this->Lookup)
    {
    delete this->Lookup;
    }
}

//----------------------------------------------------------------------------
unsigned char *vtkBitArray2::WritePointer(vtkIdType id, vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  this->DataChanged();
  return this->Array + id/8;
}

//----------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The 
// array argument is a pointer to the data.  size is the size of 
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data 
// from the supplied array.
void vtkBitArray2::SetArray(unsigned char* array, vtkIdType size, int save)
{
  
  if ((this->Array) && (!this->SaveUserArray))
    {
      vtkDebugMacro (<< "Deleting the array...");
      delete [] this->Array;
    }
  else 
    {
      vtkDebugMacro (<<"Warning, array not deleted, but will point to new array.");
    }

  vtkDebugMacro(<<"Setting array to: " << array);

  this->Array = array;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Get the data at a particular index.
int vtkBitArray2::GetValue(vtkIdType id)
{
  if (this->Array[id/8]&(0x80 >> (id%8)))
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.
int vtkBitArray2::Allocate(vtkIdType sz, vtkIdType vtkNotUsed(ext))
{
  if ( sz > this->Size )
    {
    if (( this->Array != NULL ) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new unsigned char[(this->Size+7)/8]) == NULL )
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;
  this->DataChanged();

  return 1;
}

//----------------------------------------------------------------------------
// Release storage and reset array to initial state.
void vtkBitArray2::Initialize()
{
  if (( this->Array != NULL ) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
  this->Array = NULL;
  this->Size = 0;
  this->MaxId = -1;
  this->SaveUserArray = 0;
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Deep copy of another bit array.
void vtkBitArray2::DeepCopy(vtkDataArray *ia)
{
  // Do nothing on a NULL input.
  if (ia == NULL)
    {
    return;
    }

  this->DataChanged();
  
  if (ia->GetDataType() != VTK_BIT)
    {
    vtkIdType numTuples = ia->GetNumberOfTuples();
    this->NumberOfComponents = ia->GetNumberOfComponents();
    this->SetNumberOfTuples(numTuples);

    for (vtkIdType i = 0; i < numTuples; i = i+8)
      {
        this->SetTuple(i+7, ia->GetTuple(i));
        this->SetTuple(i+6, ia->GetTuple(i+1));
        this->SetTuple(i+5, ia->GetTuple(i+2));
        this->SetTuple(i+4, ia->GetTuple(i+3));
        this->SetTuple(i+3, ia->GetTuple(i+4));
        this->SetTuple(i+2, ia->GetTuple(i+5));
        this->SetTuple(i+1, ia->GetTuple(i+6));
        this->SetTuple(i, ia->GetTuple(i+7));
      }
    return;
    }

  if ( this != ia )
    {
    if ((this->Array) && (!this->SaveUserArray))
      {
      delete [] this->Array;
      }

    this->NumberOfComponents = ia->GetNumberOfComponents();
    this->MaxId = ia->GetMaxId();
    this->Size = ia->GetSize();
    this->SaveUserArray = 0;
    this->Array = new unsigned char[(this->Size+7)/8];
    memcpy(this->Array, static_cast<unsigned char*>(ia->GetVoidPointer(0)),
           static_cast<size_t>((this->Size+7)/8)*sizeof(unsigned char));
    }
}

//----------------------------------------------------------------------------
void vtkBitArray2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Array)
    {
    os << indent << "Array: " << this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

//----------------------------------------------------------------------------
// Private function does "reallocate". Sz is the number of "bits", and we
// can allocate only 8-bit bytes.
unsigned char *vtkBitArray2::ResizeAndExtend(vtkIdType sz)
{
  unsigned char *newArray;
  vtkIdType newSize;

  if ( sz > this->Size ) 
    {
    newSize = this->Size + sz;
    }
  else if (sz == this->Size)
    {
    return this->Array;
    }
  else 
    {
    newSize = sz;
    }

  if (newSize <= 0)
    {
    this->Initialize();
    return 0;
    }

  if ( (newArray = new unsigned char[(newSize+7)/8]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Array)
    {
    vtkIdType usedSize = (sz < this->Size) ? sz : this->Size;

    memcpy(newArray, this->Array, 
         static_cast<size_t>((usedSize+7)/8)*sizeof(unsigned char));
    if (!this->SaveUserArray)
      {
        delete[] this->Array;
      }
    }

  if (newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;
  this->DataChanged();
  
  return this->Array;
}

//----------------------------------------------------------------------------
int vtkBitArray2::Resize(vtkIdType sz)
{
  unsigned char *newArray;
  vtkIdType newSize = sz*this->NumberOfComponents;

  if (newSize == this->Size)
    {
    return 1;
    }

  if (newSize <= 0)
    {
    this->Initialize();
    return 1;
    }

  if ( (newArray = new unsigned char[(newSize+7)/8]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Array)
    {
    vtkIdType usedSize = (newSize < this->Size) ? newSize : this->Size;

    memcpy(newArray, this->Array, 
           static_cast<size_t>((usedSize+7)/8)*sizeof(unsigned char));
    if (!this->SaveUserArray)
      {
        delete[] this->Array;
      }
    }

  if (newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;
  this->DataChanged();

  return 1;
}

//----------------------------------------------------------------------------
// Set the number of n-tuples in the array.
void vtkBitArray2::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
}

//----------------------------------------------------------------------------
// Description:
// Set the tuple at the ith location using the jth tuple in the source array.
// This method assumes that the two arrays have the same type
// and structure. Note that range checking and memory allocation is not 
// performed; use in conjunction with SetNumberOfTuples() to allocate space.
void vtkBitArray2::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkBitArray2* ba = vtkBitArray2::SafeDownCast(source);
  if (!ba)
    {
    vtkWarningMacro("Input and output arrays types do not match.");
    return;
    }
  
  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * ba->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->SetValue(loci + cur, ba->GetValue(locj + cur));
    }
  this->DataChanged();
}


//----------------------------------------------------------------------------
// Description:
// Insert the jth tuple in the source array, at ith location in this array. 
// Note that memory allocation is performed as necessary to hold the data.
void vtkBitArray2::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkBitArray2* ba = vtkBitArray2::SafeDownCast(source);
  if (!ba)
    {
    vtkWarningMacro("Input and output arrays types do not match.");
    return;
    }
  
  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * ba->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->InsertValue(loci + cur, ba->GetValue(locj + cur));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Description:
// Insert the jth tuple in the source array, at the end in this array. 
// Note that memory allocation is performed as necessary to hold the data.
// Returns the location at which the data was inserted.
vtkIdType vtkBitArray2::InsertNextTuple(vtkIdType j, vtkAbstractArray* source)
{
  vtkBitArray2* ba = vtkBitArray2::SafeDownCast(source);
  if (!ba)
    {
    vtkWarningMacro("Input and output arrays types do not match.");
    return -1;
    }
  
  vtkIdType locj = j * ba->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->InsertNextValue( ba->GetValue(locj + cur));
    }
  this->DataChanged();
  return (this->GetNumberOfTuples()-1);
}

//----------------------------------------------------------------------------
// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
double *vtkBitArray2::GetTuple(vtkIdType i)
{
  if ( this->TupleSize < this->NumberOfComponents )
    {
    this->TupleSize = this->NumberOfComponents;
    delete [] this->Tuple;
    this->Tuple = new double[this->TupleSize];
    }

  vtkIdType loc = this->NumberOfComponents*i;
  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->Tuple[j] = static_cast<double>(this->GetValue(loc+j));
    }

  return this->Tuple;
}

//----------------------------------------------------------------------------
// Copy the tuple value into a user-provided array.
void vtkBitArray2::GetTuple(vtkIdType i, double * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    tuple[j] = static_cast<double>(this->GetValue(loc+j));
    }
}

//----------------------------------------------------------------------------
// Set the tuple value at the ith location in the array.
void vtkBitArray2::SetTuple(vtkIdType i, const float * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,static_cast<int>(tuple[j]));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkBitArray2::SetTuple(vtkIdType i, const double * tuple)
{
  vtkIdType loc = i * this->NumberOfComponents; 

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->SetValue(loc+j,static_cast<int>(tuple[j]));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkBitArray2::InsertTuple(vtkIdType i, const float * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,static_cast<int>(tuple[j]));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkBitArray2::InsertTuple(vtkIdType i, const double * tuple)
{
  vtkIdType loc = this->NumberOfComponents*i;

  for (int j=0; j<this->NumberOfComponents; j++)
    {
    this->InsertValue(loc+j,static_cast<int>(tuple[j]));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple onto the end of the array.
vtkIdType vtkBitArray2::InsertNextTuple(const float * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue(static_cast<int>(tuple[i]));
    }

  this->DataChanged();
  return this->MaxId / this->NumberOfComponents;
}

//----------------------------------------------------------------------------
vtkIdType vtkBitArray2::InsertNextTuple(const double * tuple)
{
  for (int i=0; i<this->NumberOfComponents; i++)
    {
    this->InsertNextValue(static_cast<int>(tuple[i]));
    }

  this->DataChanged();
  return this->MaxId / this->NumberOfComponents;
}


//----------------------------------------------------------------------------
void vtkBitArray2::InsertComponent(vtkIdType i, int j, double c)
{
  this->InsertValue(i*this->NumberOfComponents + j, 
                    static_cast<int>(c));
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and 
// SetNumberOfComponents()).
void vtkBitArray2::SetComponent(vtkIdType i, int j, double c)
{
  this->SetValue(i*this->NumberOfComponents + j, static_cast<int>(c));
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkBitArray2::RemoveTuple(vtkIdType id)
{
  if ( id < 0 || id >= this->GetNumberOfTuples())
    {
    // Nothing to be done
    return;
    }
  if ( id == this->GetNumberOfTuples() - 1 )
    {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
    }
  this->DataChanged();
  vtkErrorMacro("Not yet implemented...");
}

//----------------------------------------------------------------------------
void vtkBitArray2::RemoveFirstTuple()
{
  this->RemoveFirstTuple();
  this->DataChanged();
}

//----------------------------------------------------------------------------
void vtkBitArray2::RemoveLastTuple()
{
  this->Resize(this->GetNumberOfTuples()- 1);
  this->DataChanged();
}

//----------------------------------------------------------------------------
vtkArrayIterator* vtkBitArray2::NewIterator()
{
  vtkArrayIterator* iter = vtkBitArrayIterator::New();
  iter->Initialize(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkBitArray2::UpdateLookup()
{
  if (!this->Lookup)
    {
    this->Lookup = new vtkBitArrayLookup();
    this->Lookup->ZeroArray = vtkIdList::New();
    this->Lookup->OneArray = vtkIdList::New();
    }
  if (this->Lookup->Rebuild)
    {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->ZeroArray->Allocate(numComps*numTuples);
    this->Lookup->OneArray->Allocate(numComps*numTuples);
    for (vtkIdType i = 0; i < numComps*numTuples; i++)
      {
      if (this->GetValue(i))
        {
        this->Lookup->OneArray->InsertNextId(i);
        }
      else
        {
        this->Lookup->ZeroArray->InsertNextId(i);
        }
      }
    this->Lookup->Rebuild = false;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkBitArray2::LookupValue(vtkVariant var)
{
  return this->LookupValue(var.ToInt());
}

//----------------------------------------------------------------------------
void vtkBitArray2::LookupValue(vtkVariant var, vtkIdList* ids)
{
  this->LookupValue(var.ToInt(), ids);
}

//----------------------------------------------------------------------------
vtkIdType vtkBitArray2::LookupValue(int value)
{
  this->UpdateLookup();
  
  if (value == 1 && this->Lookup->OneArray->GetNumberOfIds() > 0)
    {
    return this->Lookup->OneArray->GetId(0);
    }
  else if (value == 0 && this->Lookup->ZeroArray->GetNumberOfIds() > 0)
    {
    return this->Lookup->ZeroArray->GetId(0);
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkBitArray2::LookupValue(int value, vtkIdList* ids)
{
  this->UpdateLookup();
  
  if (value == 1)
    {
    ids->DeepCopy(this->Lookup->OneArray);
    }
  else if (value == 0)
    {
    ids->DeepCopy(this->Lookup->ZeroArray);
    }
  else
    {
    ids->Reset();
    }
}

//----------------------------------------------------------------------------
void vtkBitArray2::DataChanged()
{
  if (this->Lookup)
    {
    this->Lookup->Rebuild = true;
    }
}

//----------------------------------------------------------------------------
void vtkBitArray2::ClearLookup()
{
  if (this->Lookup)
    {
    delete this->Lookup;
    this->Lookup = NULL;
    }
}
