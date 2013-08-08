/*=========================================================================
  Program: Cerebra
  Module:  cbStageManager.h

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

#ifndef CBSTAGEMANAGER_H
#define CBSTAGEMANAGER_H

#include <QDockWidget>
#include <QString>
#include <vector>

class QFrame;
class QDir;
class QWidget;
class QVBoxLayout;
class QPushButton;
class QTextEdit;
class vtkDataManager;
class cbStage;
class cbPerfusionToolBarWidget;
class cbElectrodeToolBarWidget;

//! cbStageManager is the management entity for cbStage objects.
/*!
 *  The stage manager is responsible for controlling pipeline flow. It
 *  is the mediator between cbMainWindow and cbStage, requesting a stage's
 *  sidebar and displaying it on the main window when necessary.
 *
 *  The manager accepts the mainwindow and a vector of stages as its
 *  constructor arguments.
*/
class cbStageManager : public QDockWidget
{
  Q_OBJECT
public:
  cbStageManager(QWidget *parent, QFrame *toolbar, vtkDataManager *dataManager, QDir dir, 
                 const std::vector<cbStage *> &stages);
  ~cbStageManager();

public slots:
  void enableResolutionTool();

  //! Actions to perform when the 'Previous' stage button is pressed.
  void prevButtonAction();

  //! Actions to perform when the 'Next' stage button is pressed.
  void nextButtonAction();

  //! Allow a component to force the manager to jump to the last stage.
  void jumpToLastStage();

private slots:
  //! Slot to enable the 'Next' button.
  /*!
   *  The manager automatically connects this slot with the current stage's
   *  'finished()' signal. This allows the pipeline to be blocked until the
   *  current stage has been executed.
   *
   *  Due to the controller being in a seperate thread, and the stages
   *  having no knowledge of the connection between themselves and the
   *  controller, the finished() signal is emitted right after *requesting*
   *  the processing from the controller. Further improvements can be made
   *  by coordinating with the view or the controller further, allowing for
   *  this slot to be called only when the processing is complete.
  */
  void enableNextButton();

  void Log(QString message);

private:
  //! Convenience method for retrieving the widget for a stage at index i.
  void getStageWidget(int i);

  //! A list of stages, provided in the constructor.
  std::vector<cbStage *> stages;

  //! The index of the current stage. [0,stages.size()-1]
  int current;
  int performed;

  //! Pointer to the current sidebar stage widget.
  QWidget *sidebar;

  //! Pointer to the current sidebar stage widget.
  //cbPerfusionToolBarWidget *toolbarWidget;
  cbElectrodeToolBarWidget *toolbarWidget;

  //! Pointer to clinical infomation widget.
  QWidget *clinicalInfoWidget;

  //! Pointer to clinical information textbox.
  QTextEdit *clinicalInfo;

  //! Layout for the whole sidebar.
  QVBoxLayout *sidebarLayout;

  //! Previous and Next buttons.
  QPushButton *prev;
  QPushButton *next;
};

#endif /* end of include guard: CBSTAGEMANAGER_H */
