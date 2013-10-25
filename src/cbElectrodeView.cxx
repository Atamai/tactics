/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeView.cxx

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

#include "LeksellFiducial.h"
#include "cbElectrodeView.h"
#include "cbProbe.h"
#include "cbProbeSpecification.h"

// QT INCLUDES
#include <QDebug>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QProgressBar>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QtGui>

// VTK INCLUDES
#include "qvtkViewToolCursorWidget.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkCollection.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkContourToPolygonFilter.h"
#include "vtkCornerAnnotation.h"
#include "vtkCutter.h"
#include "vtkDICOMMetaData.h"
#include "vtkDICOMValue.h"
#include "vtkDataManager.h"
#include "vtkDataSetMapper.h"
#include "vtkDummyViewPane.h"
#include "vtkDynamicViewFrame.h"
#include "vtkErrorCode.h"
#include "vtkFiducialPointsTool.h"
#include "vtkFollower.h"
#include "vtkFollowerPlane.h"
#include "vtkFrameRegistration.h"
#include "vtkImageData.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkImageImport.h"
#include "vtkImageNode.h"
#include "vtkImageProperty.h"
#include "vtkImageReslice.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkImageStack.h"
#include "vtkImageViewPane.h"
#include "vtkIntArray.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLinearTransform.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOutlineSource.h"
#include "vtkPNGWriter.h"
#include "vtkPanCameraTool.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkPushPlaneTool.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRotateCameraTool.h"
#include "vtkSliceImageTool.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStripper.h"
#include "vtkTextProperty.h"
#include "vtkToolCursor.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkVectorText.h"
#include "vtkViewPane.h"
#include "vtkViewRect.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWindowLevelTool.h"
#include "vtkWindowToImageFilter.h"
#include "vtkZoomCameraTool.h"

// SYSTEM INCLUDES
#include <assert.h>
#include <sstream>

namespace cb {
  template<class T>
  bool arrays_equal(T *a, T *b, size_t size);
} /* namespace cb */

cbElectrodeView::cbElectrodeView(vtkDataManager *dataManager, QWidget *parent)
: cbMainWindow(dataManager, parent), dataKey(), ctKey(), SelectedIndex(0), SaveFile(), SavedState(false)
{
  QDesktopWidget desktop;
  int width = desktop.width();
  int height = desktop.height();
  this->resize(width, height);

  this->showMaximized();

  this->setWindowTitle("Tactics - Stereotaxy Planner");
  this->setWindowIcon(QIcon(QFile(appDirectoryOf("Icons")
                                  .absoluteFilePath("Cerebra-Elec.png"))
                            .fileName()));

  QCoreApplication::setOrganizationName("CIPAC");
  QCoreApplication::setOrganizationDomain("http://wcmprod2.ucalgary.ca/caincalgary/");
  QCoreApplication::setApplicationName("Tactics");

  this->CreateMenu();
  this->CreateAndConfigurePanes();
  this->CreateAndBindTools();
  this->CreateFrameObjects();
  this->CreatePlanVisualization();
  this->CreateLabelsAndAnnotations();

  this->qvtkWidget->SetSynchronized(false);

  this->SavedState = true;

  this->ImageProperty = vtkImageProperty::New();
  this->CTProperty = vtkImageProperty::New();

  this->viewRect->Start();
}

cbElectrodeView::~cbElectrodeView()
{
  this->DestroyMembers();
}

