/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodePlanStage.cxx

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

#include "cbElectrodePlanStage.h"

#include "cbMainWindow.h"

#include <QWidget>
#include <QApplication>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QSpinBox>
#include <QSlider>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QShortcut>
#include <QMessageBox>

#include <iostream>
#include <fstream>
#include <sstream>

#include <math.h>

cbElectrodePlanStage::cbElectrodePlanStage()
  : cbStage(), Plan(), catalogue_(this->get_probe_dir())
{
  this->widget = new QWidget;
    QVBoxLayout *vertical = new QVBoxLayout;
      QTextEdit *desc = new QTextEdit;
      tabWidget = new QTabWidget;
        QWidget *planWidget = new QWidget;
          QPushButton *addButton = new QPushButton("&Place Probe");
          QGroupBox *nameGroup = new QGroupBox;
            QLabel *nameLabel = new QLabel("ID:");
            nameEdit = new QLineEdit(QString::number(0));
          QLabel *typeLabel = new QLabel("Probe Catalogue:");
          typeList = new QComboBox;
          QGroupBox *dGroup = new QGroupBox;
            declinationSlider = new QSlider;
            QSpinBox *declinationLabel = new QSpinBox;
          QGroupBox *aGroup = new QGroupBox;
            azimuthSlider = new QSlider;
            QSpinBox *azimuthLabel = new QSpinBox;
          QGroupBox *depthGroup = new QGroupBox;
            depthSpin = new QDoubleSpinBox;
            depthSlider = new QSlider;
          QGroupBox *posBox = new QGroupBox;
            xSpin = new QDoubleSpinBox;
            ySpin = new QDoubleSpinBox;
            zSpin = new QDoubleSpinBox;
          QLabel *placedLabel = new QLabel("Probes:");
          QWidget *crudWidget = new QWidget;
            QPushButton *removeButton = new QPushButton("&Remove");
          placedList = new QListWidget;
          QPushButton *saveButton = new QPushButton("&Generate Operation Report");
          QPushButton *exportButton = new QPushButton("Sa&ve Screenshot");
        QWidget *optionWidget = new QWidget;
          QGroupBox *opacityBox = new QGroupBox;
            QSpinBox *opacityLabel = new QSpinBox;
            opacitySlider = new QSlider;
          QHBoxLayout *precisionLayout = new QHBoxLayout;
            QComboBox *precisionSelect = new QComboBox();
            QLabel *precisionLabel = new QLabel("Set plan coord precision.");
          QCheckBox *frameToggle = new QCheckBox("Show frame.");
          QCheckBox *axialToggle = new QCheckBox("Show axial plane.");
          QCheckBox *sagittalToggle = new QCheckBox("Show sagittal plane.");
          QCheckBox *coronalToggle = new QCheckBox("Show coronal plane.");
          QCheckBox *probeToggle = new QCheckBox("Show all probes.");
          QCheckBox *tagToggle = new QCheckBox("Show loaded tags.");
          QCheckBox *helpToggle = new QCheckBox("Show help annotation.");
          QCheckBox *patientToggle = new QCheckBox("Show patient information.");

  // important state information: precision of plan coordinates
  sliderSubdivisions = 1;
  spinBoxDecimals = 0;

  desc->setReadOnly(true);
  desc->insertHtml(
      "\
      <p>To place a probe:</p>\
      <ol>\
        <li>Click the <i>Place Probe</i> button, then use the mouse to place the probe tip.</li>\
        <li>(Optional) Provide the probe with an ID.</li>\
        <li>Select the desired probe specification from the catalogue.</li>\
        <li>Configure the frame-coordinates and orientation of the probe.</li>\
      </ol>\
      <p>You may select a probe from the list at any time and reconfigure.</p>\
      <p>Delete a probe from the plan by selecting the probe and clicking the <i>Remove</i> button.</p>\
      <p>Generate an operation plan by clicking the <i>Generate Operation Report</i> button.</p>\
      <p>Save a screenshot at any time by clicking the <i>Save Screenshot</i> button.</p>\
      <p>You may click the <i>Options</i> tab to find several display options, such as disabling the frame, showing specific planes, and controlling the opacity of the secondary image overlay.</p>\
      <p>Save a plan at any time by using the file-menu or hitting <code>command+s</code>.</p>\
      <p>Open a plan at any time by using the file-menu or hitting <code>command+o</code>.</p>\
      "
      );
  desc->setStyleSheet("background-color: aliceblue");

  connect(addButton, SIGNAL(clicked()), this, SLOT(placeProbeCallback()));

  // populate the type list
  std::vector<std::string> list = this->catalogue_.list_as_strings();
  for (size_t i = 0; i < list.size(); i++) {
    typeList->addItem(list.at(i).c_str());
  }

  nameGroup->setLayout(new QFormLayout);
  dGroup->setLayout(new QFormLayout);
  aGroup->setLayout(new QFormLayout);
  depthGroup->setLayout(new QFormLayout);
  posBox->setLayout(new QHBoxLayout);
  planWidget->setLayout(new QVBoxLayout);
  QVBoxLayout *optionWidgetLayout = new QVBoxLayout;
  optionWidget->setLayout(optionWidgetLayout);

  this->widget->setContentsMargins(0,0,0,0);
  vertical->setContentsMargins(0,0,0,0);
  tabWidget->setContentsMargins(0,0,0,0);

  nameGroup->layout()->setContentsMargins(0,0,0,0);
  dGroup->layout()->setContentsMargins(0,0,0,0);
  aGroup->layout()->setContentsMargins(0,0,0,0);
  depthGroup->layout()->setContentsMargins(0,0,0,0);
  posBox->layout()->setContentsMargins(0,0,0,0);

  planWidget->layout()->setContentsMargins(0,0,0,0);
  optionWidget->layout()->setContentsMargins(0,0,0,0);
  planWidget->layout()->setSpacing(0);
  optionWidget->layout()->setSpacing(0);

  QFormLayout *dLayout = qobject_cast<QFormLayout *>(dGroup->layout());
  QFormLayout *aLayout = qobject_cast<QFormLayout *>(aGroup->layout());
  QFormLayout *depthLayout = qobject_cast<QFormLayout *>(depthGroup->layout());
  dLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  aLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  depthLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  QFormLayout *nameLayout = qobject_cast<QFormLayout *>(nameGroup->layout());
  nameLayout->addRow(nameLabel, nameEdit);
  nameLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  planWidget->layout()->addWidget(addButton);
  planWidget->layout()->addWidget(nameGroup);
  planWidget->layout()->addWidget(typeLabel);
  planWidget->layout()->addWidget(typeList);
  planWidget->layout()->addWidget(posBox);
  planWidget->layout()->addWidget(dGroup);
  planWidget->layout()->addWidget(aGroup);
  planWidget->layout()->addWidget(depthGroup);
  planWidget->layout()->addWidget(placedLabel);
  planWidget->layout()->addWidget(placedList);
  planWidget->layout()->addWidget(crudWidget);
  planWidget->layout()->addWidget(saveButton);
  planWidget->layout()->addWidget(exportButton);

  precisionSelect->addItem("1.0");
  precisionSelect->addItem("0.5");
  precisionSelect->addItem("0.2");
  precisionSelect->addItem("0.1");
  precisionLayout->setContentsMargins(0,0,0,0);
  precisionLayout->setSpacing(0);
  precisionLayout->addWidget(precisionSelect);
  precisionLayout->addSpacing(6);
  precisionLayout->addWidget(precisionLabel, 1, Qt::AlignVCenter);

  optionWidgetLayout->addWidget(opacityBox);
  optionWidgetLayout->addLayout(precisionLayout);
  optionWidgetLayout->addWidget(frameToggle);
  optionWidgetLayout->addWidget(axialToggle);
  optionWidgetLayout->addWidget(sagittalToggle);
  optionWidgetLayout->addWidget(coronalToggle);
  optionWidgetLayout->addWidget(probeToggle);
  optionWidgetLayout->addWidget(tagToggle);
  optionWidgetLayout->addWidget(helpToggle);
  optionWidgetLayout->addWidget(patientToggle);
  optionWidgetLayout->addStretch();

  QFormLayout *opacity_layout = new QFormLayout;
  opacity_layout->addRow(opacityLabel, opacitySlider);
  opacity_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  opacity_layout->setAlignment(opacitySlider, Qt::AlignVCenter);
  opacity_layout->setContentsMargins(0,0,6,0);
  opacityBox->setContentsMargins(0,0,0,0);
  opacityBox->setTitle("Secondary Series Opacity");
  opacityBox->setLayout(opacity_layout);

  connect(opacitySlider, SIGNAL(valueChanged(int)),
          this, SLOT(opacitySliderChanged(int)));
  connect(opacitySlider,SIGNAL(valueChanged(int)),
          opacityLabel, SLOT(setValue(int)));
  connect(opacityLabel, SIGNAL(valueChanged(int)),
          opacitySlider, SLOT(setValue(int)));

  opacitySlider->setMinimum(0);
  opacitySlider->setMaximum(100);
  opacitySlider->setValue(30);
  opacitySlider->setOrientation(Qt::Horizontal);

  tabWidget->addTab(planWidget, "&Planning");
  tabWidget->addTab(optionWidget, "&Options");

  connect(precisionSelect, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(setPrecision(QString)));

  frameToggle->setChecked(true);
  axialToggle->setChecked(true);
  sagittalToggle->setChecked(true);
  coronalToggle->setChecked(true);
  probeToggle->setChecked(true);
  tagToggle->setChecked(true);
  helpToggle->setChecked(true);
  patientToggle->setChecked(true);

  connect(frameToggle, SIGNAL(stateChanged(int)),
          this, SLOT(toggleFrameVisualization(int)));

  connect(axialToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleAxialVisualization(int)));
  connect(sagittalToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleSagittalVisualization(int)));
  connect(coronalToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleCoronalVisualization(int)));

  connect(probeToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleProbeVisualizationMode(int)));
  connect(tagToggle, SIGNAL(stateChanged(int)),
          this, SLOT(toggleTagVisualization(int)));
  connect(helpToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleHelpAnnotations(int)));
  connect(patientToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(TogglePatientAnnotations(int)));

  xSpin->setAccelerated(true);
  xSpin->setMinimum(0);
  xSpin->setMaximum(200);
  xSpin->setDecimals(spinBoxDecimals);
  xSpin->setSingleStep(1.0/sliderSubdivisions);

  ySpin->setAccelerated(true);
  ySpin->setMinimum(0);
  ySpin->setMaximum(200);
  ySpin->setDecimals(spinBoxDecimals);
  ySpin->setSingleStep(1.0/sliderSubdivisions);

  zSpin->setAccelerated(true);
  zSpin->setMinimum(0);
  zSpin->setMaximum(200);
  zSpin->setDecimals(spinBoxDecimals);
  zSpin->setSingleStep(1.0/sliderSubdivisions);

  xSpin->setValue(0);
  ySpin->setValue(0);
  zSpin->setValue(0);

  QRegExp rx("^[A-Za-z0-9_]+$");
  QRegExpValidator *validator = new QRegExpValidator(rx, this);
  nameEdit->setValidator(validator);

  connect(xSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));
  connect(ySpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));
  connect(zSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));
  connect(nameEdit, SIGNAL(textChanged(QString)),
          this, SLOT(updateCurrentProbeName(QString)));
  connect(typeList, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(updateCurrentProbeType(QString)));

  dGroup->setTitle("Anterior to Posterior Angle");
  aGroup->setTitle("Left to Right Angle");
  depthGroup->setTitle("Probe Depth");
  posBox->setTitle("R to L, P to A, S to I");

  qobject_cast<QFormLayout *>(dGroup->layout())->addRow(declinationLabel,
                                                        declinationSlider);
  qobject_cast<QFormLayout *>(aGroup->layout())->addRow(azimuthLabel,
                                                        azimuthSlider);
  qobject_cast<QFormLayout *>(depthGroup->layout())->addRow(depthSpin,
                                                        depthSlider);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(xSpin);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(ySpin);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(zSpin);

  dGroup->layout()->setAlignment(declinationSlider, Qt::AlignVCenter);
  aGroup->layout()->setAlignment(azimuthSlider, Qt::AlignVCenter);
  depthGroup->layout()->setAlignment(depthSlider, Qt::AlignVCenter);

  int declinationRange[2] = {0, 180};
  int azimuthRange[2] = {0, 180};
  int depthRange[2] = {-100, 100};

  declinationSlider->setOrientation(Qt::Horizontal);
  declinationSlider->setTracking(true);
  declinationSlider->setTickInterval(1);
  declinationSlider->setMinimum(declinationRange[0]);
  declinationSlider->setMaximum(declinationRange[1]);
  declinationLabel->setMinimum(declinationRange[0]);
  declinationLabel->setMaximum(declinationRange[1]);
  connect(declinationSlider,SIGNAL(valueChanged(int)),
          declinationLabel, SLOT(setValue(int)));
  connect(declinationLabel, SIGNAL(valueChanged(int)),
          declinationSlider, SLOT(setValue(int)));

  azimuthSlider->setOrientation(Qt::Horizontal);
  azimuthSlider->setTracking(true);
  azimuthSlider->setTickInterval(1);
  azimuthSlider->setMinimum(azimuthRange[0]);
  azimuthSlider->setMaximum(azimuthRange[1]);
  azimuthLabel->setMinimum(azimuthRange[0]);
  azimuthLabel->setMaximum(azimuthRange[1]);
  connect(azimuthSlider,SIGNAL(valueChanged(int)),
          azimuthLabel, SLOT(setValue(int)));
  connect(azimuthLabel, SIGNAL(valueChanged(int)),
          azimuthSlider, SLOT(setValue(int)));

  depthSlider->setOrientation(Qt::Horizontal);
  depthSlider->setTracking(true);
  depthSlider->setTickInterval(1);
  depthSlider->setMinimum(depthRange[0]*sliderSubdivisions);
  depthSlider->setMaximum(depthRange[1]*sliderSubdivisions);

  depthSpin->setMinimum(depthRange[0]);
  depthSpin->setMaximum(depthRange[1]);
  depthSpin->setDecimals(spinBoxDecimals);
  depthSpin->setSingleStep(1.0/sliderSubdivisions);

  connect(depthSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateDepthSliderDouble(double)));
  connect(this, SIGNAL(updateDepthSliderInt(int)),
          depthSlider, SLOT(setValue(int)));
  connect(depthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateDepthSpinBoxInt(int)));
  connect(this, SIGNAL(updateDepthSpinBoxDouble(double)),
          depthSpin, SLOT(setValue(double)));

  azimuthSlider->setValue(90);
  declinationSlider->setValue(90);
  depthSpin->setValue(0);

  connect(azimuthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(declinationSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(depthSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbeDepth()));

  connect(removeButton, SIGNAL(clicked()), this, SLOT(RemoveCurrentFromPlan()));

  crudWidget->setLayout(new QGridLayout);
  qobject_cast<QGridLayout *>(crudWidget->layout())->addWidget(removeButton, 1, 0);

  connect(saveButton, SIGNAL(clicked()), this, SLOT(savePlanReport()));
  connect(exportButton, SIGNAL(clicked()), this, SIGNAL(ExportScreenshot()));

  connect(placedList, SIGNAL(itemSelectionChanged()),
          this, SLOT(updateForCurrentSelection()));

  vertical->addWidget(desc);
  vertical->addWidget(tabWidget);

  QShortcut *shortcut_del =
    new QShortcut(QKeySequence(Qt::Key_Delete), this->placedList);
  QShortcut *shortcut_bs =
    new QShortcut(QKeySequence(Qt::Key_Backspace), this->placedList);

  connect(shortcut_del, SIGNAL(activated()),
          this, SLOT(RemoveCurrentFromPlan()));
  connect(shortcut_bs, SIGNAL(activated()),
          this, SLOT(RemoveCurrentFromPlan()));

  this->widget->setLayout(vertical);
  emit finished();
}

