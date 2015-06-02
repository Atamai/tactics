#!/usr/bin/env python

import math
import sys
import vtk
import toolcursor

from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# The images are img1 and img2, or the same if only 1 image
img1 = 0
img2 = 0

filenames = []

if len(sys.argv) > 1:
    filenames.extend(sys.argv[1:])
    if len(sys.argv) > 2:
        img2 = 1
else:
    print "Usage: python " + sys.argv[0] + " sbrain.mnc sbrain_stenciled.mnc"
    print
    sys.exit(0)

print "Instructions: "
print " 1) Use 'Tab' to cycle through different views"
print " 2) Use 'Space' to drop a fiducial"
print " 3) Use 'Delete/Backspace' to delete last fiducial"

#---------------------------------------------------------
# read the volume
readers = []
for filename in filenames:
    reader = vtk.vtkMINCImageReader()
    reader.SetFileName(filename)
    readers.append(reader)

#reader = vtk.vtkImageReader2()
#reader.SetDataExtent(0,63,0,63,0,92)
#reader.SetFileNameSliceOffset(1)
#reader.SetDataScalarTypeToUnsignedShort()
#reader.SetDataByteOrderToLittleEndian()
#reader.SetFilePrefix(str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
#reader.SetDataSpacing(3.2,3.2,1.5)

#---------------------------------------------------------
# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
istyle = vtk.vtkInteractorStyleImage()
iren = vtk.vtkRenderWindowInteractor()
iren.SetInteractorStyle(istyle)
iren.SetRenderWindow(renWin)

#---------------------------------------------------------
# arrange the panes in a 1 + 3 gridding
viewports = [(0.00, 0.00, 0.67, 1.00),
             (0.67, 0.67, 1.00, 1.00),
             (0.67, 0.33, 1.00, 0.67),
             (0.67, 0.00, 1.00, 0.33)]

viewports = [(0.00, 0.00, 1.00, 1.00),
             (0.67, 0.67, 1.00, 1.00),
             (0.67, 0.33, 1.00, 0.67),
             (0.67, 0.00, 1.00, 0.33)]

ren.SetViewport(viewports[0])
ren.GetActiveCamera().SetWindowCenter(0.33, 0.0)

xren = vtk.vtkRenderer()
xren.EraseOff()
xren.SetViewport(viewports[1])
xren.GetActiveCamera().ParallelProjectionOn()
renWin.AddRenderer(xren)

yren = vtk.vtkRenderer()
yren.EraseOff()
yren.SetViewport(viewports[2])
yren.GetActiveCamera().ParallelProjectionOn()
renWin.AddRenderer(yren)

zren = vtk.vtkRenderer()
zren.EraseOff()
zren.SetViewport(viewports[3])
zren.GetActiveCamera().ParallelProjectionOn()
renWin.AddRenderer(zren)

#xren.SetBackground(1,0,0)
#yren.SetBackground(0,1,0)
#zren.SetBackground(0,0,1)

readers[img1].Update()
bounds = readers[img1].GetOutput().GetBounds()
x = 0.5*(bounds[0] + bounds[1])
y = 0.5*(bounds[2] + bounds[3])
z = 0.5*(bounds[4] + bounds[5])

xren.GetActiveCamera().SetParallelScale(100)
xren.GetActiveCamera().SetFocalPoint(x,y,z)
xren.GetActiveCamera().SetPosition(-200,y,z)
xren.GetActiveCamera().SetClippingRange(10,400)
xren.GetActiveCamera().SetViewUp(0,0,1)

yren.GetActiveCamera().SetParallelScale(100)
yren.GetActiveCamera().SetFocalPoint(x,y,z)
yren.GetActiveCamera().SetPosition(x,-200,z)
yren.GetActiveCamera().SetClippingRange(10,400)
yren.GetActiveCamera().SetViewUp(0,0,1)

zren.GetActiveCamera().SetParallelScale(100)
zren.GetActiveCamera().SetFocalPoint(x,y,z)
zren.GetActiveCamera().SetPosition(x,y,200)
zren.GetActiveCamera().SetClippingRange(10,400)
zren.GetActiveCamera().SetViewUp(0,1,0)

renWin.SetSize(800,600)

#---------------------------------------------------------
# For rescaling second volume to the first volume
rescaleSlope1 = readers[img1].GetRescaleSlope()
rescaleIntercept1 = readers[img1].GetRescaleIntercept()

