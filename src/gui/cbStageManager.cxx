/*=========================================================================
  Program: Cerebra
  Module:  cbStageManager.cxx

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

#include "cbStageManager.h"
#include "cbStage.h"

#include "vtkDataManager.h"

#include <QMainWindow>
#include <QtGui>
#include <assert.h>
#include <QDebug>
#include <QFrame>


cbStageManager::cbStageManager(QWidget *parent, QFrame *toolbar,
                               vtkDataManager *dataManager, QDir dir,
                               const std::vector<cbStage *> &stages)
: QDockWidget(parent), current(0), performed(0)
{
  this->sidebar = 0;
  this->stages = stages;

  QMainWindow *window = qobject_cast<QMainWindow *>(parent);
  assert(window && "cbStageManager must have a QMainWindow as a parent!");

  this->setAllowedAreas(Qt::LeftDockWidgetArea);
  this->setFeatures(QDockWidget::NoDockWidgetFeatures);
  window->addDockWidget(Qt::LeftDockWidgetArea, this);

  // Hierarchy of widgets and layouts for the dock.
  QWidget *containerWidget = new QWidget;
      sidebarLayout = new QVBoxLayout;
      // Previous and Next buttons
      QWidget *buttonWidget = new QWidget;
        QHBoxLayout *horizontal = new QHBoxLayout;
          prev = new QPushButton("&Previous");
          next = new QPushButton("&Next");
      //sidebar
      //clinical information for future log info
      clinicalInfoWidget = new QWidget;
      QVBoxLayout *clinicalInfoLayout = new QVBoxLayout;
      clinicalInfo = new QTextEdit(containerWidget);
      clinicalInfo->setReadOnly(true);
      clinicalInfo->insertHtml("<p>Log information will appear here.</p>");
      clinicalInfo->setStyleSheet("background-color: aliceblue");
      clinicalInfo->setMinimumHeight(parent->height()*0.28);
      clinicalInfoLayout->addWidget(clinicalInfo);
      clinicalInfoWidget->setLayout(clinicalInfoLayout);

  this->toolbarWidget = toolbar;
  sidebarLayout->addWidget(toolbar);
  sidebarLayout->addWidget(buttonWidget);

  getStageWidget(0);

  connect(prev, SIGNAL(clicked()), this, SLOT(prevButtonAction()));
  connect(next, SIGNAL(clicked()), this, SLOT(nextButtonAction()));

  horizontal->addWidget(prev);
  horizontal->addWidget(next);

  buttonWidget->setLayout(horizontal);
  containerWidget->setLayout(sidebarLayout);

  this->setWidget(containerWidget);
}

cbStageManager::~cbStageManager()
{
}

void cbStageManager::prevButtonAction()
{
  if (current == 0)
    {
    return;
    }
  getStageWidget(--current);
  if (current < performed+1) next->setEnabled(true);
}

void cbStageManager::nextButtonAction()
{
  if (current == stages.size() - 1)
    {
    return;
    }
  getStageWidget(++current);
  if (current >= performed+1)
    {
    this->next->setDisabled(true);
    }
}

void cbStageManager::getStageWidget(int i)
{
  if (this->sidebar)
    {
    // Remove the widget from the layout
    this->sidebarLayout->removeWidget(this->sidebar);

    // Must hide the widget as it still renders
    this->sidebar->hide();
    }

  // Replace the sidebar pointer with a new widget
  this->sidebar = stages.at(i)->Widget();
  assert(this->sidebar && "Stage should not return a NULL sidebar!");

  // Add the new widget to the layout
  this->sidebarLayout->addWidget(this->sidebar);
  //this->sidebarLayout->addWidget(this->clinicalInfoWidget);

  // Render the widget
  this->sidebar->show();
}

void cbStageManager::enableNextButton()
{
  if (current != stages.size() - 1)
    {
    performed = current;
    this->next->setEnabled(true);
    }
  if (current > 0)
    {
    //toolbarWidget->setBasicToolEnabled(true);
    //toolbarWidget->onPlaneButtonDown();
    }
}

void cbStageManager::enableResolutionTool()
{
  qDebug()<< "Enable Resolution Tool " << endl;
  //toolbarWidget->setResolutionToolEnabled(true);
  //toolbarWidget->showHighResolution();
}

void cbStageManager::Log(QString message)
{
  this->clinicalInfo->append(message);
  this->clinicalInfo->ensureCursorVisible();
}

void cbStageManager::jumpToLastStage()
{
  this->current = this->stages.size() - 1;
  getStageWidget(this->current);

  this->performed = this->current - 1;
  this->enableNextButton();
}
