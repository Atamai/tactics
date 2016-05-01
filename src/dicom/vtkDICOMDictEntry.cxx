/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2012-2015 David Gobbi
  All rights reserved.
  See Copyright.txt or http://dgobbi.github.io/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDICOMDictEntry.h"

const vtkDICOMDictEntry::Entry vtkDICOMDictEntry::InvalidEntry = {
  0, 0, 0, 0, 0, "" };

ostream& operator<<(ostream& o, const vtkDICOMDictEntry& a)
{
  if (!a.IsValid())
    {
    o << "INVALID";
    }
  else
    {
    o << a.GetTag() << "," << a.GetVR() << "," << a.GetVM() << ","
      << "\"" << a.GetName() << "\"";
    }

  return o;
}