rescaleSlope2 = readers[img2].GetRescaleSlope()
rescaleIntercept2 = readers[img2].GetRescaleIntercept()

# Here is the mapping of 2nd reader to 1st reader
slope2 = rescaleSlope2/rescaleSlope1
intercept2 = rescaleIntercept2 - slope2*rescaleIntercept1
slope2 = 2.5
intercept2 = -18

class VolumeLOD(vtk.vtkLODProp3D):

    def __init__(self):

        #---------------------------------------------------------
        # prep the volume for rendering at 128x128x128

        self.ShiftScale = vtk.vtkImageShiftScale()
        self.ShiftScale.SetOutputScalarTypeToUnsignedShort()

        self.Reslice = vtk.vtkImageReslice()
        self.Reslice.SetInput(self.ShiftScale.GetOutput())
        self.Reslice.SetOutputExtent(0, 127, 0, 127, 0, 127)
        self.Reslice.SetInterpolationModeToCubic()

        #---------------------------------------------------------
        # set up the volume rendering

        self.Mapper = vtk.vtkVolumeRayCastMapper()
        self.Mapper.SetInput(self.Reslice.GetOutput())
        volumeFunction = vtk.vtkVolumeRayCastCompositeFunction()
        self.Mapper.SetVolumeRayCastFunction(volumeFunction)

        self.Mapper3D = vtk.vtkVolumeTextureMapper3D()
        self.Mapper3D.SetInput(self.Reslice.GetOutput())

        self.Mapper2D = vtk.vtkVolumeTextureMapper2D()
        self.Mapper2D.SetInput(self.Reslice.GetOutput())

        self.Color = vtk.vtkColorTransferFunction()
        self.Color.AddRGBPoint(0,0.0,0.0,0.0)
        self.Color.AddRGBPoint(180,0.3,0.1,0.2)
        self.Color.AddRGBPoint(1200,1.0,0.7,0.6)
        self.Color.AddRGBPoint(2500,1.0,1.0,0.9)

        self.ScalarOpacity = vtk.vtkPiecewiseFunction()
        self.ScalarOpacity.AddPoint(0,0.0)
        self.ScalarOpacity.AddPoint(180,0.0)
        self.ScalarOpacity.AddPoint(1200,0.2)
        self.ScalarOpacity.AddPoint(2500,0.8)

        self.GradientOpacity = vtk.vtkPiecewiseFunction()
        self.GradientOpacity.AddPoint(0,0.0)
        self.GradientOpacity.AddPoint(90,0.5)
        self.GradientOpacity.AddPoint(100,1.0)

        self.Property = vtk.vtkVolumeProperty()
        self.Property.SetColor(self.Color)
        self.Property.SetScalarOpacity(self.ScalarOpacity)
        #self.Property.SetGradientOpacity(self.GradientOpacity)
        self.Property.SetInterpolationTypeToLinear()
        self.Property.ShadeOff()
        self.Property.SetAmbient(0.6)
        self.Property.SetDiffuse(0.6)
        self.Property.SetSpecular(0.1)

        self.lod2D = self.AddLOD(self.Mapper2D, self.Property, 0.01)
        self.lod3D = self.AddLOD(self.Mapper3D, self.Property, 0.1)
        self.lodRC = self.AddLOD(self.Mapper, self.Property, 1.0)
        self.SetLODLevel(self.lod2D, 2.0)
        self.SetLODLevel(self.lod3D, 1.0)
        self.SetLODLevel(self.lodRC, 0.0)

        # disable ray casting
        #self.DisableLOD(self.lod3D)
        #self.DisableLOD(self.lod2D)
        self.DisableLOD(self.lodRC)

    def SetEnabled(self, mode, enabled):
        if enabled:
            if mode == '2D':
                self.EnableLOD(self.lod2D)
            if mode == '3D':
                self.EnableLOD(self.lod3D)
            if mode == 'RC':
                self.EnableLOD(self.lodRC)
        else:
            if mode == '2D':
                self.DisableLOD(self.lod2D)
            if mode == '3D':
                self.DisableLOD(self.lod3D)
            if mode == 'RC':
                self.DisableLOD(self.lodRC)

    def SetInputConnection(self, algorithmOutput):
        self.ShiftScale.SetInputConnection(algorithmOutput)
        self.UpdatePipelineIvars()

    def SetInput(self, input):
        self.ShiftScale.SetInput(input)
        self.UpdatePipelineIvars()

    def GetInput(self):
        return self.ShiftScale.GetInput()

    def UpdatePipelineIvars(self):
        self.ShiftScale.GetOutput().UpdateInformation()
        origin = self.ShiftScale.GetOutput().GetOrigin()
        spacing = self.ShiftScale.GetOutput().GetSpacing()
        extent = self.ShiftScale.GetOutput().GetWholeExtent()
        newspacing = (spacing[0]*(extent[1] - extent[0])/127.0,
                      spacing[1]*(extent[3] - extent[2])/127.0,
                      spacing[2]*(extent[5] - extent[4])/127.0)
        self.Reslice.SetOutputOrigin(origin)
        self.Reslice.SetOutputSpacing(newspacing)

    def SetCroppingRegionFlags(self, cropflags):
        self.Mapper.CroppingOn()
        self.Mapper3D.CroppingOn()
        self.Mapper2D.CroppingOn()
        self.Mapper.SetCroppingRegionFlags(cropflags)
        self.Mapper3D.SetCroppingRegionFlags(cropflags)
        self.Mapper2D.SetCroppingRegionFlags(cropflags)

    def SetCroppingRegion(self, cropping):
        self.Mapper.CroppingOn()
        self.Mapper3D.CroppingOn()
        self.Mapper2D.CroppingOn()
        self.Mapper.SetCroppingRegionPlanes(cropping)
        self.Mapper3D.SetCroppingRegionPlanes(cropping)
        self.Mapper2D.SetCroppingRegionPlanes(cropping)

    def CroppingOn(self):
        self.Mapper.CroppingOn()
        self.Mapper3D.CroppingOn()
        self.Mapper2D.CroppingOn()

    def CroppingOff(self):
        self.Mapper.CroppingOff()
        self.Mapper3D.CroppingOff()
        self.Mapper2D.CroppingOff()

    def AddClippingPlane(self, plane):
        self.Mapper.AddClippingPlane(plane)
        self.Mapper3D.AddClippingPlane(plane)
        self.Mapper2D.AddClippingPlane(plane)

    def RemoveAllClippingPlanes(self):
        self.Mapper.RemoveAllClippingPlanes()
        self.Mapper3D.RemoveAllClippingPlanes()
        self.Mapper2D.RemoveAllClippingPlanes()

