/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeController.cxx

  Copyright (c) 2011-2016 David Adair, David Gobbi
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

#include "cbProbeCatalogue.h"
#include "cbQtDicomDirDialog.h"

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
#include "vtkTimerLog.h"

#include "vtkMatrix4x4.h"
#include "vtkImageNode.h"
#include "vtkSurfaceNode.h"
#include "vtkStringArray.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkDICOMMetaData.h"
#include "vtkDICOMReader.h"
#include "vtkDICOMFileSorter.h"
#include "vtkDICOMToRAS.h"
#include "vtkNIFTIReader.h"
#include "vtkNIFTIWriter.h"
#include "vtkMNITagPointReader2.h"
#include "vtkTransformPolyDataFilter.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QMessageBox>
#include <QDebug>

#include <vector>
#include <sstream>
#include <iostream>

#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkStringArray.h"

#include "json/json.h"

void ReadImage(vtkStringArray *sarray, vtkImageData *data,
               vtkMatrix4x4 *matrix, vtkDICOMMetaData *meta);

void ReadDICOMImage(vtkStringArray *sarray, vtkImageData *data,
                    vtkMatrix4x4 *matrix, vtkDICOMMetaData *meta);

void ReadNIFTIImage(const std::string& fileName, vtkImageData *data,
                    vtkMatrix4x4 *matrix);

cbElectrodeController::cbElectrodeController(vtkDataManager *dataManager)
: cbApplicationController(dataManager), dataKey(), volumeKey(), ctKey(),
  Plan(0), FrameMatrix(0)
{
  vtkSmartPointer<vtkImageNode> dataNode =
    vtkSmartPointer<vtkImageNode>::New();
  this->dataManager->AddDataNode(dataNode, this->dataKey);
  vtkSmartPointer<vtkImageNode> volumeNode =
    vtkSmartPointer<vtkImageNode>::New();
  this->dataManager->AddDataNode(volumeNode, this->volumeKey);

  this->useAnteriorPosteriorFiducials = false;
}

cbElectrodeController::~cbElectrodeController()
{
  if (this->FrameMatrix) {
    this->FrameMatrix->Delete();
  }
}

void ReadImage(vtkStringArray *sarray, vtkImageData *data,
               vtkMatrix4x4 *matrix, vtkDICOMMetaData *meta)
{
  if (sarray->GetNumberOfValues() == 0) {
    return;
  }

  std::string fileName = sarray->GetValue(0);
  size_t l = fileName.length();
  if ((l >= 4 && fileName.compare(l-4, std::string::npos, ".nii") == 0) ||
      (l >= 7 && fileName.compare(l-7, std::string::npos, ".nii.gz") == 0)) {
    ReadNIFTIImage(fileName, data, matrix);
  }
  else {
    ReadDICOMImage(sarray, data, matrix, meta);
  }
}

void ReadDICOMImage(vtkStringArray *sarray, vtkImageData *data,
                    vtkMatrix4x4 *matrix, vtkDICOMMetaData *meta)
{
  vtkNew<vtkDICOMReader> reader;

  reader->SetFileNames(sarray);
  reader->SetMemoryRowOrderToFileNative();
  reader->Update();

  vtkImageData *output = reader->GetOutput();
  data->CopyStructure(output);
  data->GetPointData()->PassData(output->GetPointData());

  matrix->DeepCopy(reader->GetPatientMatrix());

  meta->DeepCopy(reader->GetMetaData());
}

void ReadNIFTIImage(const std::string& fileName, vtkImageData *data,
                    vtkMatrix4x4 *matrix)
{
  vtkNew<vtkNIFTIReader> reader;

  reader->SetFileName(fileName.c_str());
  reader->Update();

  // switch from NIFTI to DICOM coordinates
  vtkNew<vtkDICOMToRAS> reorder;

  reorder->RASToDICOMOn();
  reorder->RASMatrixHasPositionOn();
  reorder->SetInputConnection(reader->GetOutputPort());
  if (reader->GetQFormMatrix())
    {
    reorder->SetRASMatrix(reader->GetQFormMatrix());
    }
  else if (reader->GetSFormMatrix())
    {
    reorder->SetRASMatrix(reader->GetSFormMatrix());
    }
  reorder->Update();

  vtkImageData *output = reorder->GetOutput();
  data->CopyStructure(output);
  data->GetPointData()->PassData(output->GetPointData());

  matrix->DeepCopy(reorder->GetPatientMatrix());
}

