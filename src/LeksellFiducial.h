/*=========================================================================
  Program: Cerebra
  Module:  LeksellFiducial.h

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

#ifndef LEKSELLFIDUCIAL_H
#define LEKSELLFIDUCIAL_H

class LeksellFiducial
{
 public:
  enum Side {left = 0, right = 1, front = 2, back = 3};

  LeksellFiducial()
    {
    points = new double*[4];
    for (int i = 0; i < 4; i++)
      {
      points[i] = new double[3];
      }
    points[0][0] = 0.0;
    points[0][1] = 0.0;
    points[0][2] = 0.0;
    points[1][0] = 0.0;
    points[1][1] = 0.0;
    points[1][2] = 0.0;
    points[2][0] = 0.0;
    points[2][1] = 0.0;
    points[2][2] = 0.0;
    points[3][0] = 0.0;
    points[3][1] = 0.0;
    points[3][2] = 0.0;
    }

  LeksellFiducial(Side s)
    {
    points = new double*[4];
    for (int i = 0; i < 4; i++)
      {
      points[i] = new double[3];
      }
    switch(s)
      {
      case left:
        points[0][0] = 196.0;
        points[0][1] = 40.0;
        points[0][2] = 160.0;
        points[1][0] = 196.0;
        points[1][1] = 40.0;
        points[1][2] = 40.0;
        points[2][0] = 196.0;
        points[2][1] = 160.0;
        points[2][2] = 160.0;
        points[3][0] = 196.0;
        points[3][1] = 160.0;
        points[3][2] = 40.0;
        break;
      case right:
        points[0][0] = 4.0;
        points[0][1] = 40.0;
        points[0][2] = 160.0;
        points[1][0] = 4.0;
        points[1][1] = 40.0;
        points[1][2] = 40.0;
        points[2][0] = 4.0;
        points[2][1] = 160.0;
        points[2][2] = 160.0;
        points[3][0] = 4.0;
        points[3][1] = 160.0;
        points[3][2] = 40.0;
        break;
      case front:
        points[0][0] = 40.0;
        points[0][1] = 217.5;
        points[0][2] = 160.0;
        points[1][0] = 40.0;
        points[1][1] = 217.5;
        points[1][2] = 40.0;
        points[2][0] = 160.0;
        points[2][1] = 217.5;
        points[2][2] = 160.0;
        points[3][0] = 160.0;
        points[3][1] = 217.5;
        points[3][2] = 40.0;
        break;
      case back:
        points[0][0] = 40.0;
        points[0][1] = -17.5;
        points[0][2] = 160.0;
        points[1][0] = 40.0;
        points[1][1] = -17.5;
        points[1][2] = 40.0;
        points[2][0] = 160.0;
        points[2][1] = -17.5;
        points[2][2] = 160.0;
        points[3][0] = 160.0;
        points[3][1] = -17.5;
        points[3][2] = 40.0;
        break;
      default:
        points[0][0] = 0.0;
        points[0][1] = 0.0;
        points[0][2] = 0.0;
        points[1][0] = 0.0;
        points[1][1] = 0.0;
        points[1][2] = 0.0;
        points[2][0] = 0.0;
        points[2][1] = 0.0;
        points[2][2] = 0.0;
        points[3][0] = 0.0;
        points[3][1] = 0.0;
        points[3][2] = 0.0;
        break;
      }
    }

  ~LeksellFiducial()
    {
      // TODO segfault here
    //for (int i = 0; i < 4; i++)
    //  {
    //  delete[] points[i];
    //  }
    //delete[] points;
    }

  void GetCornerOriginPoints(double p[4][3]) const
    {
    for (int i = 0; i < 4; i++)
      {
      for (int j = 0; j < 3; j++)
        {
        p[i][j] = this->points[i][j];
        }
      }
    }

 private:
  double **points;
};

#endif /* end of include guard: LEKSELLFIDUCIAL_H */