# original bounds: (-90.0, 90.0, -126.0, 90.0, -72.0, 108.0)
cropping = (0.0, 90.0, -126.0, 0.0, -72.0, 0.0)

# there are 27 blocks that can be cropped, just like a Rubik's cube
cropblock = (0,2,2)
cropblockbit = (1 << (cropblock[2]*9 + cropblock[1]*3 + cropblock[0]))
cropflags = (0x07ffffff & ~cropblockbit);

volume = VolumeLOD()
volume.SetInputConnection(readers[img1].GetOutputPort())
volume.SetCroppingRegionFlags(cropflags)
volume.SetCroppingRegion(cropping)

cropOutlineSource = vtk.vtkVolumeOutlineSource()
cropOutlineSource.SetVolumeMapper(volume.Mapper)
cropOutlineSource.GenerateScalarsOn()
cropOutlineSource.SetActivePlaneId(0)

cropOutlineMapper = vtk.vtkPolyDataMapper()
cropOutlineMapper.SetInputConnection(cropOutlineSource.GetOutputPort())

cropOutline = vtk.vtkActor()
cropOutline.SetMapper(cropOutlineMapper)

#---------------------------------------------------------
# Do the surface rendering
skinExtractor = vtk.vtkMarchingCubes()
skinExtractor.SetInputConnection(readers[img2].GetOutputPort())
skinExtractor.SetValue(0,1) # 500)

skinSmooth = vtk.vtkSmoothPolyDataFilter()
skinSmooth.SetInputConnection(skinExtractor.GetOutputPort())

skinDecimate = vtk.vtkDecimatePro()
skinDecimate.SetInputConnection(skinSmooth.GetOutputPort())
skinDecimate.SetFeatureAngle(180)
skinDecimate.PreserveTopologyOn()
skinDecimate.SetTargetReduction(0.2)

skinNormals = vtk.vtkPolyDataNormals()
skinNormals.SetInputConnection(skinDecimate.GetOutputPort())
skinNormals.SetFeatureAngle(60.0)

skinStripper = vtk.vtkStripper()
skinStripper.SetMaximumLength(10)
skinStripper.SetInputConnection(skinNormals.GetOutputPort())

skinLocator = vtk.vtkCellLocator()
skinLocator.SetDataSet(skinStripper.GetOutput())
skinLocator.LazyEvaluationOn()

