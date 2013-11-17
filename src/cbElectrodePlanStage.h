/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodePlanStage.h

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

#ifndef CBELECTRODEPLANSTAGE_H
#define CBELECTRODEPLANSTAGE_H

#include "cbProbe.h"
#include "cbProbeCatalogue.h"
#include "cbStage.h"

#include <vector>

class QComboBox;
class QLineEdit;
class QListWidget;
class QSlider;
class QDoubleSpinBox;
class QTabWidget;

//! Plan stage for the application. Provides a pipeline description.
class cbElectrodePlanStage : public cbStage
{
  Q_OBJECT
public:
  cbElectrodePlanStage();
  ~cbElectrodePlanStage();

  //! Returns the stages name (ie. "Plan")
  virtual const char *getStageName() const;

signals:
  //! Outgoing signal to allow view to render a probe.
  void CreateProbeCallback(cbProbe);
  //! Outgoing signal to update probe positions in the view.
  void UpdateProbeCallback(int index, cbProbe p);
  //! Outgoing signal to destroy a probe from the renderer.
  void DestroyProbeCallback(int);
  //! Outgoing signal to bind pick action for placing probe.
  void InitiatePlaceProbeCallback();

  //! Outgoing signal to set the CT layer opacity.
  void SetCTOpacity(double o);

  //! Outgoing signal to toggle frame visualization.
  void EnableFrameVisualization();
  void DisableFrameVisualization();

  //! Outgoing signals to toggle plane display.
  void ToggleAxialVisualization(int);
  void ToggleCoronalVisualization(int);
  void ToggleSagittalVisualization(int);

  //! Outgoing signal to display one-or-all probes on slice view.
  void ToggleProbeVisualizationMode(int);

  //! Outgoing signal to toggle the help annotation.
  void ToggleHelpAnnotations(int);

  //! Outgoing signal to toggle the patient information annotation.
  void TogglePatientAnnotations(int);

  //! Outgoing signal to save a screenshot of the render window.
  void ExportScreenshot();

  //! Outgoing signal specifically for depth slider and nothing else.
  void updateDepthSliderInt(int);

  //! Outgoing signal specifically for depth label and nothing else.
  void updateDepthSpinBoxDouble(double);

public slots:
  //! Incoming signal to destroy all placed probes currently in plan.
  void ClearCurrentPlan();

  //! Incoming signal to save the plan to disk.
  void SavePlanToFile(std::string path);

  //! Actions to perform for the stage
  virtual void Execute();

  //! Remove the current electrode(s) from the plan.
  void RemoveCurrentFromPlan();

  //! Incoming request from the view to create a new probe.
  void CreateProbeRequest(double x, double y, double z, double a, double d,
                          double depth, std::string n, std::string s);

private slots:
  void updateForCurrentSelection();
  void updateCurrentProbeOrientation();
  void updateCurrentProbePosition();
  void updateCurrentProbeName(QString);
  void updateCurrentProbeType(QString n);
  void updateCurrentProbeDepth();
  void updateDepthSliderDouble(double);
  void updateDepthSpinBoxInt(int);

  void opacitySliderChanged(int);
  void setPrecision(QString);
  void toggleFrameVisualization(int);

  void savePlanReport();

  void placeProbeCallback();

private:
  void computeDepthVector(double depth, double vector[3]) const;

  QLineEdit *nameEdit;
  QComboBox *typeList;
  QListWidget *placedList;

  QSlider *declinationSlider;
  QSlider *azimuthSlider;
  QSlider *opacitySlider;

  int sliderSubdivisions;
  int spinBoxDecimals;

  QDoubleSpinBox *xSpin;
  QDoubleSpinBox *ySpin;
  QDoubleSpinBox *zSpin;
  QDoubleSpinBox *depthSpin;
  QSlider *depthSlider;

  QTabWidget *tabWidget;

  std::vector<cbProbe> Plan;
  cbProbeCatalogue catalogue_;

  std::string get_probe_dir();
};

#endif /* end of include guard: CBELECTRODEPLANSTAGE_H */
