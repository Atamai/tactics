/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2012-2013 David Gobbi
  All rights reserved.
  See Copyright.txt or http://www.cognitive-antics.net/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkDICOMDictEntry_h
#define __vtkDICOMDictEntry_h

#include "vtkDICOMModule.h"
#include "vtkDICOMVR.h"
#include "vtkDICOMVM.h"
#include "vtkDICOMTag.h"

//! An entry in the DICOM dictionary.
class VTK_DICOM_EXPORT vtkDICOMDictEntry
{
public:
  // The struct that actually stores the vtkDICOMDictEntry information.
  struct Entry
  {
    unsigned short Group;
    unsigned short Element;
    unsigned char  Owner;
    unsigned char  VR;
    unsigned short VM;
    const char    *Name;
  };

  //! Construct an invalid DictEntry object.
  vtkDICOMDictEntry() : I(&InvalidEntry) {}

  //! Check whether the returned entry is valid.
  bool IsValid() {
    return (this->I != &InvalidEntry); }

  //! Get the DICOM tag for this dictionary entry.
  vtkDICOMTag GetTag() {
    return vtkDICOMTag(this->I->Group, this->I->Element); }

  //! Get the VR for this dictionary entry.
  vtkDICOMVR GetVR() {
    return vtkDICOMVR(static_cast<vtkDICOMVR::EnumType>(this->I->VR)); }

  //! Get the VM for this dictionary entry.
  vtkDICOMVM GetVM() {
    return vtkDICOMVM(static_cast<vtkDICOMVM::EnumType>(this->I->VM)); }

  //! Get a human-readable name for this dictionary entry.
  const char *GetName() {
    return this->I->Name; }

  //! Check whether this entry has been retired from the DICOM standard.
  bool IsRetired() {
    return (this->I->Owner == 1); }

private:
  vtkDICOMDictEntry(const Entry *o) : I(o) {}

  friend class vtkDICOMDictionary;
  friend ostream& operator<<(ostream& o, vtkDICOMDictEntry a);

  const Entry *I;

  static const Entry InvalidEntry;
};

VTK_DICOM_EXPORT ostream& operator<<(ostream& o, vtkDICOMDictEntry a);

#endif /* __vtkDICOMDictEntry_h */
