/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeToolBarWidget.h

  Copyright (c) 2011-2013 David Adair
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

#ifndef CBELECTRODETOOLBARWIDGET_H
#define CBELECTRODETOOLBARWIDGET_H

#include <QtGui/QFrame>

class QAction;
class QActionGroup;
class QDir;
class QWidget;
class QString;
class cbElectrodeView;
class vtkDataManager;

//! The layout and operation for the ToolBar of the Cerebra-Electrode application
class cbElectrodeToolBarWidget : public QFrame
{
  Q_OBJECT

public:
  cbElectrodeToolBarWidget(vtkDataManager *dataManager, QDir iconDir, QWidget *parent);
  ~cbElectrodeToolBarWidget();

public slots:

  //! Set up availability for toolbar button groups.
  void enableBasicTool();

  //! Click to change the window/level of current viewing image
  void onWinlevButtonDown();

  //! Click to pan current viewing image
  void onPanButtonDown();

  //! Click to zoom in/out current viewing image
  void onZoomButtonDown();

  //! Click to plane-slice the current viewing image
  void onPlaneButtonDown();

  //! Click to reset the view orientations
  void onResetButtonDown();

  //! Click to toggle maximizing the surface renderer
  void onMaximizeToggleButtonDown();

protected:
  //! Create the actions signal-slot for all the toolbar buttons
  void createActions();

private:
  // first line actions
  QAction *actionWinLevel;
  QAction *actionPan;
  QAction *actionZoom;
  QAction *actionRotate;
  QAction *actionPlane;
  QAction *actionPick;
  QAction *actionReset;
  QAction *actionMaximizeToggle;

  // actions group
  QActionGroup *m_groupMouseControl;

  // get cursor and button icons directory
  QString m_iconPath;
  QString m_cursorPath;

  cbElectrodeView *m_window;
};

#endif //CBELECTRODETOOLBARWIDGET_H

