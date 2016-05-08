/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirModel.h

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

#ifndef __cbQtDicomDirModel_h
#define __cbQtDicomDirModel_h

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>

#include <vtkDICOMTag.h>

class vtkDICOMDirectory;
class vtkDICOMValue;

class cbQtDicomDirThread;

//! A Qt data model for the vtkDICOMDirectory class.
/*!
 *  This is a model class that allows vtkDICOMDirectory to be used with
 *  a Qt view.  When given a directory name via setDirName(), it spawns
 *  a thread that uses vtkDICOMDirectory to search for DICOM files within
 *  the directory.  Once the scan is complete, a listing of all DICOM
 *  studies and series in the directory is added to the model.
 */
class cbQtDicomDirModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  //! Construct an unpopulated model.
  explicit cbQtDicomDirModel(QObject *parent = 0);

  //! Destructor.
  ~cbQtDicomDirModel();

  //! Set the directory (this causes the directory to be scanned).
  void setDirName(const QString& d);

  //! Get the directory name that was set.
  const QString& dirName() const { return m_DirName; }

  //! Set a list of input files or directories.
  void setPaths(const QStringList& sl);

  //! Get a list of input files or directories.
  const QStringList& paths() const { return m_Paths; }

  //! Set the recursion depth for the directory scan.
  void setScanDepth(int depth);

  //! Get the recursion depth.
  int scanDepth() const { return m_ScanDepth; };

  //! Get the study from the index.
  /*!
   *  This will return -1 if the index is not valid.
   */
  int getStudy(const QModelIndex& idx) const;

  //! Get the series from the index.
  /*!
   *  This will return -1 if the index is invalid or if it is
   *  at the study level, rather than the series level.
   */
  int getSeries(const QModelIndex& idx) const;

  //! Get the file names for the series specified by "index".
  QStringList fileNames(const QModelIndex& idx) const;

  //! Used by the view to get the table to display.
  QVariant data(const QModelIndex &index, int role) const;

  //! Used by a the view (overridden method).
  Qt::ItemFlags flags(const QModelIndex &idx) const;

  //! Used by the view (overridden method).
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;

  //! Get the index for the child at row,col within the parent.
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;

  //! Get the parent of the given item in the table.
  QModelIndex parent(const QModelIndex &idx) const;

  //! Get the total number of child rows for the given parent.
  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  //! Get the total number of colums in the table.
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  //! Used by the view to check if an update is required.
  bool canFetchMore(const QModelIndex &idx) const;

  //! Used by the view to request updates.
  void fetchMore(const QModelIndex &idx);

protected:
  //! Listen for timer events.
  void timerEvent(QTimerEvent *event);

private:
  //! A helper class used to create a default list of columns.
  struct StaticColumnInfo
  {
    const char *title;
    vtkDICOMTag::StaticTag studyTag;
    vtkDICOMTag::StaticTag seriesTag;
  };

  //! A helper class to store the current columns in the table.
  class ColumnInfo
  {
  public:
    ColumnInfo(const QString& t, vtkDICOMTag t1, vtkDICOMTag t2) :
      m_Title(t), m_StudyTag(t1), m_SeriesTag(t2) {}

    ColumnInfo(const StaticColumnInfo& i) :
      m_Title(i.title), m_StudyTag(i.studyTag), m_SeriesTag(i.seriesTag) {}

    const QString& title() const { return m_Title; }
    vtkDICOMTag studyTag() const { return m_StudyTag; }
    vtkDICOMTag seriesTag() const { return m_SeriesTag; }

  private:
    QString m_Title;
    vtkDICOMTag m_StudyTag;
    vtkDICOMTag m_SeriesTag;
  };

  //! Compute a unique Id, given the row under the study element.
  quint32 computeInternalId(int study, int row) const;

  //! Compute a unique Id for the study itself.
  quint32 computeInternalId(int study) const;

  //! Create the data value for the view to display.
  /*!
   *  Qt models use QVariant to return data values.  This method
   *  converts a vtkDICOMValue into a QVariant.  Dates and times
   *  are stored in the QVariant as QDate and QTime.  Patient names
   *  are converted into QString after filtering out the "^" separators
   *  that DICOM uses in patient names.  All DICOM strings are assumed
   *  to either be ASCII or Latin1 (ISO_IR 100), other encodings are not
   *  yet supported.
   */
  static QVariant makeVariantFromValue(const vtkDICOMValue& val);

  QString m_DirName;
  QStringList m_Paths;
  cbQtDicomDirThread *m_Thread;
  vtkDICOMDirectory *m_Directory;
  QList<ColumnInfo> m_Columns;
  int m_ScanDepth;
  int m_TimerId;
  int m_TimerCount;
  int m_Progress;
  bool m_Fetched;
  QString m_Status;

  static StaticColumnInfo s_StaticColumns[];
};

#endif /* __cbQtDicomDirModel_h */
