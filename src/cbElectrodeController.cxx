/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeController.cxx

  Copyright (c) 2011-2013 David Adair, David Gobbi
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

#include "cbElectrodeController.h"

#include "vtkTransform.h"
#include "cbMRIRegistration.h"
#include "vtkImageResize.h"
#include "vtkImageReslice.h"

#include "vtkLinearTransform.h"
#include "vtkFrameFinder.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkImageStencil.h"
#include "vtkImageMRIBrainExtractor.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkPolyData.h"

#include "vtkMatrix4x4.h"
#include "vtkImageNode.h"
#include "vtkStringArray.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkDICOMMetaData.h"
#include "vtkDICOMParser.h"
#include "vtkDICOMReader.h"
#include "vtkDICOMSorter.h"
#include "vtkGlobFileNames.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QString>
#include <QDebug>

#include <vector>

#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkStringArray.h"

#include "LeksellFiducial.h"

void ReadDICOMImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, vtkStringArray *sarray,
  vtkDICOMMetaData *meta, const char *file_folder);

cbElectrodeController::cbElectrodeController(vtkDataManager *dataManager)
: cbApplicationController(dataManager), dataKey(), volumeKey()
{
  vtkSmartPointer<vtkImageNode> dataNode =
    vtkSmartPointer<vtkImageNode>::New();
  this->dataManager->AddDataNode(dataNode, this->dataKey);
  vtkSmartPointer<vtkImageNode> volumeNode =
    vtkSmartPointer<vtkImageNode>::New();
  this->dataManager->AddDataNode(volumeNode, this->volumeKey);

  this->useAnteriorPosteriorFiducials = true;
}

cbElectrodeController::~cbElectrodeController()
{
}


void ReadDICOMImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, vtkStringArray *sarray,
  vtkDICOMMetaData *meta, const char *file_folder)
{
  vtkSmartPointer<vtkDICOMReader> reader =
    vtkSmartPointer<vtkDICOMReader>::New();

  vtkSmartPointer<vtkGlobFileNames> directory =
    vtkSmartPointer<vtkGlobFileNames>::New();

  sarray->SetNumberOfValues(0);

  directory->SetDirectory(file_folder);
  directory->AddFileNames("*");

  vtkSmartPointer<vtkStringArray> files =
    vtkSmartPointer<vtkStringArray>::New();

  int n = directory->GetNumberOfFileNames();
  for (int i = 0; i < n; i++)
    {
    const char *filename = directory->GetNthFileName(i);
    if (reader->CanReadFile(filename))
      {
      files->InsertNextValue(filename);
      }
    }

  vtkSmartPointer<vtkDICOMSorter> sorter =
    vtkSmartPointer<vtkDICOMSorter>::New();

  sorter->SetInputFileNames(files);
  sorter->Update();
  sarray->DeepCopy(sorter->GetOutputFileNames());

  reader->SetFileNames(sarray);
  reader->SetMemoryRowOrderToFileNative();
  reader->Update();

  // for now, just get meta data from first file
  if (meta && sarray->GetNumberOfValues() != 0)
    {
    vtkSmartPointer<vtkDICOMParser> parser =
      vtkSmartPointer<vtkDICOMParser>::New();

    parser->SetMetaData(meta);
    parser->SetFileName(sarray->GetValue(0));
    parser->Update();
    }

  vtkImageData *output = reader->GetOutput();
  //get the data
  data->CopyStructure(output);
  data->GetPointData()->PassData(output->GetPointData());
  data->SetOrigin(0,0,0);

  vtkDataArray *scalars = NULL;
  if (data)
    {
    scalars = data->GetPointData()->GetScalars();
    }

  if (data == NULL || scalars == NULL)
    {
    //cerr <<"no image data to interpolate!" << endl;
    return;
    }

  matrix->DeepCopy(reader->GetPatientMatrix());
}

void cbElectrodeController::log(QString m)
{
  // Get the current system datetime
  QDateTime dateTime = QDateTime::currentDateTime();
  QString dateTimeString("[");
  dateTimeString.append(dateTime.toString());
  dateTimeString.append("]: ");
  emit Log(dateTimeString.append(m));
}