void cbElectrodeView::displayData(vtkDataManager::UniqueKey k)
{
  this->initializeProgress(0,1);
  this->dataKey = k;
  this->SaveFile = "";

  // If this is not the first time rendering, clear stack actors
  for (int i = 0; i < this->Slices.size(); i++) {
    planar->GetRenderer()->RemoveViewProp(this->Slices[i].Stack);
    planar->GetRenderer()->RemoveViewProp(this->Slices[i].Border);
    planar->GetRenderer()->RemoveViewProp(this->Slices[i].Intersect);
  }
  this->surface->RemoveAllViewProps();

  this->Slices.clear();

  vtkImageNode *primary_node = this->dataManager->FindImageNode(k);
  vtkImageData *data = primary_node->GetImage();
  vtkMatrix4x4 *matrix = this->frameTransform;

  // append the patient name to the patient data annotation
  vtkDICOMMetaData *primary_meta_data = primary_node->GetMetaData();
  const vtkDICOMValue patient_name_val =
    primary_meta_data->GetAttributeValue(DC::PatientName);
  std::string patient_name = patient_name_val.AsString();
  if (patient_name.empty()) {
    patient_name = "unnamed";
  }

  std::string primary_name_label = "Primary Patient: ";
  primary_name_label.append(patient_name);

  vtkSmartPointer<vtkCornerAnnotation> meta_annotation = this->MetaAnnotation;
  meta_annotation->SetMaximumFontSize(12);
  meta_annotation->SetMinimumFontSize(10);
  planar->GetRenderer()->AddActor2D(meta_annotation);

  this->MetaAnnotation->SetText(3, primary_name_label.c_str());

  vtkImageProperty *property = this->ImageProperty;
  property->BackingOn();

  // Set window/level to display up to the 99th percentile of image
  // pixels (i.e. the brightest 1% of the pixels will saturate).
  // This ensures that a few abnormally bright pixels will not
  // cause the Window/Level to be miscalculated.
  double range[2];
  cbElectrodeView::ComputePercentileRange(data, 99.0, range);
  property->SetColorWindow(range[1]-range[0]);
  property->SetColorLevel(0.5*(range[1]+range[0]));
  property->SetInterpolationTypeToCubic();

  // Add data to panes
  this->addDataToPanes(data, matrix, property);

  // Create the orthogonal planes
  double center[4];
  int extent[6];
  data->GetWholeExtent(extent);
  double spacing[3];
  data->GetSpacing(spacing);

  center[0] = 0.5*(extent[0] + extent[1])*spacing[0];
  center[1] = 0.5*(extent[2] + extent[3])*spacing[1];
  center[2] = 0.5*(extent[4] + extent[5])*spacing[2];
  center[3] = 1.0;
  matrix->MultiplyPoint(center, center);

  // Create the orthogonal planar slices for the main view

  // This loop creates the slices, and adds the prop
  for (int i = 0; i < 3; i++) {
    double normal[4] = { 0.0, 0.0, 0.0, 0.0 };
    normal[i] = 1.0;
    matrix->MultiplyPoint(normal, normal);
    this->Slices.push_back(cbElectrodeView::Slice(normal));
    planar->GetRenderer()->AddViewProp(this->Slices[i].Stack);
  }

  this->AddLayer(property);

  // This loop configures the planes
  for (int i = 0; i < 3; i++) {
    double normal[4] = { 0.0, 0.0, 0.0, 0.0 };
    normal[i] = 1.0;
    matrix->MultiplyPoint(normal, normal);

    this->Slices[i].WorldPlane->SetOrigin(center);
    this->Slices[i].WorldPlane->SetNormal(normal);

    vtkImageSlice *image = this->GetImageSlice(cbElectrodeView::kMR, i);
    image->GetMapper()->SetInput(data);
    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5*(range[1] + range[0]));
    image->SetUserMatrix(matrix);

    // Set up the DataPlane
    this->Slices[i].DataPlane->SetFollowMatrix(matrix);
    this->Slices[i].DataPlane->InvertFollowMatrixOn();
  }

  // This loop adds the borders and configures the focal point of the planes
  for (int i = 0; i < 3; i++) {
    // At this point, the orthogonal plane (i) has been created
    vtkImageSlice *image = this->GetImageSlice(0, i);

    // Create an outline that will be cut to create the borders
    double bounds[6];
    image->GetMapper()->GetBounds(bounds);
    for (int j = 0; j < 3; j++) {
      // expand the bounds by a small tolerance value
      const double e = 1e-3;
      bounds[2*j] -= e;
      bounds[2*j + 1] += e;
    }
    vtkSmartPointer<vtkOutlineSource> planeOutlineSource =
      vtkSmartPointer<vtkOutlineSource>::New();
    planeOutlineSource->GenerateFacesOn();
    planeOutlineSource->SetBounds(bounds);

    vtkSmartPointer<vtkCutter> planeBorderCutter =
      vtkSmartPointer<vtkCutter>::New();
    planeBorderCutter->SetInputConnection(planeOutlineSource->GetOutputPort());
    planeBorderCutter->SetCutFunction(this->Slices[i].DataPlane);

    vtkSmartPointer<vtkDataSetMapper> planeBorderMapper =
      vtkSmartPointer<vtkDataSetMapper>::New();
    planeBorderMapper->SetInputConnection(planeBorderCutter->GetOutputPort());

    vtkSmartPointer<vtkActor> planeBorderActor = this->Slices[i].Border;
    planeBorderActor->SetUserMatrix(matrix);
    planeBorderActor->SetVisibility(1);
    planeBorderActor->SetPickable(0);
    planeBorderActor->GetProperty()->BackfaceCullingOn();
    planeBorderActor->GetProperty()->SetColor(0.2, 0.3, 0.3);
    planeBorderActor->SetMapper(planeBorderMapper);

    // Add the plane outline actor to the scene
    planar->GetRenderer()->AddActor(planeBorderActor);

    // Create the intersecting lines/borders
    vtkSmartPointer<vtkContourToPolygonFilter> ctp =
      vtkSmartPointer<vtkContourToPolygonFilter>::New();
    vtkSmartPointer<vtkCutter> ctpC = vtkSmartPointer<vtkCutter>::New();
    vtkSmartPointer<vtkStripper> ctpS = vtkSmartPointer<vtkStripper>::New();
    vtkSmartPointer<vtkPolyDataMapper> ctpM =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> ctpA = this->Slices[i].Intersect;

    ctp->SetInput(planeBorderCutter->GetOutput());
    ctpC->SetInputConnection(ctp->GetOutputPort());
    ctpC->SetCutFunction(this->Slices[(i + 1)%3].DataPlane);
    ctpS->SetInputConnection(ctpC->GetOutputPort());
    ctpM->SetInputConnection(ctpS->GetOutputPort());

    // Configure the actor
    ctpA->SetMapper(ctpM);
    ctpA->SetUserMatrix(matrix);
    ctpA->SetVisibility(1);
    ctpA->SetPickable(0);
    ctpA->GetProperty()->BackfaceCullingOn();
    ctpA->GetProperty()->SetColor(0.5, 0.0, 0.0);
    planar->GetRenderer()->AddActor(ctpA);

    // !!!! this smells!
    // Set the pane's focal point to be the discovered point
    vtkImageViewPane *currentPane = NULL;
    switch(i) {
      case 0:
        currentPane = sagittalPane;
        break;
      case 1:
        currentPane = coronalPane;
        break;
      case 2:
        currentPane = axialPane;
        break;
      default:
        break;
    }

    // Get slice plane of the orthogonal plane's mapper to get the normal
    image->GetMapper()->UpdateInformation();
    vtkPlane *slicePlane = image->GetMapper()->GetSlicePlane();
    double normal[3];
    slicePlane->GetNormal(normal);

    double focalPoint[3];
    currentPane->GetRenderer()->GetActiveCamera()->SetFocalPoint(center);
    currentPane->GetRenderer()->GetActiveCamera()->GetFocalPoint(focalPoint);

    // Define two points for intersection
    double p1[3], p2[3];
    p1[0] = focalPoint[0] + 1000*normal[0];
    p1[1] = focalPoint[1] + 1000*normal[1];
    p1[2] = focalPoint[2] + 1000*normal[2];

    p2[0] = focalPoint[0] - 1000*normal[0];
    p2[1] = focalPoint[1] - 1000*normal[1];
    p2[2] = focalPoint[2] - 1000*normal[2];

    // Intersect the normal with the plane to get a point
    double parametric;
    slicePlane->IntersectWithLine(p1, p2, parametric, focalPoint);

    currentPane->GetRenderer()->GetActiveCamera()->SetFocalPoint(focalPoint);
  }

  // Configure the cameras of the planar and surface renderers
  this->surface->ResetCamera();
  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  cbMainWindow::SetViewFromMatrix(surface, style, matrix);

  double position[3];
  planar->GetRenderer()->ResetCamera();
  vtkCamera *camera = planar->GetRenderer()->GetActiveCamera();
  camera->SetFocalPoint(center);
  surface->GetActiveCamera()->SetFocalPoint(center);
  position[0] = center[0] - 600.0;
  position[1] = center[1] + 150.0;
  position[2] = center[2] - 150.0;
  camera->SetPosition(position);
  camera->SetViewUp(0.0, 0.0, 1.0);
  camera->OrthogonalizeViewUp();
  camera->SetClippingRange(200, 1000);
  surface->GetActiveCamera()->SetPosition(position);
  surface->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
  surface->GetActiveCamera()->OrthogonalizeViewUp();
  surface->GetActiveCamera()->SetClippingRange(200, 1000);

  // Add anatomical labels to surface (should be done when creating surface..)
  vtkSmartPointer<vtkVectorText> anteriorLabel =
    vtkSmartPointer<vtkVectorText>::New();
  vtkSmartPointer<vtkVectorText> posteriorLabel =
    vtkSmartPointer<vtkVectorText>::New();
  vtkSmartPointer<vtkVectorText> leftLabel =
    vtkSmartPointer<vtkVectorText>::New();
  vtkSmartPointer<vtkVectorText> rightLabel =
    vtkSmartPointer<vtkVectorText>::New();
  vtkSmartPointer<vtkVectorText> superiorLabel =
    vtkSmartPointer<vtkVectorText>::New();
  vtkSmartPointer<vtkVectorText> inferiorLabel =
    vtkSmartPointer<vtkVectorText>::New();

  anteriorLabel->SetText("A");
  posteriorLabel->SetText("P");
  leftLabel->SetText("L");
  rightLabel->SetText("R");
  superiorLabel->SetText("S");
  inferiorLabel->SetText("I");

  vtkSmartPointer<vtkDataSetMapper> anteriorLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkSmartPointer<vtkDataSetMapper> posteriorLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkSmartPointer<vtkDataSetMapper> leftLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkSmartPointer<vtkDataSetMapper> rightLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkSmartPointer<vtkDataSetMapper> superiorLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkSmartPointer<vtkDataSetMapper> inferiorLabelMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();

  anteriorLabelMapper->SetInputConnection(anteriorLabel->GetOutputPort());
  posteriorLabelMapper->SetInputConnection(posteriorLabel->GetOutputPort());
  leftLabelMapper->SetInputConnection(leftLabel->GetOutputPort());
  rightLabelMapper->SetInputConnection(rightLabel->GetOutputPort());
  superiorLabelMapper->SetInputConnection(superiorLabel->GetOutputPort());
  inferiorLabelMapper->SetInputConnection(inferiorLabel->GetOutputPort());

  vtkSmartPointer<vtkFollower> anteriorFollower =
    vtkSmartPointer<vtkFollower>::New();
  vtkSmartPointer<vtkFollower> posteriorFollower =
    vtkSmartPointer<vtkFollower>::New();
  vtkSmartPointer<vtkFollower> leftFollower =
    vtkSmartPointer<vtkFollower>::New();
  vtkSmartPointer<vtkFollower> rightFollower =
    vtkSmartPointer<vtkFollower>::New();
  vtkSmartPointer<vtkFollower> superiorFollower =
    vtkSmartPointer<vtkFollower>::New();
  vtkSmartPointer<vtkFollower> inferiorFollower =
    vtkSmartPointer<vtkFollower>::New();

  anteriorFollower->SetMapper(anteriorLabelMapper);
  posteriorFollower->SetMapper(posteriorLabelMapper);
  leftFollower->SetMapper(leftLabelMapper);
  rightFollower->SetMapper(rightLabelMapper);
  superiorFollower->SetMapper(superiorLabelMapper);
  inferiorFollower->SetMapper(inferiorLabelMapper);

  double offset = 120.0;

  anteriorFollower ->SetPosition(center[0], center[1] + offset, center[2]);
  posteriorFollower->SetPosition(center[0], center[1] - offset, center[2]);
  leftFollower     ->SetPosition(center[0] + offset, center[1], center[2]);
  rightFollower    ->SetPosition(center[0] - offset, center[1], center[2]);
  superiorFollower ->SetPosition(center[0], center[1], center[2] - offset);
  inferiorFollower ->SetPosition(center[0], center[1], center[2] + offset);

  double scale = 10.0;
  anteriorFollower ->SetScale(scale);
  posteriorFollower->SetScale(scale);
  leftFollower     ->SetScale(scale);
  rightFollower    ->SetScale(scale);
  superiorFollower ->SetScale(scale);
  inferiorFollower ->SetScale(scale);

  anteriorFollower ->SetCamera(surface->GetActiveCamera());
  posteriorFollower->SetCamera(surface->GetActiveCamera());
  leftFollower     ->SetCamera(surface->GetActiveCamera());
  rightFollower    ->SetCamera(surface->GetActiveCamera());
  superiorFollower ->SetCamera(surface->GetActiveCamera());
  inferiorFollower ->SetCamera(surface->GetActiveCamera());

  double labelColor1[3] = { 1.0, 0.0, 0.0 };
  double labelColor2[3] = { 0.0, 1.0, 0.0 };
  anteriorFollower ->GetProperty()->SetColor(labelColor2);
  posteriorFollower->GetProperty()->SetColor(labelColor1);
  leftFollower     ->GetProperty()->SetColor(labelColor1);
  rightFollower    ->GetProperty()->SetColor(labelColor2);
  superiorFollower ->GetProperty()->SetColor(labelColor2);
  inferiorFollower ->GetProperty()->SetColor(labelColor1);

  surface->AddActor(anteriorFollower);
  surface->AddActor(posteriorFollower);
  surface->AddActor(leftFollower);
  surface->AddActor(rightFollower);
  surface->AddActor(superiorFollower);
  surface->AddActor(inferiorFollower);

  // --- end of adding labels to the surface renderer

  // For each pane add the left/right follower labels
  for (int i = 1; i < 3; i++) {
    vtkImageViewPane *currentPane = NULL;
    vtkSmartPointer<vtkFollower> leftFollower;
    vtkSmartPointer<vtkFollower> rightFollower;

    switch(i) {
      case 1:
        currentPane = coronalPane;
        leftFollower = leftCoronalFollower;
        rightFollower = rightCoronalFollower;
        break;
      case 2:
        currentPane = axialPane;
        leftFollower = leftAxialFollower;
        rightFollower = rightAxialFollower;
        break;
      default:
        break;
    }

    vtkSmartPointer<vtkDataSetMapper> leftLabelMapper =
      vtkSmartPointer<vtkDataSetMapper>::New();
    vtkSmartPointer<vtkDataSetMapper> rightLabelMapper =
      vtkSmartPointer<vtkDataSetMapper>::New();

    leftLabelMapper->SetInputConnection(leftLabel->GetOutputPort());
    rightLabelMapper->SetInputConnection(rightLabel->GetOutputPort());

    leftLabel->GetOutput()->Update();
    rightLabel->GetOutput()->Update();
    leftFollower->SetOrigin(leftLabel->GetOutput()->GetCenter());
    rightFollower->SetOrigin(rightLabel->GetOutput()->GetCenter());

    leftFollower->SetMapper(leftLabelMapper);
    rightFollower->SetMapper(rightLabelMapper);

    this->Slices[i].WorldPlane->GetOrigin(center);

    leftFollower ->SetScale(scale);
    rightFollower->SetScale(scale);

    vtkCamera *current_cam = currentPane->GetRenderer()->GetActiveCamera();

    leftFollower ->SetCamera(current_cam);
    rightFollower->SetCamera(current_cam);

    leftFollower ->GetProperty()->SetColor(labelColor1);
    rightFollower->GetProperty()->SetColor(labelColor2);

    currentPane->GetRenderer()->AddActor(leftFollower);
    currentPane->GetRenderer()->AddActor(rightFollower);
  }

  vtkSmartPointer<vtkCornerAnnotation> annotation = this->helpAnnotation;
  annotation->SetText(2,
    "Left Click: Active Tool\nRight Click: Rotate View\nMousewheel : Zoom\nMousewheel(side panes): Slice");

  annotation->SetMaximumFontSize(12);
  annotation->SetMinimumFontSize(10);
  planar->GetRenderer()->AddActor2D(annotation);

  this->resetViewOrientations();
  this->displayProgress(1);
  viewRect->Start();

  this->SetSavedState(false);
}

