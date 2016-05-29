/*=========================================================================
  Program: Cerebra
  Module:  cbMainWindow.cxx

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

#include "cbMainWindow.h"
#include "cbStageManager.h"
#include "vtkDataManager.h"

#include "vtkRenderer.h"
#include "vtkInteractorStyleImage.h"
#include "vtkMatrix4x4.h"

#include "vtkViewRect.h"
#include "qvtkViewToolCursorWidget.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QString>
#include <QProgressBar>
#include <QDockWidget>
#include <QStringList>
#include <QStatusBar>
#include <QMessageBox>

#include <assert.h>

cbMainWindow::cbMainWindow(vtkDataManager *dataManager, QWidget *parent)
: QMainWindow(parent)
{
  int width = 1600;
  int height = width / 16 * 9;

  assert(dataManager && "dataManager cannot be null!");
  this->dataManager = dataManager;

  QStatusBar *statusBar = this->statusBar();
  progressBar = new QProgressBar;
  statusBar->addPermanentWidget(progressBar);

  viewRect = vtkViewRect::New();

  qvtkWidget = new qvtkViewToolCursorWidget(this);
  qvtkWidget->SetViewRect(viewRect);
  qvtkWidget->SetRenderWindow(viewRect->GetRenderWindow());
  qvtkWidget->SetSynchronized(true);

  setCentralWidget(qvtkWidget);
  initializeProgress(0,1);
  displayProgress(1);
  resize(width, height);
}

cbMainWindow::~cbMainWindow()
{
  viewRect->Delete();
}

void cbMainWindow::displayStatus(const QString &message, int timeout)
{
  this->statusBar()->showMessage(message, timeout);
}

void cbMainWindow::clearStatus()
{
  this->statusBar()->clearMessage();
}

void cbMainWindow::displayProgress(int v)
{
  this->progressBar->setValue(v);
  QApplication::processEvents();
}

void cbMainWindow::initializeProgress(int min, int max)
{
  this->progressBar->reset();
  this->progressBar->setMinimum(min);
  this->progressBar->setMaximum(max);
}

void cbMainWindow::displaySuccessMessage(QString message)
{
  QString success("Success! ");
  QMessageBox::information(NULL, "Success!", success.append(message));
}

void cbMainWindow::displayNoticeMessage(QString message)
{
  QString notice("Notice: ");
  QMessageBox::warning(NULL, "Notice!", notice.append(message));
}

void cbMainWindow::displayErrorMessage(QString message)
{
  QString error("Error: ");
  QMessageBox::warning(NULL, "Error!", error.append(message));
}

QDir cbMainWindow::appDirectoryOf(const QString &directoryname)
{
  QDir dir(QApplication::applicationDirPath());
  
#if defined(Q_OS_WIN)
  if (dir.dirName().toLower() == "debug"
      ||dir.dirName().toLower() == "release"
      ||dir.dirName().toLower() == "bin")
    {
    dir.cdUp();
    }
#elif defined(Q_OS_MAC)
  if (dir.dirName() == "MacOS") {
    dir.cdUp();
  }
#endif
  dir.cd(directoryname); 
  return dir;
}

void cbMainWindow::setActiveToolCursor(QCursor cur)
{
  qvtkWidget->SetFocusCursorShape(cur);
}

void cbMainWindow::SetViewFromMatrix(vtkRenderer *renderer,
                                     vtkInteractorStyleImage *istyle,
                                     vtkMatrix4x4 *matrix)
{
  istyle->SetCurrentRenderer(renderer);

  // This view assumes the data uses the DICOM Patient Coordinate System.
  // It provides a right-is-left view of axial and coronal images
  static double viewRight[4] = { 1.0, 0.0, 0.0, 0.0 };
  static double viewUp[4] = { 0.0, -1.0, 0.0, 0.0 };

  matrix->MultiplyPoint(viewRight, viewRight);
  matrix->MultiplyPoint(viewUp, viewUp);

  istyle->SetImageOrientation(viewRight, viewUp);
}

