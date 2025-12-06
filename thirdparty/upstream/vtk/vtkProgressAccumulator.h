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

#ifndef vtkProgressAccumulator_h
#define vtkProgressAccumulator_h

#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vector>

class vtkCallbackCommand;
class vtkAlgorithm;

class vtkProgressAccumulator : public vtkObject
{
public:
  static vtkProgressAccumulator* New();
  vtkTypeMacro(vtkProgressAccumulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Reset global and per-filter progress
  void ResetProgress();

  // Add a filter with a relative weight (must be called before execution)
  void RegisterFilter(vtkAlgorithm* filter, float weight = 1.0f);

  // Remove all filter records
  void UnregisterAllFilters();

  // Base progress offset (useful when accumulating stages)
  vtkSetMacro(AccumulatedProgressBase, float);
  vtkGetMacro(AccumulatedProgressBase, float);

  // Total accumulated progress [0–1]
  vtkGetMacro(AccumulatedProgress, float);

  // If enabled, emit an EndEvent() when the last filter completes
  vtkSetMacro(EmitEndEvent, bool);
  vtkGetMacro(EmitEndEvent, bool);

protected:
  vtkProgressAccumulator();
  ~vtkProgressAccumulator() override;

private:
  struct FilterRecordEntry
  {
    vtkAlgorithm* Filter = nullptr;
    float Weight = 1.0f;
    float CurrentProgress = 0.0f;

    unsigned long ProgressTag = 0;
  };

  static void FilterProgressCallback(
      vtkObject* caller, unsigned long eventId,
      void* clientData, void* callData);

  void OnFilterProgress(vtkAlgorithm* filter, double progress);

  // Internal records of child filters
  std::vector<FilterRecordEntry> Records;

  vtkSmartPointer<vtkCallbackCommand> ProgressCallbackCommand;

  float AccumulatedProgress = 0.0f;
  float AccumulatedProgressBase = 0.0f;
  bool EmitEndEvent = true;
};

#endif
