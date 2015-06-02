#!/usr/bin/env python

import math
import sys
import vtk
import toolcursor

from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

if len(sys.argv) > 1:
  filename = sys.argv[1]
else:
  filename = "/beck/data/dgobbi/Atamai/atamai/examples/sbrain.mnc"

#---------------------------------------------------------
# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
istyle = vtk.vtkInteractorStyleTrackballCamera()
iren = vtk.vtkRenderWindowInteractor()
iren.SetInteractorStyle(istyle)
iren.SetRenderWindow(renWin)

#---------------------------------------------------------
# read the volume
reader = vtk.vtkMINCImageReader()
reader.SetFileName(filename)
#reader = vtk.vtkImageReader2()
#reader.SetDataExtent(0,63,0,63,0,92)
#reader.SetFileNameSliceOffset(1)
#reader.SetDataScalarTypeToUnsignedShort()
#reader.SetDataByteOrderToLittleEndian()
#reader.SetFilePrefix(str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
#reader.SetDataSpacing(3.2,3.2,1.5)

#---------------------------------------------------------
# prep the volume for rendering at 128x128x128

shiftScale = vtk.vtkImageShiftScale()
shiftScale.SetInput(reader.GetOutput())
shiftScale.SetOutputScalarTypeToUnsignedShort()

shiftScale.GetOutput().UpdateInformation()
origin = shiftScale.GetOutput().GetOrigin()
spacing = shiftScale.GetOutput().GetSpacing()
extent = shiftScale.GetOutput().GetWholeExtent()
spacing = (spacing[0]*(extent[1] - extent[0])/127.0,
           spacing[1]*(extent[3] - extent[2])/127.0,
           spacing[2]*(extent[5] - extent[4])/127.0)

reslice = vtk.vtkImageReslice()
reslice.SetInput(shiftScale.GetOutput())
reslice.SetOutputExtent(0, 127, 0, 127, 0, 127)
reslice.SetOutputOrigin(origin)
reslice.SetOutputSpacing(spacing)
reslice.SetInterpolationModeToCubic()

#---------------------------------------------------------
# set up the volume rendering

# original bounds: (-90.0, 90.0, -126.0, 90.0, -72.0, 108.0)
cropping = (0.0, 90.0, -126.0, 0.0, -72.0, 0.0)

# there are 27 blocks that can be cropped, just like a Rubik's cube
cropblock = (0,2,2)
cropblockbit = (1 << (cropblock[2]*9 + cropblock[1]*3 + cropblock[0]))
cropflags = (0x07ffffff & ~cropblockbit);

volumeMapper = vtk.vtkVolumeRayCastMapper()
volumeMapper.SetInput(reslice.GetOutput())
volumeFunction = vtk.vtkVolumeRayCastCompositeFunction()
volumeMapper.SetVolumeRayCastFunction(volumeFunction)
volumeMapper.CroppingOn()
volumeMapper.SetCroppingRegionPlanes(cropping)
volumeMapper.SetCroppingRegionFlags(cropflags)

volumeMapper3D = vtk.vtkVolumeTextureMapper3D()
volumeMapper3D.SetInput(reslice.GetOutput())
volumeMapper3D.CroppingOn()
volumeMapper3D.SetCroppingRegionPlanes(cropping)
volumeMapper3D.SetCroppingRegionFlags(cropflags)

volumeMapper2D = vtk.vtkVolumeTextureMapper2D()
volumeMapper2D.SetInput(reslice.GetOutput())
volumeMapper2D.CroppingOn()
volumeMapper2D.SetCroppingRegionPlanes(cropping)
volumeMapper2D.SetCroppingRegionFlags(cropflags)

volumeColor = vtk.vtkColorTransferFunction()
volumeColor.AddRGBPoint(0,0.0,0.0,0.0)
volumeColor.AddRGBPoint(180,0.3,0.1,0.2)
volumeColor.AddRGBPoint(1200,1.0,0.7,0.6)
volumeColor.AddRGBPoint(2500,1.0,1.0,0.9)

volumeScalarOpacity = vtk.vtkPiecewiseFunction()
volumeScalarOpacity.AddPoint(0,0.0)
volumeScalarOpacity.AddPoint(180,0.0)
volumeScalarOpacity.AddPoint(1200,0.2)
volumeScalarOpacity.AddPoint(2500,0.8)

