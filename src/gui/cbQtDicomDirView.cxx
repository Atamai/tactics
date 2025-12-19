/*=========================================================================
  Program: Cerebra
  Module:  cbQtDicomDirView.cxx

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

#include "cbQtDicomDirView.h"
#include "cbQtDicomDirModel.h"

#include <QDropEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMimeData>
#include <QList>
#include <QScrollBar>
#include <QStringList>
#include <QUrl>

#include <iostream>

//--------------------------------------------------------------------------
cbQtDicomDirView::cbQtDicomDirView(QWidget *parent)
  : QTreeView(parent)
{
  this->setAlternatingRowColors(true);

  this->connect(
    this, SIGNAL(activated(const QModelIndex&)),
    this, SLOT(setActiveIndex(const QModelIndex&)));
}

//--------------------------------------------------------------------------
cbQtDicomDirView::~cbQtDicomDirView()
{
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::setActiveIndex(const QModelIndex& idx)
{
  cbQtDicomDirModel *model =
    qobject_cast<cbQtDicomDirModel *>(this->model());
  if (model && model->getSeries(idx) >= 0) {
    QStringList files = model->fileNames(idx);
    emit seriesActivated(files);
  }
  else if (model && model->getStudy(idx) >= 0) {
    QStringList files = model->fileNames(idx);
    emit seriesActivated(files);
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::reset()
{
  this->QTreeView::reset();
  if (this->model() && this->model()->rowCount(QModelIndex()) <= 5) {
    this->expandAll();
  }
  int n = this->header()->count();
  for (int i = 0; i < n; i++) {
    int w = this->sizeHintForColumn(i);
    if (w > 0) {
      this->setColumnWidth(i, w);
    }
  }
}

//--------------------------------------------------------------------------
int cbQtDicomDirView::sizeHintForColumn(int col) const
{
  // Sample text for each column, for setting column width
  const char *sampleText[] = {
    "xMxxxxxxxxxxxx Mxxxxxxxxxxxx",
    "x1970-01-01x",
    "x10:30 AMx",
    "xModality",
    "x1234567x",
    "x12345x",
    "xThis is a lengthy study descriptionx",
    "xMM00000000000000x",
    "xMM00000000000000x",
    0
  };

  // Make sure that there is sample text available
  for (int i = 0; i < col; i++) {
    if (sampleText[i] == 0) {
      return -1;
    }
  }

  // Compute the width of the text
  QFontMetrics fm(this->viewOptions().font);
  int w = fm.horizontalAdvance(sampleText[col]);

  // For first column, add indentation
  if (col == 0) {
    w += 2*this->indentation();
    int iconWidth = this->viewOptions().decorationSize.width();
    if (iconWidth > 0) {
      w += iconWidth;
    }
  }

  // Check the width of the header text
  int hw = this->header()->sectionSizeHint(col);
  if (hw > w) {
    w = hw;
  }

  return w;
}

//--------------------------------------------------------------------------
QSize cbQtDicomDirView::sizeHint() const
{
  int w = this->header()->length();
  w += this->header()->offset();
  w += this->contentsMargins().left();
  w += this->contentsMargins().right();
  w += 2*this->frameWidth();
  int sw = this->verticalScrollBar()->sizeHint().width();
  if (sw > 0) {
    w += sw;
  }
  return QSize(w, (w*2 + 1)/3);
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::dragEnterEvent(QDragEnterEvent *event)
{
  if ((event->possibleActions() & Qt::CopyAction) != 0) {
    const QMimeData *data = event->mimeData();
    if (data && data->hasUrls()) {
      event->setDropAction(Qt::CopyAction);
      event->accept();
    }
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::dragMoveEvent(QDragMoveEvent *event)
{
  event->accept();
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::dropEvent(QDropEvent *event)
{
  if ((event->possibleActions() & Qt::CopyAction) != 0) {
    event->setDropAction(Qt::CopyAction);

    const QMimeData *data = event->mimeData();
    if (data && data->hasUrls()) {
      cbQtDicomDirModel *model =
        qobject_cast<cbQtDicomDirModel *>(this->model());
      if (model) {
        QStringList pathList;
        const QList<QUrl>& urlList = data->urls();

        for (int i = 0; i < urlList.size(); i++) {
          const QString& s = urlList[i].toLocalFile();
          if (!s.isEmpty()) {
            pathList.append(s);
          }
        }
        if (pathList.size() > 0) {
          model->setPaths(pathList);
          event->accept();
        }
      }
    }
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Return ||
      e->key() == Qt::Key_Enter) {
    e->accept();
    emit activated(this->currentIndex());
  }
  else {
    QTreeView::keyPressEvent(e);
  }
}

//--------------------------------------------------------------------------
void cbQtDicomDirView::keyReleaseEvent(QKeyEvent *e)
{
  QTreeView::keyReleaseEvent(e);
}
