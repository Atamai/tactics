/*=========================================================================
  Program: Cerebra
  Module:  ElectrodeMain.cxx

  Copyright (c) 2011-2013 David Adair, David Gobbi
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

#include "cbElectrodeAutomator.h"
#include "cbElectrodeController.h"
#include "cbElectrodeIntroductionStage.h"
#include "cbElectrodeOpenStage.h"
#include "cbElectrodePlanStage.h"
#include "cbElectrodeView.h"
#include "cbElectrodeToolBarWidget.h"
#include "cbProbe.h"
#include "cbQtVTKOutputWindow.h"
#include "cbStageManager.h"

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QThread>
#include <QApplication>

#include "LeksellFiducial.h"

#include "vtkDataManager.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyData.h"

#include <vector>

void AutoOpen(cbStageManager *m, cbElectrodeOpenStage *s, const char *arg)
{
  m->nextButtonAction();
  cbElectrodeAutomator a(arg);
  QObject::connect(&a, SIGNAL(open(const QStringList&)),
                   s, SIGNAL(requestOpenImage(const QStringList&)));
  a.activate();
  QObject::disconnect(&a, SIGNAL(open(const QStringList&)),
                      s, SIGNAL(requestOpenImage(const QStringList&)));
  m->nextButtonAction();
}

int main(int argc, char *argv[])
{
  // install the VTK-to-Qt error message translator
  cbQtVTKOutputWindow::Install();

  QApplication a(argc, argv);

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
  dir.cd("Icons");

  // Register UniqueKey as a Qt Meta-Type so it can be queued in signals/slots.
  qRegisterMetaType<vtkDataManager::UniqueKey>("vtkDataManager::UniqueKey");

  vtkDataManager *dataManager = vtkDataManager::New();
  cbElectrodeView window(dataManager);
  cbElectrodeController controller(dataManager);

  QObject::connect(&controller, SIGNAL(displayStatus(const QString&, int)),
                   &window, SLOT(displayStatus(const QString&, int)));
  QObject::connect(&controller, SIGNAL(clearStatus()),
                   &window, SLOT(clearStatus()));

  QObject::connect(&controller, SIGNAL(initializeProgress(int, int)),
                   &window, SLOT(initializeProgress(int, int)));
  QObject::connect(&controller, SIGNAL(displayProgress(int)),
                   &window, SLOT(displayProgress(int)));

  QObject::connect(&controller,
                   SIGNAL(displayLeksellFrame(vtkMatrix4x4 *)),
                   &window,
                   SLOT(displayLeksellFrame(vtkMatrix4x4 *)));
  QObject::connect(&controller,
                   SIGNAL(DisableFrameVisualization()),
                   &window,
                   SLOT(DisableFrameVisualization()));
  QObject::connect(&controller,
                   SIGNAL(EnableFrameVisualization()),
                   &window,
                   SLOT(EnableFrameVisualization()));
  QObject::connect(&controller,
                   SIGNAL(displayTags(vtkDataManager::UniqueKey)),
                   &window,
                   SLOT(displayTags(vtkDataManager::UniqueKey)));
  QObject::connect(&controller,
                   SIGNAL(displaySurfaceVolume(vtkDataManager::UniqueKey)),
                   &window,
                   SLOT(displaySurfaceVolume(vtkDataManager::UniqueKey)));

  QObject::connect(&window, SIGNAL(OpenCTData(const QStringList&)),
                   &controller, SLOT(OpenCTData(const QStringList&)));
  QObject::connect(&controller, SIGNAL(DisplayCTData(vtkDataManager::UniqueKey)),
                   &window, SLOT(DisplayCTData(vtkDataManager::UniqueKey)));

  cbElectrodeOpenStage openStage;
  QObject::connect(&openStage, SIGNAL(requestOpenImage(const QStringList&)),
                   &controller, SLOT(requestOpenImage(const QStringList&)));
  QObject::connect(&openStage, SIGNAL(registerAntPost(int)),
                   &controller, SLOT(registerAntPost(int)));
  QObject::connect(&openStage, SIGNAL(requestOpenCT(const QStringList&)),
                   &controller, SLOT(OpenCTData(const QStringList&)));
  QObject::connect(&controller, SIGNAL(displayData(vtkDataManager::UniqueKey)),
                   &window, SLOT(displayData(vtkDataManager::UniqueKey)));

  cbElectrodePlanStage planStage;
  QObject::connect(&controller, SIGNAL(CreateProbeRequest(double, double, double, double, double, double, std::string, std::string)),
                   &planStage, SLOT(CreateProbeRequest(double, double, double, double, double, double, std::string, std::string)));

  QObject::connect(&window, SIGNAL(CreateProbeRequest(double, double, double, double, double, double, std::string, std::string)),
                   &planStage, SLOT(CreateProbeRequest(double, double, double, double, double, double, std::string, std::string)));

  QObject::connect(&window, SIGNAL(OpenPlan(const QString&)),
                   &controller, SLOT(OpenPlan(const QString&)));

  QObject::connect(&window, SIGNAL(SavePlan(const QString&)),
                   &controller, SLOT(SavePlan(const QString&)));

  QObject::connect(&controller, SIGNAL(ClearCurrentPlan()),
                   &planStage, SLOT(ClearCurrentPlan()));

  QObject::connect(&planStage, SIGNAL(SetCTOpacity(double)),
                   &window, SLOT(SetCTOpacity(double)));

  qRegisterMetaType<cbProbe>("cbProbe");
  QObject::connect(&planStage, SIGNAL(InitiatePlaceProbeCallback()),
                   &window, SLOT(InitiatePlaceProbeCallback()));
  QObject::connect(&planStage, SIGNAL(CreateProbeCallback(cbProbe)),
                   &window, SLOT(CreateProbeCallback(cbProbe)));
  QObject::connect(&planStage, SIGNAL(DestroyProbeCallback(int)),
                   &window, SLOT(DestroyProbeCallback(int)));
  QObject::connect(&planStage, SIGNAL(UpdateProbeCallback(int, cbProbe)),
                   &window, SLOT(UpdateProbeCallback(int, cbProbe)));
  QObject::connect(&planStage, SIGNAL(EnableFrameVisualization()),
                   &window, SLOT(EnableFrameVisualization()));
  QObject::connect(&planStage, SIGNAL(DisableFrameVisualization()),
                   &window, SLOT(DisableFrameVisualization()));

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ToggleAxialVisualization,
                   &window,
                   &cbElectrodeView::ToggleAxialVisualization);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ToggleCoronalVisualization,
                   &window,
                   &cbElectrodeView::ToggleCoronalVisualization);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ToggleSagittalVisualization,
                   &window,
                   &cbElectrodeView::ToggleSagittalVisualization);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ToggleProbeVisualizationMode,
                   &window,
                   &cbElectrodeView::ToggleProbeVisualizationMode);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ToggleHelpAnnotations,
                   &window,
                   &cbElectrodeView::ToggleHelpAnnotations);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::TogglePatientAnnotations,
                   &window,
                   &cbElectrodeView::TogglePatientAnnotations);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::EnableTagVisualization,
                   &window,
                   &cbElectrodeView::EnableTagVisualization);

  QObject::connect(&planStage,
                   &cbElectrodePlanStage::DisableTagVisualization,
                   &window,
                   &cbElectrodeView::DisableTagVisualization);


  QObject::connect(&planStage,
                   &cbElectrodePlanStage::ExportScreenshot,
                   &window,
                   &cbElectrodeView::ExportScreenshot);

  std::vector<cbStage *> stages;
  stages.push_back(new cbElectrodeIntroductionStage);
  stages.push_back(&openStage);
  stages.push_back(&planStage);

  cbElectrodeToolBarWidget *toolBarWidget =
    new cbElectrodeToolBarWidget(dataManager, dir, &window);

  cbStageManager manager(&window, toolBarWidget, dataManager, dir, stages);

  QObject::connect(&controller, SIGNAL(finished()),
                   toolBarWidget, SLOT(enableBasicTool()));

  QObject::connect(&controller, SIGNAL(finished()),
                   &manager, SLOT(enableNextButton()));

  QObject::connect(&controller, SIGNAL(Log(QString)),
                   &manager, SLOT(Log(QString)));

  QObject::connect(&controller, SIGNAL(jumpToLastStage()),
                   &manager, SLOT(jumpToLastStage()));

  controller.setPlan(planStage.getPlan());

  //QThread controllerThread;
  //controller.moveToThread(&controllerThread);
  //controllerThread.start();

  // Display the main window.
  window.show();

  if (argc > 1 && strcmp(argv[1], "--auto") == 0)
    {
    AutoOpen(&manager, &openStage, "/Public/CAIN/Clinical Data/Electrode/Electrode05/IM-0001-0001.dcm");
    }

  // Run the program.
  return a.exec();
}
