/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeController.h

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

#ifndef CBELECTRODECONTROLLER_H
#define CBELECTRODECONTROLLER_H

#include "cbApplicationController.h"
#include "cbProbe.h"
#include "vtkDataManager.h"
#include "LeksellFiducial.h"

#include <vector>
#include <QString>

class vtkImageData;
class vtkImageStencilData;
class vtkMatrix4x4;
class vtkPolyData;

//! Realization of cbApplicationController to provide Perfusion processing.
class cbElectrodeController : public cbApplicationController
{
  Q_OBJECT
public:
  cbElectrodeController(vtkDataManager *dataManager);
  ~cbElectrodeController();

  void setPlan(std::vector<cbProbe> *plan) { this->Plan = plan; }

public slots:
  void OpenLegacyPlan(const QString& file);
  void OpenPlan(const QString& file);
  void SavePlan(const QString& file);
  void OpenCTData(const QStringList& files);
  void OpenCTData(const QStringList& files, vtkMatrix4x4 *matrix);
  void requestOpenImage(const QStringList& files);
  void registerAntPost(int s);

signals:
  void DisplayCTData(vtkDataManager::UniqueKey k);
  void displayData(vtkDataManager::UniqueKey);

  //! Outgoing signal to clear the current probe plan.
  void ClearCurrentPlan();

  //! Outgoing signal to create a new probe.
  void CreateProbeRequest(double x, double y, double z,
                          double a, double d, double depth,
                          std::string n, std::string s);

  //! Signal for logging controller actions.
  void Log(QString message);

  //! Outgoing signal to get manager on last stage.
  void jumpToLastStage();

  //! Signal to observers that a process has finished
  void finished();

  //! Tell the view to display the frame
  void displayLeksellFrame(vtkMatrix4x4 *);

  //! Tell the view to display the tags
  void displayTags(vtkDataManager::UniqueKey);

  //! Tell the viw to display the surface volume of the brain.
  void displaySurfaceVolume(vtkDataManager::UniqueKey);

private:
  //! Convenience method for adding timestamp to log messages.
  void log(QString m);

  //! Build the frame and tell view to display it.
  void buildAndDisplayFrame(vtkImageData *data, vtkMatrix4x4 *matrix);

  //! Extract the brain volume and tell view to display it.
  void extractAndDisplaySurface(vtkImageData *data, vtkMatrix4x4 *matrix);

  //! Register the CT data to the MR data.
  void RegisterCT(vtkImageData *ct_d, vtkMatrix4x4 *ct_m);

  void OpenCTWithMatrix(const QStringList& files, vtkMatrix4x4 *matrix);
  void OpenImageWithMatrix(const QStringList& files, vtkMatrix4x4 *matrix);

  vtkDataManager::UniqueKey dataKey;
  vtkDataManager::UniqueKey volumeKey;
  vtkDataManager::UniqueKey ctKey;
  vtkDataManager::UniqueKey tagKey;

  bool useAnteriorPosteriorFiducials;

  std::vector<cbProbe> *Plan;
  vtkMatrix4x4 *FrameMatrix;
};

#endif // CBELECTRODECONTROLLER_H