void cbElectrodeView::SetOrientationToAxial(vtkRenderer *r)
{
  static const double ZViewRightVector[3] = {-1.0, 0.0, 0.0};
  static const double ZViewUpVector[3] = {0.0, 1.0, 0.0};
  double rightVector[3], upVector[3];

  cbElectrodeView::SnapViewVectorsToVolume(
    this->frameTransform, ZViewRightVector, ZViewUpVector,
    rightVector, upVector);

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetCurrentRenderer(r);
  style->SetImageOrientation(rightVector, upVector);

  vtkCamera *c = r->GetActiveCamera();
  this->FixCameraPosition(c);
  this->SetClippingRange(c);

  viewRect->Start();
}

void cbElectrodeView::SetOrientationToSagittal(vtkRenderer *r)
{
  static const double XViewRightVector[3] = {0.0, 1.0, 0.0};
  static const double XViewUpVector[3] = {0.0, 0.0, -1.0};
  double rightVector[3], upVector[3];

  cbElectrodeView::SnapViewVectorsToVolume(
    this->frameTransform, XViewRightVector, XViewUpVector,
    rightVector, upVector);

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetCurrentRenderer(r);
  style->SetImageOrientation(rightVector, upVector);

  vtkCamera *c = r->GetActiveCamera();
  this->FixCameraPosition(c);
  this->SetClippingRange(c);

  viewRect->Start();
}

void cbElectrodeView::SetOrientationToCoronal(vtkRenderer *r)
{
  static double YViewRightVector[3] = {-1.0, 0.0, 0.0};
  static double YViewUpVector[3] = {0.0, 0.0, -1.0};
  double rightVector[3], upVector[3];

  cbElectrodeView::SnapViewVectorsToVolume(
    this->frameTransform, YViewRightVector, YViewUpVector,
    rightVector, upVector);

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetCurrentRenderer(r);
  style->SetImageOrientation(rightVector, upVector);

  vtkCamera *c = r->GetActiveCamera();
  this->FixCameraPosition(c);
  this->SetClippingRange(c);

  viewRect->Start();
}

cbElectrodeView::Slice::Slice(const double orientation[3])
{
  this->Stack = vtkSmartPointer<vtkImageStack>::New();
  this->Stack->SetActiveLayer(0);
  this->WorldPlane = vtkSmartPointer<vtkPlane>::New();
  this->DataPlane = vtkSmartPointer<vtkFollowerPlane>::New();
  this->Border = vtkSmartPointer<vtkActor>::New();
  this->Intersect = vtkSmartPointer<vtkActor>::New();

  // Orientation is the original orientation and it will never change,
  // it is used later when we want to reset the slice
  this->Orientation[0] = orientation[0];
  this->Orientation[1] = orientation[1];
  this->Orientation[2] = orientation[2];
  vtkMath::Normalize(this->Orientation);

  this->WorldPlane->SetNormal(this->Orientation);

  // Set up DataPlane
  this->DataPlane->SetFollowPlane(this->WorldPlane);
}

cbElectrodeView::Slice::~Slice()
{
}

vtkImageSlice *cbElectrodeView::GetImageSlice(int layer, int orientation)
{
  if (static_cast<size_t>(orientation) >= this->Slices.size()) {
    return 0;
  }

  vtkImageStack *stack = this->Slices[orientation].Stack;
  vtkImageSliceCollection *images = stack->GetImages();

  vtkCollectionSimpleIterator pit;
  images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ((image = images->GetNextImage(pit)) != 0) {
    if (image->GetProperty()->GetLayerNumber() == layer) {
      break;
    }
  }

  return image;
}

void cbElectrodeView::AddLayer(vtkImageProperty *p)
{
  size_t n = this->Slices.size();
  for (size_t i = 0; i < n; i++) {
    vtkSmartPointer<vtkImageResliceMapper> mapper =
      vtkSmartPointer<vtkImageResliceMapper>::New();
    mapper->SetSlicePlane(this->Slices[i].WorldPlane);
    mapper->ResampleToScreenPixelsOff();

    vtkSmartPointer<vtkImageSlice> slice =
      vtkSmartPointer<vtkImageSlice>::New();
    slice->SetMapper(mapper);
    slice->SetProperty(p);
    this->Slices[i].Stack->AddImage(slice);
  }
}

void cbElectrodeView::bindToolCursorAction(int cursortool, int mousebutton)
{
  // Cache the current tool (on left click only)
  if (mousebutton == Qt::LeftButton) {
    this->currentTool = static_cast<cbElectrodeView::cursortool>(cursortool);
  }

  // Toggle guide visibility depending on the tool being used
  if (this->currentTool == cbElectrodeView::Plane) {
    this->planar->GetToolCursor()->GuideVisibilityOn();
  } else {
    this->planar->GetToolCursor()->GuideVisibilityOff();
  }

  if (cursortool == cbElectrodeView::Plane) {
    if (mousebutton == Qt::LeftButton) {
      int bind = planar->GetToolCursor()->AddAction(this->planeTool);
      planar->GetToolCursor()->BindAction(bind, 0, 0, VTK_TOOL_B1);
    }
  }
  else if (cursortool == cbElectrodeView::Pick) {
    if (mousebutton == Qt::LeftButton) {
      int bind = planar->GetToolCursor()->AddAction(this->pickTool);
      planar->GetToolCursor()->BindAction(bind, 0, 0, VTK_TOOL_B1);
    }
  }
  else if (cursortool == cbElectrodeView::Rotate) {
    if (mousebutton == Qt::RightButton) {
      int bind = planar->GetToolCursor()->AddAction(this->rotateTool);
      planar->GetToolCursor()->BindAction(bind, 0, 0, VTK_TOOL_B2);
    }
  }
  else {
    vtkTool *actionTool = NULL;
    switch (cursortool) {
      case cbElectrodeView::Zoom:
        actionTool = vtkZoomCameraTool::New();
        break;
      case cbElectrodeView::Pan:
        actionTool = vtkPanCameraTool::New();
        break;
      case cbElectrodeView::Winlev:
        actionTool = vtkWindowLevelTool::New();
        break;
      case cbElectrodeView::Rotate:
        actionTool = vtkRotateCameraTool::New();
        break;
      case cbElectrodeView::Plane:
        actionTool = vtkPushPlaneTool::New();
        break;
      case cbElectrodeView::Pick:
        actionTool = vtkFiducialPointsTool::New();
        break;
      default:
        break;
    }

    switch (mousebutton) {
      case Qt::LeftButton:
        planar->GetToolCursor()
              ->BindAction(planar->GetToolCursor()->AddAction(actionTool),
                           0, 0, VTK_TOOL_B1);
        break;
      case Qt::RightButton:
        planar->GetToolCursor()
              ->BindAction(planar->GetToolCursor()->AddAction(actionTool),
                           0, 0, VTK_TOOL_B2);
        break;
      default:
        break;
    }
  }
}

void cbElectrodeView::resetViewOrientations()
{
  vtkMatrix4x4 *m = this->frameTransform;

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();

  this->viewRect->GetRenderWindow()->SwapBuffersOff();

  vtkImageData *data = this->dataManager->FindImageNode(this->dataKey)->GetImage();

  double center[4];
  int extent[6];
  data->GetWholeExtent(extent);
  double spacing[3];
  data->GetSpacing(spacing);

  center[0] = 0.5*(extent[0] + extent[1])*spacing[0];
  center[1] = 0.5*(extent[2] + extent[3])*spacing[1];
  center[2] = 0.5*(extent[4] + extent[5])*spacing[2];
  center[3] = 1.0;

  this->frameTransform->MultiplyPoint(center, center);

  double position[3];
  vtkCamera *camera = this->planar->GetRenderer()->GetActiveCamera();
  camera->SetFocalPoint(center);
  surface->GetActiveCamera()->SetFocalPoint(center);
  position[0] = center[0] - 600.0;
  position[1] = center[1] + 150.0;
  position[2] = center[2] - 150.0;
  camera->SetPosition(position);
  surface->GetActiveCamera()->SetPosition(position);

  cbMainWindow::SetViewFromMatrix(axialPane->GetRenderer(), style, m);
  cbMainWindow::SetViewFromMatrix(sagittalPane->GetRenderer(), style, m);
  cbMainWindow::SetViewFromMatrix(coronalPane->GetRenderer(), style, m);

  SetOrientationToAxial(axialPane->GetRenderer());
  SetOrientationToSagittal(sagittalPane->GetRenderer());
  SetOrientationToCoronal(coronalPane->GetRenderer());

  this->planar->GetRenderer()->GetActiveCamera()->SetViewUp(0.0, 0.0, -1.0);
  this->surface->GetActiveCamera()->SetViewUp(0.0, 0.0, -1.0);

  for (int i = 0; i < 3; i++) {
    vtkImageSlice *image = this->GetImageSlice(0, i);

    double normal[4];
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 0.0;
    normal[3] = 0.0;
    normal[i] = 1.0;

    this->frameTransform->MultiplyPoint(normal, normal);

    // Get slice plane of the orthogonal plane's mapper to get the normal
    image->GetMapper()->UpdateInformation();
    vtkPlane *slicePlane = image->GetMapper()->GetSlicePlane();
    slicePlane->SetNormal(normal);
    slicePlane->SetOrigin(center);
  }

  this->FocalPointSliceCallback(this->planeTool, 0, NULL);

  this->viewRect->GetRenderWindow()->SwapBuffersOn();
  this->viewRect->GetRenderWindow()->Frame();
  this->viewRect->Start();
}

