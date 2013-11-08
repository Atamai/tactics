/*=========================================================================

Program:   Atamai Image Registration and Segmentation
Module:    register.cxx

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/

// Image registration is done first on a blurred, low-resolution version of
// the image before being done on the full resolution image, and is also
// done first with no interpolation before being done with linear interpolation.
// This multi-stage approach increases the robustness and often the speed of
// the registration.

#include <vtkSmartPointer.h>

#include <vtkImageReslice.h>
#include <vtkImageResize.h>
#include <vtkImageSincInterpolator.h>
#include <vtkImageHistogramStatistics.h>
#include <vtkROIStencilSource.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>
#include <vtkMath.h>
#include <vtkCommand.h>

#include <vtkMINCImageReader.h>
#include <vtkMINCImageWriter.h>
#include <vtkMNITransformReader.h>
#include <vtkMNITransformWriter.h>
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
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkJPEGWriter.h>

#include <vtkTimerLog.h>

#include <vtksys/SystemTools.hxx>

#include "AIRSConfig.h"
#include "vtkITKXFMReader.h"
#include "vtkITKXFMWriter.h"
#include "vtkImageRegistration.h"

// optional readers
#ifdef AIRS_USE_DICOM
#define AIRS_USE_NIFTI
#include <vtkNIFTIReader.h>
#include <vtkNIFTIWriter.h>
#include <vtkDICOMReader.h>
#include <vtkDICOMSorter.h>
#include <vtkDICOMMRGenerator.h>
#include <vtkDICOMCTGenerator.h>
#include <vtkDICOMWriter.h>
#include <vtkDICOMMetaData.h>
#include <vtkGlobFileNames.h>
#endif

// coord systems
enum { NativeCoords, DICOMCoords, NIFTICoords };

// file types
enum { DICOMImage, NIFTIImage, MINCImage,
         LastImageType = MINCImage,
       MNITransform, ITKTransform, CSVTransform, TXTTransform,
         LastTransformType = TXTTransform };

// internal methods for reading images, these methods read the image
// into the specified data object and also provide a matrix for converting
// the data coordinates into patient coordinates.
namespace {

int GuessFileType(const char *filename)
{
  size_t n = strlen(filename);

  if (n > 4 && strcmp(&filename[n-4], ".xfm") == 0)
    {
    return MNITransform;
    }
  if (n > 4 && strcmp(&filename[n-4], ".tfm") == 0)
    {
    return ITKTransform;
    }
  if ((n > 4 && strcmp(&filename[n-4], ".txt") == 0) ||
      (n > 4 && strcmp(&filename[n-4], ".mat") == 0))
    {
    return TXTTransform;
    }
  if (n > 4 && strcmp(&filename[n-4], ".csv") == 0)
    {
    return CSVTransform;
    }

  if (n > 4 && strcmp(&filename[n-4], ".mnc") == 0)
    {
    return MINCImage;
    }
  if ((n > 4 && strcmp(&filename[n-4], ".nii") == 0) ||
      (n > 7 && strcmp(&filename[n-7], ".nii.gz") == 0))
    {
    return NIFTIImage;
    }

  return DICOMImage;
}

#ifdef AIRS_USE_DICOM
vtkDICOMReader *ReadDICOMImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *directoryName,
  int coordSystem)
{
  // get the files
  vtkSmartPointer<vtkGlobFileNames> glob =
    vtkSmartPointer<vtkGlobFileNames>::New();
  glob->SetDirectory(directoryName);
  glob->AddFileNames("*");

  // sort the files
  vtkSmartPointer<vtkDICOMSorter> sorter =
    vtkSmartPointer<vtkDICOMSorter>::New();
  sorter->SetInputFileNames(glob->GetFileNames());
  sorter->Update();

  if (sorter->GetNumberOfSeries() == 0)
    {
    fprintf(stderr, "Folder contains no DICOM files: %s\n", directoryName);
    exit(1);
    }
  else if (sorter->GetNumberOfSeries() > 1)
    {
    fprintf(stderr, "Folder contains more than one DICOM series: %s\n",
            directoryName);
    exit(1);
    }

  // read the image
  vtkDICOMReader *reader = vtkDICOMReader::New();
  reader->SetFileNames(sorter->GetFileNamesForSeries(0));

  if (coordSystem == NIFTICoords)
    {
    reader->SetMemoryRowOrderToBottomUp();
    }
  else
    {
    reader->SetMemoryRowOrderToFileNative();
    }

  reader->UpdateInformation();
  if (reader->GetErrorCode())
    {
    exit(1);
    }

  // when reading images, only read 1st component if the
  // image has multiple components or multiple time points
  vtkIntArray *fileArray = reader->GetFileIndexArray();

  // create a filtered list of files
  vtkSmartPointer<vtkStringArray> fileNames =
    vtkSmartPointer<vtkStringArray>::New();
  vtkIdType n = fileArray->GetNumberOfTuples();
  for (vtkIdType i = 0; i < n; i++)
    {
    fileNames->InsertNextValue(
      reader->GetFileNames()->GetValue(fileArray->GetComponent(i, 0)));
    }
  reader->SetDesiredTimeIndex(0);
  reader->SetFileNames(fileNames);

  reader->Update();
  if (reader->GetErrorCode())
    {
    exit(1);
    }

  vtkImageData *image = reader->GetOutput();

  // get the data
  data->CopyStructure(image);
  data->GetPointData()->PassData(image->GetPointData());

  // get the matrix
  matrix->DeepCopy(reader->GetPatientMatrix());

  return reader;
}

void WriteDICOMImage(
  vtkImageReader2 *sourceReader, vtkImageReader2 *targetReader,
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *directoryName,
  int vtkNotUsed(coordSystem))
{
  if (vtksys::SystemTools::FileExists(directoryName))
    {
    if (!vtksys::SystemTools::FileIsDirectory(directoryName))
      {
      fprintf(stderr, "option -o must give a DICOM directory, not a file.\n");
      exit(1);
      }
    }
  else if (!vtksys::SystemTools::MakeDirectory(directoryName))
    {
    fprintf(stderr, "Cannot create directory: %s\n", directoryName);
    exit(1);
    }

  // get the meta data
  vtkDICOMReader *reader = vtkDICOMReader::SafeDownCast(targetReader);
  vtkDICOMReader *reader2 = vtkDICOMReader::SafeDownCast(sourceReader);

  vtkSmartPointer<vtkDICOMMetaData> meta =
    vtkSmartPointer<vtkDICOMMetaData>::New();

  if (reader)
    {
    // copy the bulk of the meta data from the target image
    meta->DeepCopy(reader->GetMetaData());
    meta->SetAttributeValue(DC::SeriesNumber,
      meta->GetAttributeValue(DC::SeriesNumber).AsUnsignedInt() + 1000);
    std::string seriesDescription =
      meta->GetAttributeValue(DC::SeriesDescription).AsString() + " REG";
    if (seriesDescription.size() < 64)
      {
      meta->SetAttributeValue(DC::SeriesDescription, seriesDescription);
      }
    }
  if (reader2)
    {
    // set the frame of reference from the source image
    meta->SetAttributeValue(DC::FrameOfReferenceUID,
      reader2->GetMetaData()->GetAttributeValue(
      DC::FrameOfReferenceUID));
    }

  // make the generator
  vtkSmartPointer<vtkDICOMMRGenerator> mrgenerator =
    vtkSmartPointer<vtkDICOMMRGenerator>::New();
  vtkSmartPointer<vtkDICOMCTGenerator> ctgenerator =
    vtkSmartPointer<vtkDICOMCTGenerator>::New();
  vtkDICOMGenerator *generator = 0;
  if (reader)
    {
    std::string SOPClass =
      meta->GetAttributeValue(DC::SOPClassUID).AsString();
    if (SOPClass == "1.2.840.10008.5.1.4.1.1.2")
      {
      generator = ctgenerator;
      }
    else if (SOPClass == "1.2.840.10008.5.1.4.1.1.4")
      {
      generator = mrgenerator;
      }
    }

  // prepare the writer to write the image
  vtkSmartPointer<vtkDICOMWriter> writer =
    vtkSmartPointer<vtkDICOMWriter>::New();
  if (generator)
    {
    writer->SetGenerator(generator);
    }
  writer->SetMetaData(meta);
  writer->SetFilePrefix(directoryName);
  writer->SetFilePattern("%s/IM-0001-%04.4d.dcm");
  writer->TimeAsVectorOn();
  if (reader)
    {
    if (reader->GetTimeDimension() > 1)
      {
      writer->SetTimeDimension(reader->GetTimeDimension());
      writer->SetTimeSpacing(reader->GetTimeSpacing());
      }
    if (reader->GetRescaleSlope() > 0)
      {
      writer->SetRescaleSlope(reader->GetRescaleSlope());
      writer->SetRescaleIntercept(reader->GetRescaleIntercept());
      }
    writer->SetMemoryRowOrder(reader->GetMemoryRowOrder());
    }
  writer->SetInput(data);
  writer->SetPatientMatrix(matrix);
  writer->Write();
}

#else

vtkDICOMImageReader *ReadDICOMImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *directoryName,
  int coordSystem)
{
  // read the image
  vtkDICOMImageReader *reader = vtkDICOMImageReader::New();

  reader->SetDirectoryName(directoryName);
  reader->Update();
  if (reader->GetErrorCode())
    {
    exit(1);
    }

  vtkSmartPointer<vtkImageData> image = reader->GetOutput();

  if (coordSystem != NIFTICoords)
    {
    // the reader flips the image and reverses the ordering, so undo these
    vtkSmartPointer<vtkImageReslice> flip =
      vtkSmartPointer<vtkImageReslice>::New();

    flip->SetInputConnection(reader->GetOutputPort());
    flip->SetResliceAxesDirectionCosines(
      1,0,0, 0,-1,0, 0,0,-1);
    flip->Update();

    image = flip->GetOutput();
    }

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

  if (coordSystem == NIFTICoords)
    {
    double spacing[3], origin[3];
    int extent[6];
    image->GetSpacing(spacing);
    image->GetOrigin(origin);
    image->GetExtent(extent);
    // account fo the y and z flips
    double point[4];
    point[0] = origin[0] + spacing[0]*extent[0];
    point[1] = origin[1] + spacing[1]*extent[3];
    point[2] = origin[2] + spacing[2]*extent[5];
    point[3] = 1.0;
    matrix->MultiplyPoint(point, point);
    for (int j = 0; j < 3; j++)
      {
      matrix->Element[j][1] = -matrix->Element[j][1];
      matrix->Element[j][2] = -matrix->Element[j][2];
      matrix->Element[j][3] = point[j];
      }
    // do the DICOM to NIFTI coord conversion
    for (int k = 0; k < 4; k++)
      {
      matrix->Element[0][k] = -matrix->Element[0][k];
      matrix->Element[1][k] = -matrix->Element[1][k];
      }
    }

  matrix->Modified();

  return reader;
}
#endif

vtkMINCImageReader *ReadMINCImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *fileName,
  int coordSystem)
{
  // read the image
  vtkMINCImageReader *reader = vtkMINCImageReader::New();

  reader->SetFileName(fileName);
  reader->Update();
  if (reader->GetErrorCode())
    {
    exit(1);
    }

  vtkSmartPointer<vtkImageData> image = reader->GetOutput();

  if (coordSystem == DICOMCoords)
    {
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

    image = flip->GetOutput();
    }

  // get the data
  data->CopyStructure(image);
  data->GetPointData()->PassData(image->GetPointData());

  if (coordSystem == DICOMCoords)
    {
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
  else
    {
    matrix->DeepCopy(reader->GetDirectionCosines());
    }

  return reader;
}

void WriteMINCImage(
  vtkImageReader2 *vtkNotUsed(sourceReader),
  vtkImageReader2 *vtkNotUsed(targetReader),
  vtkImageData *data, vtkMatrix4x4 *vtkNotUsed(matrix), const char *fileName,
  int vtkNotUsed(coordSystem))
{
  fprintf(stderr, "Writing MINC images is not supported yet, "
          "the output file will have incorrect information\n");
  vtkSmartPointer<vtkMINCImageWriter> writer =
    vtkSmartPointer<vtkMINCImageWriter>::New();
  writer->SetFileName(fileName);
  writer->SetInput(data);
  // the input matrix must be converted
  //writer->SetDirectionCosines(matrix);
  writer->Write();
}

#ifdef AIRS_USE_NIFTI
vtkNIFTIReader *ReadNIFTIImage(
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *fileName,
  int coordSystem)
{
  // read the image
  vtkNIFTIReader *reader = vtkNIFTIReader::New();

  reader->SetFileName(fileName);
  reader->Update();
  if (reader->GetErrorCode())
    {
    exit(1);
    }

  vtkSmartPointer<vtkImageData> image = reader->GetOutput();

  if (coordSystem == DICOMCoords)
    {
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

    image = flip->GetOutput();
    }

  // get the data
  data->CopyStructure(image);
  data->GetPointData()->PassData(image->GetPointData());

  // get the SForm or QForm matrix if present
  static double nMatrix[16] =
    { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 };
  if (reader->GetQFormMatrix())
    {
    vtkMatrix4x4::DeepCopy(nMatrix, reader->GetQFormMatrix());
    }
  else if (reader->GetSFormMatrix())
    {
    vtkMatrix4x4::DeepCopy(nMatrix, reader->GetSFormMatrix());
    }

  if (coordSystem == DICOMCoords)
    {
    // generate the matrix, but modify to use DICOM coords
    static double xyFlipMatrix[16] =
      { -1, 0, 0, 0,  0, -1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 };
    // correct for the flip that was done earlier
    vtkMatrix4x4::Multiply4x4(nMatrix, xyFlipMatrix, *matrix->Element);
    // do the left/right, up/down dicom-to-minc transformation
    vtkMatrix4x4::Multiply4x4(xyFlipMatrix, *matrix->Element, *matrix->Element);
    matrix->Modified();
    }
  else
    {
    matrix->DeepCopy(nMatrix);
    }

  return reader;
}

void WriteNIFTIImage(
  vtkImageReader2 *vtkNotUsed(sourceReader), vtkImageReader2 *targetReader,
  vtkImageData *data, vtkMatrix4x4 *matrix, const char *fileName,
  int vtkNotUsed(coordSystem))
{
  vtkNIFTIReader *reader = vtkNIFTIReader::SafeDownCast(targetReader);

  vtkSmartPointer<vtkNIFTIWriter> writer =
    vtkSmartPointer<vtkNIFTIWriter>::New();
  if (reader)
    {
    writer->SetNIFTIHeader(reader->GetNIFTIHeader());
    if (reader->GetTimeDimension() > 1)
      {
      writer->SetTimeDimension(reader->GetTimeDimension());
      writer->SetTimeSpacing(reader->GetTimeSpacing());
      }
    if (reader->GetQFac() < 0)
      {
      writer->SetQFac(-1.0);
      }
    }
  writer->SetInput(data);
  writer->SetQFormMatrix(matrix);
  writer->SetSFormMatrix(matrix);
  writer->SetFileName(fileName);
  writer->Write();
}

#endif /* AIRS_USE_NIFTI */

