/*=========================================================================
  Program: Cerebra
  Module:  cbProgressBar.h

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

#ifndef CBPROGRESSBAR_H
#define CBPROGRESSBAR_H

#include <qprogressbar.h>
#include "itkProcessObject.h"
#include "itkCommand.h"

class QLabel;
class QObject;
class QString;
class QWidget;
class itkObject;
class vtkCallbackCommand;
class vtkObject;

//! Convert ITK, VTK progress event to QProgressBar 
/*! 
 *  cbProgressBar receive start, end and progress event from either ITK event 
 *  object or VTK event object, then set callback to QProgressBar. 
 */
class cbProgressBar : public ::QProgressBar
{

   Q_OBJECT
  
public:
  cbProgressBar(QWidget *p);
  ~cbProgressBar();
  
  typedef itk::MemberCommand<cbProgressBar> itkCommandType;
  
  //! Manage VTK start event
  static void StartEventVTK(vtkObject *caller, unsigned long eventId, 
                            void* clientData, void* callData);
  
  //! Manage VTK end event
  static void EndEventVTK(vtkObject *caller, unsigned long eventId, 
                          void* clientData, void* callData);
  
  //! Manage VTK progress event
  static void ProgressEventVTK(vtkObject *caller, unsigned long eventId, 
                               void* clientData, void* callData);
  
  //! Set an observer for VTK event
  void Observe(vtkObject *caller);

  //! Manage ITK start event
  void StartEventITK(itk::Object * caller, 
                     const itk::EventObject & event);
  
  //! Manage ITK end event
  void EndEventITK(itk::Object * caller, 
                   const itk::EventObject & event);
  
  //! Manage ITK progress event
  void ProgressEventITK(itk::Object * caller, 
                        const itk::EventObject & event);
  
  //! Set an observer for ITK event
  void Observe(itk::Object *caller);
  
  //! Send progress label to cbStatusBar
  void SetTextWidget(QLabel *textWidget);
  
private:
  void CallbackProgress(double progress);
  void CallbackStart();
  void CallbackEnd();
  vtkCallbackCommand *m_ProgressCommandVTK;
  vtkCallbackCommand *m_StartCommandVTK;
  vtkCallbackCommand *m_EndCommandVTK;
  itkCommandType::Pointer m_StartCommandITK;
  itkCommandType::Pointer m_EndCommandITK;
  itkCommandType::Pointer m_ProgressCommandITK; 
  
  QString m_text;
  QLabel *m_TextWidget;
};

#endif // CBPROGRESSBAR_H