skinMapper = vtk.vtkPolyDataMapper()
skinMapper.SetInputConnection(skinStripper.GetOutputPort())
skinMapper.ScalarVisibilityOff()

skinProperty = vtk.vtkProperty()
skinProperty.SetColor(1.0,1.0,0.9)

skin = vtk.vtkActor()
skin.SetMapper(skinMapper)
skin.SetProperty(skinProperty)

#---------------------------------------------------------
# Create an image actor
table = vtk.vtkLookupTable()
table.SetRange(intercept2, 2000*slope2 + intercept2)
table.SetRange(170, 2500)
table.SetRampToLinear()
table.SetValueRange(0,1)
table.SetHueRange(0,0)
table.SetSaturationRange(0,0)
table.Build()
table.SetTableValue(0, 0.0, 0.0, 0.0, 0.0)
#for i in range(1, 5):
#    table.SetTableValue(i, i/255.0, i/255.0, i/255.0, i/5.0)

table2 = vtk.vtkLookupTable()
table2.SetRange(intercept2, 2000*slope2 + intercept2)
table2.SetRange(170, 2500)
table2.SetRampToLinear()
table2.SetValueRange(0,1)
table2.SetHueRange(0,0)
table2.SetSaturationRange(0,0)
table2.Build()

mapToColors = vtk.vtkImageMapToColors()
mapToColors.SetInputConnection(readers[img1].GetOutputPort())
mapToColors.SetLookupTable(table)
mapToColors.GetOutput().Update()

mapToColors2 = vtk.vtkImageMapToColors()
mapToColors2.SetInputConnection(readers[img1].GetOutputPort())
mapToColors2.SetLookupTable(table2)
mapToColors2.GetOutput().Update()

extent = mapToColors.GetOutput().GetWholeExtent()
xslice = int((extent[0] + extent[1])/2)
yslice = int((extent[2] + extent[3])/2)
zslice = int((extent[4] + extent[5])/2)

imageActorX = vtk.vtkImageActor()
imageActorX.PickableOff()
#imageActorX.SetOpacity(253/255.0)
imageActorX.SetInput(mapToColors.GetOutput())
imageActorX.SetDisplayExtent(xslice, xslice,
                             extent[2], extent[3],
                             extent[4], extent[5])

imageActorY = vtk.vtkImageActor()
imageActorY.PickableOff()
#imageActorY.SetOpacity(253/255.0)
imageActorY.SetInput(mapToColors.GetOutput())
imageActorY.SetDisplayExtent(extent[0], extent[1],
                             yslice, yslice,
                             extent[4], extent[5])

imageActorZ = vtk.vtkImageActor()
imageActorZ.PickableOff()
#imageActorZ.SetOpacity(253/255.0)
imageActorZ.SetInput(mapToColors.GetOutput())
imageActorZ.SetDisplayExtent(extent[0], extent[1],
                             extent[2], extent[3],
                             zslice, zslice)

imageActorX2 = vtk.vtkImageActor()
imageActorX2.SetInput(mapToColors2.GetOutput())
imageActorX2.SetDisplayExtent(xslice, xslice,
                             extent[2], extent[3],
                             extent[4], extent[5])

imageActorY2 = vtk.vtkImageActor()
imageActorY2.SetInput(mapToColors2.GetOutput())
imageActorY2.SetDisplayExtent(extent[0], extent[1],
                             yslice, yslice,
                             extent[4], extent[5])

imageActorZ2 = vtk.vtkImageActor()
imageActorZ2.SetInput(mapToColors2.GetOutput())
imageActorZ2.SetDisplayExtent(extent[0], extent[1],
                             extent[2], extent[3],
                             zslice, zslice)


#---------------------------------------------------------
# make a transform and some clipping planes
transform = vtk.vtkTransform()
transform.RotateWXYZ(-20,0.0,-0.7,0.7)

#volume.SetUserTransform(transform)
#skin.SetUserTransform(transform)
#imageActor.SetUserTransform(transform)

#c = volume.GetCenter()
c = (0.0, 0.0, 0.0)

volumeClip = vtk.vtkPlane()
volumeClip.SetNormal(1,0,0)
volumeClip.SetOrigin(c[0],c[1],c[2])

cropping = volume.Mapper.GetCroppingRegionPlanes()

skinClipX = vtk.vtkPlane()
skinClipX.SetNormal(-1,0,0)
skinClipX.SetOrigin(cropping[0],cropping[3],cropping[5])

