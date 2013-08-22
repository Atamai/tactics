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

#include <QtGui>
#include <iostream>
#include <fstream>
#include <sstream>

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
            depthSlider = new QSlider;
            QSpinBox *depthLabel = new QSpinBox;
          QGroupBox *posBox = new QGroupBox;
            xSpin = new QSpinBox;
            ySpin = new QSpinBox;
            zSpin = new QSpinBox;
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
          QCheckBox *frameToggle = new QCheckBox("Show frame.");
          QCheckBox *axialToggle = new QCheckBox("Show axial plane.");
          QCheckBox *sagittalToggle = new QCheckBox("Show sagittal plane.");
          QCheckBox *coronalToggle = new QCheckBox("Show coronal plane.");
          QCheckBox *probeToggle = new QCheckBox("Show all probes.");
          QCheckBox *helpToggle = new QCheckBox("Show help annotation.");
          QCheckBox *patientToggle = new QCheckBox("Show patient information.");

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
  optionWidget->setLayout(new QVBoxLayout);

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

  optionWidget->layout()->addWidget(opacityBox);
  optionWidget->layout()->addWidget(frameToggle);
  optionWidget->layout()->addWidget(axialToggle);
  optionWidget->layout()->addWidget(sagittalToggle);
  optionWidget->layout()->addWidget(coronalToggle);
  optionWidget->layout()->addWidget(probeToggle);
  optionWidget->layout()->addWidget(helpToggle);
  optionWidget->layout()->addWidget(patientToggle);

  QVBoxLayout *optionLayout = qobject_cast<QVBoxLayout *>(optionWidget->layout());
  optionLayout->addStretch();

  QFormLayout *opacity_layout = new QFormLayout;
  opacity_layout->addRow(opacityLabel, opacitySlider);
  opacity_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
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

  frameToggle->setChecked(true);
  axialToggle->setChecked(true);
  sagittalToggle->setChecked(true);
  coronalToggle->setChecked(true);
  probeToggle->setChecked(true);
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
  connect(helpToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(ToggleHelpAnnotations(int)));
  connect(patientToggle, SIGNAL(stateChanged(int)),
          this, SIGNAL(TogglePatientAnnotations(int)));

  xSpin->setMinimum(0);
  xSpin->setMaximum(200);

  ySpin->setMinimum(0);
  ySpin->setMaximum(200);

  zSpin->setMinimum(0);
  zSpin->setMaximum(200);

  xSpin->setValue(0);
  ySpin->setValue(0);
  zSpin->setValue(0);

  QRegExp rx("^[A-Za-z0-9_]+$");
  QRegExpValidator *validator = new QRegExpValidator(rx, this);
  nameEdit->setValidator(validator);

  connect(xSpin, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbePosition()));
  connect(ySpin, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbePosition()));
  connect(zSpin, SIGNAL(valueChanged(int)),
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
  qobject_cast<QFormLayout *>(depthGroup->layout())->addRow(depthLabel,
                                                        depthSlider);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(xSpin);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(ySpin);
  qobject_cast<QHBoxLayout *>(posBox->layout())->addWidget(zSpin);

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
  depthSlider->setMinimum(depthRange[0]);
  depthSlider->setMaximum(depthRange[1]);
  depthLabel->setMinimum(depthRange[0]);
  depthLabel->setMaximum(depthRange[1]);
  connect(depthSlider,SIGNAL(valueChanged(int)),
          depthLabel, SLOT(setValue(int)));
  connect(depthLabel, SIGNAL(valueChanged(int)),
          depthSlider, SLOT(setValue(int)));

  azimuthSlider->setValue(90);
  declinationSlider->setValue(90);
  depthSlider->setValue(0);

  connect(azimuthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(declinationSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(depthSlider, SIGNAL(valueChanged(int)),
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

char *cbElectrodePlanStage::getStageName() const
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

void cbElectrodePlanStage::CreateProbeRequest(int x, int y, int z, int a, int d, std::string n, std::string s)
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
  cbProbe temp(x, y, z, a, d, name.c_str());

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
  disconnect(this->xSpin, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbePosition()));
  disconnect(this->ySpin, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbePosition()));
  disconnect(this->zSpin, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbePosition()));

  disconnect(this->nameEdit, SIGNAL(textChanged(QString)),
             this, SLOT(updateCurrentProbeName(QString)));

  disconnect(this->typeList, SIGNAL(currentIndexChanged(QString)),
             this, SLOT(updateCurrentProbeType(QString)));

  disconnect(this->azimuthSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbeOrientation()));
  disconnect(this->declinationSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbeOrientation()));
  disconnect(this->depthSlider, SIGNAL(valueChanged(int)),
             this, SLOT(updateCurrentProbeDepth()));

  if (pos == -1 || this->placedList->count() <= 0) {
    this->xSpin->setValue(0);
    this->ySpin->setValue(0);
    this->zSpin->setValue(0);

    this->azimuthSlider->setValue(90);
    this->declinationSlider->setValue(90);

    this->depthSlider->setValue(0);

    this->nameEdit->setText(QString::number(0));
    this->typeList->setCurrentIndex(0);

    return;
  }

  cbProbe temp = this->Plan.at(pos);

  double p[3];
  double o[2];
  temp.GetPosition(p);
  temp.GetOrientation(o);

  double depth = temp.depth();

  if (this->xSpin->value() == static_cast<int>(p[0]) &&
      this->ySpin->value() == static_cast<int>(p[1]) &&
      this->zSpin->value() == static_cast<int>(p[2]) &&
      this->azimuthSlider->value() == static_cast<int>(o[0]) &&
      this->declinationSlider->value() == static_cast<int>(o[1]) &&
      this->depthSlider->value() == static_cast<int>(depth)) {
    // No need to update anything
    return;
  }

  this->xSpin->setValue(p[0]);
  this->ySpin->setValue(p[1]);
  this->zSpin->setValue(p[2]);

  this->azimuthSlider->setValue(o[0]);
  this->declinationSlider->setValue(o[1]);

  this->depthSlider->setValue(depth);

  this->nameEdit->setText(QString(temp.GetName().c_str()));

  int type_index = this->typeList->findText(temp.specification().catalogue_number().c_str());
  this->typeList->setCurrentIndex(type_index);

  // Gather coords and tell view to update display
  this->updateCurrentProbePosition();
  this->updateCurrentProbeOrientation();
  this->updateCurrentProbeDepth();

  // Reconnect the slider signals, now that updating has occured.
  connect(this->xSpin, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbePosition()));
  connect(this->ySpin, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbePosition()));
  connect(this->zSpin, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbePosition()));

  connect(this->nameEdit, SIGNAL(textChanged(QString)),
          this, SLOT(updateCurrentProbeName(QString)));

  connect(this->typeList, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(updateCurrentProbeType(QString)));

  connect(this->azimuthSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(this->declinationSlider, SIGNAL(valueChanged(int)),
          this, SLOT(updateCurrentProbeOrientation()));
  connect(this->depthSlider, SIGNAL(valueChanged(int)),
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
  box.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
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

  double depth = this->depthSlider->value();

  // Update the depth in the probe
  this->Plan.at(pos).set_depth(depth);

  // Update the listing
  QListWidgetItem *item = this->placedList->item(pos);

  QString str(this->Plan.at(pos).ToString().c_str());
  item->setText(str);

  emit UpdateProbeCallback(pos, this->Plan.at(pos));
}
