/*=========================================================================

Program:   ToolCursor
Module:    LassoImageTool.cxx

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/

// This example demonstrates Region-of-Interest editing.

// Two image file formats are supported for this example: MINC and DICOM.
// DICOM images are read with the troublesome vtkDICOMImageReader, which
// may get the slice spacing or ordering wrong, or even fail to read the
// images altogether.

#include <vtkSmartPointer.h>

#include <vtkImageReslice.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkMath.h>

#include <vtkMINCImageReader.h>
#include <vtkDICOMImageReader.h>

#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkImageSlice.h>
#include <vtkImageStack.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkLookupTable.h>

#include "vtkROIContourDataToPolyData.h"
#include "vtkImageToROIContourData.h"
#include "vtkROIContourData.h"

#include "vtkToolCursor.h"
#include "vtkWindowLevelTool.h"
#include "vtkSliceImageTool.h"
#include "vtkPanCameraTool.h"
#include "vtkZoomCameraTool.h"
#include "vtkLassoImageTool.h"

#include "vtkToolCursorInteractorObserver.h"

// internal methods for reading images, these methods read the image
// into the specified data object and also provide a matrix for converting
// the data coordinates into patient coordinates.
namespace {

void ReadDICOMImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *directoryName)
{
  // read the image
  vtkSmartPointer<vtkDICOMImageReader> reader =
    vtkSmartPointer<vtkDICOMImageReader>::New();

  reader->SetDirectoryName(directoryName);
  reader->Update();

  // the reader flips the image and reverses the ordering, so undo these
  vtkSmartPointer<vtkImageReslice> flip =
    vtkSmartPointer<vtkImageReslice>::New();

  flip->SetInputConnection(reader->GetOutputPort());
  flip->SetResliceAxesDirectionCosines(
    1,0,0, 0,-1,0, 0,0,-1);
  flip->Update();

  vtkImageData *image = flip->GetOutput();

  // get the data
  data->CopyStructure(image);
  data->GetPointData()->PassData(image->GetPointData());
  data->SetOrigin(0,0,0);

  // generate the matrix
  float *position = reader->GetImagePositionPatient();
  float *orientation = reader->GetImageOrientationPatient();
  float *xdir = &orientation[0];
  float *ydir = &orientation[3];
  float zdir[3];
  vtkMath::Cross(xdir, ydir, zdir);

  for (int i = 0; i < 3; i++)
    {
    matrix->Element[i][0] = xdir[i];
    matrix->Element[i][1] = ydir[i];
    matrix->Element[i][2] = zdir[i];
    matrix->Element[i][3] = position[i];
    }
  matrix->Element[3][0] = 0;
  matrix->Element[3][1] = 0;
  matrix->Element[3][2] = 0;
  matrix->Element[3][3] = 1;
  matrix->Modified();
}

void ReadMINCImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *fileName)
{
  // read the image
  vtkSmartPointer<vtkMINCImageReader> reader =
    vtkSmartPointer<vtkMINCImageReader>::New();

  reader->SetFileName(fileName);
  reader->Update();

  double spacing[3];
  reader->GetOutput()->GetSpacing(spacing);
  spacing[0] = fabs(spacing[0]);
  spacing[1] = fabs(spacing[1]);
  spacing[2] = fabs(spacing[2]);

  // flip the image rows into a DICOM-style ordering
  vtkSmartPointer<vtkImageReslice> flip =
    vtkSmartPointer<vtkImageReslice>::New();

  flip->SetInputConnection(reader->GetOutputPort());
  flip->SetResliceAxesDirectionCosines(
    -1,0,0, 0,-1,0, 0,0,1);
  flip->SetOutputSpacing(spacing);
  flip->Update();

  vtkImageData *image = flip->GetOutput();

  // get the data
  data->CopyStructure(image);
  data->GetPointData()->PassData(image->GetPointData());

  // generate the matrix, but modify to use DICOM coords
  static double xyFlipMatrix[16] =
    { -1, 0, 0, 0,  0, -1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 };
  // correct for the flip that was done earlier
  vtkMatrix4x4::Multiply4x4(*reader->GetDirectionCosines()->Element,
                            xyFlipMatrix, *matrix->Element);
  // do the left/right, up/down dicom-to-minc transformation
  vtkMatrix4x4::Multiply4x4(xyFlipMatrix, *matrix->Element, *matrix->Element);
  matrix->Modified();
}

void SetViewFromMatrix(
  vtkRenderer *renderer,
  vtkInteractorStyleImage *istyle,
  vtkMatrix4x4 *matrix)
{
  istyle->SetCurrentRenderer(renderer);

  // This view assumes the data uses the DICOM Patient Coordinate System.
  // It provides a right-is-left view of axial and coronal images
  double viewRight[4] = { 1.0, 0.0, 0.0, 0.0 };
  double viewUp[4] = { 0.0, -1.0, 0.0, 0.0 };

  matrix->MultiplyPoint(viewRight, viewRight);
  matrix->MultiplyPoint(viewUp, viewUp);

  istyle->SetImageOrientation(viewRight, viewUp);
}

};

int main (int argc, char *argv[])
{
  if (argc < 2)
    {
    cout << "Usage 1: " << argv[0] << " image.mnc" << endl;
    cout << "Usage 2: " << argv[0] << " dicomdir/" << endl;
    return EXIT_FAILURE;
    }

  // -------------------------------------------------------
  // load the images

  int n = 0;

  // Read the source image
  vtkSmartPointer<vtkImageData> sourceImage =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> sourceMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  n = strlen(argv[1]);
  if (n > 4 && strcmp(&argv[1][n-4], ".mnc") == 0)
    {
    ReadMINCImage(sourceImage, sourceMatrix, argv[1]);
    }
  else
    {
    ReadDICOMImage(sourceImage, sourceMatrix, argv[1]);
    }

  // -------------------------------------------------------
  // display the images

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyleImage> istyle =
    vtkSmartPointer<vtkInteractorStyleImage>::New();

  istyle->SetInteractionModeToImage3D();
  interactor->SetInteractorStyle(istyle);
  renderWindow->SetInteractor(interactor);
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkImageSlice> sourceActor =
    vtkSmartPointer<vtkImageSlice>::New();
  vtkSmartPointer<vtkImageResliceMapper> sourceMapper =
    vtkSmartPointer<vtkImageResliceMapper>::New();
  vtkSmartPointer<vtkImageProperty> sourceProperty =
    vtkSmartPointer<vtkImageProperty>::New();

  sourceMapper->SetInput(sourceImage);
  sourceMapper->SliceAtFocalPointOn();
  sourceMapper->SliceFacesCameraOn();
  sourceMapper->JumpToNearestSliceOn();
  sourceMapper->ResampleToScreenPixelsOff();

  double sourceRange[2];
  sourceImage->GetScalarRange(sourceRange);
  sourceProperty->SetInterpolationTypeToLinear();
  sourceProperty->SetColorWindow((sourceRange[1]-sourceRange[0]));
  sourceProperty->SetColorLevel(0.5*(sourceRange[0]+sourceRange[1]));
  sourceProperty->SetCheckerboardSpacing(40,40);

  sourceActor->SetMapper(sourceMapper);
  sourceActor->SetProperty(sourceProperty);
  sourceActor->SetUserMatrix(sourceMatrix);

  vtkSmartPointer<vtkImageStack> imageStack =
    vtkSmartPointer<vtkImageStack>::New();
  imageStack->AddImage(sourceActor);

  renderer->AddViewProp(imageStack);
  renderer->SetBackground(0,0,0);

  renderWindow->SetSize(720,720);

  double bounds[6], center[4];
  sourceImage->GetBounds(bounds);
  center[0] = 0.5*(bounds[0] + bounds[1]);
  center[1] = 0.5*(bounds[2] + bounds[3]);
  center[2] = 0.5*(bounds[4] + bounds[5]);
  center[3] = 1.0;
  sourceMatrix->MultiplyPoint(center, center);

  vtkCamera *camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->SetFocalPoint(center);
  camera->ParallelProjectionOn();
  camera->SetParallelScale(132);
  SetViewFromMatrix(renderer, istyle, sourceMatrix);
  renderer->ResetCameraClippingRange();

  renderWindow->Render();

  // -------------------------------------------------------
  // ToolCursor items

  vtkSmartPointer<vtkToolCursor> cursor =
    vtkSmartPointer<vtkToolCursor>::New();
  cursor->SetRenderer(renderer);
  cursor->SetScale(1.0);

  // Create all the tools
  vtkSmartPointer<vtkWindowLevelTool> winlevTool =
    vtkSmartPointer<vtkWindowLevelTool>::New();
  int winlevId = cursor->AddAction(winlevTool);

  vtkSmartPointer<vtkSliceImageTool> sliceTool =
    vtkSmartPointer<vtkSliceImageTool>::New();
  int sliceId = cursor->AddAction(sliceTool);

  vtkSmartPointer<vtkPanCameraTool> panTool =
    vtkSmartPointer<vtkPanCameraTool>::New();
  int panId = cursor->AddAction(panTool);

  vtkSmartPointer<vtkZoomCameraTool> zoomTool =
    vtkSmartPointer<vtkZoomCameraTool>::New();
  int zoomId = cursor->AddAction(zoomTool);

  vtkSmartPointer<vtkLassoImageTool> lassoTool =
    vtkSmartPointer<vtkLassoImageTool>::New();
  int lassoId = cursor->AddAction(lassoTool);

  // Bind all the tools
  cursor->BindAction(winlevId, 0, 0, VTK_TOOL_CONTROL | VTK_TOOL_B1);
  cursor->BindAction(sliceId, 0, 0, VTK_TOOL_SHIFT | VTK_TOOL_B1);
  cursor->BindAction(panId, 0, 0, VTK_TOOL_SHIFT | VTK_TOOL_B2);
  cursor->BindAction(zoomId, 0, 0, VTK_TOOL_B2);
  cursor->BindAction(lassoId, 0, 0, VTK_TOOL_B1);

  // Don't interpolate between slices while slicing the image
  sliceTool->JumpToNearestSliceOn();

  // -------------------------------------------------------
  // Region of Interest items

  // blur the image to make a smoother mask
  vtkSmartPointer<vtkImageGaussianSmooth> imageBlur =
    vtkSmartPointer<vtkImageGaussianSmooth>::New();
  imageBlur->SetInput(sourceImage);
  imageBlur->SetStandardDeviations(4,4,4);

  // set threshold to 10% of the data range
  double threshold = 0.1*(sourceRange[0]+sourceRange[1]);

  // generate an ROI from the mask
  vtkSmartPointer<vtkImageToROIContourData> maskToROI =
    vtkSmartPointer<vtkImageToROIContourData>::New();
  maskToROI->SetInputConnection(imageBlur->GetOutputPort());
  maskToROI->SetValue(threshold);
  maskToROI->Update();

  // copy the ROI into a new data set so that we can edit it
  vtkSmartPointer<vtkROIContourData> roiData =
    vtkSmartPointer<vtkROIContourData>::New();
  roiData->DeepCopy(maskToROI->GetOutput());

  // add the ROI data to the tool
  lassoTool->SetROIContourData(roiData);
  lassoTool->SetROIMatrix(sourceMatrix);
  lassoTool->AddViewPropsToRenderer(renderer);

  // convert the ROI into a new mask
  vtkSmartPointer<vtkROIContourDataToPolyData> roiDataToPolyData =
    vtkSmartPointer<vtkROIContourDataToPolyData>::New();
  roiDataToPolyData->SetInput(roiData);
  roiDataToPolyData->SubdivisionOn();

  vtkSmartPointer<vtkPolyDataToImageStencil> makeStencil =
    vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  makeStencil->SetTolerance(0.0);
  makeStencil->SetInputConnection(roiDataToPolyData->GetOutputPort());
  makeStencil->SetInformationInput(sourceImage);

  vtkSmartPointer<vtkImageReslice> applyStencil =
    vtkSmartPointer<vtkImageReslice>::New();
  applyStencil->SetInput(sourceImage);
  applyStencil->SetStencil(makeStencil->GetOutput());
  applyStencil->Update();

  // display the new mask
  vtkSmartPointer<vtkImageResliceMapper> maskMapper =
    vtkSmartPointer<vtkImageResliceMapper>::New();
  maskMapper->SliceFacesCameraOn();
  maskMapper->SliceAtFocalPointOn();
  maskMapper->JumpToNearestSliceOn();
  maskMapper->SetInputConnection(applyStencil->GetOutputPort());

  vtkSmartPointer<vtkLookupTable> maskLUT =
    vtkSmartPointer<vtkLookupTable>::New();
  maskLUT->SetHueRange(0.0, 0.0);
  maskLUT->SetValueRange(1.0, 1.0);
  maskLUT->SetSaturationRange(1.0, 1.0);
  maskLUT->SetAlphaRange(0.0, 1.0);
  maskLUT->SetRampToLinear();
  maskLUT->Build();

  vtkSmartPointer<vtkImageProperty> maskProperty =
    vtkSmartPointer<vtkImageProperty>::New();
  maskProperty->SetLookupTable(maskLUT);
  maskProperty->SetColorWindow(1.0);
  maskProperty->SetColorLevel(0.5);
  maskProperty->SetInterpolationTypeToNearest();
  maskProperty->SetLayerNumber(2);
  maskProperty->SetOpacity(0.2);

  vtkSmartPointer<vtkImageSlice> maskSlice =
    vtkSmartPointer<vtkImageSlice>::New();
  maskSlice->SetUserMatrix(sourceMatrix);
  maskSlice->SetMapper(maskMapper);
  maskSlice->SetProperty(maskProperty);

  imageStack->AddImage(maskSlice);

  // -------------------------------------------------------
  // allow user to interact

  // Add observer (if no Qt)
  vtkSmartPointer<vtkToolCursorInteractorObserver> observer =
    vtkSmartPointer<vtkToolCursorInteractorObserver>::New();
  observer->SetToolCursor(cursor);
  observer->SetInteractor(interactor);
  observer->SetEnabled(1);

  interactor->Start();

  return 1;
}
