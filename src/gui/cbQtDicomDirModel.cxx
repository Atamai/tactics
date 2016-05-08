/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirModel.cxx

  Copyright (c) 2013-2014 David Gobbi
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

#include "cbQtDicomDirModel.h"
#include "cbQtDicomDirThread.h"

#include <vtkDICOMDictionary.h>
#include <vtkDICOMDirectory.h>
#include <vtkDICOMItem.h>
#include <vtkStringArray.h>

#include <QTimerEvent>
#include <QDate>
#include <QTime>

//--------------------------------------------------------------------------
// These are the columns available in this model.
cbQtDicomDirModel::StaticColumnInfo cbQtDicomDirModel::s_StaticColumns[] = {
  { "Patient Name", { DC::PatientName }, { DC::SeriesDescription } },
  { "Date", { DC::StudyDate }, { DC::SeriesDate } },
  { "Time", { DC::StudyTime }, { DC::SeriesTime } },
  { "Modality", { DC::Modality }, { DC::Modality } },
  { "ID", { DC::StudyID }, { DC::SeriesNumber } },
  { "# Images", { 0 }, { 0 } },
  { "Study Description", { DC::StudyDescription }, { DC::ProtocolName } },
  { "Patient ID", { DC::PatientID }, { 0 } },
  { "Accession #", { DC::AccessionNumber }, { 0 } },
  { 0, { 0 }, { 0 } }
};

//--------------------------------------------------------------------------
cbQtDicomDirModel::cbQtDicomDirModel(QObject *parent)
  : QAbstractItemModel(parent), m_Thread(0), m_Directory(0),
    m_ScanDepth(1), m_TimerId(-1), m_TimerCount(0), m_Progress(0),
    m_Fetched(false)
{
  // Set up the columns.
  for (int i = 0; s_StaticColumns[i].title != 0; i++) {
    m_Columns.append(s_StaticColumns[i]);
  }
}

