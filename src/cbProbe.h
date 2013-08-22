/*=========================================================================
  Program: Cerebra
  Module:  cbProbe.h

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

#ifndef CBPROBE_H
#define CBPROBE_H

#include <string>

#include "cbProbeSpecification.h"

class cbProbe
{
public:
  cbProbe(double x, double y, double z, double a, double d, const char *n = "noid");
  cbProbe();
  ~cbProbe();

  cbProbe(const cbProbe& other);
  cbProbe& operator=(const cbProbe& rhs);

  void GetPosition(double p[3]) const;
  void SetPosition(const double p[3]);
  void GetOrientation(double o[2]) const;
  void SetOrientation(const double o[2]);

  cbProbeSpecification specification() const;
  void set_specification(cbProbeSpecification s);

  void SetName(const std::string n);
  std::string GetName() const;
  std::string ToString() const;

private:
  double x_; // x position
  double y_; // y position
  double z_; // z position
  double a_; // azimuth
  double d_; // declination
  std::string name_;

  cbProbeSpecification specification_;

  friend std::ostream& operator<<(std::ostream &os, const cbProbe& p);
};

#endif /* end of include guard: CBPROBE_H */
