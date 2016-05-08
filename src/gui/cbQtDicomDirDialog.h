/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirDialog.h

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

#ifndef __cbQtDicomDirDialog_h
#define __cbQtDicomDirDialog_h

#include <QDialog>
#include <QStringList>

class QFileDialog;

class cbQtDicomDirView;
class cbQtDicomDataView;
class cbQtDicomDirModel;
class cbQtDicomDataModel;

//! A dialog for opening a DICOM series.
/*!
 *  When executed, this dialog will first bring up a native file dialog,
 *  and if a directory or a DICOM file is selected, then a secondary
 *  dialog will appear that allows the user to choose a DICOM series.
 */
class cbQtDicomDirDialog : public QDialog
{
  Q_OBJECT

public:
  //! Construct the view widget.
  explicit cbQtDicomDirDialog(
    QWidget *parent, const QString& caption, const QString& directory);

  //! Destructor.
  ~cbQtDicomDirDialog();

  //! Execute the dialog.
  int exec();

  //! Get the files in the selected DICOM series.
  QStringList selectedFiles();

public slots:
  //! Add files for viewing.
  void addFiles(const QStringList& files);

  //! Called when the model resets.
  void resetSelection();

  //! Set the files to be returned.
  void setSelectedFiles(const QStringList& files);

protected:
  void showDicomBrowser(const QStringList& files);

  QFileDialog *m_FileDialog;
  cbQtDicomDirView *m_DirView;
  cbQtDicomDirModel *m_DirModel;
  QStringList m_SelectedFiles;
};

#endif /* __cbQtDicomDirDialog_h */