void cbElectrodeView::FixCameraPosition(vtkCamera *c)
{
  double focal[3];
  double position[3];
  double vec[3];
  c->GetFocalPoint(focal);
  c->GetPosition(position);

  vec[0] = position[0] - focal[0];
  vec[1] = position[1] - focal[1];
  vec[2] = position[2] - focal[2];
  vtkMath::Normalize(vec);

  double mm = 1000.0;
  c->SetPosition(vec[0]*mm+focal[0], vec[1]*mm+focal[1], vec[2]*mm+focal[2]);
}

void cbElectrodeView::PositionLRLabelsIn2DPanes()
{
  // This function will position the left/right (L R) labels
  // in the axial and sagittal plane such that they are just
  // behind the near clipping plane of the camera.

  // loop over the panes (only axial and coronal)
  for (int i = 0; i < 2; i++) {
    vtkActor *leftActor = this->leftAxialFollower;
    vtkActor *rightActor = this->rightAxialFollower;
    vtkImageViewPane *pane = this->axialPane;
    if (i == 0) {
      leftActor = this->leftCoronalFollower;
      rightActor = this->rightCoronalFollower;
      pane = this->coronalPane;
    }

    vtkCamera *camera = pane->GetRenderer()->GetActiveCamera();
    double clippingRange[2];
    camera->GetClippingRange(clippingRange);
    vtkMatrix4x4 *matrix = camera->GetModelViewTransformMatrix();

    // pull the horizontal view direction straight from camera matrix
    // (it is the first three elements of the 1st row of the matrix)
    double horizVector[3], zVector[3];
    horizVector[0] = matrix->GetElement(0, 0);
    horizVector[1] = matrix->GetElement(0, 1);
    horizVector[2] = matrix->GetElement(0, 2);
    zVector[0] = matrix->GetElement(2, 0);
    zVector[1] = matrix->GetElement(2, 1);
    zVector[2] = matrix->GetElement(2, 2);

    // use camera position plus near clipping range plus a tolerance
    const double tol = 0.5; // fraction of distance to focus
    double distance = clippingRange[0]*(1.0 - tol) + camera->GetDistance()*tol;
    double position[3];
    camera->GetPosition(position);
    position[0] -= zVector[0]*distance;
    position[1] -= zVector[1]*distance;
    position[2] -= zVector[2]*distance;

    // check the x component to ensure it is positive
    // (THIS CHECK IS CRITICAL... IT MAKES SURE LEFT IS LEFT)
    if (horizVector[0] < 0) {
      horizVector[0] = -horizVector[0];
      horizVector[1] = -horizVector[1];
      horizVector[2] = -horizVector[2];
    }

    // make invisible if angle is greater than 45 degrees
    leftActor->SetVisibility(horizVector[0] >= 0.70710678118654757);
    rightActor->SetVisibility(horizVector[0] >= 0.70710678118654757);

    // position the label actors at an offset left or right
    int *size = pane->GetRenderer()->GetSize(); // in pixels
    double height = camera->GetParallelScale(); // in millimetres
    double width = height*size[0]/(size[1] > 1 ? size[1] : 1);
    double offset = 0.9*width;
    vtkActor *actor = leftActor;
    for (int j = 0; j < 2; j++) {
      double actorPosition[3];
      actorPosition[0] = position[0] + horizVector[0]*offset;
      actorPosition[1] = position[1] + horizVector[1]*offset;
      actorPosition[2] = position[2] + horizVector[2]*offset;
      actor->SetPosition(actorPosition);

      // repeat for right
      offset = -offset;
      actor = rightActor;
    }
  }
}

void cbElectrodeView::FocalPointSliceCallback(vtkObject *o, unsigned long, void *)
{
  vtkPushPlaneTool *tool = vtkPushPlaneTool::SafeDownCast(o);
  if (!tool) {
    return;
  }

  for (int i = 0; i < 3; i++) {
    // Set the pane's focal point to be the discovered point
    vtkImageViewPane *currentPane = NULL;

    switch(i) {
      case 0:
        currentPane = sagittalPane;
        break;
      case 1:
        currentPane = coronalPane;
        break;
      case 2:
        currentPane = axialPane;
        break;
      default:
        break;
    }

    // Use the plane to set the camera position
    vtkPlane *worldplane = this->Slices[i].WorldPlane;
    double normal[3], origin[3];
    worldplane->GetNormal(normal);
    worldplane->GetOrigin(origin);
    vtkMath::Normalize(normal);

    double focalPoint[3];
    currentPane->GetRenderer()->GetActiveCamera()->GetFocalPoint(focalPoint);

    // Choose the orientation that does not flip the image
    if (normal[i] > 0) {
      normal[0] = -normal[0];
      normal[1] = -normal[1];
      normal[2] = -normal[2];
    }

    // Define two points for intersection
    double mm = 1000.0;
    double p1[3], p2[3];
    p1[0] = focalPoint[0] + mm*normal[0];
    p1[1] = focalPoint[1] + mm*normal[1];
    p1[2] = focalPoint[2] + mm*normal[2];

    p2[0] = focalPoint[0] - mm*normal[0];
    p2[1] = focalPoint[1] - mm*normal[1];
    p2[2] = focalPoint[2] - mm*normal[2];

    // Intersect the normal with the plane to get a point
    double parametric;
    worldplane->IntersectWithLine(p1, p2, parametric, focalPoint);

    vtkCamera *currentCamera = currentPane->GetRenderer()->GetActiveCamera();
    currentCamera->SetFocalPoint(focalPoint);
    currentCamera->SetPosition(normal[0]*mm + focalPoint[0],
                               normal[1]*mm + focalPoint[1],
                               normal[2]*mm + focalPoint[2]);
  }

  this->PositionLRLabelsIn2DPanes();
}

void cbElectrodeView::RotateVolumeCallback(vtkObject *o, unsigned long, void *)
{
  vtkRotateCameraTool *tool = vtkRotateCameraTool::SafeDownCast(o);
  if (!tool) {
    return;
  }
  vtkCamera *c = this->planar->GetRenderer()->GetActiveCamera();
  vtkCamera *surfaceCamera = this->surface->GetActiveCamera();

  double focalPoint[3], position[3], viewPlaneNormal[3], viewUp[3];
  c->GetFocalPoint(focalPoint);
  c->GetPosition(position);
  c->GetViewUp(viewUp);

  viewPlaneNormal[0] = position[0] - focalPoint[0];
  viewPlaneNormal[1] = position[1] - focalPoint[1];
  viewPlaneNormal[2] = position[2] - focalPoint[2];

  vtkMath::Normalize(viewPlaneNormal);

  double distance = surfaceCamera->GetDistance();
  surfaceCamera->GetFocalPoint(focalPoint);

  position[0] = viewPlaneNormal[0]*distance + focalPoint[0];
  position[1] = viewPlaneNormal[1]*distance + focalPoint[1];
  position[2] = viewPlaneNormal[2]*distance + focalPoint[2];

  surfaceCamera->SetPosition(position);
  surfaceCamera->SetViewUp(viewUp);
}

void cbElectrodeView::MaximizeSurfaceRenderer()
{
  this->surface->SetViewport(this->planarPort);
  this->planar->GetRenderer()->SetViewport(this->surfacePort);
  this->viewRect->Start();
}

void cbElectrodeView::MinimizeSurfaceRenderer()
{
  this->surface->SetViewport(this->surfacePort);
  this->planar->GetRenderer()->SetViewport(this->planarPort);
  this->viewRect->Start();
}

void cbElectrodeView::ToggleMaximizeSurface()
{
  double viewport[4];
  this->surface->GetViewport(viewport);
  if (cb::arrays_equal<double>(viewport, this->surfacePort, 4)) {
    this->MaximizeSurfaceRenderer();
    return;
  }
  this->MinimizeSurfaceRenderer();
}

namespace cb {
  template<class T>
  bool arrays_equal(T *a, T *b, size_t size)
  {
    for (size_t i = 0; i < size; i++) {
      if (a[i] != b[i]) {
        return false;
      }
    }
    return true;
  }
} /* namespace cb */

void cbElectrodeView::displayLeksellFrame(vtkPolyData *frame,
                                          vtkMatrix4x4 *transform)
{
  assert("Input frame can't be null!" && frame);
  assert("Input transform can't be null!" && transform);

  this->frameTransform->DeepCopy(transform);
  transform->Delete();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput(frame);

  vtkSmartPointer<vtkActor> actor = this->Frame;
  actor->SetMapper(mapper);
  actor->SetPosition(0,0,0);
  actor->GetProperty()->SetColor(0.9,0.0,0.8);
  actor->SetVisibility(1);
  this->planar->GetRenderer()->AddActor(actor);

  this->viewRect->Start();
}

