/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirThread.cxx

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

#include "cbQtDicomDirThread.h"

#include <vtkDICOMDirectory.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>
#include <vtkCommand.h>

//--------------------------------------------------------------------------
cbQtDicomDirThread::cbQtDicomDirThread(
  const QString& dirname, int depth, QObject *parent)
  : QThread(parent), m_Directory(0), m_Progress(0), m_AbortFlag(false)
{
  vtkSmartPointer<vtkStringArray> a =
    vtkSmartPointer<vtkStringArray>::New();
  a->InsertNextValue(dirname.toLocal8Bit());

  m_Directory = vtkDICOMDirectory::New();
  m_Directory->SetInputFileNames(a);
  m_Directory->SetScanDepth(depth);
  m_Directory->RequirePixelDataOff();
  m_Directory->AddObserver(
    vtkCommand::ProgressEvent, this, &cbQtDicomDirThread::abortCheck);
}

//--------------------------------------------------------------------------
cbQtDicomDirThread::cbQtDicomDirThread(
  const QStringList& paths, int depth, QObject *parent)
  : QThread(parent), m_Directory(0), m_Progress(0), m_AbortFlag(false)
{
  vtkSmartPointer<vtkStringArray> a =
    vtkSmartPointer<vtkStringArray>::New();
  for (int i = 0; i < paths.size(); i++)
    {
    a->InsertNextValue(paths[i].toLocal8Bit());
    }

  m_Directory = vtkDICOMDirectory::New();
  m_Directory->SetInputFileNames(a);
  m_Directory->SetScanDepth(depth);
  m_Directory->RequirePixelDataOff();
  m_Directory->AddObserver(
    vtkCommand::ProgressEvent, this, &cbQtDicomDirThread::abortCheck);
}

//--------------------------------------------------------------------------
cbQtDicomDirThread::~cbQtDicomDirThread()
{
  m_Directory->Delete();
}

//--------------------------------------------------------------------------
void cbQtDicomDirThread::run()
{
  m_AbortFlag = false;
  m_Directory->Update();
}

//--------------------------------------------------------------------------
void cbQtDicomDirThread::abort()
{
  m_AbortFlag = true;
}

//--------------------------------------------------------------------------
void cbQtDicomDirThread::abortCheck()
{
  if (m_AbortFlag) {
    m_Directory->SetAbortExecute(true);
  }
  else {
    m_Progress = static_cast<int>(m_Directory->GetProgress()*100);
  }
}

//--------------------------------------------------------------------------
vtkDICOMDirectory *cbQtDicomDirThread::directory()
{
  if (isFinished() && !m_AbortFlag) {
    return m_Directory;
  }

  return 0;
}
