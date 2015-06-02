/*=========================================================================
  Program: Cerebra
  Module:  cbProbeCatalogue.cxx

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

#include "cbProbeCatalogue.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QTextStream>
#include <QDebug>
#include <QtGui>
#include <QString>
#include <QProgressBar>
#include <QDockWidget>
#include <QStringList>

#include <iostream>

cbProbeCatalogue::cbProbeCatalogue(std::string directory_path)
  : catalogue_()
{
  this->catalogue_ = this->import_catalogue(directory_path);
}

cbProbeCatalogue::~cbProbeCatalogue()
{
}

std::vector<std::string> cbProbeCatalogue::list_as_strings() const
{
  std::vector<std::string> list;
  for (size_t i = 0; i < this->catalogue_.size(); i++) {
    const cbProbeSpecification *p = &(this->catalogue_.at(i));
    list.push_back(p->catalogue_number());
  }
  return list;
}

std::vector<cbProbeSpecification> cbProbeCatalogue::import_catalogue(std::string dir_path)
{
  QDir dir(dir_path.c_str());

  std::vector<cbProbeSpecification> catalogue;
  if (!dir.exists()) {
    return catalogue;
  }

  dir.setFilter(QDir::Files | QDir::NoSymLinks);

  QStringList list = dir.entryList();
  for (int i = 0; i < list.size(); i++) {
    std::string file = dir.absoluteFilePath(list.at(i)).toStdString();
    catalogue.push_back(file);
  }
  return catalogue;
}

std::vector<cbProbeSpecification> cbProbeCatalogue::catalogue() const
{
  return std::vector<cbProbeSpecification>(this->catalogue_);
}

cbProbeSpecification cbProbeCatalogue::specification(std::string catalogue_number) const
{
  for (size_t i = 0; i < this->catalogue_.size(); i++) {
    const cbProbeSpecification *s = &(this->catalogue_.at(i));
    if (s->catalogue_number().compare(catalogue_number) == 0) {
      return *s;
    }
  }
  return cbProbeSpecification();
}