//--------------------------------------------------------------------------
cbQtDicomDirModel::~cbQtDicomDirModel()
{
  // Wait for any running threads to abort.
  if (m_Thread && m_Thread->isRunning()) {
    m_Thread->abort();
    m_Thread->wait();
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirModel::setDirName(const QString& name)
{
  // Start scan thread as soon as diretory is set.
  if (m_Thread) {
    // Cancel any currently running scan thread.
    if (m_Thread->isRunning()) {
      m_Thread->abort();
      m_Thread->wait();
    }
    delete m_Thread;
    beginResetModel();
    m_Directory = 0;
    endResetModel();
  }

  // Kill the timer if it is active.
  if (m_TimerId != -1) {
    killTimer(m_TimerId);
    m_TimerId = -1;
  }

  // Create a new scan thread and a timer to poll the thread.
  m_Thread = new cbQtDicomDirThread(name, m_ScanDepth);
  m_Thread->start();
  m_TimerId = startTimer(100);
  m_TimerCount = 0;
  m_Progress = 0;
  m_Fetched = false;
  m_Status = QObject::tr("Loading...");
}

//--------------------------------------------------------------------------
void cbQtDicomDirModel::setPaths(const QStringList& paths)
{
  // Start scan thread as soon as diretory is set.
  if (m_Thread) {
    // Cancel any currently running scan thread.
    if (m_Thread->isRunning()) {
      m_Thread->abort();
      m_Thread->wait();
    }
    delete m_Thread;
    beginResetModel();
    m_Directory = 0;
    endResetModel();
  }

  // Kill the timer if it is active.
  if (m_TimerId != -1) {
    killTimer(m_TimerId);
    m_TimerId = -1;
  }

  // Create a new scan thread and a timer to poll the thread.
  m_Thread = new cbQtDicomDirThread(paths, m_ScanDepth);
  m_Thread->start();
  m_TimerId = startTimer(100);
  m_TimerCount = 0;
  m_Progress = 0;
  m_Fetched = false;
  m_Status = QObject::tr("Loading...");
}

//--------------------------------------------------------------------------
void cbQtDicomDirModel::setScanDepth(int depth)
{
  m_ScanDepth = depth;
}

//--------------------------------------------------------------------------
QStringList cbQtDicomDirModel::fileNames(
  const QModelIndex &idx) const
{
  QStringList files;

  if (!idx.isValid()) {
    return files;
  }

  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    return files;
  }

  int study = getStudy(idx);
  int series = getSeries(idx);

  // Check if this is a study (level 0) rather than a series (level 1).
  if (series < 0) {
    series = m_Directory->GetFirstSeriesForStudy(study);
  }

  vtkStringArray *a = m_Directory->GetFileNamesForSeries(series);
  vtkIdType n = a->GetNumberOfValues();
  for (vtkIdType i = 0; i < n; i++) {
    files.append(QString::fromUtf8(a->GetValue(i)));
  }

  return files;
}

//--------------------------------------------------------------------------
void cbQtDicomDirModel::timerEvent(QTimerEvent *event)
{
  if (m_TimerId != -1 && event->timerId() == m_TimerId) {
    // Check to see if the the directory is ready
    vtkDICOMDirectory *dirobj = m_Thread->directory();
    if (m_Directory == 0 && dirobj != 0) {
      killTimer(m_TimerId);
      m_TimerId = -1;
      m_Fetched = true;
      if (dirobj->GetNumberOfStudies() == 0) {
        m_Status = QObject::tr("No DICOM.");
      }
      beginResetModel();
      m_Directory = dirobj;
      endResetModel();
    }
    else {
      m_TimerCount++;
      int progress = m_Thread->progress();
      if (progress == 0) {
        // Perform an animation on the "Loading..." text.
        if (m_TimerCount % 10 == 0) {
          if (((m_TimerCount / 10) % 2) == 0) {
            m_Status = QObject::tr("Loading...");
          }
          else {
            m_Status = QObject::tr("Loading..");
          }
          quint32 internalId = 0;
          QModelIndex idx = createIndex(0, 0, internalId);
          dataChanged(idx, idx);
        }
      }
      else if (progress > m_Progress && m_TimerCount > 10) {
        // Provide a percentage readout for progress.
        m_Progress = progress;
        m_Status = QObject::tr("Read ") + QString::number(progress) + "%";
        quint32 internalId = 0;
        QModelIndex idx = createIndex(0, 0, internalId);
        dataChanged(idx, idx);
      }
    }
  }
}

//--------------------------------------------------------------------------
bool cbQtDicomDirModel::canFetchMore(const QModelIndex&) const
{
  // The view component calls this to check if more data is available.
  return !m_Fetched;
}

//--------------------------------------------------------------------------
void cbQtDicomDirModel::fetchMore(const QModelIndex&)
{
  // The view component calls this to try to fetch more data.
  if (m_Thread && m_Thread->isFinished() && !m_Fetched) {
    m_Fetched = true;
  }
}

//--------------------------------------------------------------------------
int cbQtDicomDirModel::columnCount(const QModelIndex &) const
{
  return m_Columns.length();
}

//--------------------------------------------------------------------------
QVariant cbQtDicomDirModel::data(const QModelIndex &idx, int role) const
{
  // This method returns data for the view to display.
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (!idx.isValid()) {
    return QVariant();
  }

  // If there is no date, provide status text instead.
  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    if (idx.row() == 0 && idx.column() == 0) {
      return QVariant(m_Status);
    }
    else {
      return QVariant();
    }
  }

  int level = 1;
  int study = getStudy(idx);
  int series = getSeries(idx);

  // Check if this is a study (level 0) rather than a series (level 1).
  if (series < 0) {
    level = 0;
    series = m_Directory->GetFirstSeriesForStudy(study);
  }

  QVariant v;

  // If the column title start with "#", then return the number
  // of images present in the series or in the whole study.
  if (m_Columns[idx.column()].title()[0] == '#') {
    if (level > 0) {
      v = static_cast<int>(m_Directory->GetFileNamesForSeries(
                             series)->GetNumberOfValues());
    }
    else {
      int n = 0;
      int i0 = m_Directory->GetFirstSeriesForStudy(study);
      int i1 = m_Directory->GetLastSeriesForStudy(study);
      for (int i = i0; i <= i1; i++) {
        n += static_cast<int>(m_Directory->GetFileNamesForSeries(
                              i)->GetNumberOfValues());
      }
      v = n;
    }
  }

  // Do a lookup of the DICOM tag for this column of the model.
  vtkDICOMTag tag = m_Columns[idx.column()].studyTag();

  // Check if this is the series level
  if (!v.isValid() && level > 0) {
    tag = m_Columns[idx.column()].seriesTag();
    v = makeVariantFromValue(m_Directory->GetSeriesRecord(
      series).GetAttributeValue(tag));
  }
  // If not found, check the study record
  if (!v.isValid()) {
    v = makeVariantFromValue(m_Directory->GetStudyRecord(
      study).GetAttributeValue(tag));
  }
  // If still not found, check the patient record
  if (!v.isValid()) {
    v = makeVariantFromValue(m_Directory->GetPatientRecordForStudy(
      study).GetAttributeValue(tag));
  }

  return v;
}

//--------------------------------------------------------------------------
Qt::ItemFlags cbQtDicomDirModel::flags(const QModelIndex &idx) const
{
  if (!idx.isValid()) {
    return 0;
  }

  // Let the superclass take care of this.
  return QAbstractItemModel::flags(idx);
}

//--------------------------------------------------------------------------
QVariant cbQtDicomDirModel::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  // Return the column title.
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return QVariant(m_Columns[section].title());
  }

  return QVariant();
}