void cbElectrodeView::buildProbeMarker(vtkPolyData *probeData, cbProbe p)
{
  assert("Input parameter can't be null!" && probeData);

  cbProbeSpecification spec = p.specification();

  std::vector<double> point_list = spec.points();

  /* if specification is missing, make a fully-highlighted probe */
  if (point_list.size() < 2) {
    point_list.clear();
    point_list.push_back(0.0);
    point_list.push_back(250.0);
  }

  double table[2][4] = {
    {0.0, 0.0, 0.0, 1.0},
    {1.0, 1.0, 1.0, 1.0},
  };

  if (!spec.tip_is_contact()) {
    table[0][0] = 1.0;
    table[0][1] = 1.0;
    table[0][2] = 1.0;
    table[1][0] = 0.0;
    table[1][1] = 0.0;
    table[1][2] = 0.0;
  }

  vtkSmartPointer<vtkPoints> spoints = vtkSmartPointer<vtkPoints>::New();
  spoints->SetNumberOfPoints(point_list.size());
  for (size_t i = 0; i < point_list.size(); i++) {
    spoints->InsertPoint(i, -1.0*point_list[i], 0.0, 0.0);
  }

  vtkSmartPointer<vtkCellArray> electrodeShaftPoints =
    vtkSmartPointer<vtkCellArray>::New();

  vtkSmartPointer<vtkIntArray> colors =
    vtkSmartPointer<vtkIntArray>::New();
  for (size_t i = 0; i < point_list.size() - 1; i++) {
    electrodeShaftPoints->InsertNextCell(2);
    electrodeShaftPoints->InsertCellPoint(i);
    electrodeShaftPoints->InsertCellPoint(i + 1);
    colors->InsertNextValue(table[i%2][0]);
  }

  vtkSmartPointer<vtkPolyData> electrodeShaftProfile =
    vtkSmartPointer<vtkPolyData>::New();
  electrodeShaftProfile->SetPoints(spoints);
  electrodeShaftProfile->SetLines(electrodeShaftPoints);
  electrodeShaftProfile->GetCellData()->SetScalars(colors);

  vtkSmartPointer<vtkTubeFilter> electrodeShaft =
    vtkSmartPointer<vtkTubeFilter>::New();
  electrodeShaft->SetInput(electrodeShaftProfile);
  electrodeShaft->SetNumberOfSides(26);
  electrodeShaft->SetRadius(0.5);
  electrodeShaft->SetCapping(1);
  electrodeShaft->Update();

  probeData->DeepCopy(electrodeShaft->GetOutput());
}

void cbElectrodeView::displaySurfaceVolume(vtkDataManager::UniqueKey k)
{
  this->initializeProgress(0,1);

  vtkSmartPointer<vtkImageNode> node = vtkSmartPointer<vtkImageNode>::New();
  node = this->dataManager->FindImageNode(k);

  assert("node should not be NULL!" && node);

  vtkSmartPointer<vtkImageData> data = vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  data = node->GetImage();
  matrix = this->frameTransform;

  assert("data should not be NULL!" && data);
  assert("matrix should not be NULL!" && matrix);

  vtkSmartPointer<vtkColorTransferFunction> color =
    vtkSmartPointer<vtkColorTransferFunction>::New();

  vtkSmartPointer<vtkPiecewiseFunction> opacity =
    vtkSmartPointer<vtkPiecewiseFunction>::New();

  double range[2];
  cbElectrodeView::ComputePercentileRange(data, 98.0, range);

  static double table[][5] = {
    { 0.00, 0.0, 0.0, 0.0, 0.0 },
    { 0.07, 0.4, 0.0, 0.1, 0.0 },
    { 0.48, 1.0, 0.7, 0.6, 0.2 },
    { 1.00, 1.0, 1.0, 0.9, 0.8 },
  };

  for (int i = 0; i < 4; i++) {
    double x = table[i][0]*(range[1] - range[0]) + range[0];
    color->AddRGBPoint(x, table[i][1], table[i][2], table[i][3]);
    opacity->AddPoint(x, table[i][4]);
  }

  vtkSmartPointer<vtkVolumeProperty> volumeProperty =
    vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(color);
  volumeProperty->SetScalarOpacity(opacity);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOff();

  vtkSmartPointer<vtkSmartVolumeMapper> mapper =
    vtkSmartPointer<vtkSmartVolumeMapper>::New();
  mapper->SetInput(data);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  volume->SetUserMatrix(matrix);

  this->addRendererLabel(surface, "Surface Volume View", 0);
  this->surface->AddViewProp(volume);
  this->surface->InteractiveOn();
  this->surface->SetAllocatedRenderTime(0.2);

  this->viewRect->GetRenderWindow()->Render();

  this->displayProgress(1);
}

void cbElectrodeView::addRendererLabel(vtkRenderer *r, const char *str,
                                       int corner)
{
  vtkSmartPointer<vtkCornerAnnotation> annotation =
    vtkSmartPointer<vtkCornerAnnotation>::New();
  annotation->SetText(corner, str);
  annotation->SetMaximumFontSize(12);
  annotation->SetMinimumFontSize(10);
  r->AddActor2D(annotation);
}

void cbElectrodeView::addDataToPanes(vtkImageData *d, vtkMatrix4x4 *m,
                                     vtkImageProperty *p)
{
  this->axialPane->AddImage(d, m, p);
  this->sagittalPane->AddImage(d, m, p);
  this->coronalPane->AddImage(d, m, p);
}

// --- Invoked by the Fiducial Tool when a click is performed
void cbElectrodeView::ProbePlacementCallback(vtkObject *o, unsigned long, void *)
{
  vtkFiducialPointsTool *tool = vtkFiducialPointsTool::SafeDownCast(o);
  if (!tool) {
    return;
  }

  // Get the world position from the toolcursor
  double position[3];
  tool->GetToolCursor()->GetPosition(position);

  this->qvtkWidget->SetFocusCursorShape(this->lastCursor);
  this->bindToolCursorAction(this->lastTool, Qt::LeftButton);

  // Tell the stage that a new probe was requested at x/y/z
  emit CreateProbeRequest(position[0], position[1], position[2],
                          90, 90, 0, "", "");
}

void cbElectrodeView::CreateProbeCallback(cbProbe p)
{
  double position[3]; double orientation[2];
  p.GetPosition(position);
  p.GetOrientation(orientation);

  double depth = p.GetDepth();

  // Create the poly data
  vtkSmartPointer<vtkPolyData> data =
    vtkSmartPointer<vtkPolyData>::New();
  this->buildProbeMarker(data, p);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput(data);

  static double table[2][4] = {
    {56.0/256.0, 117.0/256.0, 215.0/256.0, 1.0},
    {1.0, 1.0, 1.0, 1.0},
  };

  vtkSmartPointer<vtkLookupTable> active =
    vtkSmartPointer<vtkLookupTable>::New();

  active->SetTableRange(0, 1);
  active->SetNanColor(1.0, 0.0, 0.0, 1.0);
  active->SetNumberOfTableValues(2);
  active->Build();
  for (int i = 0; i < 2; i++) {
    active->SetTableValue(i, table[i]);
  }

  mapper->SetLookupTable(active);
  mapper->SetScalarModeToUseCellData();
  mapper->SetScalarVisibility(1);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Build the transform necessary to build the user matrix for the probes
  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transform->PostMultiply();

  transform->Translate(-1.0*depth, 0.0, 0.0);

  // azimuth = Left-to-Right angle (rotate about -Z axis)
  // declination = Posterior-to-Anterior angle (rotate about -X axis)
  transform->RotateWXYZ(orientation[0],  0.0, 0.0, -1.0);
  transform->RotateWXYZ(orientation[1], -1.0, 0.0,  0.0);
  transform->Translate(position);

  vtkSmartPointer<vtkMatrix4x4> matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->DeepCopy(transform->GetMatrix());

  // Set the position and orientation for the probe
  actor->SetUserMatrix(matrix);

  // Add actor to collection
  this->Probes->AddItem(actor);

  // Add actor to the renderer
  this->planar->GetRenderer()->AddActor(actor);
  this->surface->AddActor(actor);

  this->viewRect->Start();

  this->SetSavedState(false);
}

void cbElectrodeView::UpdateProbeCallback(int index, cbProbe p)
{
  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  this->buildProbeMarker(data, p);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

  mapper->SetInput(data);
  actor->SetMapper(mapper);

  vtkObject *o = this->Probes->GetItemAsObject(index);
  vtkActor *a = vtkActor::SafeDownCast(o);

  this->planar->GetRenderer()->RemoveActor(a);
  this->surface->RemoveActor(a);

  this->Probes->ReplaceItem(index, actor);

  a = actor;

  a->GetProperty()->SetAmbient(0.5);
  a->GetProperty()->SetDiffuse(0.2);

  this->planar->GetRenderer()->AddActor(a);
  this->surface->AddActor(a);

  if (!a) {
    std::cout << "Could not find at " << index << std::endl;
  }

  // Get the actor collections
  vtkActorCollection *planar_c = this->planar->GetRenderer()->GetActors();
  vtkActorCollection *surface_c = this->surface->GetActors();

  // Make sure the actor is present in the collections
  if (!planar_c->IsItemPresent(a)) {
    std::cout << "Actor not found in planar!" << std::endl;
  }
  if (!surface_c->IsItemPresent(a)) {
    std::cout << "Actor not found in surface!" << std::endl;
  }

  // Cache the index for later use
  this->SelectedIndex = index;

  // Update the probe's position
  double position[3];
  p.GetPosition(position);

  // Update the probe's orientation
  double orientation[2];
  p.GetOrientation(orientation);

  // Update the probe's depth
  double depth = p.GetDepth();

  // Build the transform necessary to build the user matrix for the probes
  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transform->PostMultiply();

  transform->Translate(-1.0*depth, 0.0, 0.0);

  // azimuth = Left-to-Right angle (rotate about -Z axis)
  // declination = Posterior-to-Anterior angle (rotate about -X axis)
  transform->RotateWXYZ(orientation[0],  0.0, 0.0, -1.0);
  transform->RotateWXYZ(orientation[1], -1.0, 0.0,  0.0);
  transform->Translate(position);

  vtkSmartPointer<vtkMatrix4x4> matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->DeepCopy(transform->GetMatrix());

  // Set the position and orientation for the probe
  a->SetUserMatrix(matrix);

  static double active_table[2][4] = {
    {56.0/256.0, 117.0/256.0, 215.0/256.0, 1.0},
    {1.0, 1.0, 1.0, 1.0},
  };

  vtkSmartPointer<vtkLookupTable> active =
    vtkSmartPointer<vtkLookupTable>::New();
  active->SetTableRange(0, 1);
  active->SetNanColor(1.0, 0.0, 0.0, 1.0);
  active->SetNumberOfTableValues(2);
  active->Build();
  active->SetTableValue(0, active_table[0]);
  active->SetTableValue(1, active_table[1]);

  a->GetMapper()->SetLookupTable(active);

  static double passive_table[2][4] = {
    {0.0, 0.0, 0.0, 1.0},
    {1.0, 1.0, 1.0, 1.0},
  };

  vtkSmartPointer<vtkLookupTable> passive =
    vtkSmartPointer<vtkLookupTable>::New();
  passive->SetTableRange(0, 1);
  passive->SetNanColor(1.0, 0.0, 0.0, 1.0);
  passive->SetNumberOfTableValues(2);
  passive->Build();
  passive->SetTableValue(0, passive_table[0]);
  passive->SetTableValue(1, passive_table[1]);

  vtkCollectionSimpleIterator iter;
  this->Probes->InitTraversal(iter);

  vtkActor *act = NULL;
  while ((act = this->Probes->GetNextActor(iter))) {
    if (act != a) {
      act->GetMapper()->SetLookupTable(passive);
    }
  }

  this->viewRect->Start();

  this->SetSavedState(false);
}

