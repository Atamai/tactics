/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirDialog.cxx

  Copyright (c) 2015 David Gobbi
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

#include "cbQtDicomDirDialog.h"

#include "cbQtDicomDirView.h"
#include "cbQtDicomDirModel.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStringList>

//--------------------------------------------------------------------------
cbQtDicomDirDialog::cbQtDicomDirDialog(
  QWidget *parent, const QString& caption, const QString& directory)
  : QDialog(parent), m_FileDialog(0), m_DirView(0), m_DirModel(0)
{
  m_FileDialog = new QFileDialog(parent, caption, directory);
}

//--------------------------------------------------------------------------
cbQtDicomDirDialog::~cbQtDicomDirDialog()
{
  delete m_FileDialog;
  delete m_DirModel;
}

//--------------------------------------------------------------------------
void cbQtDicomDirDialog::showDicomBrowser(const QStringList& files)
{
  this->setWindowTitle("Browse For DICOM Series");

  m_DirModel = new cbQtDicomDirModel;
  m_DirModel->setScanDepth(4);

  m_DirView = new cbQtDicomDirView(this);
  m_DirView->setAcceptDrops(true);

  m_DirView->setModel(m_DirModel);
  m_DirView->setFocus();

  QPushButton *openButton = new QPushButton("Open", this);
  QPushButton *closeButton = new QPushButton("Close", this);
  openButton->setDefault(true);

  QVBoxLayout *l = new QVBoxLayout;
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(0);
  l->addWidget(m_DirView);

  QHBoxLayout *l2 = new QHBoxLayout;
  l2->setContentsMargins(11, 11, 11, 11);
  l2->addWidget(closeButton);
  l2->addStretch();
  l2->addWidget(openButton);
  l->addLayout(l2);

  this->setLayout(l);

  QObject::connect(
    m_DirModel, SIGNAL(modelReset()),
    this, SLOT(resetSelection()));

  QObject::connect(
     m_DirView, SIGNAL(clicked(const QModelIndex&)),
     m_DirView, SLOT(setActiveIndex(const QModelIndex&)));
  QObject::connect(
    m_DirView, SIGNAL(seriesActivated(const QStringList&)),
    this, SLOT(setSelectedFiles(const QStringList&)));

  QObject::connect(
    closeButton, SIGNAL(clicked()), this, SLOT(reject()));
  QObject::connect(
    openButton, SIGNAL(clicked()), this, SLOT(accept()));

  m_DirModel->setPaths(files);

  this->show();
}

//--------------------------------------------------------------------------
void cbQtDicomDirDialog::resetSelection()
{
  // Check how many series are present
  if (m_DirModel->rowCount(QModelIndex()) >= 1 &&
      m_DirModel->rowCount(m_DirModel->index(0, 0)) == 1) {
    // If only one series is present, then show it
    QModelIndex idx = m_DirModel->index(0, 0);
    idx = m_DirModel->index(0, 0, idx);
    this->setSelectedFiles(m_DirModel->fileNames(idx));
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirDialog::setSelectedFiles(const QStringList& files)
{
  m_SelectedFiles = files;
}

//--------------------------------------------------------------------------
void cbQtDicomDirDialog::addFiles(const QStringList& files)
{
  m_DirModel->setPaths(files);
}

//--------------------------------------------------------------------------
int cbQtDicomDirDialog::exec()
{
  if (m_FileDialog->exec()) {
    QStringList files = m_FileDialog->selectedFiles();
    QStringList dirs;
    QStringList niftiFiles;
    for (int i = 0; i < files.size(); i++) {
      QFileInfo info(files[i]);
      if (info.isDir()) {
        dirs.append(files[i]);
      }
      else if (files[i].endsWith(".nii", Qt::CaseInsensitive) ||
               files[i].endsWith(".nii.gz", Qt::CaseInsensitive)) {
        niftiFiles.append(files[i]);
      }
      else {
        dirs.append(info.path());
      }
    }

    if (niftiFiles.size() > 1) {
      this->setSelectedFiles(niftiFiles);
      return 1;
    }
    else if (dirs.size() > 0) {
      dirs.removeDuplicates();
      this->showDicomBrowser(dirs);
      if (this->QDialog::exec()) {
        return (this->selectedFiles().size() > 0);
      }
    }
  }

  return false;
}

//--------------------------------------------------------------------------
QStringList cbQtDicomDirDialog::selectedFiles()
{
  return m_SelectedFiles;
}
