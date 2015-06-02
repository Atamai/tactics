/*=========================================================================
  Program: Cerebra
  Module:  cbProgressBar.cxx

  Copyright (c) 2011-2013 Qian Lu
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of the Calgary Image Processing and Analysis Centre
    (CIPAC), the University of Calgary, nor the names of any authors nor
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "cbProgressBar.h"

// QT includes
#include <QApplication>
#include <QLabel>
#include <QObject>
#include <QTimer>
#include <QWidget>

// VTK includes
#include "vtkAlgorithm.h"
#include "vtkCallbackCommand.h"
#include "vtkObject.h"

// ITK includes
#include "itkCommand.h"
#include "itkObject.h"

//----------------------------------------------------------------------------
// constructor
cbProgressBar::cbProgressBar( QWidget *p):QProgressBar(p)
{
  m_StartCommandVTK = vtkCallbackCommand::New();
  m_StartCommandVTK->SetCallback(cbProgressBar::StartEventVTK);
  m_StartCommandVTK->SetClientData(this);
  
  m_EndCommandVTK = vtkCallbackCommand::New();
  m_EndCommandVTK->SetCallback(cbProgressBar::EndEventVTK);
  m_EndCommandVTK->SetClientData(this);
  
  m_ProgressCommandVTK = vtkCallbackCommand::New();
  m_ProgressCommandVTK->SetCallback(cbProgressBar::ProgressEventVTK);
  m_ProgressCommandVTK->SetClientData(this);
  
  m_StartCommandITK = itkCommandType::New();
  m_StartCommandITK->SetCallbackFunction(this,&cbProgressBar::StartEventITK);
  
  m_EndCommandITK = itkCommandType::New();
  m_EndCommandITK->SetCallbackFunction(this,&cbProgressBar::EndEventITK);
  
  m_ProgressCommandITK = itkCommandType::New();
  m_ProgressCommandITK->SetCallbackFunction(this,
                                            &cbProgressBar::ProgressEventITK); 
  
  this->setMaximumWidth(450);
  this->setMinimum(0);
  this->setMaximum(100);
  this->setTextVisible(false);
  this->reset();
  //this->hide();
  
  m_TextWidget = 0;
}

// destructor
cbProgressBar::~cbProgressBar()
{
  
}

//----------------------------------------------------------------------------
void cbProgressBar::SetTextWidget(QLabel *textWidget)
{
  m_TextWidget = new QLabel;
  m_TextWidget = textWidget;
}

//----------------------------------------------------------------------------
void cbProgressBar::StartEventVTK(vtkObject *caller, unsigned long, 
                                  void* clientData, void*)
{
  vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
  cbProgressBar *self = static_cast<cbProgressBar *>(clientData);
  cerr << filter->GetClassName() << " is now start progress. " << endl;
  self->CallbackStart();
}

void cbProgressBar::EndEventVTK(vtkObject *caller, unsigned long, 
                                void* clientData, void*)
{
  vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
  cbProgressBar *self = static_cast<cbProgressBar *>(clientData);
  cerr << filter->GetClassName() << " is now end progress. " << endl;
  self->CallbackEnd();
}

void cbProgressBar::ProgressEventVTK(vtkObject *caller, unsigned long, 
                                     void* clientData, void*)
{
  vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
  cbProgressBar *self = static_cast<cbProgressBar *>(clientData);
  const double val = static_cast<double> ((filter->GetProgress())*100);  
  self->CallbackProgress(val);
}

void cbProgressBar::Observe(vtkObject *caller )
{
  caller->AddObserver(vtkCommand::StartEvent,m_StartCommandVTK);
  caller->AddObserver(vtkCommand::ProgressEvent, m_ProgressCommandVTK);
  caller->AddObserver(vtkCommand::EndEvent,m_EndCommandVTK);
}

//----------------------------------------------------------------------------
void cbProgressBar::StartEventITK( itk::Object * caller, 
                                  const itk::EventObject & e )
{
  if( typeid( itk::StartEvent )   ==  typeid( e ) )
  {
    ::itk::ProcessObject::Pointer  process = 
    dynamic_cast< itk::ProcessObject *>( caller );
    this->CallbackStart();
  }
}

void cbProgressBar::EndEventITK( itk::Object * caller, 
                                const itk::EventObject & e )
{
  if( typeid( itk::EndEvent )   ==  typeid( e ) )
    {
    ::itk::ProcessObject::Pointer  process = 
    dynamic_cast< itk::ProcessObject *>( caller );
    this->CallbackEnd();
    }
}

void cbProgressBar::ProgressEventITK( itk::Object * caller, 
                                     const itk::EventObject & e )
{
  if( typeid( itk::ProgressEvent )   ==  typeid( e ) )
  {
    ::itk::ProcessObject::Pointer  process = 
    dynamic_cast< itk::ProcessObject *>( caller );
    
    const double val = static_cast<double>(process->GetProgress() * 100 );    
    this->CallbackProgress(val);
  }
}

void cbProgressBar::Observe(itk::Object *caller)
{
  caller->AddObserver(itk::StartEvent(), m_StartCommandITK.GetPointer());
  caller->AddObserver(itk::ProgressEvent(), m_ProgressCommandITK.GetPointer());
  caller->AddObserver(itk::EndEvent(), m_EndCommandITK.GetPointer());
}

//----------------------------------------------------------------------------
void cbProgressBar::CallbackProgress(double progress)
{
  //cerr << "Current Process Value " << progress << endl;
  this->setValue(progress);
  //this->setFormat("Your text here. "+QString::number(progress)+"%");
  m_text = this->text();
  if (m_TextWidget != 0) {
    m_TextWidget->setText(m_text);
  }
  QApplication::processEvents();
}

//----------------------------------------------------------------------------
void cbProgressBar::CallbackStart()
{
  this->show();
  this->setValue(0); 
  if (m_TextWidget != 0) {
    m_TextWidget->show();
  }
}

//----------------------------------------------------------------------------
void cbProgressBar::CallbackEnd()
{
  this->setValue(100); 
  if (m_TextWidget != 0) {
    m_TextWidget->setText("Done");
  }
  
#ifdef Q_WS_MAC
  QApplication::flush();
#endif
}