void WriteNIFTIImage(const std::string& fileName, vtkImageData *data,
                     vtkMatrix4x4 *matrix)
{
  // switch from DICOM to NIFTI coordinates
  vtkNew<vtkDICOMToRAS> reorder;

  reorder->SetInputData(data);
  reorder->SetPatientMatrix(matrix);
  reorder->Update();

  vtkNew<vtkNIFTIWriter> writer;

  writer->SetFileName(fileName.c_str());
  writer->SetInputConnection(reorder->GetOutputPort());
  writer->SetQFormMatrix(reorder->GetRASMatrix());
  writer->SetSFormMatrix(reorder->GetRASMatrix());
  writer->Write();
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

void cbElectrodeController::requestOpenImage(const QStringList& files)
{
  //assert(path && "Path can't be NULL!");

  emit initializeProgress(0, 100);
  emit displayStatus("Loading primary image...");

  this->log(QString("Opening Data: "));

  // Open and display the image data
  vtkNew<vtkImageData> data;
  vtkNew<vtkMatrix4x4> matrix;
  vtkNew<vtkStringArray> sarray;
  vtkNew<vtkDICOMMetaData> meta;

  for (int i = 0; i < files.size(); i++) {
    sarray->InsertNextValue(files[i].toUtf8());
  }

  ReadImage(sarray, data, matrix, meta);

  emit displayProgress(25);
  emit displayStatus("Finding frame, registering to frame space...");

  // Right now there is an order issue with these functions. The
  // buildAndDisplayFrame function uses matrix in the registration where it
  // is changed within the registration filter. This means that the function
  // MUST be called before displayData(dataKey), so that the matrix used
  // for the actors is correct
  this->buildAndDisplayFrame(data, matrix);

  emit displayProgress(50);
  emit displayStatus("Extracting brain from image...");
  this->extractAndDisplaySurface(data, matrix);

  emit displayProgress(75);
  emit displayStatus("Rendering brain volume...");

  this->dataManager->FindImageNode(dataKey)->ShallowCopyImage(data);
  this->dataManager->FindImageNode(dataKey)->SetMatrix(matrix);
  this->dataManager->FindImageNode(dataKey)->SetMetaData(meta);

  emit displayData(dataKey);
  emit displaySurfaceVolume(volumeKey);
  emit displayProgress(100);
  emit displayStatus("Finished loading primary image.", 5000);
  emit finished();
}

// Open primary image, providing a matrix
void cbElectrodeController::OpenImageWithMatrix(
  const QStringList& files, vtkMatrix4x4 *m)
{
  //assert(path && "Path can't be NULL!");

  emit initializeProgress(0, 100);
  emit displayStatus("Loading primary image...");

  this->log(QString("Opening Data: "));

  // Open and display the image data
  vtkNew<vtkImageData> data;
  vtkNew<vtkMatrix4x4> matrix;
  vtkNew<vtkStringArray> sarray;
  vtkNew<vtkDICOMMetaData> meta;

  for (int i = 0; i < files.size(); i++) {
    sarray->InsertNextValue(files[i].toUtf8());
  }

  ReadImage(sarray, data, matrix, meta);

  emit displayProgress(50);
  emit displayStatus("Extracting brain from image...");
  this->extractAndDisplaySurface(data, m);

  emit displayProgress(75);
  emit displayStatus("Rendering brain volume...");

  this->dataManager->FindImageNode(dataKey)->ShallowCopyImage(data);
  this->dataManager->FindImageNode(dataKey)->SetMatrix(m);
  this->dataManager->FindImageNode(dataKey)->SetMetaData(meta);

  emit displayData(dataKey);
  emit displaySurfaceVolume(volumeKey);
  emit displayProgress(100);
  emit displayStatus("Finished loading primary image.", 5000);
  emit finished();
}

void cbElectrodeController::extractAndDisplaySurface(vtkImageData *data,
                                                     vtkMatrix4x4 *matrix)
{
  int extent[6];
  double spacing[3];
  double origin[3];
  data->GetExtent(extent);
  data->GetSpacing(spacing);
  data->GetOrigin(origin);

  vtkNew<vtkImageMRIBrainExtractor> extractor;
  extractor->SetInputData(data);

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

  vtkNew<vtkImageResize> resize;
  resize->SetInputData(data);
  resize->SetOutputDimensions(dimensions);
  resize->CroppingOn();
  resize->SetCroppingRegion(bounds);
  resize->Update();

  vtkNew<vtkPolyDataToImageStencil> makeStencil;
  makeStencil->SetInputData(mesh);
  makeStencil->SetInformationInput(resize->GetOutput());
  makeStencil->Update();

  vtkNew<vtkImageStencil> brainStencil;
  brainStencil->SetInputData(resize->GetOutput());
  brainStencil->SetStencilConnection(makeStencil->GetOutputPort());
  brainStencil->Update();

  vtkImageData *brainSurface = brainStencil->GetOutput();

  this->dataManager->FindImageNode(volumeKey)->ShallowCopyImage(brainSurface);
  this->dataManager->FindImageNode(volumeKey)->SetMatrix(matrix);
}

void cbElectrodeController::buildAndDisplayFrame(vtkImageData *data,
                                                 vtkMatrix4x4 *matrix)
{
  vtkNew<vtkFrameFinder> regist;
  regist->SetInputData(data);
  regist->SetDICOMPatientMatrix(matrix);
  regist->SetUsePosteriorFiducial(this->useAnteriorPosteriorFiducials);
  regist->SetUseAnteriorFiducial(this->useAnteriorPosteriorFiducials);
  regist->Update();

  if (this->FrameMatrix) {
    this->FrameMatrix->Delete();
  }
  this->FrameMatrix = vtkMatrix4x4::New();

  if (regist->GetSuccess()) {
    this->FrameMatrix->DeepCopy(regist->GetImageToFrameMatrix());
  }
  else {
    vtkNew<vtkMatrix4x4> flipMatrix;
    flipMatrix->Identity();
    flipMatrix->SetElement(1, 1, -1.0);  // Flip Y (Anterior-Posterior)
    flipMatrix->SetElement(2, 2, -1.0);  // Flip Z (Superior-Inferior)
     
     // Combine flip with patient matrix
    vtkMatrix4x4::Multiply4x4(flipMatrix, matrix, this->FrameMatrix);
  }
  emit displayLeksellFrame(this->FrameMatrix);
  if (regist->GetSuccess()){
    emit EnableFrameVisualization();
  }
  else {
    emit DisableFrameVisualization();
  }
}

namespace { // helper functions for json

bool cbJsonReadTransform(const Json::Value& value, double matrix[16])
{
  for (int i = 0; i < 16; i++) {
    matrix[i] = ((i%4) == (i/4));
  }

  Json::ArrayIndex arraySize = value.size();
  if (arraySize == 12) {
    for (Json::ArrayIndex i = 0; i < 12; i++) {
      matrix[i] = value[i].asDouble();
    }
    // test that this matrix is valid
    if (vtkMatrix4x4::Determinant(matrix) > 0.1) {
      return true;
    }
  }

  return false;
}

};

void cbElectrodeController::OpenLegacyPlan(const QString& file)
{
  std::ifstream ifile(file.toLocal8Bit().constData());

  std::string image_path, ct_path;
  std::getline(ifile, image_path);
  std::getline(ifile, ct_path);

  // get the directory
  QDir startDir = QFileInfo(file).dir();

  QString fullpath = QString::fromLocal8Bit(image_path.c_str());
  QFileInfo finfo(fullpath);
  if (!finfo.exists() || !finfo.isReadable()) {
    QMessageBox box;
    box.setText("Unable to find planning image.");
    box.setInformativeText(fullpath);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
    // Bring up the file dialog
    cbQtDicomDirDialog dialog(NULL, "Open Primary Series", startDir.path());
    if (dialog.exec()) {
      this->requestOpenImage(dialog.selectedFiles());
      startDir = QFileInfo(dialog.directory().absolutePath()).dir();
    }
  }
  else {
    // Open the image path from the save file
    vtkSmartPointer<vtkDICOMFileSorter> sorter =
      vtkSmartPointer<vtkDICOMFileSorter>::New();
    sorter->SetInputFileName(image_path.c_str());
    sorter->Update();
    vtkStringArray *fileArray = sorter->GetOutputFileNames();
    QStringList image_files;
    for (vtkIdType i = 0; i < fileArray->GetNumberOfValues(); i++) {
      image_files.append(QString::fromLocal8Bit(fileArray->GetValue(i).c_str()));
    }
    this->requestOpenImage(image_files);
  }

  if (ct_path.empty()) {
    std::cout << "empty ct, don't do anything" << std::endl;
  }
  else {
    std::string line;
    std::getline(ifile, line);
    std::istringstream iss(line);

    double matrix[16];
    for (int i = 0; i < 16; i++) {
      iss >> matrix[i];
    }

    vtkMatrix4x4 *matrix_obj = vtkMatrix4x4::New();
    matrix_obj->DeepCopy(matrix);

    QString fullpath2 = QString::fromLocal8Bit(ct_path.c_str());
    QFileInfo finfo(fullpath2);
    if (!finfo.exists() || !finfo.isReadable()) {
      QMessageBox box;
      box.setText("Unable to find secondary image.");
      box.setInformativeText(fullpath2);
      box.setStandardButtons(QMessageBox::Ok);
      box.exec();
      // Bring up the file dialog
      cbQtDicomDirDialog dialog(NULL, "Open Secondary Series", startDir.path());
      if (dialog.exec()) {
        this->OpenCTData(dialog.selectedFiles(), matrix_obj);
      }
    }
    else {
      // Open the image path from the save file
      vtkSmartPointer<vtkDICOMFileSorter> sorter =
        vtkSmartPointer<vtkDICOMFileSorter>::New();
      sorter->SetInputFileName(ct_path.c_str());
      sorter->Update();
      vtkStringArray *fileArray = sorter->GetOutputFileNames();
      QStringList ct_files;
      for (vtkIdType i = 0; i < fileArray->GetNumberOfValues(); i++) {
        ct_files.append(QString::fromLocal8Bit(fileArray->GetValue(i).c_str()));
      }
      this->OpenCTData(ct_files, matrix_obj);
    }
  }

  // Open the probes
  std::string line;
  while (std::getline(ifile, line)) {
    if (line.empty()) {
      break;
    }
    std::istringstream iss(line);
    std::string name, spec;
    double pos[3], orient[2], depth;

    iss >> name >> spec >> pos[0] >> pos[1] >> pos[2]
        >> orient[0] >> orient[1] >> depth;

    emit CreateProbeRequest(pos[0], pos[1], pos[2],
                            orient[1], orient[0], depth,
                            name, spec);
  }

  emit jumpToLastStage();
}

void cbElectrodeController::OpenPlan(const QString& file)
{
  // clear the current plan for a fresh state.
  emit ClearCurrentPlan();

  // read the json object for the plan
  std::ifstream ifile(file.toLocal8Bit().constData());
  if (!ifile.good()) {
    QMessageBox box;
    box.setText("Unable to load plan file.");
    box.setInformativeText(file);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
    return;
  }

  // check for an old Tactics 1.0 plan file
  if (ifile.peek() == '/') {
    ifile.close();
    this->OpenLegacyPlan(file);
    return;
  }

  // get the directory
  QDir planDir = QFileInfo(file).dir();

  // create a json object from the file
  Json::Value plan;
  std::string errs;
  Json::CharReaderBuilder builder;
  if (!Json::parseFromStream(builder, ifile, &plan, &errs)) {
    QMessageBox box;
    box.setText("Unable to read plan file.");
    box.setInformativeText(file);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
    return;
  }
  ifile.close();

  if (!plan.isObject()) {
    QMessageBox box;
    box.setText("Unable to read plan file.");
    box.setInformativeText(file);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
    return;
  }

  if (this->FrameMatrix) {
    this->FrameMatrix->Delete();
    this->FrameMatrix = 0;
  }

  Json::Value frame = plan["frame"];
  if (frame.isObject()) {
    Json::Value transform = frame["transform"];
    double matrix[16];
    if (cbJsonReadTransform(transform, matrix)) {
      this->FrameMatrix = vtkMatrix4x4::New();
      this->FrameMatrix->DeepCopy(matrix);
      emit displayLeksellFrame(this->FrameMatrix);
    }
  }

  if (this->FrameMatrix == 0) {
    QMessageBox box;
    box.setText("No frame found in plan file.");
    box.setInformativeText("Planning is not possible.");
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
  }

  Json::Value volumes = plan["volumes"];
  if (volumes.isArray()) {
    Json::ArrayIndex volsSize = volumes.size();
    for (Json::ArrayIndex i = 0; i < volsSize; i++) {
      Json::Value volume = volumes[i];
      if (volume.isObject()) {
        Json::Value transform = volume["transform"];
        vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
        double mat[16];
        if (cbJsonReadTransform(transform, mat)) {
          matrix->DeepCopy(mat);
        }
        Json::Value vfile = volume["file"];
        if (vfile.isString()) {
          std::string filename = vfile.asString();
          QString fullpath = planDir.filePath(
            QString::fromLocal8Bit(filename.c_str()));
          QStringList image_files(fullpath);
          QFileInfo finfo(fullpath);
          if (!finfo.exists() || !finfo.isReadable()) {
            const char *dtext[2] = {
              "Open Primary Series",
              "Open Secondary Series" };
            QMessageBox box;
            box.setText("Unable to read plan image.");
            box.setInformativeText(fullpath);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();
            cbQtDicomDirDialog dialog(NULL, dtext[(i != 0)], planDir.path());
            if (dialog.exec()) {
              if (i == 0) {
                this->OpenImageWithMatrix(dialog.selectedFiles(), matrix);
              }
              else {
                this->OpenCTWithMatrix(dialog.selectedFiles(), matrix);
              }
            }
          }
          else if (i == 0) {
            this->OpenImageWithMatrix(image_files, matrix);
          }
          else {
            this->OpenCTWithMatrix(image_files, matrix);
          }
        }
      }
    }
  }

  Json::Value tags = plan["tags"];
  if (tags.isArray()) {
    vtkSmartPointer<vtkPoints> tagPoints =
      vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> tagCells =
      vtkSmartPointer<vtkCellArray>::New();

    Json::ArrayIndex tagsSize = tags.size();
    for (Json::ArrayIndex i = 0; i < tagsSize; i++) {
      Json::Value tag = tags[i];
      if (tag.isObject()) {
        Json::Value xyz = tag["xyz"];
        if (xyz.isArray() && xyz.size() == 3) {
          double pos[3];
          for (Json::ArrayIndex j = 0; j < 3; j++) {
            pos[j] = xyz[j].asDouble();
          }
          tagCells->InsertNextCell(1);
          tagCells->InsertCellPoint(tagPoints->InsertNextPoint(pos));
        }
      }
    }

    vtkSmartPointer<vtkPolyData> tagData =
      vtkSmartPointer<vtkPolyData>::New();
    tagData->SetPoints(tagPoints);
    tagData->SetVerts(tagCells);

    vtkSmartPointer<vtkSurfaceNode> tag_node =
      vtkSmartPointer<vtkSurfaceNode>::New();
    this->dataManager->AddDataNode(tag_node, this->tagKey);
    this->dataManager->FindSurfaceNode(this->tagKey)
      ->ShallowCopySurface(tagData);

    emit displayTags(this->tagKey);
  }

  Json::Value probes = plan["probes"];
  if (probes.isArray()) {
    Json::ArrayIndex probesSize = probes.size();
    for (Json::ArrayIndex i = 0; i < probesSize; i++) {
      double pos[3] = { 0.0, 0.0, 0.0 };

      Json::Value probe = probes[i];
      if (probe.isObject()) {
        Json::Value t = probe["target"];
        if (t.isArray() && t.size() == 3) {
          for (Json::ArrayIndex j = 0; j < 3; j++) {
            pos[j] = t[j].asDouble();
          }
        }

        double azimuth = probe["azimuth"].asDouble();
        double declination = probe["declination"].asDouble();

        double depth = probe["depth"].asDouble();
        std::string name = probe["name"].asString();
        std::string spec = probe["spec"].asString();

        emit CreateProbeRequest(pos[0], pos[1], pos[2],
                                azimuth, declination, depth,
                                name, spec);
      }
    }
  }

  emit jumpToLastStage();
}

void cbElectrodeController::SavePlan(const QString& file)
{
  // create the json object for the plan
  Json::Value plan;

  // include the date and time that the plan was saved
  QDateTime dt = QDateTime::currentDateTime();
  plan["date"] = dt.toString(Qt::ISODate).toStdString();

  Json::Value frame;
  if (this->FrameMatrix) {
    frame["type"] = "Leksell";
    double frame_matrix[16];
    vtkMatrix4x4::DeepCopy(frame_matrix, this->FrameMatrix);
    Json::Value array(Json::arrayValue);
    for (int i = 0; i < 12; i++) {
      array.append(frame_matrix[i]);
    }
    frame["transform"] = array;
  }
  plan["frame"] = frame;

  // get the path and the filename with no suffix
  QFileInfo fileInfo(file);
  std::string path = fileInfo.path().toStdString();
  std::string base = fileInfo.completeBaseName().toStdString();

  Json::Value probes(Json::arrayValue);
  for (std::vector<cbProbe>::iterator probe = this->Plan->begin();
       probe != this->Plan->end(); ++probe)
  {
    double position[3];
    double orientation[2];
    probe->GetPosition(position);
    probe->GetOrientation(orientation);

    Json::Value o;

    o["name"] = probe->GetName();
    o["probe"] = probe->specification().catalogue_number();
    Json::Value t(Json::arrayValue);
    t.append(position[0]);
    t.append(position[1]);
    t.append(position[2]);
    o["target"] = t;
    o["azimuth"] = orientation[0];
    o["declination"] = orientation[1];
    o["depth"] = probe->GetDepth();

    probes.append(o);
  }

  plan["probes"] = probes;

  Json::Value tags;
  vtkSurfaceNode *tagpoints =
    this->dataManager->FindSurfaceNode(this->tagKey);
  if (tagpoints) {
    vtkPoints *points = tagpoints->GetSurface()->GetPoints();
    if (points) {
      vtkIdType nPoints = points->GetNumberOfPoints();
      for (vtkIdType ipt = 0; ipt < nPoints; ipt++) {
        double point[3];
        points->GetPoint(ipt, point);
        Json::Value tag;
        Json::Value xyz(Json::arrayValue);
        xyz.append(point[0]);
        xyz.append(point[1]);
        xyz.append(point[2]);
        tag["xyz"] = xyz;
        tags.append(tag);
      }
    }
  }

  plan["tags"] = tags;

  // create an array to hold all loaded volumes
  Json::Value volumes(Json::arrayValue);

  // names of the nifti files here.
  vtkImageNode *mr_node = this->dataManager->FindImageNode(this->dataKey);
  vtkImageNode *ct_node = this->dataManager->FindImageNode(this->ctKey);

  if (mr_node) {
    Json::Value vol;

    // image_path = mr_node->GetFileURL();
    std::string image_path = base + "_primary.nii.gz";
    vol["file"] = image_path;

    double mr_matrix[16];
    vtkMatrix4x4::DeepCopy(mr_matrix, mr_node->GetMatrix());

    Json::Value array(Json::arrayValue);
    for (int i = 0; i < 12; i++) {
      array.append(mr_matrix[i]);
    }
    vol["transform"] = array;

    volumes.append(vol);

    // write the nifti file
    image_path = path + "/" + image_path;
    WriteNIFTIImage(image_path, mr_node->GetImage(), mr_node->GetMatrix());
  }

  if (ct_node) {
    Json::Value vol;

    // ct_path = ct_node->GetFileURL();
    std::string ct_path = base + "_secondary.nii.gz";
    vol["file"] = ct_path;

    double ct_matrix[16];
    vtkMatrix4x4::DeepCopy(ct_matrix, ct_node->GetMatrix());

    Json::Value array(Json::arrayValue);
    for (int i = 0; i < 12; i++) {
      array.append(ct_matrix[i]);
    }
    vol["transform"] = array;

    volumes.append(vol);

    // write the nifti file
    ct_path = path + "/" + ct_path;
    WriteNIFTIImage(ct_path, ct_node->GetImage(), ct_node->GetMatrix());
  }

  plan["volumes"] = volumes;

  std::string text = plan.toStyledString();
  std::ofstream os(file.toLocal8Bit().constData());
  os.write(text.data(), text.length());
  os.close();
}

void cbElectrodeController::OpenCTData(const QStringList& files)
{
  vtkSmartPointer<vtkImageData> ct_data =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> ct_matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> mr_matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> work_matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkDICOMMetaData> ct_meta =
    vtkSmartPointer<vtkDICOMMetaData>::New();

  vtkSmartPointer<vtkStringArray> ct_files =
    vtkSmartPointer<vtkStringArray>::New();
  for (int i = 0; i < files.size(); i++) {
    ct_files->InsertNextValue(files[i].toUtf8());
  }

  ReadImage(ct_files, ct_data, ct_matrix, ct_meta);
  work_matrix->DeepCopy(ct_matrix);

  std::cout << "*** image matrix ***" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::cout << ct_matrix->GetElement(i, j) << " ";
    }
    std::cout << std::endl;
  }
  this->RegisterCT(ct_data, ct_matrix);

  // compute the change in coords due to the registration
  work_matrix->Invert();
  vtkMatrix4x4::Multiply4x4(ct_matrix, work_matrix, work_matrix);
  // finally, put the points into MR data coordinates
  vtkImageNode *mr = this->dataManager->FindImageNode(this->dataKey);
  mr_matrix->DeepCopy(mr->GetMatrix());
  mr_matrix->Invert();
  vtkMatrix4x4::Multiply4x4(mr_matrix, work_matrix, work_matrix);

  vtkSmartPointer<vtkImageNode> ct_node =
    vtkSmartPointer<vtkImageNode>::New();

  this->dataManager->AddDataNode(ct_node, this->ctKey);

  this->dataManager->FindImageNode(this->ctKey)->ShallowCopyImage(ct_data);
  this->dataManager->FindImageNode(this->ctKey)->SetMatrix(ct_matrix);
  this->dataManager->FindImageNode(this->ctKey)->SetMetaData(ct_meta);

  // Check to see if there is a tag file
  if (files.size() > 0) {
    QString tagFile;
    int l = files[0].size();
    if (files[0].endsWith(".nii", Qt::CaseInsensitive)) {
      tagFile = files[0].left(l - 4) + ".tag";
    }
    else if (files[0].endsWith(".nii.gz", Qt::CaseInsensitive)) {
      tagFile = files[0].left(l - 7) + ".tag";
    }
    if (tagFile.size() > 0) {
      QFileInfo info(tagFile);
      if (info.exists()) {
        // for converting tags from NIFTI to DICOM coords
        const double flipXY[16] = {
          -1.0, 0.0, 0.0, 0.0,  0.0, -1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,
          0.0, 0.0, 0.0, 1.0
        };
        vtkSmartPointer<vtkTransform> ttransform =
          vtkSmartPointer<vtkTransform>::New();
        ttransform->PostMultiply();
        ttransform->Concatenate(flipXY);
        ttransform->Concatenate(work_matrix);
        vtkSmartPointer<vtkMNITagPointReader2> treader =
          vtkSmartPointer<vtkMNITagPointReader2>::New();
        treader->SetFileName(tagFile.toLocal8Bit().constData());

        vtkSmartPointer<vtkTransformPolyDataFilter> tfilter =
          vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tfilter->SetInputConnection(treader->GetOutputPort());
        tfilter->SetTransform(ttransform);
        tfilter->Update();

        vtkSmartPointer<vtkSurfaceNode> tag_node =
          vtkSmartPointer<vtkSurfaceNode>::New();
        this->dataManager->AddDataNode(tag_node, this->tagKey);
        this->dataManager->FindSurfaceNode(this->tagKey)
          ->ShallowCopySurface(tfilter->GetOutput());

        emit displayTags(this->tagKey);
      }
    }
  }

  emit DisplayCTData(this->ctKey);
}