volumeGradientOpacity = vtk.vtkPiecewiseFunction()
volumeGradientOpacity.AddPoint(0,0.0)
volumeGradientOpacity.AddPoint(90,0.5)
volumeGradientOpacity.AddPoint(100,1.0)

volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(volumeColor)
volumeProperty.SetScalarOpacity(volumeScalarOpacity)
#volumeProperty.SetGradientOpacity(volumeGradientOpacity)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOff()
volumeProperty.SetAmbient(0.6)
volumeProperty.SetDiffuse(0.6)
volumeProperty.SetSpecular(0.1)

volume = vtk.vtkLODProp3D()
lod2D = volume.AddLOD(volumeMapper2D, volumeProperty, 0.01)
lod3D = volume.AddLOD(volumeMapper3D, volumeProperty, 0.1)
lodRC = volume.AddLOD(volumeMapper, volumeProperty, 1.0)
volume.SetLODLevel(lod2D, 2.0)
volume.SetLODLevel(lod3D, 1.0)
volume.SetLODLevel(lodRC, 0.0)
# disable ray casting
#volume.DisableLOD(lod3D)
volume.DisableLOD(lodRC)

cropOutlineSource = vtk.vtkVolumeOutlineSource()
cropOutlineSource.SetVolumeMapper(volumeMapper)
cropOutlineSource.GenerateScalarsOn()
cropOutlineSource.SetActivePlaneId(0)

cropOutlineMapper = vtk.vtkPolyDataMapper()
cropOutlineMapper.SetInputConnection(cropOutlineSource.GetOutputPort())

cropOutline = vtk.vtkActor()
cropOutline.SetMapper(cropOutlineMapper)

#---------------------------------------------------------
# Do the surface rendering
skinExtractor = vtk.vtkMarchingCubes()
skinExtractor.SetInputConnection(reader.GetOutputPort())
skinExtractor.SetValue(0,500)

skinNormals = vtk.vtkPolyDataNormals()
skinNormals.SetInputConnection(skinExtractor.GetOutputPort())
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
table.SetRange(0,2000)
table.SetRampToLinear()
table.SetValueRange(0,1)
table.SetHueRange(0,0)
table.SetSaturationRange(0,0)

mapToColors = vtk.vtkImageMapToColors()
mapToColors.SetInputConnection(reader.GetOutputPort())
mapToColors.SetLookupTable(table)
mapToColors.GetOutput().Update()

extent = mapToColors.GetOutput().GetWholeExtent()
xslice = int((extent[0] + extent[1])/2)
yslice = int((extent[2] + extent[3])/2)
zslice = int((extent[4] + extent[5])/2)

imageActorX = vtk.vtkImageActor()
imageActorX.SetInput(mapToColors.GetOutput())
imageActorX.SetDisplayExtent(xslice, xslice,
                             extent[2], extent[3],
                             extent[4], extent[5])

imageActorY = vtk.vtkImageActor()
imageActorY.SetInput(mapToColors.GetOutput())
imageActorY.SetDisplayExtent(extent[0], extent[1],
                             yslice, yslice,
                             extent[4], extent[5])

imageActorZ = vtk.vtkImageActor()
imageActorZ.SetInput(mapToColors.GetOutput())
imageActorZ.SetDisplayExtent(extent[0], extent[1],
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
volumeClip.SetNormal(0.707,0.707,0)
volumeClip.SetOrigin(c[0],c[1],c[2])

skinClip = vtk.vtkPlane()
skinClip.SetNormal(0,0,1)
skinClip.SetOrigin(c[0],c[1],c[2])

volumeMapper.AddClippingPlane(volumeClip)
volumeMapper2D.AddClippingPlane(volumeClip)
volumeMapper3D.AddClippingPlane(volumeClip)
skinMapper.AddClippingPlane(skinClip)

#---------------------------------------------------------
ren.AddViewProp(volume)
#ren.AddViewProp(cropOutline)
#ren.AddViewProp(skin)
#ren.AddViewProp(imageActorX)
#ren.AddViewProp(imageActorY)
#ren.AddViewProp(imageActorZ)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(c[0],c[1],c[2])
camera.SetPosition(c[0] - 500,c[1] + 100,c[2] + 100)
camera.SetViewUp(0,0,1)

renWin.Render()

#ren.SetBackground(0.5, 0.5, 0.5)
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

# Add an observer for when the title bar "Close Window" is pressed.
iren.AddObserver("ExitEvent", lambda o,e: o.TerminateApp())

iren.Start()
