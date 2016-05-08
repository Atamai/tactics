/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirView.h

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

#ifndef __cbQtDicomDirView_h
#define __cbQtDicomDirView_h

#include <QTreeView>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

//! A Qt tree view for the vtkDICOMDirectory class.
/*!
 *  This must be paired with a cbQtDicomDirModel object.
 */
class cbQtDicomDirView : public QTreeView
{
  Q_OBJECT

public:
  //! Construct the view widget.
  explicit cbQtDicomDirView(QWidget *parent = 0);

  //! Destructor.
  ~cbQtDicomDirView();

  //! Catch the reset (this is a virtual slot).
  void reset();

  //! Default size should be enough to see all columns.
  int sizeHintForColumn(int col) const;

  //! Default size should be enough to see all columns.
  QSize sizeHint() const;

signals:
  //! Emitted when a series is activated in the view.
  void seriesActivated(const QStringList& files);

protected slots:
  //! Called when the activated() signal is emitted.
  void setActiveIndex(const QModelIndex& idx);

protected:
  //! Listen for drag enter events.
  void dragEnterEvent(QDragEnterEvent *event);

  //! Listen for drag move events.
  void dragMoveEvent(QDragMoveEvent *event);

  //! Listen for drag drop events.
  void dropEvent(QDropEvent *event);

  //! Listen for certain key presses.
  void keyPressEvent(QKeyEvent *event);

  //! Listen for certain key presses.
  void keyReleaseEvent(QKeyEvent *event);
};

#endif /* __cbQtDicomDirView_h */