skinClipY = vtk.vtkPlane()
skinClipY.SetNormal(0,1,0)
skinClipY.SetOrigin(cropping[0],cropping[3],cropping[5])

skinClipZ = vtk.vtkPlane()
skinClipZ.SetNormal(0,0,1)
skinClipZ.SetOrigin(cropping[0],cropping[3],cropping[5])

volume2 = VolumeLOD()
volume2.SetInputConnection(readers[img2].GetOutputPort())

volume2.Color.RemoveAllPoints()
volume2.Color.AddRGBPoint(0,0.0,0.0,0.0)
volume2.Color.AddRGBPoint(180,0.0,0.0,0.0)
volume2.Color.AddRGBPoint(1200,1.0,0.4,0.3)
volume2.Color.AddRGBPoint(2500,1.0,0.9,0.8)
volume2.Property.ShadeOn()

volume2.AddClippingPlane(skinClipX)
volume2.AddClippingPlane(skinClipY)
volume2.AddClippingPlane(skinClipZ)
skinMapper.AddClippingPlane(skinClipX)
skinMapper.AddClippingPlane(skinClipY)
skinMapper.AddClippingPlane(skinClipZ)

#---------------------------------------------------------
fids = vtk.vtkFiducialPointsTool()
points = vtk.vtkPoints()
#points.InsertNextPoint(0,0,0)
fids.SetPoints(points)

#---------------------------------------------------------
#ren.AddViewProp(imageActorX)
#ren.AddViewProp(imageActorY)
#ren.AddViewProp(imageActorZ)
ren.AddViewProp(volume)
ren.AddViewProp(volume2)
#ren.AddViewProp(cropOutline)
#ren.AddViewProp(skin)
ren.AddViewProp(fids.GetActor())

xren.AddViewProp(imageActorX2)
yren.AddViewProp(imageActorY2)
zren.AddViewProp(imageActorZ2)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(c[0],c[1],c[2])
camera.SetPosition(c[0] - 500,c[1] + 100,c[2] + 100)
camera.SetViewUp(0,0,1)

renWin.Render()
ren.RemoveViewProp(volume2)
renWin.Render()

ren.ResetCameraClippingRange()

#---------------------------------------------------------
cursor = vtk.vtkToolCursor()
cursor.BindDefaultActions()
cursor.SetRenderer(ren)
cursor.SetScale(1)

observer = vtk.vtkToolCursorInteractorObserver()
observer.SetToolCursor(cursor)
observer.SetInteractor(iren)
observer.SetEnabled(1)

cyclestate = [0]
#---------------------------------------------------------
# Add an observer to hook the planes to the volume
def OnRender(o, e):
    planes = volume.Mapper.GetCroppingRegionPlanes()

    data = volume.GetInput()
    origin = data.GetOrigin()
    spacing = data.GetSpacing()
    extent = data.GetWholeExtent()

    xslice = int(math.floor((planes[0] - origin[0])/spacing[0] - 0.1))
    yslice = int(math.ceil((planes[3] - origin[1])/spacing[1] + 0.1))
    zslice = int(math.ceil((planes[5] - origin[2])/spacing[2] + 0.1))

    xslice = min(max(xslice, extent[0]), extent[1])
    yslice = min(max(yslice, extent[2]), extent[3])
    zslice = min(max(zslice, extent[4]), extent[5])

    if imageActorX.GetPickable():
        xslice = imageActorX.GetDisplayExtent()[0]
        yslice = imageActorY.GetDisplayExtent()[2]
        zslice = imageActorZ.GetDisplayExtent()[4]

        imageActorX.SetDisplayExtent(xslice, xslice,
                                     extent[2], extent[3],
                                     extent[4], extent[5])

        imageActorY.SetDisplayExtent(extent[0], extent[1],
                                     yslice, yslice,
                                     extent[4], extent[5])

        imageActorZ.SetDisplayExtent(extent[0], extent[1],
                                     extent[2], extent[3],
                                     zslice, zslice)
    else:
        imageActorX.SetDisplayExtent(xslice, xslice,
                                     yslice, extent[3],
                                     zslice, extent[5])

        imageActorY.SetDisplayExtent(extent[0], xslice,
                                     yslice, yslice,
                                     zslice, extent[5])

        imageActorZ.SetDisplayExtent(extent[0], xslice,
                                     yslice, extent[3],
                                     zslice, zslice)

    skinClipX.SetOrigin(planes[0],planes[3],planes[5])
    skinClipY.SetOrigin(planes[0],planes[3],planes[5])
    skinClipZ.SetOrigin(planes[0],planes[3],planes[5])

    imageActorX2.SetDisplayExtent(xslice, xslice,
                                  extent[2], extent[3],
                                  extent[4], extent[5])

    imageActorY2.SetDisplayExtent(extent[0], extent[1],
                                  yslice, yslice,
                                  extent[4], extent[5])

    imageActorZ2.SetDisplayExtent(extent[0], extent[1],
                                  extent[2], extent[3],
                                  zslice, zslice)

    if cyclestate[0] == 1:
        cursor.GetPicker().SetPickCroppingPlanes(0)