void cbElectrodeController::RegisterCT(vtkImageData *ct_d, vtkMatrix4x4 *ct_m)
{
  QString baseStatus = "Registering secondary series to primary.";
  QString finalStatus = "Registration complete.";
  emit initializeProgress(0, 100);
  emit displayStatus(baseStatus);

  vtkImageNode *mr = this->dataManager->FindImageNode(this->dataKey);
  vtkImageData *mr_d = mr->GetImage();
  vtkMatrix4x4 *mr_m = mr->GetMatrix();

  cbMRIRegistration *regist = cbMRIRegistration::New();
  regist->SetInputSource(ct_d);
  regist->SetInputSourceMatrix(ct_m);
  regist->SetInputTarget(mr_d);
  regist->SetInputTargetMatrix(mr_m);

  const int levels = 3;
  const double blurFactors[3] = { 4.0, 2.0, 1.0 };

  regist->Initialize();
  emit displayProgress(1);

  // make a timer
  vtkNew<vtkTimerLog> timer;
  double startTime = timer->GetUniversalTime();
  double lastTime = startTime;

  // do multi-level registration
  for (int level = 1; level <= levels; level++) {
    regist->StartLevel(blurFactors[level-1]);

    double progressSeg = pow(2.0, -(levels - level + 1.0));

    // iterate until regist level is done
    int iterations = 0;
    do {
      ++iterations;
      int evals = regist->GetNumberOfEvaluations();
      emit displayStatus(baseStatus + " Level " + QString::number(level) +
                         ", Iter " + QString::number(iterations) +
                         " (" + QString::number(evals) + " Evals).");
      double progressExp = (1.0 - exp(-0.005*evals));
      int progress = 1 + static_cast<int>(
        99*(progressSeg*(1.0 + progressExp)));
      emit displayProgress(progress);
    }
    while (regist->Iterate());
  }

  double newTime = timer->GetUniversalTime();
  lastTime = newTime;

  regist->Finish();

  emit displayProgress(100);
  emit displayStatus(finalStatus + " Time: " +
                     QString::number(lastTime - startTime) +
                     " seconds.");

  // Allow the caller get the result of the registration
  ct_m->DeepCopy(regist->GetModifiedSourceMatrix());

  vtkNew<vtkImageReslice> reslicer;
  reslicer->SetInterpolationModeToCubic();
  reslicer->SetInputData(ct_d);
  reslicer->SetInformationInput(mr_d);

  vtkNew<vtkMatrix4x4> invertedMatrix;
  invertedMatrix->DeepCopy(ct_m);
  invertedMatrix->Invert();

  vtkNew<vtkTransform> resliceTransform;
  resliceTransform->PostMultiply();
  resliceTransform->Concatenate(mr_m);
  resliceTransform->Concatenate(invertedMatrix);

  reslicer->SetResliceTransform(resliceTransform);
  reslicer->Update();

  ct_d->DeepCopy(reslicer->GetOutput());
}