void cbElectrodeView::DestroyProbeCallback(int index)
{
  // Destroy probe at 'index', removing it from the renderer
  vtkObject *o = this->Probes->GetItemAsObject(index);
  vtkActor *a = vtkActor::SafeDownCast(o);

  if (!a) {
    std::cout << "Could not find at " << index << std::endl;
  }

  vtkActorCollection *planar_c = this->planar->GetRenderer()->GetActors();
  vtkActorCollection *surface_c = this->surface->GetActors();

  if (!planar_c->IsItemPresent(a)) {
    std::cout << "Actor not found in planar!" << std::endl;
  }
  if (!surface_c->IsItemPresent(a)) {
    std::cout << "Actor not found in surface!" << std::endl;
  }

  this->planar->GetRenderer()->RemoveActor(a);
  this->surface->RemoveActor(a);

  this->Probes->RemoveItem(a);

  this->viewRect->Start();

  this->SetSavedState(false);
}

void cbElectrodeView::InitiatePlaceProbeCallback()
{
  this->lastTool = this->currentTool;
  this->lastCursor = this->qvtkWidget->GetFocusCursorShape();

  QDir dir(QApplication::applicationDirPath());

#if defined(Q_OS_WIN)
  if (dir.dirName().toLower() == "debug"
      ||dir.dirName().toLower() == "release"
      ||dir.dirName().toLower() == "bin")
    {
    dir.cdUp();
    }
#elif defined(Q_OS_MAC)
  if (dir.dirName() == "MacOS") {
    dir.cdUp();
  }
#endif
  dir.cd("Icons");

  QString cursor_path = QFile((dir).absoluteFilePath("Cursors")).fileName();
  QPixmap pix(cursor_path + "/pick.png");
  QCursor cur(pix);

  this->setActiveToolCursor(cur);
  this->bindToolCursorAction(cbElectrodeView::Pick, Qt::LeftButton);
  this->currentTool = cbElectrodeView::Pick;
}

void cbElectrodeView::EnableFrameVisualization()
{
  this->Frame->SetVisibility(1);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::DisableFrameVisualization()
{
  this->Frame->SetVisibility(0);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::ToggleSagittalVisualization(int s)
{
  this->Slices[0].Stack->SetVisibility(s);
  this->Slices[0].Stack->SetPickable(s);
  this->Slices[0].Stack->SetDragable(s);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::ToggleCoronalVisualization(int s)
{
  this->Slices[1].Stack->SetVisibility(s);
  this->Slices[1].Stack->SetPickable(s);
  this->Slices[1].Stack->SetDragable(s);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::ToggleAxialVisualization(int s)
{
  this->Slices[2].Stack->SetVisibility(s);
  this->Slices[2].Stack->SetPickable(s);
  this->Slices[2].Stack->SetDragable(s);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::ToggleProbeVisualizationMode(int s)
{
  if (s) {
    vtkCollectionSimpleIterator iter;
    this->Probes->InitTraversal(iter);

    vtkActor *act = NULL;
    while ((act = this->Probes->GetNextActor(iter))) {
      act->SetVisibility(1);
    }
  } else {
    vtkObject *o = this->Probes->GetItemAsObject(this->SelectedIndex);
    vtkActor *a = vtkActor::SafeDownCast(o);

    if (!a) {
      std::cout << "Could not find at " << index << std::endl;
    }

    vtkCollectionSimpleIterator iter;
    this->Probes->InitTraversal(iter);

    vtkActor *act = NULL;
    while ((act = this->Probes->GetNextActor(iter))) {
      if (act != a) {
        act->SetVisibility(0);
      }
    }
  }
  this->viewRect->Start();
}

void cbElectrodeView::ExportScreenshot()
{
  // Create save dialog to get save location and filename
  QString path = this->GetPlanFolder();
  QString fileName = QFileDialog::getSaveFileName(NULL, "Export Screenshot",
                                                  path + "/untitled.png",
                                                  "Image Files (*.png)");

  if (fileName.isNull()) {
    return;
  }

  QFileInfo fileInfo(fileName);
  this->SetPlanFolder(fileInfo.path());

  vtkRenderWindow *window = this->viewRect->GetRenderWindow();
  int *winsize = window->GetSize();

  unsigned char *pixels =
    window->GetPixelData(0, 0, winsize[0] - 1, winsize[1] - 1, 1);

  vtkSmartPointer<vtkImageImport> importer =
    vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataScalarTypeToUnsignedChar();
  importer->SetNumberOfScalarComponents(3);
  importer->SetDataExtent(0, winsize[0] - 1, 0, winsize[1] - 1, 0, 0);
  importer->SetWholeExtent(0, winsize[0] - 1, 0, winsize[1] - 1, 0, 0);
  importer->SetImportVoidPointer(pixels, 0);
  importer->Update();

  vtkSmartPointer<vtkPNGWriter> writer =
    vtkSmartPointer<vtkPNGWriter>::New();
  writer->SetFileName(fileName.toStdString().c_str());
  writer->SetInputConnection(importer->GetOutputPort());
  writer->Write();

  if (writer->GetErrorCode()) {
    cbMainWindow::displayNoticeMessage(
      QString("Unable to save file.  Check directory permissions."));
  }
}

void cbElectrodeView::ToggleHelpAnnotations(int s)
{
  this->helpAnnotation->SetVisibility(s);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::Open()
{
  // If the application is "dirty", ask what to do
  if (!this->SavedState) {
    int ret = this->PromptForSave();
    switch (ret) {
      case QMessageBox::Save:
        // If the user wants to save, allow them to save, and cancel open.
        this->Save();
        cbElectrodeView::displayNoticeMessage(QString("File saved."));
        return;
        break;
      case QMessageBox::Discard:
        // If the user wants to discard changes, continue with opening
        break;
      case QMessageBox::Cancel:
        // If the user wants to cancel, end the opening process
        return;
        break;
      default:
        break;
    }
  }

  // Get the file path the user wants to save to
  QString path = this->GetPlanFolder();
  QString fileName = QFileDialog::getOpenFileName(NULL, "Open Plan", path,
                                                  "Plan Files (*.pln)");

  if (fileName.isNull()) {
    return;
  }

  QFileInfo fileInfo(fileName);
  this->SetPlanFolder(fileInfo.path());
  this->SaveFile = fileName.toStdString();

  // Clear the current plan for a fresh state.
  emit ClearCurrentPlan();

  std::ifstream in(fileName.toStdString().c_str());

  // Get the image path from the save file
  std::string image_path, ct_path;
  std::getline(in, image_path);
  std::getline(in, ct_path);

  // Open the image path from the save file
  emit OpenImage(image_path);

  if (ct_path.empty()) {
    std::cout << "empty ct, don't do anything" << std::endl;
  } else {
    std::string line;
    std::getline(in, line);
    std::istringstream iss(line);

    double matrix[16];
    for (int i = 0; i < 16; i++) {
      iss >> matrix[i];
      std::cout << matrix[i] << std::endl;
    }

    vtkMatrix4x4 *matrix_obj = vtkMatrix4x4::New();
    matrix_obj->DeepCopy(matrix);

    emit OpenCTData(ct_path, matrix_obj);
  }

  // Open the probes
  std::string line;
  while (std::getline(in, line)) {
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

  this->SetSavedState(true);
}

void cbElectrodeView::Save()
{
  if (this->SaveFile.empty()) {
    this->SaveAs();
    return;
  }

  std::ofstream file;
  file.open(this->SaveFile.c_str());

  std::string image_path, ct_path;
  vtkImageNode *mr_node = this->dataManager->FindImageNode(this->dataKey);
  vtkImageNode *ct_node = this->dataManager->FindImageNode(this->ctKey);

  if (mr_node) {
    image_path = mr_node->GetFileURL();
  }

  vtkMatrix4x4 *ct_matrix = NULL;
  if (ct_node) {
    ct_path = ct_node->GetFileURL();

    ct_matrix = ct_node->GetMatrix();
  }

  file << image_path << std::endl << ct_path << std::endl;

  // if the CT exists, save it's registration matrix
  if (!ct_path.empty()) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        file << ct_matrix->GetElement(i, j) << " ";
      }
    }
    file << std::endl;
  }

  file.close();

  emit SavePlanToFile(this->SaveFile);

  this->SetSavedState(true);
}

void cbElectrodeView::SaveAs()
{
  QString path;
  if (this->SaveFile != "") {
    path = this->SaveFile.c_str();
  } else {
    path = this->GetPlanFolder();
  }

  QString fileName = QFileDialog::getSaveFileName(NULL, "Save Plan", path,
                                                  "Plan Files (*.pln)");
  if (fileName.isNull()) {
    this->SetSavedState(false);
    return;
  }

  QFileInfo fileInfo(fileName);
  this->SetPlanFolder(fileInfo.path());
  this->SaveFile = fileName.toStdString();
  this->Save();
}

void cbElectrodeView::SetSavedState(bool s)
{
  this->SavedState = s;
  if (s) {
    this->setWindowTitle(this->SaveFile.c_str());
  } else {
    std::string unsaved = this->SaveFile;
    unsaved.append("*");
    this->setWindowTitle(unsaved.c_str());
  }
}

QString cbElectrodeView::GetPlanFolder()
{
  const char *folderKey = "planFileFolder";
  QSettings settings;
  QString path;

  if (settings.value(folderKey).toString() != NULL) {
    path = settings.value(folderKey).toString();
  }

  if (path == "") {
    path = QDir::homePath();
  }

  return path;
}

void cbElectrodeView::SetPlanFolder(const QString& path)
{
  const char *folderKey = "planFileFolder";

  QSettings settings;
  settings.setValue(folderKey, path);
}

void cbElectrodeView::CreateMenu()
{
  QMenu *fileMenu = new QMenu(tr("&File"), this);
  QMenu *aboutMenu = new QMenu(tr("&About"), this);
  QMenu *windowMenu = new QMenu(tr("&Window"), this);
  this->menuBar()->addMenu(fileMenu);
  this->menuBar()->addMenu(aboutMenu);
  this->menuBar()->addMenu(windowMenu);

  QAction *openAction = fileMenu->addAction(tr("&Open Plan..."));
  QAction *saveAction = fileMenu->addAction(tr("&Save Plan"));
  QAction *saveAsAction = fileMenu->addAction(tr("Save Plan &As..."));
  QAction *openCTAction = fileMenu->addAction(tr("Open Se&condary Series..."));

  QAction *aboutAction = aboutMenu->addAction(tr("&About"));

  QAction *minimizeAction = windowMenu->addAction(tr("Mi&nimize Window"));
  QAction *maximizeAction = windowMenu->addAction(tr("Ma&ximize Window"));
  QAction *fullscreenAction = windowMenu->addAction(tr("&Fullscreen Window"));

  openAction->setShortcuts(QKeySequence::Open);
  saveAction->setShortcuts(QKeySequence::Save);
  saveAsAction->setShortcuts(QKeySequence::SaveAs);

  connect(openAction, SIGNAL(triggered()), this, SLOT(Open()));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(Save()));
  connect(saveAsAction, SIGNAL(triggered()), this, SLOT(SaveAs()));
  connect(openCTAction, SIGNAL(triggered()), this, SLOT(OpenCT()));

  connect(aboutAction, SIGNAL(triggered()), this, SLOT(About()));

  connect(minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));
  connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));
  connect(fullscreenAction, SIGNAL(triggered()), this, SLOT(showFullScreen()));
}

