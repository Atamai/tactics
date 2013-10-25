/*=========================================================================
  Program: Cerebra
  Module:  cbProbe.cxx

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

#include "cbProbe.h"

#include <sstream>
#include <math.h>

// Constructor
cbProbe::cbProbe(double x, double y, double z, double a, double d, double depth,
                 const char *n) : specification_()
{
  this->x_ = x;
  this->y_ = y;
  this->z_ = z;

  this->a_ = a;
  this->d_ = d;

  this->name_ = n;

  this->depth_ = depth;
}

// Constructor
cbProbe::cbProbe() : specification_()
{
  this->x_ = 0;
  this->y_ = 0;
  this->z_ = 0;

  this->a_ = 0.0;
  this->d_ = 0.0;

  this->name_ = "noid";

  this->depth_ = 0.0;
}

// Destructor
cbProbe::~cbProbe()
{
}

// Copy Constructor
cbProbe::cbProbe(const cbProbe& other)
{
  double p[3];
  double o[2];

  other.GetPosition(p);
  other.GetOrientation(o);

  this->x_ = p[0];
  this->y_ = p[1];
  this->z_ = p[2];

  this->a_ = o[0];
  this->d_ = o[1];

  this->specification_ = other.specification();

  this->name_ = other.GetName();

  this->depth_ = other.GetDepth();
}

// Assignment Operator
cbProbe& cbProbe::operator=(const cbProbe& rhs)
{
  if (&rhs == this) {
    return *this;
  }

  double p[3];
  double o[2];

  rhs.GetPosition(p);
  rhs.GetOrientation(o);

  this->x_ = p[0];
  this->y_ = p[1];
  this->z_ = p[2];

  this->a_ = o[0];
  this->d_ = o[1];

  this->specification_ = rhs.specification();

  this->name_ = rhs.GetName();

  this->depth_ = rhs.GetDepth();

  return *this;
}

void cbProbe::GetPosition(double p[3]) const
{
  p[0] = this->x_;
  p[1] = this->y_;
  p[2] = this->z_;
}

void cbProbe::GetOrientation(double o[2]) const
{
  o[0] = this->a_;
  o[1] = this->d_;
}

double cbProbe::GetDepth() const
{
  return this->depth_;
}

std::string cbProbe::ToString() const
{
  std::string temp;
  std::stringstream stream;

  stream << std::fixed;
  stream.precision(0);
  stream << this->name_ << " | "
         << this->specification_.catalogue_number() << " | "
         << "x:" << floor(this->x_ + 0.5) << " "
         << "y:" << floor(this->y_ + 0.5) << " "
         << "z:" << floor(this->z_ + 0.5) << " | "
         << "AP:" << floor(this->d_ + 0.5) << " | "
         << "LR:" << floor(this->a_ + 0.5) << " | "
         << "D:" << floor(this->depth_ + 0.5);

  temp = stream.str();

  return temp;
}

void cbProbe::SetPosition(const double p[3])
{
  this->x_ = p[0];
  this->y_ = p[1];
  this->z_ = p[2];
}

void cbProbe::SetOrientation(const double o[2])
{
  this->a_ = o[0];
  this->d_ = o[1];
}

void cbProbe::SetDepth(double depth)
{
  this->depth_ = depth;
}

std::ostream& operator<<(std::ostream &os, const cbProbe& p)
{
  double position[3], orientation[2];
  p.GetPosition(position);
  p.GetOrientation(orientation);
  double depth = p.GetDepth();

  std::string catalogue_number = p.specification().catalogue_number();

  return os << p.GetName() << " "
            << catalogue_number << " "
            << position[0] << " "
            << position[1] << " "
            << position[2] << " "
            << orientation[1] << " "
            << orientation[0] << " "
            << depth;
}

std::string cbProbe::GetName() const
{
  return std::string(this->name_);
}

void cbProbe::SetName(const std::string n)
{
  this->name_ = n;
}

cbProbeSpecification cbProbe::specification() const
{
  return cbProbeSpecification(this->specification_);
}

void cbProbe::set_specification(cbProbeSpecification s)
{
  this->specification_ = s;
}
