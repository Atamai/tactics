/*=========================================================================

  Program:   ToolCursor
  Module:    vtkROIContourData.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkROIContourData - A series of contours for an image.
// .SECTION Description
// This class defines a set of contours for an image ROI.
// The result is the equivalent of a mask image, but described via 2D
// geometry rather than via pixel values.  The contours are meant to
// be associated with an image, so they only lie on the slices of the
// image.

#ifndef __vtkROIContourData_h
#define __vtkROIContourData_h

#include "vtkDataObject.h"

class vtkPoints;
class vtkIdList;
class vtkROIContourVector;

class VTK_EXPORT vtkROIContourData : public vtkDataObject
{
public:
  // Description:
  // Instantiate the object.
  static vtkROIContourData *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkROIContourData,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the object by removing all contours.
  void Initialize();

  // Description:
  // Make a deep copy from another object, which means copying all of the
  // points from the old object to the new object.
  void DeepCopy(vtkDataObject *o);

  // Description:
  // Make a shallow copy from another object, which means referencing the
  // points in the old object without copying them.
  void ShallowCopy(vtkDataObject *o);

  // Description:
  // The various types of contours that are allowed.
  enum {
    POINT,
    OPEN_PLANAR,
    OPEN_NONPLANAR,
    CLOSED_PLANAR,
  };

  // Description:
  // Return what type of data this is.  Since it doesn't have
  // an entry in vtkType.h, just call it "VTK_DATA_OBJECT."
  int GetDataObjectType() { return VTK_DATA_OBJECT; }

  // Description:
  // The number of contours.
  void SetNumberOfContours(int n);
  int GetNumberOfContours() { return this->NumberOfContours; }

  // Description:
  // The points that define a contour.  These might be NULL.
  void SetContourPoints(int contour, vtkPoints *points);
  vtkPoints *GetContourPoints(int contour);

  // Description:
  // The type of contour.
  void SetContourType(int contour, int t);
  int GetContourType(int contour);

  // Description:
  // Remove a contour.  This will cause the numbering of the contours to change,
  // if the removed contour is not the last contour.
  void RemoveContour(int contour);

protected:
  vtkROIContourData();
  ~vtkROIContourData();

  int NumberOfContours;
  vtkROIContourVector *Contours;

private:
  vtkROIContourData(const vtkROIContourData&);  //Not implemented
  void operator=(const vtkROIContourData&);  //Not implemented
};

#endif