vtkImageReader2 *ReadImage(
  vtkImageData *image, vtkMatrix4x4 *matrix,
  const char *filename, int coordSystem)
{
  int t = GuessFileType(filename);

  if (t == MINCImage)
    {
    return ReadMINCImage(image, matrix, filename, coordSystem);
    }
  else if (t == NIFTIImage)
    {
#ifdef AIRS_USE_NIFTI
    return ReadNIFTIImage(image, matrix, filename, coordSystem);
#else
    fprintf(stderr, "NIFTI files are not supported.\n");
    exit(1);
#endif
    }

  return ReadDICOMImage(image, matrix, filename, coordSystem);
}

int CoordSystem(const char *filename)
{
  int t = GuessFileType(filename);

  if (t == MINCImage || t == NIFTIImage)
    {
    return NIFTICoords;
    }

  return DICOMCoords;
}

void WriteImage(
  vtkImageReader2 *sourceReader, vtkImageReader2 *targetReader,
  vtkImageData *image, vtkMatrix4x4 *matrix,
  const char *filename, int coordSystem)
{
  int t = GuessFileType(filename);

  if (t == MINCImage)
    {
    WriteMINCImage(
      sourceReader, targetReader, image, matrix, filename, coordSystem);
    }
  else if (t == NIFTIImage)
    {
#ifdef AIRS_USE_NIFTI
    WriteNIFTIImage(
      sourceReader, targetReader, image, matrix, filename, coordSystem);
#else
    fprintf(stderr, "NIFTI files are not supported.\n");
    exit(1);
#endif
    }
  else
    {
#ifdef AIRS_USE_DICOM
    WriteDICOMImage(
      sourceReader, targetReader, image, matrix, filename, coordSystem);
#else
    fprintf(stderr, "Writing DICOM files is not supported.\n");
    exit(1);
#endif
    }
}


