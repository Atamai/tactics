/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeToolBarWidget.cxx

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

#include "cbElectrodeToolBarWidget.h"

// QT includes
#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QSizePolicy>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>

#include <assert.h>

// CB includes
#include "cbElectrodeView.h"
#include "vtkDataManager.h"

//----------------------------------------------------------------------------
// Constructor
cbElectrodeToolBarWidget::cbElectrodeToolBarWidget(vtkDataManager *dataManager,
                                                   QDir iconDir, QWidget *parent)
: QFrame(parent)
{
  cbMainWindow *window = qobject_cast<cbMainWindow *>(parent);
  assert(window && "cbElectrodeToolBarWidget must have a cbMainWindow as a parent!");

  m_window = qobject_cast<cbElectrodeView *>(parent);

  // get icons path
  m_iconPath = QFile((iconDir).absoluteFilePath("Icons")).fileName();
  m_cursorPath = QFile((iconDir).absoluteFilePath("Cursors")).fileName();

  this->createActions();

  // first line toolbar
  QToolBar *fileToolBar = new QToolBar;
  fileToolBar->addAction(actionWinLevel);
  fileToolBar->addAction(actionPan);
  fileToolBar->addAction(actionZoom);
  fileToolBar->addAction(actionPlane);
  fileToolBar->addSeparator();
  fileToolBar->addAction(actionReset);
  fileToolBar->addAction(actionMaximizeToggle);

  QVBoxLayout *iconBarLayout = new QVBoxLayout;
  iconBarLayout->setContentsMargins(9, 0, 9, 0);
  iconBarLayout->setSpacing(0);
  iconBarLayout->addWidget(fileToolBar);
  this->setLayout(iconBarLayout);
}

// Destructor
cbElectrodeToolBarWidget::~cbElectrodeToolBarWidget()
{
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::createActions()
{
  // Mouse control tool actions group
  m_groupMouseControl = new QActionGroup(this);

  actionWinLevel = new QAction(QIcon(m_iconPath + "/winlev.png"),
                               tr("&Window/Level..."), this);
  actionWinLevel->setCheckable(true);
  actionWinLevel->setStatusTip("Change contrast and brightness of images.");
  connect(actionWinLevel, SIGNAL(triggered()), this, SLOT(onWinlevButtonDown()));

  actionPan = new QAction(QIcon(m_iconPath + "/pan.png"),
                          tr("&Pan..."), this);
  actionPan->setCheckable(true);
  actionPan->setStatusTip("Move the display around the screen.");
  connect(actionPan, SIGNAL(triggered()), this, SLOT(onPanButtonDown()));

  actionZoom = new QAction(QIcon(m_iconPath + "/zoom.png"),
                           tr("&Zoom..."), this);
  actionZoom->setCheckable(true);
  actionZoom->setStatusTip("Zoom in/out the display.");
  connect(actionZoom, SIGNAL(triggered()), this, SLOT(onZoomButtonDown()));

  actionPlane = new QAction(QIcon(m_iconPath + "/slice.png"),
                            tr("&Plane..."), this);
  actionPlane->setCheckable(true);
  actionPlane->setChecked(true);
  actionPlane->setStatusTip("Slice through the display, or rotate a slice.");
  connect(actionPlane, SIGNAL(triggered()), this, SLOT(onPlaneButtonDown()));

  actionReset = new QAction(QIcon(m_iconPath + "/undo.png"), tr("&Reset"), this);
  actionReset->setCheckable(false);
  actionReset->setEnabled(false);
  actionReset->setStatusTip("Reset the display to the default view.");
  connect(actionReset, SIGNAL(triggered()), this, SLOT(onResetButtonDown()));

  actionMaximizeToggle = new QAction(QIcon(m_iconPath + "/brain.png"), tr("&3D View"), this);
  actionMaximizeToggle->setCheckable(true);
  actionMaximizeToggle->setEnabled(false);
  actionMaximizeToggle->setStatusTip("Maximize/minimize the 3D rendering view.");
  connect(actionMaximizeToggle, SIGNAL(triggered()), this, SLOT(onMaximizeToggleButtonDown()));

  m_groupMouseControl->setExclusive(true);
  m_groupMouseControl->setEnabled(false);
  m_groupMouseControl->addAction(actionPan);
  m_groupMouseControl->addAction(actionZoom);
  m_groupMouseControl->addAction(actionWinLevel);
  m_groupMouseControl->addAction(actionPlane);
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::onPanButtonDown()
{
  QPixmap pix(m_cursorPath + "/pan.png");
  QCursor cur(pix);
  m_window->setActiveToolCursor(cur);
  m_window->bindToolCursorAction(cbElectrodeView::Pan, Qt::LeftButton);
  m_window->bindToolCursorAction(cbElectrodeView::Rotate, Qt::RightButton);
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::onZoomButtonDown()
{
  QPixmap pix(m_cursorPath + "/zoom.png");
  QCursor cur(pix);
  m_window->setActiveToolCursor(cur);
  m_window->bindToolCursorAction(cbElectrodeView::Zoom, Qt::LeftButton);
  m_window->bindToolCursorAction(cbElectrodeView::Rotate, Qt::RightButton);
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::onWinlevButtonDown()
{
  QPixmap pix(m_cursorPath + "/winlev16.png");
  QCursor cur(pix);
  m_window->setActiveToolCursor(cur);
  m_window->bindToolCursorAction(cbElectrodeView::Winlev, Qt::LeftButton);
  m_window->bindToolCursorAction(cbElectrodeView::Rotate, Qt::RightButton);
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::onPlaneButtonDown()
{
  QPixmap pix(m_cursorPath + "/slice.png");
  QCursor cur(pix);
  m_window->setActiveToolCursor(cur);
  m_window->bindToolCursorAction(cbElectrodeView::Plane, Qt::LeftButton);
  m_window->bindToolCursorAction(cbElectrodeView::Rotate, Qt::RightButton);
}

//----------------------------------------------------------------------------
void cbElectrodeToolBarWidget::enableBasicTool()
{
  m_groupMouseControl->setEnabled(true);
  actionReset->setEnabled(true);
  actionMaximizeToggle->setEnabled(true);
  this->onPlaneButtonDown();
}

void cbElectrodeToolBarWidget::onResetButtonDown()
{
  m_window->resetViewOrientations();
}

void cbElectrodeToolBarWidget::onMaximizeToggleButtonDown()
{
  if (actionMaximizeToggle->isChecked()) {
    m_groupMouseControl->setEnabled(false);
  } else {
    m_groupMouseControl->setEnabled(true);
  }

  m_window->ToggleMaximizeSurface();
}
