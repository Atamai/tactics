/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2012-2013 David Gobbi
  All rights reserved.
  See Copyright.txt or http://www.cognitive-antics.net/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkDICOMConfig_h
#define __vtkDICOMConfig_h

/* Configuration information. */
/* #undef DICOM_BUILD_SHARED_LIBS */
/* #undef DICOM_BUILD_TESTING */
#define DICOM_USE_GDCM

/* Version number. */
#define DICOM_MAJOR_VERSION 0
#define DICOM_MINOR_VERSION 1
#define DICOM_BUILD_VERSION 0
#define DICOM_SHORT_VERSION "0.1"
#define DICOM_VERSION "0.1.0"

#endif