cbElectrodePlanStage::~cbElectrodePlanStage()
{
}

void cbElectrodePlanStage::Execute()
{
}

const char *cbElectrodePlanStage::getStageName() const
{
  return "Plan";
}

void cbElectrodePlanStage::RemoveCurrentFromPlan()
{
  if (!this->widget->isVisible()) {
    return;
  }

  int pos = this->placedList->currentRow();

  if (pos == -1) {
    std::cout << "Nothing in list." << std::endl;
    return;
  }

  QMessageBox dialog;
  dialog.setText("Are you sure you want to delete probe:");
  dialog.setInformativeText(this->Plan.at(pos).ToString().c_str());
  dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  dialog.setDefaultButton(QMessageBox::Cancel);
  int ret = dialog.exec();

  if (ret == QMessageBox::Cancel) {
    return;
  }

  QListWidgetItem *item = this->placedList->takeItem(pos);
  delete item;

  // need to remove from this->Plan
  this->Plan.erase(this->Plan.begin()+pos);

  emit DestroyProbeCallback(pos);
}

void cbElectrodePlanStage::CreateProbeRequest(
  double x, double y, double z, double a, double d, double depth,
  std::string n, std::string s)
{
  this->tabWidget->setEnabled(true);

  std::string name;
  if (n.empty()) {
    name = this->nameEdit->text().toStdString();
  } else {
    name = n;
  }

  std::string query;
  if (s.empty()) {
    query = this->typeList->currentText().toStdString();
  } else {
    query = s;
  }

  // Build the standard probe
  cbProbe temp(x, y, z, a, d, depth, name.c_str());

  // Get the specification from the catalogue using the query
  temp.set_specification(this->catalogue_.specification(query));

  emit CreateProbeCallback(temp);

  this->Plan.push_back(temp);

  // Get string representation of probe
  std::string str = temp.ToString();

  // Convert to QString and add to list
  QString qstr(str.c_str());

  new QListWidgetItem(qstr, this->placedList);
  this->placedList->setCurrentRow(this->placedList->count() - 1);

  if (n.empty()) {
    this->nameEdit->setText(QString::number(this->Plan.size()));
  } else {
    this->nameEdit->setText(QString(n.c_str()));
  }
}