void SetViewFromMatrix(
  vtkRenderer *renderer,
  vtkInteractorStyleImage *istyle,
  vtkMatrix4x4 *matrix,
  int coordSystem)
{
  istyle->SetCurrentRenderer(renderer);

  // This view assumes the data uses the DICOM Patient Coordinate System.
  // It provides a right-is-left view of axial and coronal images
  double viewRight[4] = { 1.0, 0.0, 0.0, 0.0 };
  double viewUp[4] = { 0.0, 1.0, 0.0, 0.0 };

  if (coordSystem == DICOMCoords)
    {
    viewUp[1] = -1.0;
    }

  matrix->MultiplyPoint(viewRight, viewRight);
  matrix->MultiplyPoint(viewUp, viewUp);

  istyle->SetImageOrientation(viewRight, viewUp);
}

// a class to look for errors when reading transforms.
class ErrorObserver : public vtkCommand
{
public:
  static ErrorObserver *New() { return new ErrorObserver; }
  vtkTypeMacro(ErrorObserver, vtkCommand);
  virtual void Execute(vtkObject *o, unsigned long eventId, void *callData);
};

void ErrorObserver::Execute(
  vtkObject *, unsigned long, void *callData)
{
  if (callData)
    {
    fprintf(stderr, "%s\n", static_cast<char *>(callData));
    }
  exit(1);
}

void ReadMatrix(vtkMatrix4x4 *matrix, const char *xfminput)
{
  vtkSmartPointer<ErrorObserver> observer =
    vtkSmartPointer<ErrorObserver>::New();
  int t = GuessFileType(xfminput);

  if (t == MNITransform) // .xfm
    {
    // MNI transform file (always in RAS coords)
    vtkSmartPointer<vtkMNITransformReader> reader =
      vtkSmartPointer<vtkMNITransformReader>::New();
    reader->SetFileName(xfminput);
    reader->AddObserver(vtkCommand::ErrorEvent, observer);
    reader->Update();
    vtkLinearTransform *transform =
      vtkLinearTransform::SafeDownCast(reader->GetTransform());
    if (transform)
      {
      matrix->DeepCopy(transform->GetMatrix());
      }
    else
      {
      fprintf(stderr, "Unable to read input transform %s\n", xfminput);
      exit(1);
      }
    }
  else if (t == ITKTransform) // .tfm
    {
    // ITK transform file (always in DICOM coords)
    vtkSmartPointer<vtkITKXFMReader> reader =
      vtkSmartPointer<vtkITKXFMReader>::New();
    reader->SetFileName(xfminput);
    reader->AddObserver(vtkCommand::ErrorEvent, observer);
    reader->Update();
    vtkLinearTransform *transform =
      vtkLinearTransform::SafeDownCast(reader->GetTransform());
    if (transform)
      {
      matrix->DeepCopy(transform->GetMatrix());
      }
    else
      {
      fprintf(stderr, "Unable to read input transform %s\n", xfminput);
      exit(1);
      }
    }
  else
    {
    // text file
    double elements[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0 };

    ifstream infile(xfminput);
    int i = 0;
    while (infile.good() && i < 16)
      {
      infile >> elements[i];
      if (infile.fail() && !infile.bad())
        {
        infile.clear();
        infile.get();
        }
      else
        {
        i++;
        }
      }
    infile.close();
    if (i < 16)
      {
      fprintf(stderr, "Unable to read input transform %s\n", xfminput);
      exit(1);
      }
    matrix->DeepCopy(elements);
    }
}

void WriteMatrix(
  vtkMatrix4x4 *matrix, const char *xfmfile, const double center[3])
{
  vtkSmartPointer<ErrorObserver> observer =
    vtkSmartPointer<ErrorObserver>::New();
  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transform->Concatenate(matrix);

  int t = GuessFileType(xfmfile);

  if (t == MNITransform) // .xfm
    {
    // MNI transform file (always in RAS coords)
    vtkSmartPointer<vtkMNITransformWriter> writer =
      vtkSmartPointer<vtkMNITransformWriter>::New();
    writer->SetFileName(xfmfile);
    writer->SetTransform(transform);
    writer->AddObserver(vtkCommand::ErrorEvent, observer);
    writer->Update();
    }
  else if (t == ITKTransform) // .tfm
    {
    // ITK transform file (always in DICOM coords)
    vtkSmartPointer<vtkITKXFMWriter> writer =
      vtkSmartPointer<vtkITKXFMWriter>::New();
    writer->SetFileName(xfmfile);
    writer->SetTransform(transform);
    writer->SetTransformCenter(center);
    writer->AddObserver(vtkCommand::ErrorEvent, observer);
    writer->Write();
    }
  else
    {
    // Delimited text file
    const char *delim = ((t == CSVTransform) ? "," : "\t");
    ofstream outfile(xfmfile, ios::out);
    for (int i = 0; i < 4; i++)
      {
      outfile << matrix->Element[i][0] << delim
              << matrix->Element[i][1] << delim
              << matrix->Element[i][2] << delim
              << matrix->Element[i][3] << "\n";
      }
    if (!outfile.good())
      {
      fprintf(stderr, "Unable to write output transform.\n");
      exit(1);
      }
    outfile.close();
    }
}