void cbElectrodeView::CreateAndBindTools()
{
  this->planeTool = vtkPushPlaneTool::New();
  this->planeTool->SetMaximumRotationDegree(60);
  this->planeTool->AddObserver(vtkCommand::InteractionEvent,
                               this, &cbElectrodeView::FocalPointSliceCallback);

  this->rotateTool = vtkRotateCameraTool::New();
  this->rotateTool->AddObserver(vtkCommand::InteractionEvent,
                                this, &cbElectrodeView::RotateVolumeCallback);

  this->pickTool = vtkFiducialPointsTool::New();
  this->pickTool->AddObserver(vtkCommand::InteractionEvent,
                              this, &cbElectrodeView::ProbePlacementCallback);

  this->scrollTool = vtkSliceImageTool::New();
  this->scrollTool->AddObserver(vtkCommand::InteractionEvent,
                                this, &cbElectrodeView::PaneScrollCallback);

  this->lastCursor = this->qvtkWidget->GetFocusCursorShape();
  this->lastTool = cbElectrodeView::Plane;
  this->currentTool = cbElectrodeView::Plane;

  vtkToolCursor *planar_cursor = this->planar->GetToolCursor();
  planar_cursor->GuideVisibilityOn();
  int planar_bind = planar_cursor->AddAction(vtkZoomCameraTool::New());
  planar_cursor->BindAction(planar_bind, 0, 0, VTK_TOOL_WHEEL_FWD);
  planar_cursor->BindAction(planar_bind, 0, 0, VTK_TOOL_WHEEL_BWD);

  vtkToolCursor *axial_cursor = this->axialPane->GetToolCursor();
  int axial_bind = axial_cursor->AddAction(this->scrollTool);
  axial_cursor->BindAction(axial_bind, 0, 0, VTK_TOOL_WHEEL_FWD);
  axial_cursor->BindAction(axial_bind, 0, 0, VTK_TOOL_WHEEL_BWD);

  vtkToolCursor *coronal_cursor = this->coronalPane->GetToolCursor();
  int coronal_bind = coronal_cursor->AddAction(this->scrollTool);
  coronal_cursor->BindAction(coronal_bind, 0, 0, VTK_TOOL_WHEEL_FWD);
  coronal_cursor->BindAction(coronal_bind, 0, 0, VTK_TOOL_WHEEL_BWD);

  vtkToolCursor *sagittal_cursor = this->sagittalPane->GetToolCursor();
  int sagittal_bind = sagittal_cursor->AddAction(this->scrollTool);
  sagittal_cursor->BindAction(sagittal_bind, 0, 0, VTK_TOOL_WHEEL_FWD);
  sagittal_cursor->BindAction(sagittal_bind, 0, 0, VTK_TOOL_WHEEL_BWD);

  this->planar->SetCursorTracking(true);
}

void cbElectrodeView::CreateAndConfigurePanes()
{
  this->outerFrame = vtkDynamicViewFrame::New();
    this->planar = vtkImageViewPane::New();
    this->surface = vtkRenderer::New();
    vtkDynamicViewFrame *orthoFrame = vtkDynamicViewFrame::New();
      this->axialPane = vtkImageViewPane::New();
      this->coronalPane = vtkImageViewPane::New();
      this->sagittalPane = vtkImageViewPane::New();

  this->planar->SetStretchFactor(2.3);
  this->planar->SetBorderEnabled(false);
  this->axialPane->SetBorderEnabled(true);
  this->coronalPane->SetBorderEnabled(true);
  this->sagittalPane->SetBorderEnabled(true);

  this->outerFrame->SetOrientation(vtkDynamicViewFrame::HORIZONTAL);
  this->outerFrame->SetViewRect(this->viewRect);
  this->outerFrame->AddChild(this->planar);
  this->outerFrame->AddChild(orthoFrame);

  orthoFrame->SetOrientation(vtkDynamicViewFrame::VERTICAL);
  orthoFrame->AddChild(this->axialPane);
  orthoFrame->AddChild(this->coronalPane);
  orthoFrame->AddChild(this->sagittalPane);

  this->viewRect->SetMainFrame(this->outerFrame);

  this->viewRect->Start();

  double port[4]; // xmin, ymin, xmax, ymax
  this->planar->GetRenderer()->GetViewport(port);

  for (int i = 0; i < 4; i++) {
    this->planarPort[i] = port[i];
  }

  port[2] = fabs((port[2] - port[0])/4.0);
  port[3] = fabs((port[3] - port[1])/4.0);

  for (int i = 0; i < 4; i++) {
    this->surfacePort[i] = port[i];
  }
  this->surface->SetViewport(port);

  this->viewRect->GetRenderWindow()->AddRenderer(this->surface);
  this->viewRect->Start();
}

void cbElectrodeView::CreateFrameObjects()
{
  this->Frame = vtkActor::New();
  this->frameTransform = vtkMatrix4x4::New();
}

void cbElectrodeView::CreatePlanVisualization()
{
  this->Probes = vtkActorCollection::New();
}

void cbElectrodeView::CreateLabelsAndAnnotations()
{
  this->helpAnnotation = vtkCornerAnnotation::New();
  this->leftAxialFollower = vtkFollower::New();
  this->leftCoronalFollower = vtkFollower::New();
  this->rightAxialFollower = vtkFollower::New();
  this->rightCoronalFollower = vtkFollower::New();
  this->MetaAnnotation = vtkCornerAnnotation::New();
}

void cbElectrodeView::DestroyMembers()
{
  this->helpAnnotation->Delete();
  this->leftAxialFollower->Delete();
  this->leftCoronalFollower->Delete();
  this->rightAxialFollower->Delete();
  this->rightCoronalFollower->Delete();

  this->MetaAnnotation->Delete();

  this->Probes->Delete();

  this->Frame->Delete();
  this->frameTransform->Delete();

  this->outerFrame->Delete();
    this->planar->Delete();
    this->surface->Delete();
      this->axialPane->Delete();
      this->coronalPane->Delete();
      this->sagittalPane->Delete();

  this->planeTool->Delete();
  this->rotateTool->Delete();
  this->pickTool->Delete();

  this->ImageProperty->Delete();
  this->CTProperty->Delete();
}

void cbElectrodeView::ClearCurrentWorkSpace()
{
  //this->DestroyMembers();

  this->CreateAndConfigurePanes();
  this->CreateAndBindTools();
  this->CreateFrameObjects();
  this->CreatePlanVisualization();
  this->CreateLabelsAndAnnotations();

  this->viewRect->Start();
}

void cbElectrodeView::closeEvent(QCloseEvent *e)
{
  if (!this->SavedState) {
    int ret = this->PromptForSave();
    switch (ret) {
      // If the user saved or discarded, close
      case QMessageBox::Save:
        this->Save();
        e->accept();
        break;
      case QMessageBox::Discard:
        e->accept();
        break;
      // If the user cancelled, do not close
      case QMessageBox::Cancel:
        e->ignore();
        break;
      default:
        break;
    }
  }
}

