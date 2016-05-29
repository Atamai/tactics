/*=========================================================================
  Program: Cerebra
  Module:  cbMainWindow.h

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

#ifndef CBMAINWINDOW_H
#define CBMAINWINDOW_H

#include <QMainWindow>

class qvtkViewToolCursorWidget;
class vtkViewRect;
class vtkDataManager;
class vtkRenderer;
class vtkMatrix4x4;
class vtkInteractorStyleImage;

class QAction;
class QToolBar;
class QString;
class QProgressBar;
class QDockWidget;
class QPushButton;
class QTextEdit;
class QCursor;
class QDir;

class cbStageManager;

//! Base class for all application views.
/*!
 *  Handles progress and messaging events from the controller.
 *  Handles setting the cbStageManager.
*/
class cbMainWindow : public QMainWindow
{
  Q_OBJECT
public:
  cbMainWindow(vtkDataManager *dataManager, QWidget *parent = 0);
  ~cbMainWindow();

public:

public slots:
  //! Display a success message.
  static void displaySuccessMessage(QString message);

  //! Display a notice message.
  static void displayNoticeMessage(QString message);

  //! Display an error message.
  static void displayErrorMessage(QString message);

  //! Display a status message for the specified number of milliseconds.
  void displayStatus(const QString &message, int timeout = 0);

  //! Clear the status message.
  void clearStatus();

  //! Update the progress bar.
  void displayProgress(int);

  //! Initialize the progress bar.
  void initializeProgress(int min, int max);

  //! Allows an external entity to set the active tool for all toolcursors.
  void setActiveToolCursor(QCursor cur);

  //! Bind actions to the tool cursor.
  virtual void bindToolCursorAction(int cursortool, int mousebutton) = 0;

protected:
  //! Convinience method to retrieve the application directory of provided name.
  QDir appDirectoryOf(const QString &directoryname);

  //! The application windows progress bar.
  QProgressBar *progressBar;

  //! The base class for cbStageManager.
  QDockWidget *dock;

  //! The widget to interact with VTK render windows and vtkToolCursors.
  qvtkViewToolCursorWidget *qvtkWidget;

  //! The view rect for the application.
  vtkViewRect *viewRect;

  //! Pointer to the application's shared datamanager.
  vtkDataManager *dataManager;

  static void SetViewFromMatrix(vtkRenderer *renderer,
                                vtkInteractorStyleImage *istyle,
                                vtkMatrix4x4 *matrix);

};

#endif /* end of include guard: CBPERFUSONMAINWINDOW_H */