void WriteScreenshot(vtkWindow *window, const char *filename)
{
  vtkSmartPointer<vtkWindowToImageFilter> snap =
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  snap->SetInput(window);
  snap->Update();

  size_t l = strlen(filename);
  if (l >= 4 && strcmp(filename + (l - 4), ".png") == 0)
    {
    vtkSmartPointer<vtkPNGWriter> snapWriter =
      vtkSmartPointer<vtkPNGWriter>::New();
    snapWriter->SetInputConnection(snap->GetOutputPort());
    snapWriter->SetFileName(filename);
    snapWriter->Write();
    }
  else if ((l >= 4 && strcmp(filename + (l - 4), ".jpg") == 0) ||
           (l >= 5 && strcmp(filename + (l - 5), ".jpeg") == 0))
    {
    vtkSmartPointer<vtkJPEGWriter> snapWriter =
      vtkSmartPointer<vtkJPEGWriter>::New();
    snapWriter->SetInputConnection(snap->GetOutputPort());
    snapWriter->SetFileName(filename);
    snapWriter->Write();
    }
  else if ((l >= 4 && strcmp(filename + (l - 4), ".tif") == 0) ||
           (l >= 5 && strcmp(filename + (l - 5), ".tiff") == 0))
    {
    vtkSmartPointer<vtkTIFFWriter> snapWriter =
      vtkSmartPointer<vtkTIFFWriter>::New();
    snapWriter->SetInputConnection(snap->GetOutputPort());
    snapWriter->SetFileName(filename);
    snapWriter->Write();
    }
}

void ComputeRange(vtkImageData *image, double range[2])
{
  // compute the range within a cylinder that is slightly smaller than
  // the image bounds (the idea is to capture only the reconstructed
  // portion of a CT image).
  double spacing[3];
  double origin[3];
  int extent[6];
  double bounds[6];
  image->GetSpacing(spacing);
  image->GetOrigin(origin);
  image->GetWholeExtent(extent);
  for (int i = 0; i < 3; ++i)
    {
    double b1 = extent[2*i]*spacing[i] + origin[i];
    double b2 = extent[2*i+1]*spacing[i] + origin[i];
    b1 = (i == 2 ? b1 : -b1); // flip if not Z
    b2 = (i == 2 ? b2 : -b2); // flip if not Z
    bounds[2*i] = (b1 < b2 ? b1 : b2);
    bounds[2*i+1] = (b1 < b2 ? b2 : b1);
    spacing[i] = fabs(spacing[i]);
    origin[i] = bounds[2*i];
    // reduce bounds by 2% in X and Y for use in cylinder generation
    double bl = (i == 2 ? 0.0 : 0.01*(bounds[2*i+1] - bounds[2*i]));
    bounds[2*i] += bl;
    bounds[2*i+1] -= bl;
    }

  // extract just the reconstructed portion of CT image
  vtkSmartPointer<vtkROIStencilSource> cylinder =
    vtkSmartPointer<vtkROIStencilSource>::New();

  cylinder->SetShapeToCylinderZ();
  cylinder->SetOutputSpacing(spacing);
  cylinder->SetOutputOrigin(origin);
  cylinder->SetOutputWholeExtent(extent);
  cylinder->SetBounds(bounds);

  // get the range within the cylinder
  vtkSmartPointer<vtkImageHistogramStatistics> rangeFinder =
    vtkSmartPointer<vtkImageHistogramStatistics>::New();

  rangeFinder->SetInput(image);
  rangeFinder->SetStencil(cylinder->GetOutput());
  rangeFinder->Update();

  rangeFinder->GetAutoRange(range);
}

};

struct register_options
{
  int dimensionality;  // -D --dimensionality
  int metric;          // -M --metric
  int transform;       // -T --transform
  int interpolator;    // -I --interpolator
  int coords;          // -C --coords
  int maxiter[3];      // -M --maxiter
  int display;         // -d --display
  int silent;          // -s --silent
  int invert;          // -i --invert (input transform)
  const char *outxfm;  // -o (output transform)
  const char *output;  // -o (output image)
  const char *screenshot; // -j (output screenshot)
  const char *source;
  const char *target;
  const char *initial;
};

void register_initialize_options(register_options *options)
{
  options->dimensionality = 3;
  options->metric = vtkImageRegistration::MutualInformation;
  options->transform = vtkImageRegistration::Rigid;
  options->interpolator = vtkImageRegistration::Linear;
  options->coords = NativeCoords;
  options->invert = 0;
  options->maxiter[0] = 500;
  options->maxiter[1] = 500;
  options->maxiter[2] = 500;
  options->display = 0;
  options->silent = 0;
  options->screenshot = NULL;
  options->output = NULL;
  options->outxfm = NULL;
  options->source = NULL;
  options->target = NULL;
  options->initial = NULL;
}

const char *check_next_arg(
  int argc, char *argv[], int *argi, const char *possib[])
{
  const char *op = argv[*argi - 1];
  if (*argi >= argc ||
      argv[*argi][0] == '-')
    {
    fprintf(stderr, "The option \"%s\" must be followed by an argument\n", op);
    exit(1);
    }
  const char *arg = argv[(*argi)++];

  if (possib == 0)
    {
    return arg;
    }

  for (const char **t = possib; *t != 0; t++)
    {
    if (strcmp(*t, arg) == 0)
      {
      return arg;
      }
    }

  fprintf(stderr, "Incorrect value for option \"%s\": %s\n",
          op, arg);
  fprintf(stderr, "Allowed values:");
  for (const char **u = possib; *u != 0; u++)
    {
    fprintf(stderr, "%s", *u);
    }
  fprintf(stderr, "\n");
  exit(1);

  return 0;
}

void register_show_usage(FILE *fp, const char *command)
{
  const char *cp = command + strlen(command);
  while (cp > command && cp[-1] != '/' && cp[-1] != '\\') { --cp; }

  fprintf(fp,
    "Usage: %s [options] -o <output> <source image> <target image>\n", cp);
  fprintf(fp, "\n");
  fprintf(fp,
    "For more information, type \"%s --help\"\n\n", command);
}