void cbElectrodeController::requestOpenImage(const char *path)
{
  assert(path && "Path can't be NULL!");

  // Parse the path to grab the directory rather than the file.
  std::string argument(path);
  std::string key("IM");
  size_t index = argument.rfind(key);

  // If the 'IM' substr could not be found, something went wrong!
  if (index == std::string::npos)
    {
    emit displayErrorMessage(QString("Something went wrong. \
                                     Please try again."));
    return;
    }

  // Grab the directory path from the argument using the index.
  std::string base = argument.substr(0, index-1);
  const char *basePath = base.c_str();

  emit initializeProgress(0, 100);

  this->log(QString("Opening Data: ").append(QString(basePath)));

  // Open and display the image data
  vtkSmartPointer<vtkImageData> data =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkStringArray> sarray =
    vtkSmartPointer<vtkStringArray>::New();
  vtkSmartPointer<vtkDICOMMetaData> meta =
    vtkSmartPointer<vtkDICOMMetaData>::New();

  ReadDICOMImage(data, matrix, sarray, meta, basePath);

  emit displayProgress(33);

  // Right now there is an order issue with these functions. The
  // buildAndDisplayFrame function uses matrix in the registration where it
  // is changed within the registration filter. This means that the function
  // MUST be called before displayData(dataKey), so that the matrix used
  // for the actors is correct
  this->buildAndDisplayFrame(data, matrix);

  emit displayProgress(66);
  this->extractAndDisplaySurface(data, matrix);

  emit displayProgress(100);

  this->dataManager->FindImageNode(dataKey)->ShallowCopyImage(data);
  this->dataManager->FindImageNode(dataKey)->SetMatrix(matrix);
  this->dataManager->FindImageNode(dataKey)->SetMetaData(meta);
  this->dataManager->FindImageNode(dataKey)->SetFileURL(path);

  emit displayData(dataKey);
  emit displaySurfaceVolume(volumeKey);
  emit finished();
}

void cbElectrodeController::extractAndDisplaySurface(vtkImageData *data,
                                                     vtkMatrix4x4 *matrix)
{
  int extent[6];
  double spacing[3];
  double origin[3];
  data->GetWholeExtent(extent);
  data->GetSpacing(spacing);
  data->GetOrigin(origin);

  vtkSmartPointer<vtkImageMRIBrainExtractor> extractor =
    vtkSmartPointer<vtkImageMRIBrainExtractor>::New();
  extractor->SetInput(data);

  double bt = 0.0;
  if (spacing[2] > 1.5)
    {
    bt = 0.7;
    }
  else
    {
    bt = 0.7;
    }

  extractor->SetRMin(8.0);
  extractor->SetRMax(10.0);
  extractor->SetD1(7.0);
  extractor->SetD2(3.0);
  extractor->SetBT(bt);
  extractor->SetBrainExtent(extent[0], extent[1], extent[2],
                            extent[3], extent[4], extent[5]);
  extractor->Update();

  vtkSmartPointer<vtkPolyData> mesh = extractor->GetBrainMesh();

  double bounds[6], length[3];
  int maxi = 0;
  mesh->GetBounds(bounds);
  for (int i = 0; i < 3; i++) {
    bounds[2*i] -= 1.0;
    bounds[2*i + 1] += 1.0;
    length[i] = bounds[2*i + 1] - bounds[2*i];
    if (length[i] > length[maxi]) {
      maxi = i;
    }
  }

  int dimensions[3];
  dimensions[maxi] = static_cast<int>(length[maxi]);
  for (int j = 1; j < 3; j++) {
    int k = (maxi+j)%3;
    double aspect = length[k]/length[maxi];
    dimensions[k] = static_cast<int>(aspect*dimensions[maxi]);
  }

  vtkSmartPointer<vtkImageResize> resize =
    vtkSmartPointer<vtkImageResize>::New();
  resize->SetInput(data);
  resize->SetOutputDimensions(dimensions);
  resize->CroppingOn();
  resize->SetCroppingRegion(bounds);
  resize->Update();

  vtkSmartPointer<vtkPolyDataToImageStencil> makeStencil =
    vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  makeStencil->SetInput(mesh);
  makeStencil->SetInformationInput(resize->GetOutput());
  makeStencil->Update();

  vtkSmartPointer<vtkImageStencil> brainStencil =
    vtkSmartPointer<vtkImageStencil>::New();
  brainStencil->SetInput(resize->GetOutput());
  brainStencil->SetStencil(makeStencil->GetOutput());
  brainStencil->Update();

  vtkImageData *brainSurface = brainStencil->GetOutput();

  this->dataManager->FindImageNode(volumeKey)->ShallowCopyImage(brainSurface);
  this->dataManager->FindImageNode(volumeKey)->SetMatrix(matrix);
}

