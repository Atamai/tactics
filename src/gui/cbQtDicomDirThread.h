/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirThread.h

  Copyright (c) 2014 David Gobbi
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

#ifndef __cbQtDicomDirThread_h
#define __cbQtDicomDirThread_h

#include <QThread>
#include <QStringList>

class vtkDICOMDirectory;

//! A helper thread to scan the directory without blocking the application.
class cbQtDicomDirThread : public QThread
{
  Q_OBJECT

public:
  //! Construct the thread, but do not yet start it.
  cbQtDicomDirThread(
    const QString& dirname, int depth, QObject *parent = 0);

  //! Construct the thread, but do not yet start it.
  cbQtDicomDirThread(
    const QStringList& paths, int depth, QObject *parent = 0);

  //! Destructor.
  ~cbQtDicomDirThread();

  //! Get the vtkDICOMDirectory object that did the scan.
  /*!
   *  This method will return NULL unless the scan was completed successfully.
   */
  vtkDICOMDirectory *directory();

  //! Get the current progress, as a percentage.
  int progress() const { return m_Progress; }

  //! Abort any scans that are in progress.
  void abort();

protected:
  //! Called when the thread is told to start.
  virtual void run();

private:
  //! This method is called by vtkDICOMDirectory to check the abort flag.
  void abortCheck();

  vtkDICOMDirectory *m_Directory;
  int m_Progress;
  bool m_AbortFlag;
};

#endif /* __cbQtDicomDirThread_h */