void register_show_help(FILE *fp, const char *command)
{
  const char *cp = command + strlen(command);
  while (cp > command && cp[-1] != '/' && cp[-1] != '\\') { --cp; }

  fprintf(fp,
    "Usage: %s [options] -o <output> <source image> <target image>\n", cp);
  fprintf(fp,
    "\n"
    "Written by David Gobbi <dgobbi@ucalgary.ca> at CIPAC.  Version 0.2.3.\n"
    "\n"
    "This program performs 3D image registration on DICOM, MINC, or NIFTI\n"
    "image volumes.  It reads the image header (or the DICOM meta data) in\n"
    "order to discover the orientation of the image slices in real-world\n"
    "coordinates (e.g. the DICOM patient coodinate system for DICOM files,\n"
    "or \"world coordinates\" for MINC and NIFTI files).  The result of the\n"
    "registration is that the target image is resampled (i.e. regridded via\n"
    "trilinear interpolation) in order to put it into the same coordinate\n"
    "system as, and with the same slice geometry as, the source image.\n"
    "\n"
    "The registration is performed via a multi-resolution pyramid, starting\n"
    "with images that have been blurred to four times their original pixel\n"
    "spacing, proceeding to images that have been blurred to two times their\n"
    "original spacing, and finishing with unblurred, full-resolution images.\n"
    "At each resolution, the registration is first done with nearest-neighbor\n"
    "interpolation, and then refined with linear interpolation.\n"
    "\n"
    "The \"-o\" option allows you to specify either an output image, or an\n"
    "output transform file.  The transform file will provide the coordinate\n"
    "transformation from the source image space to the target image space.\n"
    "This \"space\" is the coordinate system specified by the image header.\n"
    "\n"
    "If you have a transform file and want to apply it to another image,\n"
    "the transform file can be provided as an input to the program, and the\n"
    "number of registration iterations can be set to zero (-N 0) in order\n"
    "to apply the transform directly without performing another registration.\n"
    "\n");
  fprintf(fp,
    " -D --dimensionality   (default: 3)\n"
    "                 2\n"
    "                 3\n"
    "\n"
    "    If the dimensionality is set to 2 for a 3D file, then the\n"
    "    registration will be limited to in-plane transformations.\n"
    "\n"
    " -M --metric           (default MutualInformation)\n"
    "                 SD        SquaredDifference\n"
    "                 CC        CrossCorrelation\n"
    "                 NCC       NormalizedCrossCorrelation\n"
    "                 NC        NeighborhoodCorrelation\n"
    "                 MI        MutualInformation\n"
    "                 NMI       NormalizedMutualInformation\n"
    "\n"
    "    Mutual information (the default) should be used in most cases.\n"
    "    Normalized Mutual information may be more robust (but not more\n"
    "    accurate) if one input or both inputs are only a small part\n"
    "    of the organ or anatomy that is being registered.\n"
    "\n"
    " -T --transform        (default: Rigid)\n"
    "                 TR        Translation\n"
    "                 RI        Rigid\n"
    "                 SI        Similarity\n"
    "                 SS        ScaleSourceAxes\n"
    "                 ST        ScaleTargetAxes\n"
    "                 AF        Affine\n"
    "\n"
    "    A rigid transform should always be used for intra-subject\n"
    "    registration.  For inter-subject registration, the SS and ST\n"
    "    transforms restrict the scale part of the transformation to be\n"
    "    either along the directions of the source image axes, or along\n"
    "    the directions of the target image axes.  It is always the source\n"
    "    image that is modified by the transform, never the target image.\n"
    "\n"
    " -I --interpolator     (default: Linear)\n"
    "                 NN        NearestNeighbor\n"
    "                 LI        Linear\n"
    "                 CU        Cubic\n"
    "                 WS        WindowedSinc\n"
    "\n"
    "    Linear interpolation is usually the best choice, it provides\n"
    "    a good balance between efficience and quality.  NearestNeighbor\n"
    "    is required if one of the images is a label image.  The Windowed\n"
    "    Sinc interpolator uses a five-lobe Blackman-windowed sinc kernel\n"
    "    and offers the highest overall quality.\n"
    "\n"
    " -C --coords           (default: guess from file type)\n"
    "                 DICOM     LPS\n"
    "                 NIFTI     RAS\n"
    "                 MINC      RAS\n"
    "\n"
    "    The DICOM standard defines a patient coordinate system where the\n"
    "    x-axis points left, the y-axis points towards the back, and the\n"
    "    z-axis points towards the head.  NIFTI and MINC use a coordinate\n"
    "    system where x points right and y points towards the front.  The\n"
    "    matrix that is written by the \"-o\" option will be in the chosen\n"
    "    coordinate system.\n"
    "\n"
    " -N --maxiter   (default: 500x500x500)\n"
    "\n"
    "    Set the maximum number of iterations per stage.  Set this to zero\n"
    "    if you want to use the initial transform as-is.\n"
    "\n"
    " -d --display      (default: off)\n"
    "\n"
    "    Display the images during the registration.\n"
    "\n"
    " -s --silent       (default: off)\n"
    "\n"
    "    Do not print information to the console during the resistration.\n"
    "    This is useful when running in batch mode.  Error messages will\n"
    "    still be printed.\n"
    "\n"
    " -j --screenshot <file>\n"
    "\n"
    "    Write a screenshot as a png, jpeg, or tiff file.  This is useful\n"
    "    when performing registration in a batch file in order to provide\n"
    "    a simple means of visually assessing the results retrospectively.\n"
    "\n"
    " -o <file>\n"
    "\n"
    "    Provide a file for the resulting transform or image to be written\n"
    "    to.  If you want to write both a transform file and an image file,\n"
    "    then use the -o option twice (once for each desired output).\n"
    "\n"
    " -i --invert <transform>\n"
    "\n"
    "    Use the inverse of the given transform as the initial transform.\n"
    "\n");
}

