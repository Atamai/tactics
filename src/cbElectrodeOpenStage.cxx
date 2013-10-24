/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeOpenStage.cxx

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

#include "cbElectrodeOpenStage.h"

#include <QtGui>

cbElectrodeOpenStage::cbElectrodeOpenStage() : cbStage()
{
  this->widget = new QWidget;
    QVBoxLayout *vertical = new QVBoxLayout;
      QTextEdit *desc = new QTextEdit;
      QCheckBox *anteriorPosteriorCheck =
        new QCheckBox("Include A/P fiducials to find frame.");
      QPushButton *openButton = new QPushButton("&Open");

  desc->setReadOnly(true);
  desc->insertHtml(
      "\
      <p>Press the <i>Open</i> button, below, to open a T1-weighted axial DICOM series. You may choose whether or not to include the anterior and posterior frame fiducials in the frame finding.</p>\
      <p>Once the primary dataset has been opened, press the <i>Next</i> button, above, \
      to begin planning probe placement.</p>\
      <p>You may now open a secondary series at any time using the file-menu.</p>\
      <p>You may now save your progress at any point using the file-menu or the <code>command+s</code> keyboard shortcut.</p>\
      <p>Mouse capabilities:</p>\
      <ul>\
        <li>Left Click: Use active tool (set in the toolbar, above).</li>\
        <li>Right Click: Rotate the view in 3D.</li>\
        <li>Mousewheel (slice-view): Zoom in/out.</li>\
        <li>Mousewheel (side-panes): Slice through view.</li>\
      </ul>\
      "
      );
  desc->setStyleSheet("background-color: aliceblue");

  anteriorPosteriorCheck->setChecked(true);

  connect(openButton, SIGNAL(clicked()), this, SLOT(Execute()));
  connect(anteriorPosteriorCheck, SIGNAL(stateChanged(int)),
          this, SIGNAL(registerAntPost(int)));

  vertical->setContentsMargins(11, 0, 11, 0);
  vertical->addWidget(desc);
  vertical->addWidget(anteriorPosteriorCheck);
  vertical->addWidget(openButton);
  vertical->addStretch();

  this->widget->setLayout(vertical);
  emit finished();
}

cbElectrodeOpenStage::~cbElectrodeOpenStage()
{
}

void cbElectrodeOpenStage::Execute()
{
  const char *folderKey = "recentFileFolder";
  QSettings settings;
  QString path;

  if (settings.value(folderKey).toString() != NULL) {
    path = settings.value(folderKey).toString();
  }
  else {
    path = QDir::homePath();
  }

  QFileDialog dialog(NULL, "Open Primary Series", path);
  if (!dialog.exec()) {
    return;
  }

  QString initialImage = dialog.selectedFiles().at(0);

  QFileInfo fileInfo(initialImage);
  path = fileInfo.path();
  settings.setValue(folderKey, path);

  if (initialImage == "") {
    return;
  }

  std::string *pathString = new std::string(initialImage.toStdString());

  emit requestOpenImage(pathString->c_str());

  emit finished();
}

char *cbElectrodeOpenStage::getStageName() const
{
  return "Open";
}

void cbElectrodeOpenStage::OpenImage(std::string path)
{
  std::string *heap_path = new std::string(path);
  emit requestOpenImage(heap_path->c_str());
}