ren.AddObserver('StartEvent', OnRender)

#---------------------------------------------------------
# Add keyboard handling
numstates = 5
def OnKeyPress(o, e):
    keysym = o.GetKeySym()

    if keysym == "Tab":
        ren.RemoveViewProp(imageActorX)
        ren.RemoveViewProp(imageActorY)
        ren.RemoveViewProp(imageActorZ)
        ren.RemoveViewProp(volume)
        ren.RemoveViewProp(volume2)
        ren.RemoveViewProp(cropOutline)
        ren.RemoveViewProp(skin)
        ren.RemoveViewProp(fids.GetActor())
        volume.SetEnabled('2D', 1)
        volume.SetEnabled('3D', 1)
        volume2.RemoveAllClippingPlanes()
        table.SetTableValue(0, 0.0, 0.0, 0.0, 1.0)
        mapToColors.UpdateWholeExtent()
        imageActorX.PickableOff()
        imageActorY.PickableOff()
        imageActorZ.PickableOff()
        ren.SetBackground(0.0, 0.0, 0.0)

        cyclestate[0] = (cyclestate[0] + 1) % numstates

        if cyclestate[0] == 0:
            ren.AddViewProp(volume)
            ren.AddViewProp(fids.GetActor())

        if cyclestate[0] == 1:
            volume2.AddClippingPlane(skinClipX)
            volume2.AddClippingPlane(skinClipY)
            volume2.AddClippingPlane(skinClipZ)
            volume.SetEnabled('2D', 0)
            ren.AddViewProp(volume)
            ren.AddViewProp(volume2)
            ren.AddViewProp(fids.GetActor())

        if cyclestate[0] == 2:
            volume2.SetEnabled('2D', 0)
            ren.AddViewProp(volume2)
            ren.AddViewProp(fids.GetActor())

        if cyclestate[0] == 3:
            table.SetTableValue(0, 0.0, 0.0, 0.0, 0.0)
            mapToColors.UpdateWholeExtent()
            ren.AddViewProp(imageActorX)
            ren.AddViewProp(imageActorY)
            ren.AddViewProp(imageActorZ)
            ren.AddViewProp(volume)
            ren.AddViewProp(fids.GetActor())

        if cyclestate[0] == 4:
            ren.SetBackground(0.5, 0.5, 0.5)
            imageActorX.PickableOn()
            imageActorY.PickableOn()
            imageActorZ.PickableOn()
            ren.AddViewProp(imageActorX)
            ren.AddViewProp(imageActorY)
            ren.AddViewProp(imageActorZ)
            #ren.AddViewProp(volume)
            #ren.AddViewProp(volume2)
            #ren.AddViewProp(cropOutline)
            #ren.AddViewProp(skin)

        iren.Render()

    if keysym == "Caps_Lock":
        x = cursor.GetPicker().GetPickCroppingPlanes()
        cursor.GetPicker().SetPickCroppingPlanes(not x)
        iren.Render()

    if keysym in ("space","Space"):
        if (ren.HasViewProp(fids.GetActor()) and
            cursor.GetPicker().GetProp3D()):
          points.InsertNextPoint(cursor.GetPosition())
          fids.SetPoints(points)
          iren.Render()

    if keysym in ("delete", "Delete", "backspace", "BackSpace"):
        n = points.GetNumberOfPoints()
        if n and ren.HasViewProp(fids.GetActor()):
            points.SetNumberOfPoints(n-1)
            fids.SetPoints(points)
            iren.Render()

iren.AddObserver('KeyPressEvent', OnKeyPress)

#---------------------------------------------------------
# Add an observer for when the title bar "Close Window" is pressed.
iren.AddObserver("ExitEvent", lambda o,e: o.TerminateApp())

iren.Start()
