/*=========================================================================
  Program: Cerebra
  Module:  cbProbeSpecification.cxx

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

#include "cbProbeSpecification.h"

#include <iostream>
#include <fstream>
#include <sstream>

cbProbeSpecification::cbProbeSpecification(std::string file_path)
  : catalogue_number_(), tip_is_contact_(false), points_()
{
  this->ImportFromFile(file_path);
}

cbProbeSpecification::cbProbeSpecification()
  : catalogue_number_(), tip_is_contact_(false), points_()
{
}

cbProbeSpecification::~cbProbeSpecification()
{
}

void cbProbeSpecification::ImportFromFile(std::string file_path)
{
  std::ifstream file(file_path.c_str());

  if (!file.is_open()) {
    std::cout << "couldn't open probe file" << std::endl;
    return;
  }


  unsigned found = file_path.find_last_of("/\\");
  unsigned extension = file_path.find_last_of(".");

  this->catalogue_number_ = file_path.substr(found+1, extension-found-1);

  std::string connector_tip;
  std::getline(file, connector_tip);

  if (connector_tip.compare("true") == 0) {
    this->tip_is_contact_ = true;
  } else if (connector_tip.compare("false") == 0) {
    this->tip_is_contact_ = false;
  } else {
    std::cout << "Invalid tip parameter value in probe file." << std::endl;
    this->tip_is_contact_ = false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    double p;
    if (!(iss >> p)) { break; } //error
    this->points_.push_back(p);
  }
}

bool cbProbeSpecification::tip_is_contact() const
{
  return tip_is_contact_;
}

std::vector<double> cbProbeSpecification::points() const
{
  std::vector<double> r(points_);
  return r;
}

std::string cbProbeSpecification::catalogue_number() const
{
  return std::string(catalogue_number_);
}

// Copy Constructor
cbProbeSpecification::cbProbeSpecification(const cbProbeSpecification& other) {
  this->catalogue_number_ = other.catalogue_number();
  this->points_ = other.points();
  this->tip_is_contact_ = other.tip_is_contact();
}

// Assignment Operator
cbProbeSpecification& cbProbeSpecification::operator=(const cbProbeSpecification& rhs) {
  if (&rhs == this) {
    return *this;
  }

  this->catalogue_number_ = rhs.catalogue_number();
  this->points_ = rhs.points();
  this->tip_is_contact_ = rhs.tip_is_contact();

  return *this;
}