int cbElectrodeView::PromptForSave()
{
  QMessageBox box;
  box.setText("The plan has been modified.");
  box.setInformativeText("Do you want to save your changes?");
  box.setStandardButtons(QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
  box.setDefaultButton(QMessageBox::Save);
  return box.exec();
}

void cbElectrodeView::PaneScrollCallback(vtkObject *o, unsigned long, void *)
{
  vtkSliceImageTool *tool = vtkSliceImageTool::SafeDownCast(o);
  if (!tool) {
    return;
  }

  double focal_point[3];
  double offset = 120.0;

  // Sagittal pane
  vtkRenderer *sagittal_renderer = this->sagittalPane->GetRenderer();
  vtkCamera *sagittal_camera = sagittal_renderer->GetActiveCamera();
  this->FixCameraPosition(sagittal_camera);
  sagittal_camera->GetFocalPoint(focal_point);

  vtkPlane *sagittal_worldplane = this->Slices[0].WorldPlane;
  sagittal_worldplane->SetOrigin(focal_point);

  // Coronal pane
  vtkRenderer *coronal_renderer = this->coronalPane->GetRenderer();
  vtkCamera *coronal_camera = coronal_renderer->GetActiveCamera();
  this->FixCameraPosition(coronal_camera);
  coronal_camera->GetFocalPoint(focal_point);

  vtkPlane *coronal_worldplane = this->Slices[1].WorldPlane;
  coronal_worldplane->SetOrigin(focal_point);

  // Axial pane
  vtkRenderer *axial_renderer = this->axialPane->GetRenderer();
  vtkCamera *axial_camera = axial_renderer->GetActiveCamera();
  this->FixCameraPosition(axial_camera);
  axial_camera->GetFocalPoint(focal_point);

  vtkPlane *axial_worldplane = this->Slices[2].WorldPlane;
  axial_worldplane->SetOrigin(focal_point);

  this->PositionLRLabelsIn2DPanes();
}

void cbElectrodeView::OpenCT()
{
  QString path;

  vtkImageNode *node = this->dataManager->FindImageNode(this->dataKey);
  if (node) {
    QFileInfo fileInfo(node->GetFileURL().c_str());
    QFileInfo pathInfo(fileInfo.path());
    path = pathInfo.path();
    }

  if (path == "")
    {
    path = this->GetPlanFolder();
    }

  QString file_path = QFileDialog::getOpenFileName(NULL, "Open Secondary Series",
                                                   path);

  if (file_path.isNull()) {
    return;
  }

  emit OpenCTData(file_path.toStdString());
}

void cbElectrodeView::DisplayCTData(vtkDataManager::UniqueKey k)
{
  this->ctKey = k;

  vtkImageNode *secondary_node = this->dataManager->FindImageNode(this->ctKey);
  vtkImageData *ct_data = secondary_node->GetImage();
  vtkMatrix4x4 *matrix = secondary_node->GetMatrix();

  vtkDICOMMetaData *secondary_meta_data = secondary_node->GetMetaData();
  const vtkDICOMValue patient_name_val =
    secondary_meta_data->GetAttributeValue(DC::PatientName);
  std::string patient_name = patient_name_val.AsString();
  if (patient_name.empty()) {
    patient_name = "unnamed";
  }

  std::string secondary_patient_label = "Secondary Patient: ";
  secondary_patient_label.append(patient_name);
  this->AppendMetaData(secondary_patient_label);

  this->CTProperty->DeepCopy(this->ImageProperty);
  this->CTProperty->SetOpacity(0.30);

  // Set window/level to display up to the 99th percentile of image
  // pixels (i.e. the brightest 1% of the pixels will saturate).
  // This ensures that a few abnormally bright pixels will not
  // cause the Window/Level to be miscalculated.
  // For CT, also set the lower range at half of the full range,
  // so that the brain case is transparent.
  double range[2];
  cbElectrodeView::ComputePercentileRange(ct_data, 99.0, range);
  range[0] += 0.5*(range[1] - range[0]);
  this->CTProperty->SetColorWindow(range[1] - range[0]);
  this->CTProperty->SetColorLevel(0.5*(range[1] + range[0]));
  this->CTProperty->SetInterpolationTypeToCubic();

  if (!ct_data || !matrix) {
    std::cout << "Error: Could not display CT data." << std::endl;
    return;
  }

  // Create the slice-view planes of the CT
  for (int i = 0; i < 3; i++) {
    vtkSmartPointer<vtkImageResliceMapper> mapper =
      vtkSmartPointer<vtkImageResliceMapper>::New();
    vtkSmartPointer<vtkImageSlice> slice =
      vtkSmartPointer<vtkImageSlice>::New();

    mapper->SetInput(ct_data);
    mapper->SetSlicePlane(this->Slices[i].WorldPlane);

    slice->SetMapper(mapper);
    slice->SetProperty(this->CTProperty);
    slice->SetUserMatrix(this->frameTransform);

    vtkImageStack *stack = this->Slices[i].Stack;
    stack->AddImage(slice);

    stack->SetActiveLayer(cbElectrodeView::kMR);
  }

  // Add the CT to the side panes
  this->addDataToPanes(ct_data, this->frameTransform, this->CTProperty);

  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::About()
{
  QString title;
  QString text;

  title = "About Tactics";
  text = "Developed at the Calgary Image Processing and Analysis Centre (CIPAC), 2013.";
  text.append("\n\n");
  text.append(
"DISCLAIMER: TACTICS ('THE SOFTWARE') IS PROVIDED AS IS. USE THE SOFTWARE AT YOUR OWN RISK. THE AUTHORS MAKE NO WARRANTIES AS TO PERFORMANCE OR FITNESS FOR A PARTICULAR PURPOSE, OR ANY OTHER WARRANTIES WHETHER EXPRESSED OR IMPLIED. NO ORAL OR WRITTEN COMMUNICATION FROM OR INFORMATION PROVIDED BY THE AUTHORS SHALL CREATE A WARRANTY. UNDER NO CIRCUMSTANCES SHALL THE AUTHORS BE LIABLE FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES RESULTING FROM THE USE, MISUSE, OR INABILITY TO USE THE SOFTWARE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. THESE EXCLUSIONS AND LIMITATIONS MAY NOT APPLY IN ALL JURISDICTIONS. YOU MAY HAVE ADDITIONAL RIGHTS AND SOME OF THESE LIMITATIONS MAY NOT APPLY TO YOU.\
\n\n\
THIS SOFTWARE IS NOT INTENDED FOR PRIMARY DIAGNOSTIC, ONLY FOR SCIENTIFIC USAGE.\
\n\n\
THIS VERSION OF TACTICS IS NOT CERTIFIED AS A MEDICAL DEVICE FOR PRIMARY DIAGNOSIS. THERE ARE NO CERTIFICATIONS. YOU CAN ONLY USE TACTICS AS A REVIEWING AND SCIENTIFIC SOFTWARE, NOT FOR PRIMARY DIAGNOSTIC.");
  QMessageBox::about(this, title, text);
}

void cbElectrodeView::SetCTOpacity(double o)
{
  assert((o <= 1.0 && o >= 0.0) && "Opacity must be between 0.0 and 1.0");
  this->CTProperty->SetOpacity(o);
  this->viewRect->GetRenderWindow()->Render();
}

void cbElectrodeView::ComputePercentileRange(
  vtkImageData *data, double percentile, double range[2])
{
  vtkSmartPointer<vtkImageHistogramStatistics> statistics =
    vtkSmartPointer<vtkImageHistogramStatistics>::New();

  statistics->SetInput(data);
  statistics->SetAutoRangePercentiles(0.0, percentile);
  statistics->SetAutoRangeExpansionFactors(0.0, 0.1);
  statistics->Update();
  statistics->GetAutoRange(range);

  statistics->SetInput(0);
}

void cbElectrodeView::SnapViewVectorsToVolume(vtkMatrix4x4 *matrix,
  const double originalViewRight[3], const double originalViewUp[3],
  double fixedViewRight[3], double fixedViewUp[3])
{
  // add a fourth component to the vectors
  double rightVector[4], upVector[4];

  rightVector[0] = originalViewRight[0];
  rightVector[1] = originalViewRight[1];
  rightVector[2] = originalViewRight[2];
  rightVector[3] = 0.0;

  upVector[0] = originalViewUp[0];
  upVector[1] = originalViewUp[1];
  upVector[2] = originalViewUp[2];
  upVector[3] = 0.0;

  // transform vectors into the data coordinate system
  double inverseMatrix[16];
  vtkMatrix4x4::DeepCopy(inverseMatrix, matrix);
  vtkMatrix4x4::Invert(inverseMatrix, inverseMatrix);
  vtkMatrix4x4::MultiplyPoint(inverseMatrix, rightVector, rightVector);
  vtkMatrix4x4::MultiplyPoint(inverseMatrix, upVector, upVector);

  int maxi = 0;
  double maxv = 0.0;
  for (int i = 0; i < 3; i++) {
    double v = fabs(rightVector[i]);
    if (v > maxv) {
      maxi = i;
      maxv = v;
    }
  }

  maxv = 0.0;
  int maxj = 0;
  for (int j = 0; j < 3; j++) {
    if (j == maxi) {
      continue;
    }
    double v = fabs(upVector[j]);
    if (v > maxv) {
      maxj = j;
      maxv = v;
    }
  }

  rightVector[(maxi + 1)%3] = 0.0;
  rightVector[(maxi + 2)%3] = 0.0;

  upVector[(maxj + 1)%3] = 0.0;
  upVector[(maxj + 2)%3] = 0.0;

  matrix->MultiplyPoint(rightVector, rightVector);
  matrix->MultiplyPoint(upVector, upVector);

  vtkMath::Normalize(rightVector);
  vtkMath::Normalize(upVector);

  fixedViewRight[0] = rightVector[0];
  fixedViewRight[1] = rightVector[1];
  fixedViewRight[2] = rightVector[2];

  fixedViewUp[0] = upVector[0];
  fixedViewUp[1] = upVector[1];
  fixedViewUp[2] = upVector[2];
}

void cbElectrodeView::AppendMetaData(const std::string text)
{
  std::string current = this->MetaAnnotation->GetText(3);
  current.append("\n"+text);
  this->MetaAnnotation->SetText(3, current.c_str());
}

void cbElectrodeView::SetClippingRange(vtkCamera *c)
{
  double position[3], focal[3];
  c->GetPosition(position);
  c->GetFocalPoint(focal);

  double distance = sqrt(vtkMath::Distance2BetweenPoints(position, focal));
  c->SetClippingRange(10, distance + 500.0);
}

void cbElectrodeView::TogglePatientAnnotations(int s)
{
  this->MetaAnnotation->SetVisibility(s);
  this->viewRect->GetRenderWindow()->Render();
}
