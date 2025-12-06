/*=========================================================================
 
 Program:   Visualization Toolkit
 Module:    vtkProgressAccumulator.cxx
 
 Copyright (c) Lucy Qian Lu
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.
 
 =========================================================================*/
#include "vtkProgressAccumulator.h"

#include <vtkAlgorithm.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkProgressAccumulator);

vtkProgressAccumulator::vtkProgressAccumulator()
{
  this->ProgressCallbackCommand = vtkSmartPointer<vtkCallbackCommand>::New();
  this->ProgressCallbackCommand->SetCallback(
      &vtkProgressAccumulator::FilterProgressCallback);
  this->ProgressCallbackCommand->SetClientData(this);

  this->ResetProgress();
}

vtkProgressAccumulator::~vtkProgressAccumulator()
{
  this->UnregisterAllFilters();
}

void vtkProgressAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AccumulatedProgress: " << this->AccumulatedProgress << "\n";
  os << indent << "AccumulatedProgressBase: " << this->AccumulatedProgressBase << "\n";
  os << indent << "EmitEndEvent: " << this->EmitEndEvent << "\n";
}

void vtkProgressAccumulator::ResetProgress()
{
  this->AccumulatedProgress = this->AccumulatedProgressBase;

  for (auto& rec : this->Records)
  {
    rec.CurrentProgress = 0.0f;
  }
}

void vtkProgressAccumulator::UnregisterAllFilters()
{
  for (auto& rec : this->Records)
  {
    if (rec.Filter)
    {
      rec.Filter->RemoveObserver(rec.ProgressTag);
    }
  }

  this->Records.clear();
  this->ResetProgress();
}

void vtkProgressAccumulator::RegisterFilter(vtkAlgorithm* filter, float weight)
{
  if (!filter)
  {
    vtkErrorMacro("Attempt to register nullptr filter!");
    return;
  }

  FilterRecordEntry rec;
  rec.Filter = filter;
  rec.Weight = weight;

  rec.ProgressTag =
      filter->AddObserver(vtkCommand::ProgressEvent,
                          this->ProgressCallbackCommand);

  this->Records.push_back(rec);

  this->ResetProgress();
}

void vtkProgressAccumulator::FilterProgressCallback(
    vtkObject* caller, unsigned long eventId,
    void* clientData, void* callData)
{
  if (eventId != vtkCommand::ProgressEvent)
    return;

  auto* filter = vtkAlgorithm::SafeDownCast(caller);
  if (!filter)
    return;

  auto* self = static_cast<vtkProgressAccumulator*>(clientData);

  double progress =
      (callData ? *static_cast<double*>(callData) : filter->GetProgress());

  self->OnFilterProgress(filter, progress);
}

void vtkProgressAccumulator::OnFilterProgress(vtkAlgorithm* filter, double progress)
{
  float totalWeight = 0.0f;
  float weightedSum = 0.0f;

  for (auto& rec : this->Records)
  {
    if (rec.Filter == filter)
    {
      rec.CurrentProgress = static_cast<float>(progress);
    }

    weightedSum += rec.CurrentProgress * rec.Weight;
    totalWeight += rec.Weight;
  }

  if (totalWeight <= 0.0f)
    return;

  this->AccumulatedProgress =
      this->AccumulatedProgressBase + weightedSum / totalWeight;

  this->InvokeEvent(vtkCommand::ProgressEvent, &this->AccumulatedProgress);

  // Check if all filters have completed
  const bool allDone = std::all_of(
      this->Records.begin(), this->Records.end(),
      [](const FilterRecordEntry& r) { return r.CurrentProgress >= 1.0f; });

  if (allDone && this->EmitEndEvent)
  {
    float endVal = 1.0f;
    this->InvokeEvent(vtkCommand::EndEvent, &endVal);
  }
}
