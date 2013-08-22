/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeIntroductionStage.cxx

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

#include "cbElectrodeIntroductionStage.h"

#include <QtGui>

cbElectrodeIntroductionStage::cbElectrodeIntroductionStage() : cbStage()
{
  this->widget = new QWidget;
    QVBoxLayout *vertical = new QVBoxLayout;
      QTextEdit *desc = new QTextEdit;

  desc->setReadOnly(true);
  desc->insertHtml(
      "\
      <p style='font:16pt bold'>Welcome to Tactics!</p>\
      <p>This software allows the user to plan deep-brain electrode placement\
      for stereotactic surgery.</p>\
      <p>To open a previously-planned session, use the file-menu or use the <code>command+o</code> keyboard shortcut.</p>\
      <p>Otherwise, press the <i>Next</i> button, above, to begin.</p>\
      "
      );
  desc->setStyleSheet("background-color: aliceblue");

  vertical->setContentsMargins(11, 0, 11, 0);
  vertical->addWidget(desc);

  this->widget->setLayout(vertical);
  emit finished();
}

cbElectrodeIntroductionStage::~cbElectrodeIntroductionStage()
{
}

void cbElectrodeIntroductionStage::Execute()
{
}

char *cbElectrodeIntroductionStage::getStageName() const
{
  return "Introduction";
}