void cbElectrodePlanStage::updateForCurrentSelection()
{
  int pos = this->placedList->currentRow();

  // disconnect the updateCurrentProbePosition, updateCurrentProbeOrientation
  // slots as they will just cause a bunch of repetitive signals to be sent.
  // A single, uniform signal will be sent at the end
  disconnect(this->xSpin, SIGNAL(valueChanged(double)),
             this, SLOT(updateCurrentProbePosition()));
  disconnect(this->ySpin, SIGNAL(valueChanged(double)),
             this, SLOT(updateCurrentProbePosition()));
  disconnect(this->zSpin, SIGNAL(valueChanged(double)),
             this, SLOT(updateCurrentProbePosition()));

  disconnect(this->nameEdit, SIGNAL(textChanged(QString)),
             this, SLOT(updateCurrentProbeName(QString)));

  disconnect(this->typeList, SIGNAL(currentIndexChanged(QString)),
             this, SLOT(updateCurrentProbeType(QString)));

  disconnect(this->azimuthSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbeOrientation()));
  disconnect(this->declinationSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbeOrientation()));
  disconnect(this->depthSpin, SIGNAL(valueChanged(double)),
             this, SLOT(updateCurrentProbeDepth()));

  if (pos == -1 || this->placedList->count() <= 0) {
    this->xSpin->setValue(0);
    this->ySpin->setValue(0);
    this->zSpin->setValue(0);

    this->azimuthSlider->setValue(90);
    this->declinationSlider->setValue(90);

    this->depthSpin->setValue(0);

    this->nameEdit->setText(QString::number(0));
    this->typeList->setCurrentIndex(0);

    return;
  }

  cbProbe temp = this->Plan.at(pos);

  double p[3];
  double o[2];
  temp.GetPosition(p);
  temp.GetOrientation(o);

  double depth = temp.GetDepth();
  double tol = pow(0.1, spinBoxDecimals);

  if (fabs(this->xSpin->value() - p[0]) < tol &&
      fabs(this->ySpin->value() - p[1]) < tol &&
      fabs(this->zSpin->value() - p[2]) < tol &&
      this->azimuthSlider->value() == static_cast<int>(o[0]) &&
      this->declinationSlider->value() == static_cast<int>(o[1]) &&
      fabs(this->depthSpin->value() - depth) < tol) {
    // No need to update anything
    return;
  }

  this->xSpin->setValue(floor(p[0]/tol + 0.5)*tol);
  this->ySpin->setValue(floor(p[1]/tol + 0.5)*tol);
  this->zSpin->setValue(floor(p[2]/tol + 0.5)*tol);

  this->azimuthSlider->setValue(o[0]);
  this->declinationSlider->setValue(o[1]);

  this->depthSpin->setValue(depth);

  this->nameEdit->setText(QString(temp.GetName().c_str()));

  int type_index = this->typeList->findText(temp.specification().catalogue_number().c_str());
  this->typeList->setCurrentIndex(type_index);

  // Gather coords and tell view to update display
  this->updateCurrentProbePosition();
  this->updateCurrentProbeOrientation();
  this->updateCurrentProbeDepth();

  // Reconnect the slider signals, now that updating has occured.
  connect(this->xSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));
  connect(this->ySpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));
  connect(this->zSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbePosition()));

  connect(this->nameEdit, SIGNAL(textChanged(QString)),
          this, SLOT(updateCurrentProbeName(QString)));

  connect(this->typeList, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(updateCurrentProbeType(QString)));

  connect(this->azimuthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(this->declinationSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(this->depthSpin, SIGNAL(valueChanged(double)),
          this, SLOT(updateCurrentProbeDepth()));
}

// Called when a spinbox has changed. If there is a selection, update the
// position of the probe object, and tell the view to re-display it.
void cbElectrodePlanStage::updateCurrentProbePosition()
{
  int pos = this->placedList->currentRow();
  if (pos == -1) {
    std::cout << "Nothing selected." << std::endl;
    return;
  }

  double position[3];
  position[0] = this->xSpin->value();
  position[1] = this->ySpin->value();
  position[2] = this->zSpin->value();

  // Update the position in the probe
  this->Plan.at(pos).SetPosition(position);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);

  QString str(this->Plan.at(pos).ToString().c_str());
  item->setText(str);

  emit UpdateProbeCallback(pos, this->Plan.at(pos));
}