// Overloaded to include a pre-registered matrix
void cbElectrodeController::OpenCTData(
  const QStringList& files, vtkMatrix4x4 *m)
{
  vtkNew<vtkImageData> ct_data;
  vtkNew<vtkMatrix4x4> ct_matrix;
  vtkNew<vtkStringArray> ct_files;
  vtkNew<vtkDICOMMetaData> ct_meta;

  for (int i = 0; i < files.size(); i++) {
    ct_files->InsertNextValue(files[i].toUtf8());
  }

  ReadImage(ct_files, ct_data, ct_matrix, ct_meta);

  vtkImageNode *mr = this->dataManager->FindImageNode(this->dataKey);
  vtkImageData *mr_d = mr->GetImage();
  vtkMatrix4x4 *mr_m = mr->GetMatrix();

  vtkSmartPointer<vtkMatrix4x4> registered_m = m;

  vtkNew<vtkImageReslice> reslicer;
  reslicer->SetInterpolationModeToCubic();
  reslicer->SetInputData(ct_data);
  reslicer->SetInformationInput(mr_d);

  vtkNew<vtkMatrix4x4> invertedMatrix;
  invertedMatrix->DeepCopy(registered_m);
  invertedMatrix->Invert();

  vtkNew<vtkTransform> resliceTransform;
  resliceTransform->PostMultiply();
  resliceTransform->Concatenate(mr_m);
  resliceTransform->Concatenate(invertedMatrix);

  reslicer->SetResliceTransform(resliceTransform);
  reslicer->Update();

  ct_data->DeepCopy(reslicer->GetOutput());

  vtkNew<vtkImageNode> ct_node;

  this->dataManager->AddDataNode(ct_node, this->ctKey);

  this->dataManager->FindImageNode(this->ctKey)->ShallowCopyImage(ct_data);
  this->dataManager->FindImageNode(this->ctKey)->SetMatrix(m);
  this->dataManager->FindImageNode(this->ctKey)->SetMetaData(ct_meta);

  emit DisplayCTData(this->ctKey);
}

// For a pre-resampled image
void cbElectrodeController::OpenCTWithMatrix(
  const QStringList& files, vtkMatrix4x4 *m)
{
  vtkNew<vtkImageData> ct_data;
  vtkNew<vtkMatrix4x4> ct_matrix;
  vtkNew<vtkStringArray> ct_files;
  vtkNew<vtkDICOMMetaData> ct_meta;

  for (int i = 0; i < files.size(); i++) {
    ct_files->InsertNextValue(files[i].toUtf8());
  }

  ReadImage(ct_files, ct_data, ct_matrix, ct_meta);

  vtkSmartPointer<vtkImageNode> ct_node =
    vtkSmartPointer<vtkImageNode>::New();

  this->dataManager->AddDataNode(ct_node, this->ctKey);

  this->dataManager->FindImageNode(this->ctKey)->ShallowCopyImage(ct_data);
  this->dataManager->FindImageNode(this->ctKey)->SetMatrix(m);
  this->dataManager->FindImageNode(this->ctKey)->SetMetaData(ct_meta);

  emit DisplayCTData(this->ctKey);
}

void cbElectrodeController::registerAntPost(int s)
{
  this->useAnteriorPosteriorFiducials = s;
}
