/*=========================================================================
 
 Program:   Visualization Toolkit
 Module:    vtkProgressAccumulator.h
 
 Copyright (c) Lucy Qian Lu
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.
 
 =========================================================================*/
// .NAME vtkProgressAccumulator - abstract base class for most VTK objects
// .SECTION Description
// vtkObject is the base class for most objects in the visualization
// toolkit. vtkObject provides methods for tracking modification time,
// debugging, printing, and event callbacks. Most objects created
// within the VTK framework should be a subclass of vtkObject or one
// of its children.  The few exceptions tend to be very small helper
// classes that usually never get instantiated or situations where
// multiple inheritance gets in the way.  vtkObject also performs
// reference counting: objects that are reference counted exist as
// long as another object uses them. Once the last reference to a
// reference counted object is removed, the object will spontaneously
// destruct.

// .SECTION Caveats
// Note: in VTK objects should always be created with the New() method
// and deleted with the Delete() method. VTK objects cannot be
// allocated off the stack (i.e., automatic objects) because the
// constructor is a protected method.

// .SECTION See also
// vtkCommand vtkTimeStamp

#ifndef __vtkProgressAccumulator_h
#define __vtkProgressAccumulator_h

#include "vtkProcessObject.h"
#include <vector>

class vtkCallbackCommand;
class vtkObject;
class vtkAlgorithm;

class VTK_COMMON_EXPORT vtkProgressAccumulator : public vtkProcessObject
{
public:
  vtkTypeMacro(vtkProgressAccumulator,vtkProcessObject);
  
  // Description:
  static vtkProgressAccumulator *New();
  
  void PrintSelf(ostream& os, vtkIndent indent);

  void ResetProgress();
  void RegisterFilter(vtkAlgorithm *filter, float weight);
  void UnregisterAllFilters();
  
  vtkSetMacro(AccumulatedProgressBase,float);
  vtkGetMacro(AccumulatedProgressBase,float);
  
  vtkGetMacro(AccumulatedProgress,float);
  
  void RegisterEndEvent();
  
protected:
  vtkProgressAccumulator();
  virtual ~vtkProgressAccumulator();
  
private:
  struct ProgressData
  {
    vtkAlgorithm *Filter;
    float Weight;
    unsigned long ProgressTag;
    unsigned long IterationTag;
    unsigned long StartTag;
    unsigned long EndTag;
    float Progress;
  };
  
  static void FilterCallback(vtkObject *caller, unsigned long eventId, void* clientData, void* callData);
  
  void CallbackStart(vtkAlgorithm *filter);
  void CallbackProgress(vtkAlgorithm *filter, double progress);
  void CallbackEnd(vtkAlgorithm *filter);
  
  typedef std::vector<struct ProgressData> FilterRecordVector;
  FilterRecordVector FilterRecord;
  vtkAlgorithm *InternalFilter;
  
  float AccumulatedProgress;
  float AccumulatedProgressBase;
  
  vtkCallbackCommand *CallbackCommand;
  
  bool AccumulateEnd;
};

#endif 
  
  