// Called when a slider has changed. If there is a selection, update the
// position of the probe object, and tell the view to re-display it.
void cbElectrodePlanStage::updateCurrentProbeOrientation()
{
  int pos = this->placedList->currentRow();
  if (pos == -1) {
    std::cout << "Nothing selected." << std::endl;
    return;
  }

  double orientation[3];
  orientation[0] = this->azimuthSlider->value();
  orientation[1] = this->declinationSlider->value();

  // Update the orientation in the probe
  this->Plan.at(pos).SetOrientation(orientation);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);
  item->setText(QString(this->Plan.at(pos).ToString().c_str()));

  emit UpdateProbeCallback(pos, this->Plan.at(pos));
}

void cbElectrodePlanStage::savePlanReport()
{
  std::stringstream plan;

  QDateTime datetime = QDateTime::currentDateTime();
  plan << datetime.toString().toStdString() << std::endl << std::endl;

  for (size_t i = 0; i < this->Plan.size(); i++)
    {
    plan << "#" << (i+1) << ": " << this->Plan.at(i).ToString() << std::endl;
    }
  plan << std::endl << "Total: " << this->Plan.size() << std::endl;

  QMessageBox box;
  box.setText(plan.str().c_str());
  box.setStandardButtons(QMessageBox::Save | QMessageBox::Close);
  box.setDefaultButton(QMessageBox::Save);

  int ret = box.exec();

  if (ret != QMessageBox::Save) {
    return;
  }

  // Check the planFileFolder setting for the save path
  const char *folderKey = "planFileFolder";
  QSettings settings;
  QString path;

  if (settings.value(folderKey).toString() != NULL) {
    path = settings.value(folderKey).toString();
  }

  if (path == "") {
    path = QDir::homePath();
  }

  // Open a file dialog to find where to save
  // If successful, open a stream to that file and append all the probes
  QString fileName = QFileDialog::getSaveFileName(NULL, "Save Operation Report",
                                                  path + "/untitled.txt",
                                                  "Text Files (*.txt)");

  if (fileName.isNull()) {
    return;
  }

  // Save the planFileFolder for the next time a plan is loaded
  QFileInfo fileInfo(fileName);
  settings.setValue(folderKey, fileInfo.path());

  std::ofstream file;
  file.open(fileName.toStdString().c_str());
  file << plan.str();
  file.close();

  cbMainWindow::displaySuccessMessage(QString("File saved."));
}

