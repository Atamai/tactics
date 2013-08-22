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

#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProgressAccumulator);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkProgressAccumulator::vtkProgressAccumulator()
{
  InternalFilter = 0;
  AccumulatedProgress = 0.0f;
  AccumulatedProgressBase = 0.0f;
  AccumulateEnd = false;
  
  this->ResetProgress();
  
  CallbackCommand = vtkCallbackCommand::New();
  CallbackCommand->SetCallback(vtkProgressAccumulator::FilterCallback );
  CallbackCommand->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkProgressAccumulator::~vtkProgressAccumulator()
{
  this->UnregisterAllFilters();
  CallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AccumulatedProgress " << this->AccumulatedProgress << "\n";
  os << indent << "AccumulatedProgressBase " << this->AccumulatedProgressBase << "\n";
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::ResetProgress()
{
  AccumulatedProgress = 0.0f;
  AccumulatedProgressBase = 0.0f;
  FilterRecordVector::iterator it;
  for (it = FilterRecord.begin(); it != FilterRecord.end(); ++it) {
    it->Progress = 0.0f;
    it->Filter->SetProgress(0.0f);
  }
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::UnregisterAllFilters()
{
  // The filters should no longer be observing us
 /* FilterRecordVector::iterator it;
  for(it = FilterRecord.begin(); it != FilterRecord.end();++it)
  {
    it->Filter->RemoveObserver(vtkCommand::ProgressEvent);
  } */
  
  // Clear the filter array
  FilterRecord.clear();
  
  // Reset the progress meter
  this->ResetProgress();  
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::RegisterFilter(vtkAlgorithm *filter, float weight)
{
  ProgressData pd;
  pd.Filter = filter;
  pd.Weight = weight;

  // Register callbacks with the filter
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(&vtkProgressAccumulator::FilterCallback);
  cbc->SetClientData(this);
  pd.StartTag = filter->AddObserver(vtkCommand::StartEvent, cbc);
  pd.EndTag = filter->AddObserver(vtkCommand::EndEvent, cbc);
  pd.ProgressTag = filter->AddObserver(vtkCommand::ProgressEvent, cbc);
  cbc->Delete();
  
  FilterRecord.push_back(pd);
  
  AccumulatedProgress = AccumulatedProgressBase;
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::FilterCallback(vtkObject *caller, unsigned long eventId, void* clientData, void*)
{
  vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
  vtkProgressAccumulator *self = static_cast<vtkProgressAccumulator *>(clientData);
    
  // Call the appropriate handler
  if(eventId == vtkCommand::ProgressEvent) 
    self->CallbackProgress(filter,filter->GetProgress());
  else if(eventId == vtkCommand::EndEvent)
    self->CallbackEnd(filter);
  else if(eventId == vtkCommand::StartEvent)
    self->CallbackStart(filter);
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::CallbackStart(vtkAlgorithm *filter)
{
  FilterRecordVector::iterator it;
  for(it = FilterRecord.begin(); it != FilterRecord.end();++it)
  {
    if (it->Filter == filter) 
    {
      if (it == FilterRecord.begin())         
      {
        this->SetProgress(AccumulatedProgressBase);
        this->InvokeEvent(vtkCommand::StartEvent,CallbackCommand);
      }
      else {
        this->InvokeEvent(vtkCommand::ProgressEvent,CallbackCommand);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::CallbackProgress(vtkAlgorithm *filter, double progress)
{
  // Accumulate progress event  
  FilterRecordVector::iterator it;
  for(it = FilterRecord.begin(); it != FilterRecord.end();++it)
  {
    if (it->Filter == filter) 
    {
      AccumulatedProgress = progress * it->Weight + AccumulatedProgressBase;
      this->SetProgress(AccumulatedProgress);      
      this->InvokeEvent(vtkCommand::ProgressEvent,CallbackCommand);
    }
  }
}

//----------------------------------------------------------------------------
void vtkProgressAccumulator::CallbackEnd(vtkAlgorithm *filter)
{
  FilterRecordVector::iterator it;  
  for(it = FilterRecord.begin(); it != FilterRecord.end();++it)
  {
    if (it->Filter == filter) 
    {
      if (it == FilterRecord.end())         
      {
        this->InvokeEvent(vtkCommand::EndEvent,CallbackCommand);
      }
      else {
        this->InvokeEvent(vtkCommand::ProgressEvent,CallbackCommand);
        AccumulatedProgressBase = AccumulatedProgress;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkProgressAccumulator::RegisterEndEvent()
{
  cerr << "Force Program End" << endl;
  AccumulatedProgress = 1;
  this->InvokeEvent(vtkCommand::EndEvent,CallbackCommand);  
}