void cbElectrodeController::buildAndDisplayFrame(vtkImageData *data,
                                                 vtkMatrix4x4 *matrix)
{
  LeksellFiducial lFrame(LeksellFiducial::left);
  LeksellFiducial rFrame(LeksellFiducial::right);
  LeksellFiducial fFrame(LeksellFiducial::front);
  LeksellFiducial bFrame(LeksellFiducial::back);

  std::vector<LeksellFiducial> frame;
  frame.push_back(lFrame);
  frame.push_back(rFrame);
  frame.push_back(fFrame);
  frame.push_back(bFrame);

  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(16);

  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();

  for (size_t i = 0; i < frame.size(); ++i)
    {
    double vertices[4][3];
    frame[i].GetCornerOriginPoints(vertices);

    points->SetPoint(i*4 + 0, vertices[0][0], vertices[0][1], vertices[0][2]);
    points->SetPoint(i*4 + 1, vertices[1][0], vertices[1][1], vertices[1][2]);
    points->SetPoint(i*4 + 2, vertices[2][0], vertices[2][1], vertices[2][2]);
    points->SetPoint(i*4 + 3, vertices[3][0], vertices[3][1], vertices[3][2]);

    cells->InsertNextCell(4);
    cells->InsertCellPoint(i*4 + 0);
    cells->InsertCellPoint(i*4 + 1);
    cells->InsertCellPoint(i*4 + 2);
    cells->InsertCellPoint(i*4 + 3);
    }

  vtkPolyData *frameData = vtkPolyData::New();
  frameData->SetPoints(points);
  frameData->SetLines(cells);

  vtkSmartPointer<vtkFrameFinder> regist =
    vtkSmartPointer<vtkFrameFinder>::New();
  regist->SetInput(data);
  regist->SetDICOMPatientMatrix(matrix);
  regist->SetUsePosteriorFiducial(this->useAnteriorPosteriorFiducials);
  regist->SetUseAnteriorFiducial(this->useAnteriorPosteriorFiducials);
  regist->Update();

  vtkMatrix4x4 *registeredImageMatrix = vtkMatrix4x4::New();
  registeredImageMatrix->DeepCopy(regist->GetImageToFrameMatrix());

  emit displayLeksellFrame(frameData, registeredImageMatrix);
}

void cbElectrodeController::OpenCTData(std::string path)
{
  vtkDataManager::UniqueKey k;

  vtkSmartPointer<vtkImageData> ct_data =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> ct_matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkStringArray> ct_array =
    vtkSmartPointer<vtkStringArray>::New();
  vtkSmartPointer<vtkDICOMMetaData> ct_meta =
    vtkSmartPointer<vtkDICOMMetaData>::New();

  unsigned found = path.find_last_of("/\\");
  if (found == std::string::npos) {
    emit displayErrorMessage(QString("Error openeing CT series. Please check the path."));
    return;
  }
  std::string dir = path.substr(0, found);

  ReadDICOMImage(ct_data, ct_matrix, ct_array, ct_meta, dir.c_str());

  std::cout << "*** image matrix ***" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::cout << ct_matrix->GetElement(i, j) << " ";
    }
    std::cout << std::endl;
  }
  this->RegisterCT(ct_data, ct_matrix);

  vtkSmartPointer<vtkImageNode> ct_node =
    vtkSmartPointer<vtkImageNode>::New();

  this->dataManager->AddDataNode(ct_node, k);

  this->dataManager->FindImageNode(k)->ShallowCopyImage(ct_data);
  this->dataManager->FindImageNode(k)->SetMatrix(ct_matrix);
  this->dataManager->FindImageNode(k)->SetMetaData(ct_meta);
  this->dataManager->FindImageNode(k)->SetFileURL(path.c_str());

  emit DisplayCTData(k);
}