//--------------------------------------------------------------------------
QModelIndex cbQtDicomDirModel::index(
  int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  quint32 internalId = 0;
  if (!parent.isValid()) {
    if (m_Directory != 0 && m_Directory->GetNumberOfStudies() != 0) {
      // An invalid parent indicates the root, and the root's children are
      // the studies (row gives the study index)
      internalId = computeInternalId(row);
    }
  }
  else {
    // A valid index gives either a study or a series
    internalId = parent.internalId();
    if (getSeries(parent) < 0) {
      // Get the study from the parent's internalId
      int study = getStudy(parent);
      // Create a new internalId that has both the study and the series
      internalId = computeInternalId(study, row);
    }
    else {
      // This means the parent is a series, and a series has no children
      return QModelIndex();
    }
  }

  return createIndex(row, column, internalId);
}

//--------------------------------------------------------------------------
QModelIndex cbQtDicomDirModel::parent(const QModelIndex &idx) const
{
  if (!idx.isValid()) {
    return QModelIndex();
  }

  if (getSeries(idx) < 0 || m_Directory == 0 ||
      m_Directory->GetNumberOfStudies() == 0) {
    // Studies have no parent
    return QModelIndex();
  }
  else {
    // A series has its study as its parent
    int study = getStudy(idx);
    quint32 internalId = computeInternalId(study);
    int row = study;
    return createIndex(row, 0, internalId);
  }
}

//--------------------------------------------------------------------------
int cbQtDicomDirModel::rowCount(const QModelIndex &parent) const
{
  if (parent.column() > 0) {
    return 0;
  }

  // If no data, then provide one row for status information
  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    // Check to make sure "parent" is the root
    if (!parent.isValid()) {
      return 1;
    }
    else {
      return 0;
    }
  }

  if (!parent.isValid()) {
    // The number of rows at the root leve is the number of studies.
    return m_Directory->GetNumberOfStudies();
  }

  if (getSeries(parent) < 0) {
    // The number of rows for a study is the number of series in study.
    int study = getStudy(parent);
    return (m_Directory->GetLastSeriesForStudy(study) -
            m_Directory->GetFirstSeriesForStudy(study) + 1);
  }
  else {
    return 0;
  }
}

//--------------------------------------------------------------------------
quint32 cbQtDicomDirModel::computeInternalId(int study, int row) const
{
  if (m_Directory == 0) {
    return 0;
  }

  // The internalId is composed of the series number and study number.
  return (study + 1 + (row + 1)*(m_Directory->GetNumberOfStudies() + 1));
}

//--------------------------------------------------------------------------
quint32 cbQtDicomDirModel::computeInternalId(int study) const
{
  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    return 0;
  }

  // Compose an internalId that only gives a study (no series).
  return (study + 1);
}

//--------------------------------------------------------------------------
int cbQtDicomDirModel::getStudy(const QModelIndex& idx) const
{
  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    return -1;
  }

  // Get the study number from the internalId.
  int i = static_cast<int>(idx.internalId());
  int n = m_Directory->GetNumberOfStudies();
  return (i % (n + 1)) - 1;
}

//--------------------------------------------------------------------------
int cbQtDicomDirModel::getSeries(const QModelIndex& idx) const
{
  if (m_Directory == 0 || m_Directory->GetNumberOfStudies() == 0) {
    return -1;
  }

  // Get the series number from the internalId.  This returns -1
  // if there is no series number for this internalId.
  int i = static_cast<int>(idx.internalId());
  int n = m_Directory->GetNumberOfStudies();
  int series = (i / (n + 1)) - 1;
  if (series >= 0) {
    int study = (i % (n + 1)) - 1;
    series += m_Directory->GetFirstSeriesForStudy(study);
  }
  return series;
}

//--------------------------------------------------------------------------
QVariant cbQtDicomDirModel::makeVariantFromValue(const vtkDICOMValue& val)
{
  if (!val.IsValid()) {
    return QVariant();
  }

  if (val.GetVR() == vtkDICOMVR::DA) {
    // Convert DICOM date into a QDate
    std::string s = val.AsString();
    QString datestring = s.c_str();
    // Old ACR-NEMA date had "." separators
    datestring.replace('.', "");
    datestring.replace('-', "");
    datestring.replace('/', "");
    return QDate::fromString(datestring, "yyyyMMdd");
  }
  else if (val.GetVR() == vtkDICOMVR::TM) {
    // Convert DICOM time into a QTime
    std::string s = val.AsString();
    QString timestring = s.c_str();
    // Old ACR-NEMA time had ":" separators
    timestring.replace(':', "");
    // Chop off the microseconds
    timestring.truncate(6);
    return QTime::fromString(timestring, "hhmmss");
  }
  else if (val.GetVR() == vtkDICOMVR::PN) {
    // Convert DICOM patient name by removing the "^" characters.
    std::string s = val.AsUTF8String();
    QString qs = QString::fromUtf8(s.data(), s.size());
    int i = qs.indexOf('^');
    if (i >= 0) {
      // Replace the first '^' with a comma
      qs.replace(i, 1, ", ");
    }
    // Replace any other '^' characters with spaces
    qs.replace('^', " ");
    return qs;
  }

  // For now, use strings instead of the native data type
  std::string s = val.AsUTF8String();
  return QVariant(QString::fromUtf8(s.data(), s.size()));
}