void cbElectrodePlanStage::toggleFrameVisualization(int s)
{
  if (s) {
    emit EnableFrameVisualization();
    return;
  }
  emit DisableFrameVisualization();
}

void cbElectrodePlanStage::toggleTagVisualization(int s)
{
  if (s) {
    emit EnableTagVisualization();
    return;
  }
  emit DisableTagVisualization();
}

void cbElectrodePlanStage::updateCurrentProbeName(QString n)
{
  int pos = this->placedList->currentRow();
  if (pos == -1) {
    std::cout << "Nothing selected." << std::endl;
    return;
  }

  std::string name = n.toStdString();

  if (n.isEmpty()) {
    name = "noid";
  }

  this->Plan.at(pos).SetName(name);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);

  QString str(this->Plan.at(pos).ToString().c_str());
  item->setText(str);
}

void cbElectrodePlanStage::SavePlanToFile(std::string path)
{
  std::ofstream file;

  // make sure to append to the file, since the view added the image path
  file.open(path.c_str(), std::ios::app);

  for (size_t i = 0; i < this->Plan.size(); i++) {
    file << this->Plan.at(i) << std::endl;
  }

  file.close();
}

void cbElectrodePlanStage::ClearCurrentPlan()
{
  // iterate through the plan (FROM THE END) destroying all probes
  for (int i = this->Plan.size()-1; i >= 0; i--) {
    emit DestroyProbeCallback(i);

    QListWidgetItem *item = this->placedList->takeItem(i);
    delete item;

    // need to remove from this->Plan
    this->Plan.erase(this->Plan.begin()+i);

    std::cout << "cleared: " << i << std::endl;
  }
}