int register_read_options(
  int argc, char *argv[], register_options *options)
{
  static const char *dimensionality_args[] = {
    "2", "3", 0 };
  static const char *metric_args[] = {
    "SquaredDifference", "SD",
    "CrossCorrelation", "CC",
    "NormalizedCrossCorrelation", "NCC",
    "NeighborhoodCorrelation", "NC",
    "MutualInformation", "MI",
    "NormalizedMutualInformation", "NMI",
    0 };
  static const char *transform_args[] = {
    "Translation", "TR",
    "Rigid", "RI",
    "Similarity", "SI",
    "ScaleSourceAxes", "SS",
    "ScaleTargetAxes", "ST",
    "Affine", "AF",
    0 };
  static const char *interpolator_args[] = {
    "NearestNeighbor", "NN",
    "Linear", "LI",
    "Cubic", "CU",
    "WindowedSinc", "WS",
    0 };
  static const char *coords_args[] = {
    "DICOM", "LPS",
    "NIFTI", "MINC", "RAS",
    0 };

  int argi = 1;
  while (argi < argc)
    {
    const char *arg = argv[argi++];
    if (arg[0] != '-')
      {
      int t = GuessFileType(arg);

      if (t <= LastImageType)
        {
        if (options->source == 0)
          {
          options->source = arg;
          }
        else if (options->target == 0)
          {
          options->target = arg;
          }
        else
          {
          fprintf(stderr, "Too many input images listed on command line\n");
          exit(1);
          }
        }
      else if (t <= LastTransformType)
        {
        if (options->initial == 0)
          {
          options->initial = arg;
          }
        else
          {
          fprintf(stderr, "Too many input transforms listed on command line\n");
          exit(1);
          }
        }
      }
    else
      {
      if (strcmp(arg, "-h") == 0 ||
          strcmp(arg, "--help") == 0)
        {
        register_show_help(stdout, argv[0]);
        exit(0);
        }
      else if (strcmp(arg, "-D") == 0 ||
               strcmp(arg, "--dimensionality") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, dimensionality_args);
        options->dimensionality = (arg[0] == '2' ? 2 : 3);
        }
      else if (strcmp(arg, "-M") == 0 ||
               strcmp(arg, "--metric") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, metric_args);
        if (strcmp(arg, "SquaredDifference") == 0 ||
            strcmp(arg, "SD") == 0)
          {
          options->metric = vtkImageRegistration::SquaredDifference;
          }
        else if (strcmp(arg, "CrossCorrelation") == 0 ||
                 strcmp(arg, "CC") == 0)
          {
          options->metric = vtkImageRegistration::CrossCorrelation;
          }
        else if (strcmp(arg, "NormalizedCrossCorrelation") == 0 ||
                 strcmp(arg, "NCC") == 0)
          {
          options->metric = vtkImageRegistration::NormalizedCrossCorrelation;
          }
        else if (strcmp(arg, "NeighborhoodCorrelation") == 0 ||
                 strcmp(arg, "NC") == 0)
          {
          options->metric = vtkImageRegistration::NeighborhoodCorrelation;
          }
        else if (strcmp(arg, "MutualInformation") == 0 ||
                 strcmp(arg, "MI") == 0)
          {
          options->metric = vtkImageRegistration::MutualInformation;
          }
        else if (strcmp(arg, "NormalizedMutualInformation") == 0 ||
                 strcmp(arg, "NMI") == 0)
          {
          options->metric = vtkImageRegistration::NormalizedMutualInformation;
          }
        }
      else if (strcmp(arg, "-T") == 0 ||
               strcmp(arg, "--transform") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, transform_args);
        if (strcmp(arg, "Translation") == 0 ||
            strcmp(arg, "TR") == 0)
          {
          options->transform = vtkImageRegistration::Translation;
          }
        else if (strcmp(arg, "Rigid") == 0 ||
                 strcmp(arg, "RI") == 0)
          {
          options->transform = vtkImageRegistration::Rigid;
          }
        else if (strcmp(arg, "Similarity") == 0 ||
                 strcmp(arg, "SI") == 0)
          {
          options->transform = vtkImageRegistration::Similarity;
          }
        else if (strcmp(arg, "ScaleSourceAxes") == 0 ||
                 strcmp(arg, "SS") == 0)
          {
          options->transform = vtkImageRegistration::ScaleSourceAxes;
          }
        else if (strcmp(arg, "ScaleTargetAxes") == 0 ||
                 strcmp(arg, "ST") == 0)
          {
          options->transform = vtkImageRegistration::ScaleTargetAxes;
          }
        else if (strcmp(arg, "Affine") == 0 ||
                 strcmp(arg, "AF") == 0)
          {
          options->transform = vtkImageRegistration::Affine;
          }
        }
      else if (strcmp(arg, "-I") == 0 ||
               strcmp(arg, "--interpolator") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, interpolator_args);
        if (strcmp(arg, "NearestNeighbor") == 0 ||
            strcmp(arg, "NN") == 0)
          {
          options->interpolator = vtkImageRegistration::Nearest;
          }
        else if (strcmp(arg, "Linear") == 0 ||
                 strcmp(arg, "LI") == 0)
          {
          options->interpolator = vtkImageRegistration::Linear;
          }
        else if (strcmp(arg, "Cubic") == 0 ||
                 strcmp(arg, "CU") == 0)
          {
          options->interpolator = vtkImageRegistration::Cubic;
          }
        else if (strcmp(arg, "WindowedSinc") == 0 ||
                 strcmp(arg, "WS") == 0)
          {
          options->interpolator = vtkImageRegistration::Sinc;
          }
        }
      else if (strcmp(arg, "-C") == 0 ||
               strcmp(arg, "--coords") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, coords_args);
        if (strcmp(arg, "DICOM") == 0 ||
            strcmp(arg, "LPS") == 0)
          {
          options->coords = DICOMCoords;
          }
        else if (strcmp(arg, "MINC") == 0 ||
                 strcmp(arg, "NIFTI") == 0 ||
                 strcmp(arg, "RAS") == 0)
          {
          options->coords = NIFTICoords;
          }
        }
      else if (strcmp(arg, "-N") == 0 ||
               strcmp(arg, "--maxiter") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, 0);
        for (int i = 0; i < 3; i++)
          {
          options->maxiter[i] = static_cast<int>(
            strtoul(arg, const_cast<char **>(&arg), 0));
          if (*arg == 'x') { arg++; }
          }
        }
      else if (strcmp(arg, "-d") == 0 ||
               strcmp(arg, "--display") == 0)
        {
        options->display = 1;
        }
      else if (strcmp(arg, "-s") == 0 ||
               strcmp(arg, "--silent") == 0)
        {
        options->silent = 1;
        }
      else if (strcmp(arg, "-i") == 0 ||
               strcmp(arg, "--invert") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, 0);
        int t = GuessFileType(arg);

        if (t > LastImageType && t <= LastTransformType)
          {
          if (options->initial == 0)
            {
            options->initial = arg;
            options->invert = 1;
            }
          else
            {
            fprintf(stderr,
                    "Too many input transforms listed on command line\n");
            exit(1);
            }
          }
        else
          {
          fprintf(stderr,
                  "The \"-i\" option must be followed by a transform\n");
          exit(1);
          }
        }
      else if (strcmp(arg, "-j") == 0 ||
               strcmp(arg, "--screenshot") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, 0);
        options->screenshot = arg;
        }
      else if (strcmp(arg, "-o") == 0)
        {
        arg = check_next_arg(argc, argv, &argi, 0);
        int t = GuessFileType(arg);
        if (t <= LastImageType)
          {
          if (options->output)
            {
            fprintf(stderr, "Too many -o options specified!\n");
            register_show_usage(stderr, argv[0]);
            }
          options->output = arg;
          }
        else if (t <= LastTransformType)
          {
          if (options->outxfm)
            {
            fprintf(stderr, "Too many -o options specified!\n");
            register_show_usage(stderr, argv[0]);
            }
          options->outxfm = arg;
          }
        }
      else
        {
        fprintf(stderr, "Unrecognized option \"%s\"\n", arg);
        register_show_usage(stderr, argv[0]);
        exit(1);
        }
      }
    }

  return 1;
}

