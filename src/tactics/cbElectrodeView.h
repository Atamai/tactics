/*=========================================================================
  Program: Cerebra
  Module:  cbElectrodeView.h

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

#ifndef CBELECTRODEVIEW_H
#define CBELECTRODEVIEW_H

#include "cbMainWindow.h"

#include "LeksellFiducial.h"
#include "cbProbe.h"

#include "vtkDataManager.h"
#include "vtkFollowerPlane.h"
#include "vtkImageSliceCollection.h"
#include "vtkImageStack.h"
#include "vtkPlane.h"
#include "vtkSmartPointer.h"

#include <QCursor>
#include <QObject>
#include <QString>
#include <QStringList>

#include <vector>

class vtkActor;
class vtkActorCollection;
class vtkCamera;
class vtkCornerAnnotation;
class vtkDynamicViewFrame;
class vtkFiducialPointsTool;
class vtkFollower;
class vtkImageData;
class vtkImageSlice;
class vtkImageViewPane;
class vtkMatrix4x4;
class vtkPlaneCollection;
class vtkPolyData;
class vtkPushPlaneTool;
class vtkRenderer;
class vtkRotateCameraTool;
class vtkSliceImageTool;

class cbElectrodeView : public cbMainWindow
{
  Q_OBJECT

public:
  cbElectrodeView(vtkDataManager *dataManager, QWidget *parent = 0);
  ~cbElectrodeView();

  //! Tool cursor type
  enum cursortool {
    Pan = 1,
    Zoom = 2,
    Rotate = 3,
    Winlev = 4,
    Plane = 5,
    Pick = 6,
  };

  enum ImageLayers {
    kMR = 0,
    kCT = 1,
  };

public slots:
  //! Incoming signal to set the CT layer opacity.
  void SetCTOpacity(double o);

  //! Incoming signal to display the secondary series.
  void DisplayCTData(vtkDataManager::UniqueKey k);

  //! Incoming signal to display the primary series.
  void displayData(vtkDataManager::UniqueKey k);

  //! Bind actions to the tool cursor.
  void bindToolCursorAction(int cursortool, int mousebutton);

  //! Reset all view orientations.
  void resetViewOrientations();

  //! Incoming signal to maximize/minimize the surface renderer.
  void ToggleMaximizeSurface();

  //! Incoming signal to display the frame polydata.
  void displayLeksellFrame(vtkPolyData *frame, vtkMatrix4x4 *transform);

  //! Incoming signal to display the tags that were loaded.
  void displayTags(vtkPolyData *points, vtkMatrix4x4 *transform);

  //! Incoming signal to display the surface rendering.
  void displaySurfaceVolume(vtkDataManager::UniqueKey);

  //! Set the Left/Right labels in the 2D panes
  void PositionLRLabelsIn2DPanes();

  //! CRUD-interface for probes
  void CreateProbeCallback(cbProbe p);
  void UpdateProbeCallback(int index, cbProbe p);
  void DestroyProbeCallback(int index);

  //! Incoming signal to activate pick tool.
  void InitiatePlaceProbeCallback();

  //! Incoming signal to visualize the frame actor
  void EnableFrameVisualization();
  //! Incoming signal to disable the frame actor
  void DisableFrameVisualization();

  //! Incoming signal to visualize the tag actor
  void EnableTagVisualization();
  //! Incoming signal to disable the tag actor
  void DisableTagVisualization();

  //! Incoming signal to toggle visualization of the plane.
  void ToggleAxialVisualization(int s);
  void ToggleSagittalVisualization(int s);
  void ToggleCoronalVisualization(int s);

  //! Incoming signal to toggle probe visualization mode.
  void ToggleProbeVisualizationMode(int s);

  //! Incoming signal to toggle help annotation.
  void ToggleHelpAnnotations(int s);

  //! Incoming signal to toggle the patient information annotation.
  void TogglePatientAnnotations(int s);

  //! Incoming signal to save a screenshot of the render window.
  void ExportScreenshot();

private slots:
  //! Action to perform when the 'Open' file menu option is activated.
  void Open();

  //! Action to perform when the 'Save' option is activated.
  void Save();

  //! Action to perform when the 'Save As...' option is activated.
  void SaveAs();

  //! Action to perform when the 'Open secondary series...' option is activated.
  void OpenCT();

  //! Action to perform when the 'About' option is activated.
  void About();

signals:
  //! Outgoing signal requesting controller to open CT data.
  void OpenCTData(const QStringList& files);

  //! Outgoing signal (overloaded) requesting the secondary series to be opened.
  void OpenCTData(const QStringList& files, vtkMatrix4x4 *matrix);

  //! Outgoing signal to get manager on last stage.
  void jumpToLastStage();

  //! Outgoing signal to create a new probe.
  void CreateProbeRequest(double x, double y, double z,
                          double a, double d, double depth,
                          std::string n, std::string s);

  //! Outgoing signal to save the probe plan to disk.
  void SavePlanToFile(std::string path);

  //! Outgoing signal to clear the current probe plan.
  void ClearCurrentPlan();

  //! Outgoing signal to open the image at path.
  void OpenImage(const QStringList& files);

private:
  //! File path to the save file.
  std::string SaveFile;

  //! Caching for previous and current tool.
  cursortool lastTool;
  QCursor lastCursor;
  cursortool currentTool;

  //! Outer render frame.
  vtkDynamicViewFrame *outerFrame;
  //! The planar slicing widget pane.
  vtkImageViewPane *planar;
  //! The surface rendering pane.
  vtkRenderer *surface;
  //! The axial view pane.
  vtkImageViewPane *axialPane;
  //! The sagittal view pane.
  vtkImageViewPane *sagittalPane;
  //! The coronal view pane.
  vtkImageViewPane *coronalPane;

  //! The frame coordinate matrix.
  vtkMatrix4x4 *frameTransform;

  //! The tag coordinate matrix.
  vtkMatrix4x4 *tagTransform;

  //! The mouse-button help annotation.
  vtkCornerAnnotation *helpAnnotation;

  //! Label followers for the axial/coronal view panes.
  vtkFollower *leftAxialFollower;
  vtkFollower *leftCoronalFollower;
  vtkFollower *rightAxialFollower;
  vtkFollower *rightCoronalFollower;

  //! The Leksell Frame actor.
  vtkActor *Frame;

  //! The tag actor.
  vtkActor *Tags;

  //! Toolcursor tools for the application. All have callbacks associated.
  vtkPushPlaneTool *planeTool;
  vtkRotateCameraTool *rotateTool;
  vtkFiducialPointsTool *pickTool;
  vtkSliceImageTool *scrollTool;

  //! Callbacks for the relevant tools.
  void FocalPointSliceCallback(vtkObject *o, unsigned long, void *);
  void RotateVolumeCallback(vtkObject *o, unsigned long, void *);
  void ProbePlacementCallback(vtkObject *o, unsigned long, void *);
  void PaneScrollCallback(vtkObject *o, unsigned long, void *);

  //! The key to use to grab the image data from the manager.
  vtkDataManager::UniqueKey dataKey;
  vtkDataManager::UniqueKey ctKey;

  //! Set the provided renderer's view up/right/etc to match a specific view.
  void SetOrientationToAxial(vtkRenderer *r);
  void SetOrientationToSagittal(vtkRenderer *r);
  void SetOrientationToCoronal(vtkRenderer *r);
  void FixCameraPosition(vtkCamera *c);

  //! The viewports.
  double surfacePort[4];
  double planarPort[4];

  //! Will Minimize or Maximize the surface rendering view.
  void MinimizeSurfaceRenderer();
  void MaximizeSurfaceRenderer();

  //! Set whether or not the application has been saved.
  void SetSavedState(bool s);
  bool SavedState;

  //! Determines the recommended save/open path.
  QString GetPlanFolder();

  //! Remembers the save/open path.
  void SetPlanFolder(const QString& p);

  //! Internal class to represent orthogonal slices.
  class Slice
  {
  public:
    Slice(const double orientation[3]);
    virtual ~Slice();

    vtkSmartPointer<vtkImageStack> Stack;
    vtkSmartPointer<vtkActor> Border;
    vtkSmartPointer<vtkActor> Intersect;
    vtkSmartPointer<vtkPlane> WorldPlane;
    vtkSmartPointer<vtkFollowerPlane> DataPlane;
    double Orientation[3];
  };

  //! Collection of orthogonal slices.
  std::vector<Slice> Slices;

  //! The image property to use for layering.
  vtkImageProperty *ImageProperty;
  vtkImageProperty *CTProperty;

  //! Collection of probe actors for rendering.
  vtkActorCollection *Probes;

  //! Cache of the latest probe selection index.
  int SelectedIndex;

  vtkCornerAnnotation *MetaAnnotation;

  //! Returns a specific image slice.
  vtkImageSlice *GetImageSlice(int layer, int orientation);

  //! Adds an image layer to the slices, useful for adding CT or mask layers.
  void AddLayer(vtkImageProperty *p);

  //! Builds the basic probe polydata into the argument object.
  void buildProbeMarker(vtkPolyData *probeData, cbProbe p);

  //! Adds a string to a renderer annotation in a specified corner.
  void addRendererLabel(vtkRenderer *r, const char *str, int corner);

  //! Adds the necessary data to the respective view panes.
  void addDataToPanes(vtkImageData *d, vtkMatrix4x4 *m, vtkImageProperty *p);

  //! Creates the planar slicing view.
  void createPlanarView(vtkImageData *data, vtkMatrix4x4 *matrix,
                        vtkImageProperty *property);

  //! Compute an appropriate window/level range for a data set.
  static void ComputePercentileRange(vtkImageData *data, double percentile,
                                     double range[2]);

  //! Snap two view vectors to the closest image volume axes.
  /* The matrix that is provided must be the matrix that goes from
   * image data coordinates to world coordinates.  The input vectors
   * and output vectors are all in world coordinates.  Internally,
   * the vectors are transformed to data coords, snapped to the axes,
   * and then transformed back to world coordinates.
   */
  static void SnapViewVectorsToVolume(vtkMatrix4x4 *matrix,
    const double originalViewRight[3], const double originalViewUp[3],
    double fixedViewRight[3], double fixedViewUp[3]);

  //! Clears the render window, and reload visualization assets.
  void ClearCurrentWorkSpace();

  //! Destroys all member variables.
  void DestroyMembers();

  //! Create the file menus and connect actions.
  void CreateMenu();

  //! Create and bind toolbar tools to the input devices.
  void CreateAndBindTools();

  //! Create the rect/frame/pane hierarchy.
  void CreateAndConfigurePanes();

  //! Create the frame renderer assets.
  void CreateFrameObjects();

  //! Create the tag assets.
  void CreateTagObjects();

  //! Create the actor collection for probes.
  void CreatePlanVisualization();

  //! Create all labels and annotations present in the view.
  void CreateLabelsAndAnnotations();

  //! Prompt the user with the ability to save, discard, or cancel.
  int PromptForSave();

  //! Capturing of the close event to ensure files are saved or discarded.
  virtual void closeEvent(QCloseEvent *e);

  //! Append data to the metadata annotation.
  void AppendMetaData(const std::string text);

  //! Set the clipping range for the specified camera.
  void SetClippingRange(vtkCamera *c);
};

#endif /* end of include guard: CBELECTRODEVIEW_H */