void cbElectrodePlanStage::updateCurrentProbeType(QString n)
{
  int pos = this->placedList->currentRow();
  if (pos == -1) {
    std::cout << "Nothing selected." << std::endl;
    return;
  }

  cbProbeSpecification s = this->catalogue_.specification(n.toStdString());
  this->Plan.at(pos).set_specification(s);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);

  QString str(this->Plan.at(pos).ToString().c_str());
  item->setText(str);

  emit UpdateProbeCallback(pos, this->Plan.at(pos));
}

std::string cbElectrodePlanStage::get_probe_dir()
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
  dir.cd("Resources");

  QString probe_path = QFile((dir).absoluteFilePath("Probes")).fileName();
  return probe_path.toStdString();
}

void cbElectrodePlanStage::opacitySliderChanged(int o)
{
  int max = this->opacitySlider->maximum();
  double val = static_cast<double>(o)/static_cast<double>(max);
  emit SetCTOpacity(val);
}

void cbElectrodePlanStage::setPrecision(QString text)
{
  double precision = text.toDouble();
  int oldSubdivisions = sliderSubdivisions;
  sliderSubdivisions = static_cast<int>(1.0/precision + 0.5);
  spinBoxDecimals = (sliderSubdivisions > 1 ? 1 : 0);

  xSpin->setDecimals(spinBoxDecimals);
  xSpin->setSingleStep(1.0/sliderSubdivisions);
  ySpin->setDecimals(spinBoxDecimals);
  ySpin->setSingleStep(1.0/sliderSubdivisions);
  zSpin->setDecimals(spinBoxDecimals);
  zSpin->setSingleStep(1.0/sliderSubdivisions);
  depthSpin->setDecimals(spinBoxDecimals);
  depthSpin->setSingleStep(1.0/sliderSubdivisions);

  disconnect(depthSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateDepthSpinBoxInt(int)));

  double depthRange[2];
  depthRange[0] = depthSlider->minimum()*1.0/oldSubdivisions;
  depthRange[1] = depthSlider->maximum()*1.0/oldSubdivisions;
  double depth = depthSlider->value()*1.0/oldSubdivisions;
  depthSlider->setMinimum(floor(depthRange[0]*sliderSubdivisions + 0.5));
  depthSlider->setMaximum(floor(depthRange[1]*sliderSubdivisions + 0.5));
  depthSlider->setValue(floor(depth*sliderSubdivisions + 0.5));

  connect(depthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateDepthSpinBoxInt(int)));
}

