/*=========================================================================
  Program: Cerebra
  Module:  cbStage.h

  Copyright (c) 2011-2013 Qian Lu, David Adair
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

#ifndef CBSTAGE_H
#define CBSTAGE_H

#include <QObject>

class QWidget;

//! Pure abstract class for representing a pipeline stage.
class cbStage : public QObject
{
  Q_OBJECT
public:
  cbStage();
  virtual ~cbStage();

  //! Returns the stage's ID. Useful for debugging sidebars.
  int ID() const { return id; }

  //! Returns the stage's name. ie: 'Open Study Data'.
  virtual const char *getStageName() const = 0;

  //! Returns the stage's internal sidebar representation.
  virtual QWidget *Widget();

public slots:
  //! Execution functionality for the stage.
  /*!
   *  The Execute function is called internally by the stage, if the stage
   *  requires execute functionality (ie, cbIntroductionStage has no
   *  execution, simply a description of the pipeline). The function can
   *  gather any necessary data from the user (through dialogs) or through
   *  the stage's sidebar's widgets, and request that processing be
   *  performed. The connection between the request and the controller that 
   *  fulfills the request is done in the 'main' file for the application.
  */
  virtual void Execute() = 0;

signals:
  //! Signals that the execute function has completed.
  void finished();

protected:
  //! QWidget representation of the stage. Has necessary widgets and desc.
  QWidget *widget;

  //! Stage's unique ID.
  int id;

private:
  //! cbStage's stage counter. Useful for debugging stages.
  static int counter;
};

#endif /* end of include guard: CBSTAGE_H */
