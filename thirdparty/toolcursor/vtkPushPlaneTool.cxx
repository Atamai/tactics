/*=========================================================================

  Program:   ToolCursor
  Module:    vtkPushPlaneTool.cxx

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommand.h"
#include "vtkPushPlaneTool.h"
#include "vtkObjectFactory.h"

#include "vtkToolCursor.h"
#include "vtkVolumePicker.h"
#include "vtkProp3DCollection.h"
#include "vtkImageActor.h"
#include "vtkImageStack.h"
#include "vtkLODProp3D.h"
#include "vtkVolumeMapper.h"
#include "vtkImageResliceMapper.h"
#include "vtkImageReslice.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"
#include "vtkResliceMath.h"
#include "vtkTransform.h"
#include "vtkImageData.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkPushPlaneTool);

//----------------------------------------------------------------------------
vtkPushPlaneTool::vtkPushPlaneTool()
{
  this->Transform = vtkTransform::New();

  this->ImageActor = 0;
  this->LODProp3D = 0;
  this->VolumeMapper = 0;
  this->ImageMapper = 0;
  this->Mapper = 0;
  this->EdgeId = -1;
  this->PlaneId = -1;
  this->PerpendicularPlane = 0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->StartNormal[0] = 0.0;
  this->StartNormal[1] = 0.0;
  this->StartNormal[2] = 1.0;

  this->StartOrigin[0] = 0.0;
  this->StartOrigin[1] = 0.0;
  this->StartOrigin[2] = 0.0;

  this->DistanceLimits[0] = -VTK_LARGE_FLOAT;
  this->DistanceLimits[1] = +VTK_LARGE_FLOAT;

  this->IsOffOfPlane = true;

  this->AllowRotation = 1;
  this->AllowSlicing = 1;

  this->MaximumRotationDegree = VTK_LARGE_FLOAT;
}

//----------------------------------------------------------------------------
vtkPushPlaneTool::~vtkPushPlaneTool()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::StartAction()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);

  this->Superclass::StartAction();

  if (!this->GetToolCursor()->GetPicker()->GetProp3D() ||
      !this->GetToolCursor()->GetPicker()->GetProp3D()->GetVisibility())
    {
    this->IsOffOfPlane = true;
    return;
    }

  this->IsOffOfPlane = false;

  // Get all the necessary information about the picked prop.
  this->GetPropInformation();

  // Used for caching the distance limits
  this->DistanceLimits[0] = -VTK_LARGE_FLOAT;
  this->DistanceLimits[1] = +VTK_LARGE_FLOAT;

  // Check whether the normal is perpendicular to the view plane.
  // If it is, then we can't use the usual interaction calculations.
  this->PerpendicularPlane = 0;
  double normal[3];
  this->GetStartNormal(normal);

  vtkCamera *camera =
    this->GetToolCursor()->GetRenderer()->GetActiveCamera();

  double position[3], focus[3];
  camera->GetPosition(position);
  camera->GetFocalPoint(focus);

  double v[3];
  v[0] = focus[0] - position[0];
  v[1] = focus[1] - position[1];
  v[2] = focus[2] - position[2];

  vtkMath::Normalize(v);
  vtkMath::Normalize(normal);

  // This gives the sin() of the angle between the vectors.
  vtkMath::Cross(normal, v, v);
  if (vtkMath::Dot(v, v) < 0.2)
    {
    this->PerpendicularPlane = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::StopAction()
{
  this->Superclass::StopAction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::DoAction()
{
  this->Superclass::DoAction();

  if (!this->IsPlaneValid())
    {
    return;
    }

  if (this->IsOffOfPlane)
    {
    return;
    }

  // Get and normalize the plane normal.
  double normal[3];
  this->GetStartNormal(normal);
  vtkMath::Normalize(normal);

  // Get the depth coordinate from the original pick.
  double ox, oy, oz;
  this->WorldToDisplay(this->GetStartPosition(), ox, oy, oz);

  // Get the initial display position.
  this->GetStartDisplayPosition(ox, oy);

  // Get the current display position.
  double x, y;
  this->GetDisplayPosition(x, y);

  // If plane is perpendicular, we only use the "x" motion.
  if (this->PerpendicularPlane) { y = oy; };

  // Get world point for the start position.
  double p1[3];
  this->DisplayToWorld(ox, oy, oz, p1);

  // Get the view ray for the current position, using old depth.
  double p2[3], viewRay[3];
  this->GetViewRay(x, y, oz, p2, viewRay);

  // The push distance, which is what we aim to calculate.
  double distance = 0.0;

  // Special action if the plane is perpendicular to view normal.
  if (this->PerpendicularPlane)
    {
    // Calculate distance moved in world coordinates.
    distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
    if (x - ox < 0)
      {
      distance = -distance;
      }

    // Check whether the normal is towards or away from the camera.
    if (vtkMath::Dot(viewRay, normal) < 0)
      {
      distance = -distance;
      }
    }
  else
    {
    // Get the vector between origin world point and current world point.
    double u[3];
    u[0] = p2[0] - p1[0];
    u[1] = p2[1] - p1[1];
    u[2] = p2[2] - p1[2];

    // Finally, I thought up a perfectly intuitive "push" action method.
    // The trick here is as follows: we need to find the position along
    // the plane normal (starting at the original pick position) that is
    // closest to the view-ray line at the current mouse position.  Not
    // sure if VTK has a math routine for this, so do calculations here:
    double a = vtkMath::Dot(normal, normal);
    double b = vtkMath::Dot(normal, viewRay);
    double c = vtkMath::Dot(viewRay, viewRay);
    double d = vtkMath::Dot(normal, u);
    double e = vtkMath::Dot(viewRay, u);

    distance = (c*d - b*e)/(a*c - b*b);
    }

  // Get the origin from before interaction began
  double origin[3];
  this->GetStartOrigin(origin);

  if (distance < this->DistanceLimits[0])
    {
    distance = this->DistanceLimits[0];
    }
  else if (distance > this->DistanceLimits[1])
    {
    distance = this->DistanceLimits[1];
    }
  // Constrain motion to the clipping bounds
  else if (this->AllowSlicing && this->ImageMapper && this->EdgeId < 0)
    {
    // clipping planes
    int numClipPlanes = this->ImageMapper->GetNumberOfClippingPlanes();
    vtkPlaneCollection *planes = this->ImageMapper->GetClippingPlanes();
    for (int i = 0; i < numClipPlanes; i++)
      {
      double pn[3], po[3];
      vtkPlane *plane = planes->GetItem(i);
      plane->GetOrigin(po);
      plane->GetNormal(pn);

      double d0 = pn[0]*po[0] + pn[1]*po[1] + pn[2]*po[2];
      double d1 = pn[0]*origin[0] + pn[1]*origin[1] + pn[2]*origin[2] - d0;
      double d2 = pn[0]*normal[0] + pn[1]*normal[1] + pn[2]*normal[2];

      double tol = 0.1;
      if (d1 + d2*distance < tol)
        {
        distance = (tol - d1)/d2;
        }
      }

    // data bounds
    vtkImageData *input = this->ImageMapper->GetInput();
    double io[3], is[3];
    int extent[6];
    input->GetWholeExtent(extent);
    input->GetSpacing(is);
    input->GetOrigin(io);

    // get the center of the volume
    double point[3];
    point[0] = 0.5*(extent[0] + extent[1])*is[0] + io[0];
    point[1] = 0.5*(extent[2] + extent[3])*is[1] + io[1];
    point[2] = 0.5*(extent[4] + extent[5])*is[2] + io[2];
    this->Transform->TransformPoint(point, point);
    vtkPlane::ProjectPoint(point, origin, normal, point);

    // get matrix for transforming normals
    double nmatrix[16];
    vtkMatrix4x4::Invert(*this->Transform->GetMatrix()->Element, nmatrix);
    vtkMatrix4x4::Transpose(nmatrix, nmatrix);

    // check against all six bounds
    for (int j = 0; j < 6; j++)
      {
      int k = (j>>1);
      double plane[4] = { 0.0, 0.0, 0.0, 0.0 };
      int ssgn = (is[k] < 0);
      int isodd = (j&1);
      plane[k] = 1.0 - 2*(isodd ^ ssgn);
      plane[3] = -plane[k]*(extent[j]*is[k] + io[k]);
      vtkMatrix4x4::MultiplyPoint(nmatrix, plane, plane);

      double d1 = plane[3];
      double d2 = 0.0;
      d1 += plane[0]*point[0] + plane[1]*point[1] + plane[2]*point[2];
      d2 += plane[0]*normal[0] + plane[1]*normal[1] + plane[2]*normal[2];

      if (d1 + d2*distance < 0.0)
        {
        distance = -d1/d2;
        }
      }
    }
  else if (this->VolumeMapper ||
           (this->Mapper && this->Mapper->IsA("vtkVolumeMapper")))
    {
    // keep planes tight to the data
    vtkVolumeMapper *mapper = this->VolumeMapper;
    if (!mapper)
      {
      mapper = vtkVolumeMapper::SafeDownCast(this->Mapper);
      }

    vtkImageData *data = mapper->GetInput();

    // get matrix for transforming normals
    double nmatrix[16];
    vtkMatrix4x4::Transpose(*this->Transform->GetMatrix()->Element, nmatrix);

    vtkImageReslice *reslice = vtkImageReslice::New();
    reslice->SetInterpolationModeToLinear();
    reslice->SetInput(data);
    reslice->GenerateStencilOutputOn();

    vtkImageHistogramStatistics *checker =
      vtkImageHistogramStatistics::New();
    checker->SetInputConnection(reslice->GetOutputPort());
    checker->SetStencil(reslice->GetStencilOutput());

    double dp = -(normal[0]*origin[0] +
                  normal[1]*origin[1] +
                  normal[2]*origin[2]);

    double dinc = static_cast<int>(distance) - distance;
    double dinc2 = (distance > 0 ? -1.0 : 1.0);

    for (;;)
      {
      // convert plane into a homogenous normal4
      double normal4[4];
      normal4[0] = normal[0];
      normal4[1] = normal[1];
      normal4[2] = normal[2];
      normal4[3] = dp - distance;

      // transform normal4 from world coords to data coords
      vtkMatrix4x4::MultiplyPoint(nmatrix, normal4, normal4);

      // drive plane inwards until it hits data
      vtkResliceMath::SetReslicePlane(reslice, normal4);
      checker->Update();
      double range[2];
      range[0] = checker->GetMinimum();
      range[1] = checker->GetMaximum();
      if (range[1] > range[0])
        {
        // plane is within image
        break;
        }
      distance += dinc;
      dinc = dinc2;
      if (distance*dinc > 0)
        {
        distance = 0;
        break;
        }
      if (dinc < 0)
        {
        this->DistanceLimits[1] = distance;
        }
      else
        {
        this->DistanceLimits[0] = distance;
        }
      }

    checker->Delete();
    reslice->Delete();
    }

  // Special action for tilting the planes
  if (this->AllowRotation && this->ImageMapper && this->EdgeId >= 0)
    {
    double mbounds[6];
    double mcenter[3];
    double mvector[3];

    this->ImageMapper->GetBounds(mbounds);
    mcenter[0] = 0.5*(mbounds[0] + mbounds[1]);
    mcenter[1] = 0.5*(mbounds[2] + mbounds[3]);
    mcenter[2] = 0.5*(mbounds[4] + mbounds[5]);

    mvector[0] = 0.0;
    mvector[1] = 0.0;
    mvector[2] = 0.0;

    mvector[this->EdgeId/2] = 2.0*(this->EdgeId % 2) - 1.0;

    this->Transform->TransformPoint(mcenter, mcenter);
    this->Transform->TransformVector(mvector, mvector);

    double u[3];
    u[0] = p2[0] - mcenter[0];
    u[1] = p2[1] - mcenter[1];
    u[2] = p2[2] - mcenter[2];

    // distance from axis to original point
    double theta = atan2(distance, vtkMath::Dot(u, mvector));
    theta = vtkMath::DegreesFromRadians(theta);
    vtkMath::Cross(mvector, normal, mvector);
    distance = 0.0;

    double max_theta = this->GetMaximumRotationDegree();
    if (theta < max_theta && theta > -1.0*max_theta)
      {
      vtkImageResliceMapper *resliceMapper =
        vtkImageResliceMapper::SafeDownCast(this->ImageMapper);
      if (resliceMapper)
        {
        vtkTransform *trans = vtkTransform::New();
        trans->PostMultiply();
        trans->Translate(-mcenter[0], -mcenter[1], -mcenter[2]);
        trans->RotateWXYZ(theta, mvector[0], mvector[1], mvector[2]);
        trans->Translate(mcenter[0], mcenter[1], mcenter[2]);
        trans->TransformPoint(origin, origin);
        trans->TransformNormal(normal, normal);
        resliceMapper->GetSlicePlane()->SetNormal(normal);
        trans->Delete();
        }
      }
    }

  // Moving relative to the original position provides a more stable
  // interaction that moving relative to the last position.
  origin[0] = origin[0] + distance*normal[0];
  origin[1] = origin[1] + distance*normal[1];
  origin[2] = origin[2] + distance*normal[2];

  this->SetOrigin(origin);
  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::ConstrainCursor(double position[3], double normal[3])
{
  // Get and normalize the plane normal.
  this->GetNormal(normal);
  vtkMath::Normalize(normal);

  // Get the display position.
  double x, y, z;
  this->WorldToDisplay(position, x, y, z);

  // Compute the view ray.
  double p1[3], p2[3];
  this->DisplayToWorld(x, y, 0.0, p1);
  this->DisplayToWorld(x, y, 1.0, p2);

  // Intersect the view ray with the plane.
  double p[3];
  double t;
  if (vtkPlane::IntersectWithLine(p1, p2, normal, this->Origin, t, p))
    {
    // Only move the position if it is closer to the camera, to ensure that
    // the cursor doesn't become hidden under other objects.  Include a
    // tolerance to ensure the distance is significant, otherwise just
    // leave the cursor where it is.

    // Compute the squared distance for the original position
    double t2 = (vtkMath::Distance2BetweenPoints(p1,position)/
                 vtkMath::Distance2BetweenPoints(p1,p2));

    // Compare to the square of the distace for the constrained position.
    if (t*t < t2 - 5e-20)
      {
      position[0] = p[0];
      position[1] = p[1];
      position[2] = p[2];
      }
    }
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::GetPropInformation()
{
  // Get all the information needed for the interaction
  vtkToolCursor *cursor = this->GetToolCursor();
  vtkVolumePicker *picker = cursor->GetPicker();

  vtkProp3D *prop = picker->GetProp3D();
  this->Mapper = picker->GetMapper();
  vtkImageStack *imageStack = vtkImageStack::SafeDownCast(prop);
  vtkImageSlice *imageSlice = vtkImageSlice::SafeDownCast(prop);
  if (imageStack)
    {
    this->Mapper = imageStack->GetMapper();
    prop = imageStack->GetActiveImage();
    }
  else if (imageSlice)
    {
    this->Mapper = imageSlice->GetMapper();
    }

  this->Transform->SetMatrix(prop->GetMatrix());
  this->ImageActor = vtkImageActor::SafeDownCast(prop);
  this->LODProp3D = vtkLODProp3D::SafeDownCast(prop);
  this->VolumeMapper = vtkVolumeMapper::SafeDownCast(this->Mapper);
  this->ImageMapper = vtkImageMapper3D::SafeDownCast(this->Mapper);

  // Initialize plane to "no plane" value
  this->PlaneId = -1;
  this->EdgeId = -1;

  // Is this a VolumeMapper cropping plane or AbstractMapper clipping plane?
  if (this->VolumeMapper && (cursor->GetPickFlags() & VTK_TOOL_CROP_PLANE))
    {
    this->Mapper = 0;
    this->PlaneId = picker->GetCroppingPlaneId();
    }
  else if (this->ImageMapper)
    {
    this->Mapper = 0;
    this->PlaneId = 0;
    if ((cursor->GetPickFlags() & VTK_TOOL_PLANE_EDGE) != 0)
      {
      double mbounds[6];
      double mpoint[3];

      this->ImageMapper->GetBounds(mbounds);
      picker->GetMapperPosition(mpoint);

      // find the closest edge that is within tolerance
      double mdist = VTK_DOUBLE_MAX;
      for (int jj = 0; jj < 6; jj++)
        {
        double dd = fabs(mpoint[jj/2] - mbounds[jj]);
        if (dd < mdist)
          {
          mdist = dd;
          this->EdgeId = jj;
          }
        }
      }
    }
  else
    {
    this->VolumeMapper = 0;
    this->PlaneId = picker->GetClippingPlaneId();
    }

  // Create a PlaneId for image actor.
  if (this->ImageActor)
    {
    int extent[6];
    this->ImageActor->GetDisplayExtent(extent);
    this->PlaneId = 4;
    if (extent[2] == extent[3]) { this->PlaneId = 2; }
    else if (extent[0] == extent[1]) { this->PlaneId = 0; }
    }

  if (this->PlaneId >= 0)
    {
    this->GetPlaneOriginAndNormal(this->Origin, this->Normal);
    this->SetStartOrigin(this->Origin);
    this->SetStartNormal(this->Normal);
    }
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::GetPlaneOriginAndNormal(double origin[3],
                                               double normal[3])
{
  if (this->Mapper)
    {
    vtkPlane *plane =
      this->Mapper->GetClippingPlanes()->GetItem(this->PlaneId);

    plane->GetNormal(normal);
    plane->GetOrigin(origin);
    }
  else if (this->ImageMapper)
    {
    vtkPlane *plane = this->ImageMapper->GetSlicePlane();

    plane->GetNormal(normal);
    plane->GetOrigin(origin);
    }
  else
    {
    double bounds[6];

    if (this->ImageActor)
      {
      this->ImageActor->GetDisplayBounds(bounds);
      }
    else if (this->VolumeMapper)
      {
      this->VolumeMapper->GetCroppingRegionPlanes(bounds);
      }
    else
      {
      return;
      }

    int i = this->PlaneId/2;

    normal[0] = normal[1] = normal[2] = 0.0;
    normal[i] = 1.0;

    origin[0] = 0.5*(bounds[0] + bounds[1]);
    origin[1] = 0.5*(bounds[2] + bounds[3]);
    origin[2] = 0.5*(bounds[4] + bounds[5]);
    origin[i] = bounds[this->PlaneId];

    // Transform from data coords to world coords.
    this->Transform->TransformNormal(normal, normal);
    this->Transform->TransformPoint(origin, origin);
    }
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::SetOrigin(const double o[3])
{
  // Store the origin that was set
  this->Origin[0] = o[0];
  this->Origin[1] = o[1];
  this->Origin[2] = o[2];

  if (this->PlaneId < 0)
    {
    return;
    }

  // Respect constness: make a copy that we can modify.
  double origin[3];
  origin[0] = o[0];
  origin[1] = o[1];
  origin[2] = o[2];

  if (this->Mapper)
    {
    vtkPlane *plane =
      this->Mapper->GetClippingPlanes()->GetItem(this->PlaneId);

    // Bounding checks needed!

    plane->SetOrigin(origin);
    }
  else if (this->ImageMapper &&
           vtkImageResliceMapper::SafeDownCast(this->ImageMapper))
    {
    vtkImageResliceMapper *resliceMapper =
      static_cast<vtkImageResliceMapper *>(this->ImageMapper);

    resliceMapper->GetSlicePlane()->SetOrigin(origin);
    }
  else
    {
    // Go from world coords to data coords
    this->Transform->GetInverse()->TransformPoint(origin, origin);

    int i = this->PlaneId/2;

    if (this->ImageActor)
      {
      double dataOrigin[3];
      this->ImageActor->GetInput()->GetOrigin(dataOrigin);
      double dataSpacing[3];
      this->ImageActor->GetInput()->GetSpacing(dataSpacing);
      int wholeExtent[6];
      this->ImageActor->GetInput()->GetWholeExtent(wholeExtent);
      int displayExtent[6];
      this->ImageActor->GetDisplayExtent(displayExtent);

      double x = (origin[i] - dataOrigin[i])/dataSpacing[i];
      if (x < wholeExtent[2*i]) { x = wholeExtent[2*i]; }
      if (x > wholeExtent[2*i+1]) { x = wholeExtent[2*i+1]; }

      int xi = int(floor(x));
      if ((x - xi) >= 0.5) { xi++; }

      displayExtent[2*i] = displayExtent[2*i+1] = xi;
      this->ImageActor->SetDisplayExtent(displayExtent);
      }
    else if (this->VolumeMapper)
      {
      double region[6];
      this->VolumeMapper->GetCroppingRegionPlanes(region);
      double bounds[6];
      this->VolumeMapper->GetBounds(bounds);

      // Get the cropping plane position
      double x = origin[i];

      // Get the minimum thickness of volume allowed.
      double t = 1.0;
      vtkImageData *data = this->VolumeMapper->GetInput();
      if (data)
        {
        double spacing[3];
        data->GetSpacing(spacing);
        t = spacing[i];
        }

      // Check for collissions with the opposing plane
      if (this->PlaneId == 2*i)
        {
        if (x > region[2*i+1] - t) { x = region[2*i+1] - t; }
        if (x > bounds[2*i+1] - t) { x = bounds[2*i+1] - t; }
        }
      else
        {
        if (x < region[2*i] + t) { x = region[2*i] + t; }
        if (x < bounds[2*i] + t) { x = bounds[2*i] + t; }
        }

      // Bounding box check
      if (x < bounds[2*i]) { x = bounds[2*i]; }
      if (x > bounds[2*i+1]) { x = bounds[2*i+1]; }

      region[this->PlaneId] = x;
      this->VolumeMapper->SetCroppingRegionPlanes(region);

      // Do the same for all other volumes in the LOD
      if (this->LODProp3D)
        {
        int n = this->LODProp3D->GetNumberOfLODs();
        for (int j = 0; j < n; j++)
          {
          vtkAbstractVolumeMapper *aMapper = 0;
          // LOD Ids start at 1000, I don't know why
          this->LODProp3D->GetLODMapper(1000+j, &aMapper);
          vtkVolumeMapper *mapper = vtkVolumeMapper::SafeDownCast(aMapper);
          if (mapper && mapper != this->VolumeMapper)
            {
            mapper->SetCroppingRegionPlanes(region);
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPushPlaneTool::SetNormal(const double n[3])
{
  // Store the normal that was set
  this->Normal[0] = n[0];
  this->Normal[1] = n[1];
  this->Normal[2] = n[2];

  if (this->PlaneId < 0)
    {
    return;
    }

  // Respect constness: make a copy rather than using ugly const cast.
  double normal[3];
  normal[0] = n[0];
  normal[1] = n[1];
  normal[2] = n[2];

  // Setting the normal is only valid for the mapper clipping planes.
  if (this->Mapper)
    {
    vtkPlane *plane =
      this->Mapper->GetClippingPlanes()->GetItem(this->PlaneId);

    // Sanity checks needed!

    plane->SetNormal(normal);
    }
}