void cbElectrodePlanStage::placeProbeCallback()
{
  this->tabWidget->setEnabled(false);
  emit InitiatePlaceProbeCallback();
}

void cbElectrodePlanStage::updateCurrentProbeDepth()
{
  int pos = this->placedList->currentRow();
  if (pos == -1) {
    std::cout << "Nothing selected." << std::endl;
    return;
  }

  double depth = this->depthSpin->value();
  double v[3];
  this->computeDepthVector(depth, v);
  // round to 1 decimal place
  for (int i = 0; i < 3; i++) {
    v[i] = 0.1*floor(v[i]/0.1 + 0.5);
  }

  // Update the depth in the probe
  this->Plan.at(pos).SetDepth(depth);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);

  QString str(this->Plan.at(pos).ToString().c_str());
  item->setText(str);

  emit UpdateProbeCallback(pos, this->Plan.at(pos));
}

void cbElectrodePlanStage::updateDepthSliderDouble(double depth)
{
  // round double into an integer after multiplying to account for
  // the difference in resolution between spin box and slider
  emit updateDepthSliderInt(
    static_cast<int>(floor(depth*sliderSubdivisions + 0.5)));
}

void cbElectrodePlanStage::updateDepthSpinBoxInt(int depthInt)
{
  // divide integer to create a double with a higher resolution
  emit updateDepthSpinBoxDouble(
    static_cast<double>(depthInt)/sliderSubdivisions);
}

void cbElectrodePlanStage::computeDepthVector(double depth, double v[3]) const
{
  const double degreesPerRadian = 180.0/3.1415926535897931;

  double theta = this->azimuthSlider->value()/degreesPerRadian;
  double phi = this->declinationSlider->value()/degreesPerRadian;

  v[0] = -depth*cos(theta);
  v[1] = depth*sin(theta)*cos(phi);
  v[2] = -depth*sin(theta)*sin(phi);
}