int main(int argc, char *argv[])
{
  register_options options;
  register_initialize_options(&options);
  register_read_options(argc, argv, &options);

  // -------------------------------------------------------
  // the files
  const char *xfminput = options.initial;
  const char *xfmfile = options.outxfm;
  const char *imagefile = options.output;
  const char *sourcefile = options.source;
  const char *targetfile = options.target;
  bool display = (options.display != 0 ||
                  options.screenshot != 0);

  if (!sourcefile || !targetfile)
    {
    register_show_usage(stderr, argv[0]);
    return 1;
    }

  // -------------------------------------------------------
  // parameters for registration

  int interpolatorType = options.interpolator;
  double transformTolerance = 0.1; // tolerance on transformation result
  int numberOfBins = 64; // for Mattes' mutual information
  double initialBlurFactor = 4.0;

  // -------------------------------------------------------
  // load the initial matrix
  vtkSmartPointer<vtkMatrix4x4> initialMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  if (xfminput)
    {
    if (!options.silent)
      {
      cout << "Reading initial transform: " << sourcefile << endl;
      if (options.invert)
        {
        cout << "Using inverse of transform." << endl;
        }
      }

    ReadMatrix(initialMatrix, xfminput);
    if (options.invert)
      {
      initialMatrix->Invert();
      }
    }

  // -------------------------------------------------------
  // load the images

  if (options.coords == NativeCoords)
    {
    int ic = CoordSystem(sourcefile);
    int oc = CoordSystem(targetfile);

    if (ic == DICOMCoords || oc == DICOMCoords)
      {
      options.coords = DICOMCoords;
      }
    else
      {
      options.coords = NIFTICoords;
      }
    }

  if (!options.silent)
    {
    cout << "Reading source image: " << sourcefile << endl;
    }

  vtkSmartPointer<vtkImageData> sourceImage =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> sourceMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkImageReader2> sourceReader =
    ReadImage(sourceImage, sourceMatrix, sourcefile, options.coords);
  sourceReader->Delete();

  if (!options.silent)
    {
    cout << "Reading target image: " << targetfile << endl;
    }

  vtkSmartPointer<vtkImageData> targetImage =
    vtkSmartPointer<vtkImageData>::New();
  vtkSmartPointer<vtkMatrix4x4> targetMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkImageReader2> targetReader =
    ReadImage(targetImage, targetMatrix, targetfile, options.coords);
  targetReader->Delete();

  if (!options.silent)
    {
    if (options.coords == DICOMCoords)
      {
      cout << "Using DICOM patient coords." << endl;;
      }
    else
      {
      cout << "Using NIFTI (or MINC) world coords." << endl;
      }
    }

  // -------------------------------------------------------
  // save the original target matrix
  vtkSmartPointer<vtkMatrix4x4> originalTargetMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  originalTargetMatrix->DeepCopy(targetMatrix);

  // apply the initialization matrix
  vtkSmartPointer<vtkMatrix4x4> matrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->DeepCopy(initialMatrix);
  matrix->Invert();
  vtkMatrix4x4::Multiply4x4(matrix, targetMatrix, targetMatrix);

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

  istyle->SetInteractionModeToImageSlicing();
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
  sourceMapper->ResampleToScreenPixelsOff();

  double sourceRange[2];
  ComputeRange(sourceImage, sourceRange);
  sourceProperty->SetInterpolationTypeToLinear();
  sourceProperty->SetColorWindow((sourceRange[1]-sourceRange[0]));
  sourceProperty->SetColorLevel(0.5*(sourceRange[0]+sourceRange[1]));
  sourceProperty->CheckerboardOn();

  sourceActor->SetMapper(sourceMapper);
  sourceActor->SetProperty(sourceProperty);
  sourceActor->SetUserMatrix(sourceMatrix);

  vtkSmartPointer<vtkImageSlice> targetActor =
    vtkSmartPointer<vtkImageSlice>::New();
  vtkSmartPointer<vtkImageResliceMapper> targetMapper =
    vtkSmartPointer<vtkImageResliceMapper>::New();
  vtkSmartPointer<vtkImageProperty> targetProperty =
    vtkSmartPointer<vtkImageProperty>::New();

  targetMapper->SetInput(targetImage);
  targetMapper->SliceAtFocalPointOn();
  targetMapper->SliceFacesCameraOn();
  targetMapper->ResampleToScreenPixelsOff();

  double targetRange[2];
  ComputeRange(targetImage, targetRange);
  targetProperty->SetInterpolationTypeToLinear();
  targetProperty->SetColorWindow((targetRange[1]-targetRange[0]));
  targetProperty->SetColorLevel(0.5*(targetRange[0]+targetRange[1]));

  targetActor->SetMapper(targetMapper);
  targetActor->SetProperty(targetProperty);
  targetActor->SetUserMatrix(targetMatrix);

  vtkSmartPointer<vtkImageStack> imageStack =
    vtkSmartPointer<vtkImageStack>::New();
  imageStack->AddImage(targetActor);
  imageStack->AddImage(sourceActor);

  renderer->AddViewProp(imageStack);
  renderer->SetBackground(0,0,0);

  renderWindow->SetSize(512,512);

  double bounds[6], center[4], tspacing[3];
  int extent[6];
  sourceImage->GetBounds(bounds);
  sourceImage->GetExtent(extent);
  sourceImage->GetSpacing(tspacing);
  center[0] = 0.5*(bounds[0] + bounds[1]);
  center[1] = 0.5*(bounds[2] + bounds[3]);
  center[2] = 0.5*(bounds[4] + bounds[5]);
  center[3] = 1.0;
  sourceMatrix->MultiplyPoint(center, center);

  vtkCamera *camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->SetFocalPoint(center);
  camera->ParallelProjectionOn();
  camera->SetParallelScale(0.5*(bounds[3] - bounds[2]));
  SetViewFromMatrix(renderer, istyle, sourceMatrix, options.coords);
  renderer->ResetCameraClippingRange();

  double checkSpacing = (extent[3] - extent[2] + 7)/7*tspacing[1];
  sourceProperty->SetCheckerboardSpacing(checkSpacing, checkSpacing);

  if (display)
    {
    renderWindow->Render();
    }

  // -------------------------------------------------------
  // prepare for registration

  // get information about the images
  double targetSpacing[3], sourceSpacing[3];
  targetImage->GetSpacing(targetSpacing);
  sourceImage->GetSpacing(sourceSpacing);

  for (int jj = 0; jj < 3; jj++)
    {
    targetSpacing[jj] = fabs(targetSpacing[jj]);
    sourceSpacing[jj] = fabs(sourceSpacing[jj]);
    }

  double minSpacing = sourceSpacing[0];
  if (minSpacing > sourceSpacing[1])
    {
    minSpacing = sourceSpacing[1];
    }
  if (minSpacing > sourceSpacing[2])
    {
    minSpacing = sourceSpacing[2];
    }

  // blur source image with Blackman-windowed sinc
  vtkSmartPointer<vtkImageSincInterpolator> sourceBlurKernel =
    vtkSmartPointer<vtkImageSincInterpolator>::New();
  sourceBlurKernel->SetWindowFunctionToBlackman();

  // reduce the source resolution
  vtkSmartPointer<vtkImageResize> sourceBlur =
    vtkSmartPointer<vtkImageResize>::New();
  sourceBlur->SetInput(sourceImage);
  sourceBlur->SetResizeMethodToOutputSpacing();
  sourceBlur->SetInterpolator(sourceBlurKernel);
  sourceBlur->SetInterpolate(
    interpolatorType != vtkImageRegistration::Nearest);

  // blur target with Blackman-windowed sinc
  vtkSmartPointer<vtkImageSincInterpolator> targetBlurKernel =
    vtkSmartPointer<vtkImageSincInterpolator>::New();
  targetBlurKernel->SetWindowFunctionToBlackman();

  // keep target at full resolution
  vtkSmartPointer<vtkImageResize> targetBlur =
    vtkSmartPointer<vtkImageResize>::New();
  targetBlur->SetInput(targetImage);
  targetBlur->SetResizeMethodToOutputSpacing();
  targetBlur->SetInterpolator(targetBlurKernel);
  targetBlur->SetInterpolate(
    interpolatorType != vtkImageRegistration::Nearest);

  // get the initial transformation
  matrix->DeepCopy(targetMatrix);
  matrix->Invert();
  vtkMatrix4x4::Multiply4x4(matrix, sourceMatrix, matrix);

  // set up the registration
  vtkSmartPointer<vtkImageRegistration> registration =
    vtkSmartPointer<vtkImageRegistration>::New();
  registration->SetTargetImageInputConnection(targetBlur->GetOutputPort());
  registration->SetSourceImageInputConnection(sourceBlur->GetOutputPort());
  registration->SetSourceImageRange(sourceRange);
  registration->SetTargetImageRange(targetRange);
  registration->SetTransformDimensionality(options.dimensionality);
  registration->SetTransformType(options.transform);
  registration->SetMetricType(options.metric);
  registration->SetInterpolatorType(interpolatorType);
  registration->SetJointHistogramSize(numberOfBins,numberOfBins);
  registration->SetMetricTolerance(1e-4);
  registration->SetTransformTolerance(transformTolerance);
  if (xfminput)
    {
    registration->SetInitializerTypeToNone();
    }
  else
    {
    registration->SetInitializerTypeToCentered();
    }
  registration->Initialize(matrix);

  // -------------------------------------------------------
  // make a timer
  vtkSmartPointer<vtkTimerLog> timer =
    vtkSmartPointer<vtkTimerLog>::New();
  double startTime = timer->GetUniversalTime();
  double lastTime = startTime;

  // -------------------------------------------------------
  // do the registration

  // the registration starts at low-resolution
  double blurFactor = initialBlurFactor;
  // two stages for each resolution:
  // first without interpolation, and then with interpolation
  int level = 0;
  int stage = 0;
  // will be set to "true" when registration is initialized
  bool initialized = false;

  while (level < 3 && options.maxiter[level] > 0)
    {
    registration->SetMaximumNumberOfIterations(options.maxiter[level]);
    if (stage == 0)
      {
      registration->SetInterpolatorTypeToNearest();
      registration->SetTransformTolerance(minSpacing*blurFactor);
      }
    else
      {
      registration->SetInterpolatorType(interpolatorType);
      registration->SetTransformTolerance(transformTolerance*blurFactor);
      }
    if (blurFactor < 1.1)
      {
      // full resolution: no blurring or resampling
      sourceBlur->SetInterpolator(0);
      sourceBlur->InterpolateOff();
      sourceBlur->SetOutputSpacing(sourceSpacing);
      sourceBlur->Update();

      targetBlur->SetInterpolator(0);
      sourceBlur->InterpolateOff();
      targetBlur->SetOutputSpacing(targetSpacing);
      targetBlur->Update();
      }
    else
      {
      // reduced resolution: set the blurring
      double spacing[3];
      for (int j = 0; j < 3; j++)
        {
        spacing[j] = blurFactor*minSpacing;
        if (spacing[j] < sourceSpacing[j])
          {
          spacing[j] = sourceSpacing[j];
          }
        }

      sourceBlurKernel->SetBlurFactors(
        spacing[0]/sourceSpacing[0],
        spacing[1]/sourceSpacing[1],
        spacing[2]/sourceSpacing[2]);

      sourceBlur->SetOutputSpacing(spacing);
      sourceBlur->Update();

      targetBlurKernel->SetBlurFactors(
        blurFactor*minSpacing/targetSpacing[0],
        blurFactor*minSpacing/targetSpacing[1],
        blurFactor*minSpacing/targetSpacing[2]);

      targetBlur->Update();
      }

    if (initialized)
      {
      // re-initialize with the matrix from the previous step
      registration->SetInitializerTypeToNone();
      matrix->DeepCopy(registration->GetTransform()->GetMatrix());
      }

    registration->Initialize(matrix);

    initialized = true;

    while (registration->Iterate())
      {
      // registration->UpdateRegistration();
      // will iterate until convergence or failure

      targetMatrix->DeepCopy(registration->GetTransform()->GetMatrix());
      targetMatrix->Invert();
      vtkMatrix4x4::Multiply4x4(
        sourceMatrix, targetMatrix, targetMatrix);
      targetMatrix->Modified();
      if (display)
        {
        interactor->Render();
        }
      }

    double newTime = timer->GetUniversalTime();
    double blurSpacing[3];
    sourceBlur->GetOutputSpacing(blurSpacing);
    double minBlurSpacing = VTK_DOUBLE_MAX;
    for (int kk = 0; kk < 3; kk++)
      {
      if (blurSpacing[kk] < minBlurSpacing)
        {
        minBlurSpacing = blurSpacing[kk];
        }
      }

    if (!options.silent)
      {
      cout << (stage ? "interpolated " : "non-interp'd ")
           << minBlurSpacing << " mm took "
           << (newTime - lastTime) << "s and "
           << registration->GetNumberOfEvaluations() << " evaluations" << endl;
      lastTime = newTime;
      }

    // prepare for next iteration
    if (stage == 1 || interpolatorType == vtkImageRegistration::Nearest)
      {
      level++;
      blurFactor /= 2.0;
      }
    if (interpolatorType != vtkImageRegistration::Nearest)
      {
      stage = (stage + 1) % 2;
      }
    }

  if (!options.silent)
    {
    cout << "registration took " << (lastTime - startTime) << "s" << endl;
    }

  // -------------------------------------------------------
  // write the output matrix
  if (xfmfile)
    {
    if (!options.silent)
      {
      cout << "Writing transform file: " << xfmfile << endl;
      }

    vtkMatrix4x4 *rmatrix = registration->GetTransform()->GetMatrix();
    vtkSmartPointer<vtkMatrix4x4> wmatrix =
      vtkSmartPointer<vtkMatrix4x4>::New();
    wmatrix->DeepCopy(sourceMatrix);
    wmatrix->Invert();
    vtkMatrix4x4::Multiply4x4(rmatrix, wmatrix, wmatrix);
    vtkMatrix4x4::Multiply4x4(originalTargetMatrix, wmatrix, wmatrix);

    WriteMatrix(wmatrix, xfmfile, center);
    }

  // -------------------------------------------------------
  // capture a screen shot
  if (options.screenshot)
    {
    WriteScreenshot(renderWindow, options.screenshot);
    }

  // -------------------------------------------------------
  // write the output file
  if (imagefile)
    {
    if (!options.silent)
      {
      cout << "Writing transformed image: " << imagefile << endl;
      }

    vtkSmartPointer<vtkImageSincInterpolator> sincInterpolator =
      vtkSmartPointer<vtkImageSincInterpolator>::New();
    sourceBlurKernel->SetWindowFunctionToBlackman();

    vtkSmartPointer<vtkImageReslice> reslice =
      vtkSmartPointer<vtkImageReslice>::New();
    reslice->SetInformationInput(sourceImage);
    reslice->SetInput(targetImage);
    switch (options.interpolator)
      {
      case vtkImageRegistration::Nearest:
        reslice->SetInterpolationModeToNearestNeighbor();
        break;
      case vtkImageRegistration::Linear:
        reslice->SetInterpolationModeToLinear();
        break;
      case vtkImageRegistration::Cubic:
        reslice->SetInterpolationModeToCubic();
        break;
      case vtkImageRegistration::Sinc:
        reslice->SetInterpolator(sincInterpolator);
        break;
      }
    reslice->SetInformationInput(sourceImage);
    reslice->SetResliceTransform(
      registration->GetTransform());
    reslice->Update();

    WriteImage(sourceReader, targetReader,
      reslice->GetOutput(), sourceMatrix, imagefile, options.coords);
    }

  // -------------------------------------------------------
  // allow user to interact

  if (options.display)
    {
    interactor->Start();
    }

  return 0;
}
