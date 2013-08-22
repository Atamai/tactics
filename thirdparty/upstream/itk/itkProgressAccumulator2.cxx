/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkProgressAccumulator2.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkProgressAccumulator2.h"

namespace itk {

ProgressAccumulator2
::ProgressAccumulator2()
{
  m_MiniPipelineFilter = 0;

  // Initialize the progress values
  this->ResetProgress();

  // Create a member command
  m_CallbackCommand = CommandType::New();
  m_CallbackCommand->SetCallbackFunction( this, & Self::ReportProgress );
}

ProgressAccumulator2
::~ProgressAccumulator2()
{
  UnregisterAllFilters();
}

void
ProgressAccumulator2
::RegisterInternalFilter(GenericFilterType *filter,float weight)
{
  // Observe the filter
  unsigned long progressTag = 
    filter->AddObserver(ProgressEvent(), m_CallbackCommand);
  unsigned long iterationTag = 
    filter->AddObserver(IterationEvent(), m_CallbackCommand);
  unsigned long startTag = 
    filter->AddObserver(StartEvent(),m_CallbackCommand);
  unsigned long endTag = 
    filter->AddObserver(EndEvent(),m_CallbackCommand);
  
  // Create a record for the filter
  struct FilterRecord record;
  record.Filter = filter;
  record.Weight = weight;
  record.ProgressObserverTag = progressTag;
  record.IterationObserverTag = iterationTag;
  record.StartObserverTag = startTag;
  record.EndObserverTag = endTag;
  record.Progress = 0.0f;

  // Add the record to the list
  m_FilterRecord.push_back(record);
  
  m_AccumulatedProgress = m_AccumulatedProgressBase;
}

void 
ProgressAccumulator2
::UnregisterAllFilters()
{
  // The filters should no longer be observing us
  FilterRecordVector::iterator it;
  for(it = m_FilterRecord.begin(); it != m_FilterRecord.end();++it)
    {
    it->Filter->RemoveObserver(it->ProgressObserverTag);
    it->Filter->RemoveObserver(it->IterationObserverTag);
    }

  // Clear the filter array
  m_FilterRecord.clear();

  // Reset the progress meter
  ResetProgress();
}

void 
ProgressAccumulator2
::ResetProgress()
{
  // Reset the accumulated progress
  m_AccumulatedProgress     = 0.0f;
  m_AccumulatedProgressBase = 0.0f;
  m_BaseAccumulatedProgress = 0.0f;
  
  // Reset each of the individial progress meters 
  FilterRecordVector::iterator it;
  for(it = m_FilterRecord.begin();it != m_FilterRecord.end();++it)
    {
    it->Progress = 0.0f;
    it->Filter->SetProgress( 0.0f );
    }
}

void 
ProgressAccumulator2
::ResetFilterProgressAndKeepAccumulatedProgress()
{
  m_BaseAccumulatedProgress = m_AccumulatedProgress;
  // Reset each of the individial progress meters 
  FilterRecordVector::iterator it;
  for(it = m_FilterRecord.begin();it != m_FilterRecord.end();++it)
    {
    it->Progress = 0.0f;
    it->Filter->SetProgress( 0.0f );
    }
}

void 
ProgressAccumulator2
::ReportProgress(Object *who, const EventObject &event)
{
  ProgressEvent  pe;
  IterationEvent ie;
  StartEvent se;
  EndEvent ee;
  if( typeid( event ) == typeid( pe ) )
  {
    // Add up the progress from different filters
    //m_AccumulatedProgress = m_BaseAccumulatedProgress;
    
    FilterRecordVector::iterator it;
    for(it = m_FilterRecord.begin();it != m_FilterRecord.end();++it)
    {
      if (it->Filter == who) {
        m_AccumulatedProgress = it->Filter->GetProgress() * it->Weight + m_AccumulatedProgressBase;
        //std::cout << "m_AccumulatedProress " << m_AccumulatedProgress << " " << it->Weight <<
        //" " << m_AccumulatedProgressBase << std::endl;
      }
    }
    
    if (m_MiniPipelineFilter) {
      // Update the progress of the client mini-pipeline filter
      m_MiniPipelineFilter->UpdateProgress(m_AccumulatedProgress);
      
      // check for abort
      if ( m_MiniPipelineFilter->GetAbortGenerateData() )
      {
        // Abort the filter that is reporting progress
        FilterRecordVector::iterator fit;
        for(fit = m_FilterRecord.begin();fit != m_FilterRecord.end();++fit)
        {
          if (who == fit->Filter)
          {
            fit->Filter->AbortGenerateDataOn();
          }
        }
      }
    }
    else {
      this->SetProgress(m_AccumulatedProgress);
      this->InvokeEvent(itk::ProgressEvent());
    }
  }
  else if (typeid( event ) == typeid ( se ) )
  {
    FilterRecordVector::iterator it;
    for(it = m_FilterRecord.begin(); it != m_FilterRecord.end();++it)
    {
      if (it->Filter == who) 
      {
        if (it == m_FilterRecord.begin())         
        {
          this->SetProgress(m_AccumulatedProgressBase);
          this->InvokeEvent(itk::StartEvent());
        }
        else {
          this->InvokeEvent(itk::ProgressEvent());
        }
      }
    }
  }
  else if (typeid( event ) == typeid ( ee ) )
  {
    FilterRecordVector::iterator it;
    for(it = m_FilterRecord.begin(); it != m_FilterRecord.end();++it)
    {
      if (it->Filter == who) 
      {
        if (it == m_FilterRecord.end())         
        {
          this->InvokeEvent(itk::EndEvent());
        }
        else {
          this->InvokeEvent(itk::ProgressEvent());
          m_AccumulatedProgressBase = m_AccumulatedProgress;
        }
      }
    }
  }
  else if (typeid( event ) == typeid ( ie ) )
  {
  }
}

void
ProgressAccumulator2
::RegisterEndEvent()
{
  m_AccumulatedProgress = 1;
  this->InvokeEvent(itk::EndEvent());
}
  
void ProgressAccumulator2
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  if (m_MiniPipelineFilter)
    {
    os << indent << m_MiniPipelineFilter << std::endl;
    }
  os << indent << m_AccumulatedProgress     << std::endl;
  os << indent << m_BaseAccumulatedProgress << std::endl;
}

} // End namespace itk