void cbElectrodeController::RegisterCT(vtkImageData *ct_d, vtkMatrix4x4 *ct_m)
{
  emit initializeProgress(0, 0);

  vtkImageNode *mr = this->dataManager->FindImageNode(this->dataKey);
  vtkImageData *mr_d = mr->GetImage();
  vtkMatrix4x4 *mr_m = mr->GetMatrix();

  //TODO: skip registration if the matrix was opened from the save file
  cbMRIRegistration *regist = cbMRIRegistration::New();
  regist->SetInputSource(ct_d);
  regist->SetInputSourceMatrix(ct_m);
  regist->SetInputTarget(mr_d);
  regist->SetInputTargetMatrix(mr_m);
  regist->Execute();

  //TODO: this should use the save-file matrix, if it exists
  vtkSmartPointer<vtkMatrix4x4> registered_m =
    regist->GetModifiedSourceMatrix();

  vtkSmartPointer<vtkImageReslice> reslicer =
    vtkSmartPointer<vtkImageReslice>::New();
  reslicer->SetInterpolationModeToCubic();
  reslicer->SetInput(ct_d);
  reslicer->SetInformationInput(mr_d);

  vtkSmartPointer<vtkMatrix4x4> invertedMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  invertedMatrix->DeepCopy(registered_m);
  invertedMatrix->Invert();

  vtkSmartPointer<vtkTransform> resliceTransform =
    vtkSmartPointer<vtkTransform>::New();
  resliceTransform->PostMultiply();
  resliceTransform->Concatenate(mr_m);
  resliceTransform->Concatenate(invertedMatrix);

  reslicer->SetResliceTransform(resliceTransform);
  reslicer->Update();

  ct_d->DeepCopy(reslicer->GetOutput());

  emit initializeProgress(0, 1);
  emit displayProgress(1);
}

// Overloaded to include a pre-registered matrix
void cbElectrodeController::OpenCTData(std::string path, vtkMatrix4x4 *m)
{
  vtkDataManager::UniqueKey k;

  vtkSmartPointer<vtkImageData> ct_data =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> ct_matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkStringArray> ct_array =
    vtkSmartPointer<vtkStringArray>::New();
  vtkSmartPointer<vtkDICOMMetaData> ct_meta =
    vtkSmartPointer<vtkDICOMMetaData>::New();

  unsigned found = path.find_last_of("/\\");
  if (found == std::string::npos) {
    emit displayErrorMessage(QString("Error openeing CT series. Please check the path."));
    return;
  }
  std::string dir = path.substr(0, found);

  ReadDICOMImage(ct_data, ct_matrix, ct_array, ct_meta, dir.c_str());

  vtkImageNode *mr = this->dataManager->FindImageNode(this->dataKey);
  vtkImageData *mr_d = mr->GetImage();
  vtkMatrix4x4 *mr_m = mr->GetMatrix();

  vtkSmartPointer<vtkMatrix4x4> registered_m = m;

  vtkSmartPointer<vtkImageReslice> reslicer =
    vtkSmartPointer<vtkImageReslice>::New();
  reslicer->SetInterpolationModeToCubic();
  reslicer->SetInput(ct_data);
  reslicer->SetInformationInput(mr_d);

  vtkSmartPointer<vtkMatrix4x4> invertedMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  invertedMatrix->DeepCopy(registered_m);
  invertedMatrix->Invert();

  vtkSmartPointer<vtkTransform> resliceTransform =
    vtkSmartPointer<vtkTransform>::New();
  resliceTransform->PostMultiply();
  resliceTransform->Concatenate(mr_m);
  resliceTransform->Concatenate(invertedMatrix);

  reslicer->SetResliceTransform(resliceTransform);
  reslicer->Update();

  ct_data->DeepCopy(reslicer->GetOutput());

  vtkSmartPointer<vtkImageNode> ct_node =
    vtkSmartPointer<vtkImageNode>::New();

  this->dataManager->AddDataNode(ct_node, k);

  this->dataManager->FindImageNode(k)->ShallowCopyImage(ct_data);
  this->dataManager->FindImageNode(k)->SetMatrix(m);
  this->dataManager->FindImageNode(k)->SetMetaData(ct_meta);
  this->dataManager->FindImageNode(k)->SetFileURL(path.c_str());

  emit DisplayCTData(k);
}

void cbElectrodeController::registerAntPost(int s)
{
  this->useAnteriorPosteriorFiducials = s;
}
