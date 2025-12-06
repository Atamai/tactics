/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourToPolygonFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourToPolygonFilter.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkLine.h"
#include "vtkMatrix4x4.h"
#include "vtkIncrementalOctreePointLocator.h"

#include <vector>
#include <algorithm>
#include <utility>

vtkStandardNewMacro(vtkContourToPolygonFilter);

//----------------------------------------------------------------------------
vtkContourToPolygonFilter::vtkContourToPolygonFilter()
{
  // A whole bunch of objects needed during execution
  this->IdList = 0;
  this->CellArray = 0;
  this->Polygon = 0;

  this->TriangulationErrorDisplay = 0;
}

//----------------------------------------------------------------------------
vtkContourToPolygonFilter::~vtkContourToPolygonFilter()
{
  if (this->IdList) { this->IdList->Delete(); }
  if (this->CellArray) { this->CellArray->Delete(); }
  if (this->Polygon) { this->Polygon->Delete(); }
}

//----------------------------------------------------------------------------
void vtkContourToPolygonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TriangulationErrorDisplay: "
     << (this->TriangulationErrorDisplay ? "On\n" : "Off\n" );
}

//----------------------------------------------------------------------------
int vtkContourToPolygonFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Create objects needed for temporary storage
  if (this->IdList == 0) { this->IdList = vtkIdList::New(); }
  if (this->CellArray == 0) { this->CellArray = vtkCellArray::New(); }
  if (this->Polygon == 0) { this->Polygon = vtkPolygon::New(); }

  vtkCellArray *lines = input->GetLines();
  if (lines == 0 || lines->GetNumberOfCells() == 0)
    {
    return 1;
    }

  input->BuildCells();

  vtkCellArray *polys = vtkCellArray::New();
  output->SetPolys(polys);
  output->SetPoints(input->GetPoints());
  polys->Delete();

  int triangulationFailed = !vtkContourToPolygonFilter::MakePolysFromContours(
    input, input->GetNumberOfVerts(), lines->GetNumberOfCells(), polys, 0,
    this->CellArray, this->Polygon, this->IdList);

  if (triangulationFailed && this->TriangulationErrorDisplay)
    {
    vtkErrorMacro("Triangulation failed, output might have holes.");
    }

  return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Everything below this point is support code for MakePolysFromContours().
// It could be separated out into its own class for generating
// polygons from contours.
//
// MakePolysFromContours uses the following steps:
// 1) Join line segments into contours, never change line directions
// 2) If any contours aren't closed, and if a loose end is on the hull
//    of the point set, try to connect it with another loose end on the hull
// 3) Remove degenerate points and points at 180 degree vertices
// 4) Group polygons according to which polygons are inside others
// 5) Cut the "hole" polygons to make simple polygons
// 6) Check for pinch-points to ensure that polygons are simple polygons
// 7) Triangulate polygons with vtkPolygon::Triangulate()
// 8) Add triangles for each point removed in Step 3
//
// In other words, this routine does a lot of work to process the contours
// so that vtkPolygon can be used to triangulate them (vtkPolygon only does
// simple polygons and even then it will fail on degenerate vertices or
// vertices with 180 degree angles).
//
// The whole mess below could be replaced by any robust triangulation code
// that can deal with holes.  Also, it is O(n^2) while available algorithms
// are O(n log n).  The vtkDelaunay2D filter will go into infinite recursion
// for some triangulations, hence it cannot be used.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// A helper class: a bitfield that is always as large as needed.
// For our purposes this is much more convenient than a bool vector,
// which would have to be resized and range-checked externally.

class vtkCTPFBitArray
{
public:
  void set(size_t bit, int val) {
    size_t n = (bit >> 5);
    size_t i = (bit & 0x1f);
    if (n >= bitstorage.size()) { bitstorage.resize(n+1); }
    unsigned int chunk = bitstorage[n];
    int bitval = 1;
    bitval <<= i;
    if (val) { chunk = chunk | bitval; }
    else { chunk = chunk & ~bitval; }
    bitstorage[n] = chunk;
  };

  int get(size_t bit) {
    size_t n = (bit >> 5);
    size_t i = (bit & 0x1f);
    if (n >= bitstorage.size()) { return 0; }
    unsigned int chunk = bitstorage[n];
    return ((chunk >> i) & 1);
  };

  void clear() {
    bitstorage.clear();
  };

private:
  std::vector<unsigned int> bitstorage;
};

//----------------------------------------------------------------------------
// Simple typedefs for stl-based polygons.

// A poly type that is just a vector of vtkIdType
typedef std::vector<vtkIdType> vtkCTPFPoly;

// A poly group type that holds indices into a vector of polys.
// A poly group is used to represent a polygon with holes.
// The first member of the group is the outer poly, and all
// other members are the holes.
typedef std::vector<size_t> vtkCTPFPolyGroup;

// Extra info for each edge in a poly
typedef std::vector<vtkIdType> vtkCTPFPolyEdges;

//----------------------------------------------------------------------------
// These are the prototypes for helper functions for manipulating
// polys that are stored in stl vectors.

// Tolerances are relative to polygon size
#define VTK_CCS_POLYGON_TOLERANCE 1e-5

int vtkCTPFDegenerateCheck(vtkCTPFPoly &poly, vtkPoints *points);
// Take a set of lines, join them tip-to-tail to create polygons
static void vtkCTPFMakePolysFromLines(
  vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
  std::vector<vtkCTPFPoly> &newPolys,
  std::vector<size_t> &incompletePolys);

// Finish any incomplete polygons by trying to join loose ends
static void vtkCTPFJoinLooseEnds(
  std::vector<vtkCTPFPoly> &polys, std::vector<size_t> &incompletePolys,
  vtkPoints *points, const double normal[3]);

// Check for polygons that contain multiple loops, and split the loops apart.
// Returns the number of splits made.
static int vtkCTPFSplitAtPinchPoints(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  std::vector<vtkCTPFPolyEdges> &polyEdges,
  const double normal[3]);

// Given three vectors p->p1, p->p2, and p->p3, this routine
// checks to see if progressing from p1 to p2 to p3 is a clockwise
// or counterclockwise progression with respect to the normal.
// The return value is -1 for clockwise, +1 for counterclockwise,
// and 0 if any two of the vectors are coincident.
static int vtkCTPFVectorProgression(
  const double p[3], const double p1[3],
  const double p2[3], const double p3[3], const double normal[3]);

// Compute polygon bounds.  Poly must have at least one point.
static double vtkCTPFPolygonBounds(
  const vtkCTPFPoly &poly, vtkPoints *points, double bounds[6]);

// Remove points that are not vertices of the polygon,
// i.e. remove any points that are on an edge but not at a corner.
// This simplifies all remaining steps and improves the triangulation.
// The original edges are appended to the originalEdges cell array,
// where each cell in this array will be a polyline consisting of two
// corner vertices and all the points in between.
static void vtkCTPFFindTrueEdges(
  std::vector<vtkCTPFPoly> &newPolys, vtkPoints *points,
  std::vector<vtkCTPFPolyEdges> &polyEdges, vtkCellArray *originalEdges);

// Set sense to 1 if the poly's normal matches the specified normal, and
// zero otherwise. Returns zero if poly is degenerate.
static int vtkCTPFCheckPolygonSense(
  vtkCTPFPoly &polys, vtkPoints *points, const double normal[3], int &sense);

// Add a triangle to the output, and subdivide the triangle if one
// of the edges originally had more than two points, as indicated
// by originalEdges.  If scalars is not null, then add a scalar for
// each triangle.
static void vtkCTPFInsertTriangle(
  vtkCellArray *polys, const vtkCTPFPoly &poly, const size_t trids[3],
  const vtkCTPFPolyEdges &polyEdges, vtkCellArray *originalEdges);

// Check for polys within other polys, i.e. find polys that are holes and
// add them to the "polyGroup" of the poly that they are inside of.
static void vtkCTPFMakeHoleyPolys(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  const double normal[3]);

// For each poly that has holes, make two cuts between each hole and
// the outer poly in order to turn the polygon+hole into two polys.
static int vtkCTPFCutHoleyPolys(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  std::vector<vtkCTPFPolyEdges> &polyEdges,
  const double normal[3]);

// Triangulate a polygon that has been simplified by FindTrueEdges.
// This will re-insert the original edges.  The output tridss are
// appended to "polys".  The final two arguments (polygon and
// triangles) are only for temporary storage.
// The return value is true if triangulation was successful.
int vtkCTPFTriangulate(
  const vtkCTPFPoly &poly, vtkPoints *points,
  const vtkCTPFPolyEdges &polyEdges, vtkCellArray *originalEdges,
  vtkCellArray *polys, vtkPolygon *polygon, vtkIdList *triangles);

//----------------------------------------------------------------------------
// This is a complex subroutine that takes a collection of lines that
// were formed by cutting a polydata with a plane, and generates
// a face that has those lines as its edges.  The lines must form one
// or more closed contours, but they need not be sorted.
//
// Only "numLine" lines starting from "firstLine" are used to create new
// polygons, and the new polygons are appended to "polys".  The normal of
// the cut plane must be provided so that polys will be correctly oriented.

// If this is defined, then the outlines of any failed polygons will be
// added to "data".  It is only meant as a debugging tool.
//#define VTK_CTPF_SHOW_FAILED_POLYS

int vtkContourToPolygonFilter::MakePolysFromContours(
  vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
  vtkCellArray *polys, const double normal[3],
  vtkCellArray *tmpCellArray, vtkPolygon *tmpPolygon,
  vtkIdList *tmpIdList)
{
  int triangulationFailure = 0;

  // If no cut lines were generated, there's nothing to do
  if (numLines <= 0)
    {
    return 0;
    }

  vtkPoints *points = data->GetPoints();

  // Join all the new lines into connected groups, i.e. polygons.
  // If we are lucky these will be simple, convex polygons.  But
  // we can't count on that.

  std::vector<vtkCTPFPoly> newPolys;
  std::vector<size_t> incompletePolys;
  // reallocating this would be expensive, so start it big
  newPolys.reserve(100);

  vtkCTPFMakePolysFromLines(data, firstLine, firstLine+numLines,
                           newPolys, incompletePolys);

  // if no normal specified, then compute one from largest contour
  double computedNormal[3] = { 0.0, 0.0, 1.0 };
  if (normal == 0)
    {
    double maxnorm2 = 0;
    size_t numNewPolys = newPolys.size();
    for (size_t i = 0; i < numNewPolys; i++)
      {
      vtkCTPFPoly &poly = newPolys[i];
      size_t numPoints = poly.size();
      double n[3] = { 0.0, 0.0, 0.0 };
      double v0[3], v1[3], v2[3];
      points->GetPoint(poly[numPoints-1], v1);
      points->GetPoint(poly[0], v2);

      for (size_t j = 1; j < numPoints; j++)
        {
        v0[0] = v1[0]; v0[1] = v1[1]; v0[2] = v1[2];
        v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
        points->GetPoint(poly[j], v2);

        double ax = v2[0] - v1[0];
        double ay = v2[1] - v1[1];
        double az = v2[2] - v1[2];
        double bx = v0[0] - v1[0];
        double by = v0[1] - v1[1];
        double bz = v0[2] - v1[2];

        n[0] += (ay * bz - az * by);
        n[1] += (az * bx - ax * bz);
        n[2] += (ax * by - ay * bx);
        }

      double norm2 = n[0]*n[0] + n[1]*n[1] + n[2]*n[2];
      if (norm2 > maxnorm2)
        {
        computedNormal[0] = -n[0]/norm2;
        computedNormal[1] = -n[1]/norm2;
        computedNormal[2] = -n[2]/norm2;
        maxnorm2 = norm2;
        }
      }
    normal = computedNormal;
    }

  // Join any loose ends.  If the input was a closed surface then there
  // will not be any loose ends, so this is provided as a service to users
  // who want to clip a non-closed surface.
  vtkCTPFJoinLooseEnds(newPolys, incompletePolys, points, normal);

  // Some points might be in the middle of straight line segments.
  // These points can be removed without changing the shape of the
  // polys, and removing them makes triangulation more stable.
  // Unfortunately removing these points also means that the polys
  // will no longer form a watertight cap over the cut.

  std::vector<vtkCTPFPolyEdges> polyEdges;
  polyEdges.reserve(100);
  vtkCellArray *originalEdges = tmpCellArray;
  originalEdges->Initialize();
  vtkCTPFFindTrueEdges(newPolys, points, polyEdges, originalEdges);

  // Next we have to check for polygons with holes, i.e. polygons that
  // have other polygons inside.  Each polygon is "grouped" with the
  // polygons that make up its holes.

  // Initialize each group to hold just one polygon.

  size_t numNewPolys = newPolys.size();
  std::vector<vtkCTPFPolyGroup> polyGroups(numNewPolys);
  for (size_t i = 0; i < numNewPolys; i++)
    {
    polyGroups[i].push_back(i);
    }

  // Find out which polys are holes in larger polys.  Create a group
  // for each poly where the first member of the group is the larger
  // poly, and all other members are the holes.  The number of polyGroups
  // will be the same as the number of polys, and any polys that are
  // holes will have a matching empty group.

  vtkCTPFMakeHoleyPolys(newPolys, points, polyGroups, normal);

  // Make cuts to create simple polygons out of the holey polys.
  // After this is done, each polyGroup will have exactly 1 polygon,
  // and no polys will be holes.  This is currently the most computationally
  // expensive part of the process.

  if (!vtkCTPFCutHoleyPolys(newPolys, points, polyGroups, polyEdges, normal))
    {
    triangulationFailure = 1;
    }

  // Some polys might be self-intersecting.  Split the polys at each
  // intersection point.

  vtkCTPFSplitAtPinchPoints(newPolys, points, polyGroups, polyEdges, normal);

  // ------ Triangulation code ------

  // Need a polygon cell and idlist for triangulation
  vtkPolygon *polygon = tmpPolygon;
  vtkIdList *triangles = tmpIdList;

  // Go through all polys and triangulate them
  for (size_t polyId = 0; polyId < polyGroups.size(); polyId++)
    {
    // If group is empty, then poly was a hole without a containing poly
    if (polyGroups[polyId].size() == 0)
      {
      continue;
      }

    if (!vtkCTPFTriangulate(newPolys[polyId], points, polyEdges[polyId],
                           originalEdges, polys, polygon, triangles))
      {
      triangulationFailure = 1;
#ifdef VTK_CTPF_SHOW_FAILED_POLYS
      // Diagnostic code: show the polys as outlines
      vtkCellArray *lines = data->GetLines();
      vtkCTPFPoly &poly = newPolys[polyId];
      lines->InsertNextCell(poly.size()+1);
      for (size_t jjj = 0; jjj < poly.size(); jjj++)
        {
        lines->InsertCellPoint(poly[jjj]);
        }
      lines->InsertCellPoint(poly[0]);
#endif
      }
    }

  // Free up some memory
  polygon->Points->Initialize();
  polygon->PointIds->Initialize();
  triangles->Initialize();
  originalEdges->Initialize();

  return !triangulationFailure;
}

// ---------------------------------------------------
int vtkContourToPolygonFilter::TriangulatePolygon(
  vtkIdList *polygon, vtkPoints *points, vtkCellArray *triangles,
  vtkCellArray *tmpCellArray, vtkPolygon *tmpPolygon,
  vtkIdList *tmpIdList)
{
  vtkIdType n = polygon->GetNumberOfIds();
  std::vector<vtkCTPFPoly> polys(1);
  vtkCTPFPoly &poly = polys[0];
  poly.resize(n);

  for (vtkIdType i = 0; i < n; i++)
    {
    poly[i] = polygon->GetId(i);
    }

  vtkCellArray *originalEdges = tmpCellArray;
  originalEdges->Initialize();

  std::vector<vtkCTPFPolyEdges> polyEdges;
  vtkCTPFFindTrueEdges(polys, points, polyEdges, originalEdges);
  vtkCTPFPolyEdges &edges = polyEdges[0];

  return vtkCTPFTriangulate(poly, points, edges, originalEdges, triangles,
                           tmpPolygon, tmpIdList);
}

// ---------------------------------------------------
// Triangulate a polygon that has been simplified by FindTrueEdges.
// This will re-insert the original edges.  The output triangles are
// appended to "polys" and, for each stored triangle, "color" will
// be added to "scalars".  The final two arguments (polygon and
// triangles) are only for temporary storage.
// The return value is true if triangulation was successful.
int vtkCTPFTriangulate(
  const vtkCTPFPoly &poly, vtkPoints *points,
  const vtkCTPFPolyEdges &polyEdges, vtkCellArray *originalEdges,
  vtkCellArray *polys, vtkPolygon *polygon, vtkIdList *triangles)
{
  int triangulationFailure = 0;
  size_t n = poly.size();

  // If the poly is a line, then skip it
  if (n < 3)
    {
    return 1;
    }
  // If the poly is a triangle, then pass it
  else if (n == 3)
    {
    size_t trids[3];
    trids[0] = 0;
    trids[1] = 1;
    trids[2] = 2;

    vtkCTPFInsertTriangle(polys, poly, trids, polyEdges, originalEdges);
    }
  // If the poly has 4 or more points, triangulate it
  else
    {
    polygon->Points->SetNumberOfPoints(n);
    polygon->PointIds->SetNumberOfIds(n);

    for (size_t j = 0; j < n; j++)
      {
      vtkIdType pointId = poly[j];
      double point[3];
      points->GetPoint(pointId, point);
      polygon->Points->SetPoint(static_cast<vtkIdType>(j), point);
      polygon->PointIds->SetId(static_cast<vtkIdType>(j), pointId);
      }

    triangles->Initialize();
    if (!polygon->Triangulate(0, triangles, polygon->Points))
      {
      triangulationFailure = 1;
      }

    vtkIdType m = triangles->GetNumberOfIds();

    for (vtkIdType k = 0; k < m; k += 3)
      {
      size_t trids[3];
      trids[0] = triangles->GetId(k + 0);
      trids[1] = triangles->GetId(k + 1);
      trids[2] = triangles->GetId(k + 2);

      vtkCTPFInsertTriangle(polys, poly, trids, polyEdges, originalEdges);
      }
    }

  return !triangulationFailure;
}

// ---------------------------------------------------
// Here is the code for creating polygons from line segments.

void vtkCTPFMakePolysFromLines(
  vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
  std::vector<vtkCTPFPoly> &newPolys,
  std::vector<size_t> &incompletePolys)
{
  vtkIdType npts = 0;
  vtkIdType const *pts = nullptr;

  // Bitfield for marking lines as used
  vtkCTPFBitArray usedLines;

  // Require cell links to get lines from pointIds
  data->BuildLinks(data->GetPoints()->GetNumberOfPoints());

  size_t numNewPolys = 0;
  vtkIdType remainingLines = numLines - firstLine;

  while (remainingLines > 0)
    {
    // Create a new poly
    size_t polyId = numNewPolys++;
    newPolys.push_back(vtkCTPFPoly());
    vtkCTPFPoly &poly = newPolys[polyId];

    vtkIdType lineId = 0;
    int completePoly = 0;

    // start the poly
    for (lineId = firstLine; lineId < numLines; lineId++)
      {
      if (!usedLines.get(lineId-firstLine))
        {
        data->GetCellPoints(lineId, npts, pts);

        vtkIdType n = npts;
        if (npts > 2 && pts[0] == pts[npts-1])
          {
          n = npts - 1;
          completePoly = 1;
          }
        poly.resize(static_cast<size_t>(n));
        for (vtkIdType i = 0; i < n; i++)
          {
          poly[i] = pts[i];
          }
        break;
        }
      }

    usedLines.set(lineId-firstLine, 1);
    remainingLines--;

    int noLinesMatch = 0;

    while (!completePoly && !noLinesMatch && remainingLines > 0)
      {
      // This is cleared if a match is found
      noLinesMatch = 1;

      // Number of points in the poly
      size_t npoly = poly.size();

      vtkIdType endPts[2];
      endPts[0] = poly[npoly-1];
      endPts[1] = poly[0];

      // For both open ends of the polygon
      for (int endIdx = 0; endIdx < 2; endIdx++)
        {
        std::vector<vtkIdType> matches;
        vtkIdType ncells = 0;
        vtkIdType *cells = nullptr;
        data->GetPointCells(endPts[endIdx], ncells, cells);

        // Go through all lines that contain this endpoint
        for (vtkIdType icell = 0; icell < ncells; icell++)
          {
          lineId = cells[icell];
          if (lineId >= firstLine && lineId < numLines &&
              !usedLines.get(lineId-firstLine))
            {
            data->GetCellPoints(lineId, npts, pts);
            vtkIdType lineEndPts[2];
            lineEndPts[0] = pts[0];
            lineEndPts[1] = pts[npts-1];

            // Check that poly end matches line end
            if (endPts[endIdx] == lineEndPts[endIdx])
              {
              matches.push_back(lineId);
              }
            }
          }

        if (matches.size() > 0)
          {
          // Multiple matches mean we need to decide which path to take
          if (matches.size() > 1)
            {
            // Remove double-backs
            size_t k = matches.size();
            do
              {
              lineId = matches[--k];
              data->GetCellPoints(lineId, npts, pts);
              if ((endIdx == 0 && poly[npoly-2] == pts[1]) ||
                  (endIdx == 1 && poly[1] == pts[npts-2]))
                {
                matches.erase(matches.begin()+k);
                }
              }
            while (k > 0 && matches.size() > 1);

            // If there are multiple matches due to intersections,
            // they should be dealt with here.
            }

          lineId = matches[0];
          data->GetCellPoints(lineId, npts, pts);

          // Do both ends match?
          if (pts[0] == poly[npoly-1] && pts[npts-1] == poly[0])
            {
            completePoly = 1;
            }

          if (endIdx == 0)
            {
            poly.insert(poly.end(), &pts[1], &pts[npts-completePoly]);
            }
          else
            {
            poly.insert(poly.begin(), &pts[completePoly], &pts[npts-1]);
            }

          usedLines.set(lineId-firstLine, 1);
          remainingLines--;
          noLinesMatch = 0;
          }
        }
      }

    // Check for incomplete polygons
    if (noLinesMatch)
      {
      incompletePolys.push_back(polyId);
      }
    }
}

// ---------------------------------------------------
// Join polys that have loose ends, as indicated by incompletePolys.
// Any polys created will have a normal opposite to the supplied normal,
// and any new edges that are created will be on the hull of the point set.
// Shorter edges will be preferred over long edges.

static void vtkCTPFJoinLooseEnds(
  std::vector<vtkCTPFPoly> &polys, std::vector<size_t> &incompletePolys,
  vtkPoints *points, const double normal[3])
{
  // Relative tolerance for checking whether an edge is on the hull
  const double tol = VTK_CCS_POLYGON_TOLERANCE;

  // A list of polys to remove when everything is done
  std::vector<size_t> removePolys;

  size_t n;
  while ( (n = incompletePolys.size()) )
    {
    vtkCTPFPoly &poly1 = polys[incompletePolys[n-1]];
    vtkIdType pt1 = poly1[poly1.size()-1];
    double p1[3], p2[3];
    points->GetPoint(pt1, p1);

    double dMin = VTK_DOUBLE_MAX;
    size_t iMin = 0;

    for (size_t i = 0; i < n; i++)
      {
      vtkCTPFPoly &poly2 = polys[incompletePolys[i]];
      vtkIdType pt2 = poly2[0];
      points->GetPoint(pt2, p2);

      // The next few steps verify that edge [p1, p2] is on the hull
      double v[3];
      v[0] = p2[0] - p1[0]; v[1] = p2[1] - p1[1]; v[2] = p2[2] - p1[2];
      double d = vtkMath::Norm(v);
      v[0] /= d; v[1] /= d; v[2] /= d;

      // Compute the midpoint of the edge
      double pm[3];
      pm[0] = 0.5*(p1[0] + p2[0]);
      pm[1] = 0.5*(p1[1] + p2[1]);
      pm[2] = 0.5*(p1[2] + p2[2]);

      // Create a plane equation
      double pc[4];
      vtkMath::Cross(v, normal, pc);
      pc[3] = -vtkMath::Dot(pc, pm);

      // Check that all points are inside the plane.  If they aren't, then
      // the edge is not on the hull of the pointset.
      int badPoint = 0;
      size_t m = polys.size();
      for (size_t j = 0; j < m && !badPoint; j++)
        {
        vtkCTPFPoly &poly = polys[j];
        size_t npts = poly.size();
        for (size_t k = 0; k < npts; k++)
          {
          vtkIdType ptId = poly[k];
          if (ptId != pt1 && ptId != pt2)
            {
            double p[3];
            points->GetPoint(ptId, p);
            double val = p[0]*pc[0] + p[1]*pc[1] + p[2]*pc[2] + pc[3];
            double r2 = vtkMath::Distance2BetweenPoints(p, pm);

            // Check distance from plane against the tolerance
            if (val < 0 && val*val > tol*tol*r2)
              {
              badPoint = 1;
              break;
              }
            }
          }

        // If no bad points, then this edge is a candidate
        if (!badPoint && d < dMin)
          {
          dMin = d;
          iMin = i;
          }
        }
      }

    // If a match was found, append the polys
    if (dMin < VTK_DOUBLE_MAX)
      {
      // Did the poly match with itself?
      if (iMin == n-1)
        {
        // Mark the poly as closed
        incompletePolys.pop_back();
        }
      else
        {
        size_t id2 = incompletePolys[iMin];

        // Combine the polys
        poly1.insert(poly1.end(), polys[id2].begin(), polys[id2].end());

        // Erase the second poly
        removePolys.push_back(id2);
        incompletePolys.erase(incompletePolys.begin() + iMin);
        }
      }
    else
      {
      // If no match, erase this poly from consideration
      removePolys.push_back(incompletePolys[n-1]);
      incompletePolys.pop_back();
      }
    }

  // Remove polys that couldn't be completed
  std::sort(removePolys.begin(), removePolys.end());
  size_t i = removePolys.size();
  while (i > 0)
    {
    // Remove items in reverse order
    polys.erase(polys.begin() + removePolys[--i]);
    }

  // Clear the incompletePolys vector, it's indices are no longer valid
  incompletePolys.clear();
}

// ---------------------------------------------------
// Check for self-intersection. Split the figure-eights.
// This assumes that all intersections occur at existing
// vertices, i.e. no new vertices will be created. Returns
// the number of splits made.

int vtkCTPFSplitAtPinchPoints(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  std::vector<vtkCTPFPolyEdges> &polyEdges,
  const double normal[3])
{
  vtkPoints *tryPoints = vtkPoints::New();
  tryPoints->SetDataTypeToDouble();

  vtkIncrementalOctreePointLocator *locator =
    vtkIncrementalOctreePointLocator::New();

  int splitCount = 0;

  for (size_t i = 0; i < polys.size(); i++)
    {
    vtkCTPFPoly &poly = polys[i];
    size_t n = poly.size();

    double bounds[6];
    double tol = VTK_CCS_POLYGON_TOLERANCE;
    tol *= sqrt(vtkCTPFPolygonBounds(poly, points, bounds));

    if (tol == 0)
      {
      continue;
      }

    tryPoints->Initialize();
    locator->SetTolerance(tol);
    locator->InitPointInsertion(tryPoints, bounds);

    int foundMatch = 0;
    size_t idx1 = 0;
    size_t idx2 = 0;
    int unique = 0;

    for (idx2 = 0; idx2 < n; idx2++)
      {
      double point[3];
      vtkIdType firstId = poly[idx2];
      points->GetPoint(firstId, point);

      vtkIdType vertIdx = 0;
      if (!locator->InsertUniquePoint(point, vertIdx))
        {
        // Need vertIdx to match poly indices, so force point insertion
        locator->InsertNextPoint(point);

        // Do the points have different pointIds?
        idx1 = static_cast<size_t>(vertIdx);
        unique = (poly[idx2] != poly[idx1]);

        if ((idx2 > idx1 + 2 - unique) && (n + idx1 > idx2 + 2 - unique))
          {
          if (normal)
            {
            // Make sure that splitting this poly won't create a hole poly
            double p1[3], p2[3], p3[3];
            size_t prevIdx = n + idx1 - 1;
            size_t midIdx = idx1 + 1;
            size_t nextIdx = idx2 + 1;
            if (prevIdx >= n) { prevIdx -= n; }
            if (midIdx >= n) { midIdx -= n; }
            if (nextIdx >= n) { nextIdx -= n; }

            points->GetPoint(poly[prevIdx], p1);
            points->GetPoint(poly[midIdx], p2);
            points->GetPoint(poly[nextIdx], p3);

            if (vtkCTPFVectorProgression(point, p1, p2, p3, normal) < 0)
              {
              foundMatch = 1;
              break;
              }
            }
          else
            {
            foundMatch = 1;
            break;
            }
          }
        }
      }

    if (foundMatch)
      {
      splitCount++;

      // Split off a new poly
      size_t m = idx2 - idx1;

      vtkCTPFPoly &oldPoly = polys[i];
      vtkCTPFPolyEdges &oldEdges = polyEdges[i];
      vtkCTPFPoly newPoly1(m + unique);
      vtkCTPFPolyEdges newEdges1(m + unique);
      vtkCTPFPoly newPoly2(n - m + unique);
      vtkCTPFPolyEdges newEdges2(n - m + unique);

      // The current poly, which is now intersection-free
      for (size_t l = 0; l < m+unique; l++)
        {
        newPoly1[l] = oldPoly[l+idx1];
        newEdges1[l] = oldEdges[l+idx1];
        }
      if (unique)
        {
        newEdges1[m] = -1;
        }

      // The poly that is split off, which might have more intersections
      for (size_t j = 0; j < idx1+unique; j++)
        {
        newPoly2[j] = oldPoly[j];
        newEdges2[j] = oldEdges[j];
        }
      if (unique)
        {
        newEdges2[idx1] = -1;
        }
      for (size_t k = idx2; k < n; k++)
        {
        newPoly2[k - m + unique] = oldPoly[k];
        newEdges2[k - m + unique] = oldEdges[k];
        }

      polys[i] = newPoly1;
      polyEdges[i] = newEdges1;
      polys.push_back(newPoly2);
      polyEdges.push_back(newEdges2);

      // Unless polygroup was clear (because poly was reversed),
      // make a group with one entry for the new poly
      polyGroups.resize(polys.size());
      if (polyGroups[i].size())
        {
        polyGroups[polys.size()-1].push_back(polys.size()-1);
        }
      }
    }

  tryPoints->Delete();
  locator->Delete();

  return splitCount;
}

// ---------------------------------------------------
// Given three vectors p->p1, p->p2, and p->p3, this routine
// checks to see if progressing from p1 to p2 to p3 is a clockwise
// or counterclockwise progression with respect to the normal.
// The return value is -1 for clockwise, +1 for counterclockwise,
// and 0 if any two of the vectors are coincident.
int vtkCTPFVectorProgression(
  const double p[3], const double p1[3],
  const double p2[3], const double p3[3], const double normal[3])
{
  double v1[3], v2[3], v3[3];

  v1[0] = p1[0] - p[0]; v1[1] = p1[1] - p[1]; v1[2] = p1[2] - p[2];
  v2[0] = p2[0] - p[0]; v2[1] = p2[1] - p[1]; v2[2] = p2[2] - p[2];
  v3[0] = p3[0] - p[0]; v3[1] = p3[1] - p[1]; v3[2] = p3[2] - p[2];

  double w1[3], w2[3];

  vtkMath::Cross(v2, v1, w1);
  vtkMath::Cross(v2, v3, w2);
  double s1 = vtkMath::Dot(w1, normal);
  double s2 = vtkMath::Dot(w2, normal);

  if (s1 != 0 && s2 != 0)
    {
    int sb1 = (s1 < 0);
    int sb2 = (s2 < 0);

    // if sines have different signs
    if ( (sb1 ^ sb2) )
      {
      // return -1 if s2 is -ve
      return (1 - 2*sb2);
      }

    double c1 = vtkMath::Dot(v2, v1);
    double l1 = vtkMath::Norm(v1);
    double c2 = vtkMath::Dot(v2, v3);
    double l2 = vtkMath::Norm(v3);

    // ck is the difference of the cosines, flipped in sign if sines are +ve
    double ck = (c2*l2 - c1*l1)*(1 - sb1*2);

    if (ck != 0)
      {
      // return the sign of ck
      return (1 - 2*(ck < 0));
      }
    }

  return 0;
}

// ---------------------------------------------------
// Simple utility method for computing polygon bounds.
// Returns the sum of the squares of the dimensions.
// Requires a poly with at least one  point.
double vtkCTPFPolygonBounds(
  const vtkCTPFPoly &poly, vtkPoints *points, double bounds[6])
{
  size_t n = poly.size();
  double p[3];

  points->GetPoint(poly[0], p);
  bounds[0] = bounds[1] = p[0];
  bounds[2] = bounds[3] = p[1];
  bounds[4] = bounds[5] = p[2];

  for (size_t j = 1; j < n; j++)
    {
    points->GetPoint(poly[j], p);
    if (p[0] < bounds[0]) { bounds[0] = p[0]; };
    if (p[0] > bounds[1]) { bounds[1] = p[0]; };
    if (p[1] < bounds[2]) { bounds[2] = p[1]; };
    if (p[1] > bounds[3]) { bounds[3] = p[1]; };
    if (p[2] < bounds[4]) { bounds[4] = p[2]; };
    if (p[2] > bounds[5]) { bounds[5] = p[2]; };
    }

  double bx = (bounds[1] - bounds[0]);
  double by = (bounds[3] - bounds[2]);
  double bz = (bounds[5] - bounds[4]);

  return (bx*bx + by*by + bz*bz);
}

// ---------------------------------------------------
// The polygons might have a lot of extra points, i.e. points
// in the middle of the edges.  Remove those points, but keep
// the original edges as polylines in the originalEdges array.
// Only original edges with more than two points will be kept.

void vtkCTPFFindTrueEdges(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyEdges> &polyEdges, vtkCellArray *originalEdges)
{
  // Tolerance^2 for angle to see if line segments are parallel
  const double atol2 = (VTK_CCS_POLYGON_TOLERANCE*VTK_CCS_POLYGON_TOLERANCE);

  for (size_t polyId = 0; polyId < polys.size(); polyId++)
    {
    vtkCTPFPoly &oldPoly = polys[polyId];
    size_t n = oldPoly.size();
    polyEdges.push_back(vtkCTPFPolyEdges());

    // Only useful if poly has more than three sides
    if (n < 4)
      {
      polyEdges[polyId].resize(3);
      polyEdges[polyId][0] = -1;
      polyEdges[polyId][1] = -1;
      polyEdges[polyId][2] = -1;
      continue;
      }

    // While we remove points, m keeps track of how many points are left
    size_t m = n;

    // Compute bounds for tolerance
    double bounds[6];
    double tol2 = vtkCTPFPolygonBounds(oldPoly, points, bounds)*atol2;

    // The new poly
    vtkCTPFPoly newPoly;
    vtkCTPFPolyEdges &newEdges = polyEdges[polyId];
    vtkIdType cornerPointId = 0;
    vtkIdType oldOriginalId = -1;

    // Allocate space
    newPoly.reserve(n);
    newEdges.reserve(n);

    // Keep the partial edge from before the first corner is found
    std::vector<vtkIdType> partialEdge;
    int cellCount = 0;

    double p0[3], p1[3], p2[3];
    double v1[3], v2[3];
    double l1, l2;

    points->GetPoint(oldPoly[n-1], p0);
    points->GetPoint(oldPoly[0], p1);
    v1[0] = p1[0] - p0[0];  v1[1] = p1[1] - p0[1];  v1[2] = p1[2] - p0[2];
    l1 = vtkMath::Dot(v1, v1);

    for (size_t j = 0; j < n; j++)
      {
      size_t k = j+1;
      if (k >= n) { k -= n; }

      points->GetPoint(oldPoly[k], p2);
      v2[0] = p2[0] - p1[0];  v2[1] = p2[1] - p1[1];  v2[2] = p2[2] - p1[2];
      l2 = vtkMath::Dot(v2, v2);

      // Dot product is |v1||v2|cos(theta)
      double c = vtkMath::Dot(v1, v2);
      // sin^2(theta) = (1 - cos^2(theta))
      // and   c*c = l1*l2*cos^2(theta)
      double s2 = (l1*l2 - c*c);

      // In the small angle approximation, sin(theta) == theta, so
      // s2/(l1*l2) is the angle that we want to check, but it's not
      // a valid check if l1 or l2 is very close to zero.

      vtkIdType pointId = oldPoly[j];

      // Keep the point if:
      // 1) removing it would create a 2-point poly OR
      // 2) it's more than "tol" distance from the prev point AND
      // 3) the angle is greater than atol:
      if (m <= 3 ||
          (l1 > tol2 &&
           (c < 0 || l1 < tol2 || l2 < tol2 || s2 > l1*l2*atol2)))
        {
        // Complete the previous edge only if the final point count
        // will be greater than two
        if (cellCount > 1)
          {
          if (pointId != oldOriginalId)
            {
            originalEdges->InsertCellPoint(pointId);
            cellCount++;
            }
          originalEdges->UpdateCellCount(cellCount);
          newEdges.push_back(originalEdges->GetInsertLocation(cellCount));
          }
        else if (cellCount == 0)
          {
          partialEdge.push_back(pointId);
          }
        else
          {
          newEdges.push_back(-1);
          }

        newPoly.push_back(pointId);

        // Start a new edge with cornerPointId as a "virtual" point
        cornerPointId = pointId;
        oldOriginalId = pointId;
        cellCount = 1;

        // Rotate to the next point
        p0[0] = p2[0]; p0[1] = p2[1]; p0[2] = p2[2];
        p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
        v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
        l1 = l2;
        }
      else
        {
        if (cellCount > 0 && pointId != oldOriginalId)
          {
          // First check to see if we have to add cornerPointId
          if (cellCount == 1)
            {
            originalEdges->InsertNextCell(1);
            originalEdges->InsertCellPoint(cornerPointId);
            }
          // Then add the new point
          originalEdges->InsertCellPoint(pointId);
          oldOriginalId = pointId;
          cellCount++;
          }
        else
          {
          // No corner yet, so save the point
          partialEdge.push_back(pointId);
          }

        // Reduce the count
        m--;

        // Join the previous two segments, since the point was removed
        p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
        v1[0] = p2[0] - p0[0];  v1[1] = p2[1] - p0[1];  v1[2] = p2[2] - p0[2];
        l1 = vtkMath::Dot(v1, v1);
        }
      }

    // Add the partial edge to the end
    size_t partialSize = partialEdge.size();
    for (size_t ii = 0; ii < partialSize; ii++)
      {
      vtkIdType pointId = partialEdge[ii];
      if (pointId != oldOriginalId)
        {
        if (cellCount == 1)
          {
          originalEdges->InsertNextCell(1);
          originalEdges->InsertCellPoint(cornerPointId);
          }
        originalEdges->InsertCellPoint(pointId);
        oldOriginalId = pointId;
        cellCount++;
        }
      }

    // Finalize
    if (cellCount > 1)
      {
      originalEdges->UpdateCellCount(cellCount);
      newEdges.push_back(originalEdges->GetInsertLocation(cellCount));
      }

    polys[polyId] = newPoly;
    }
}

// ---------------------------------------------------
// Insert a triangle, and subdivide that triangle if one of
// its edges originally had more than two points before
// vtkCTPFFindTrueEdges was called.

static void vtkCTPFInsertTriangle(
  vtkCellArray *polys, const vtkCTPFPoly &poly, const size_t trids[3],
  const vtkCTPFPolyEdges &polyEdges, vtkCellArray *originalEdges)
{
  static const size_t nextVert[3] = { 1, 2, 0 };

  // To store how many of originalEdges match
  int edgeCount = 0;
  int edgeLocs[3];
  edgeLocs[0] = -1;
  edgeLocs[1] = -1;
  edgeLocs[2] = -1;

  // Check for original edge matches
  for (int vert = 0; vert < 3; vert++)
    {
    size_t currId = trids[vert];
    vtkIdType edgeLoc = polyEdges[currId];
    if (edgeLoc >= 0)
      {
      size_t nextId = currId+1;
      if (nextId == poly.size()) { nextId = 0; }

      // Is the triangle edge a polygon edge?
      if (nextId == trids[nextVert[vert]])
        {
        edgeLocs[vert] = edgeLoc;
        edgeCount++;
        }
      }
    }

  if (edgeCount == 0)
    {
    // No special edge handling, so just do one triangle

    polys->InsertNextCell(3);
    polys->InsertCellPoint(poly[trids[0]]);
    polys->InsertCellPoint(poly[trids[1]]);
    polys->InsertCellPoint(poly[trids[2]]);
    }
  else
    {
    // Make triangle fans for edges with extra points

    vtkIdType edgePtIds[4];
    edgePtIds[0] = poly[trids[0]];
    edgePtIds[1] = poly[trids[1]];
    edgePtIds[2] = poly[trids[2]];
    edgePtIds[3] = poly[trids[0]];

    const vtkIdType *edgePts[3];
    edgePts[0] = &edgePtIds[0];
    edgePts[1] = &edgePtIds[1];
    edgePts[2] = &edgePtIds[2];

    vtkIdType edgeNPts[3];
    edgeNPts[0] = 2;
    edgeNPts[1] = 2;
    edgeNPts[2] = 2;

    // Find out which edge has the most extra points
    vtkIdType maxPoints = 0;
    int currSide = 0;
    for (int i = 0; i < 3; i++)
      {
      if (edgeLocs[i] >= 0)
        {
        vtkIdType npts=0;
        vtkIdType const *pts = nullptr;
        originalEdges->GetCell(edgeLocs[i], npts, pts);
        assert(edgePts[i][0] == pts[0]);
        assert(edgePts[i][1] == pts[npts-1]);
        if (npts > maxPoints)
          {
          maxPoints = npts;
          currSide = i;
          }
        edgeNPts[i] = npts;
        edgePts[i] = pts;
        }
      }

    // Find the edges before/after the edge with most points
    int prevSide = (currSide+2)%3;
    int nextSide = (currSide+1)%3;

    // If other edges have only 2 points, nothing to do with them
    int prevNeeded = (edgeNPts[prevSide] > 2);
    int nextNeeded = (edgeNPts[nextSide] > 2);

    // The tail is the common point in the triangle fan
    vtkIdType tailPtIds[3];
    tailPtIds[prevSide] = edgePts[currSide][1];
    tailPtIds[currSide] = edgePts[prevSide][0];
    tailPtIds[nextSide] = edgePts[currSide][edgeNPts[currSide]-2];

    // Go through the sides and make the fans
    for (int side = 0; side < 3; side++)
      {
      if ((side != prevSide || prevNeeded) &&
          (side != nextSide || nextNeeded))
        {
        vtkIdType m = 0;
        vtkIdType n = edgeNPts[side]-1;

        if (side == currSide)
          {
          m += prevNeeded;
          n -= nextNeeded;
          }

        for (int k = m; k < n; k++)
          {
          polys->InsertNextCell(3);
          polys->InsertCellPoint(edgePts[side][k]);
          polys->InsertCellPoint(edgePts[side][k+1]);
          polys->InsertCellPoint(tailPtIds[side]);
          }
        }
      }
    }
}

// ---------------------------------------------------
// Check the sense of the polygon against the given normal.  Returns
// zero if the normal is zero.

int vtkCTPFCheckPolygonSense(
  vtkCTPFPoly &poly, vtkPoints *points, const double normal[3],
  int &sense)
{
  // Compute the normal
  double pnormal[3], p0[3], p1[3], p2[3], v1[3], v2[3], v[3];
  pnormal[0] = 0.0; pnormal[1] = 0.0; pnormal[2] = 0.0;

  points->GetPoint(poly[0], p0);
  points->GetPoint(poly[1], p1);
  v1[0] = p1[0] - p0[0];  v1[1] = p1[1] - p0[1];  v1[2] = p1[2] - p0[2];

  size_t n = poly.size();
  for (size_t jj = 2; jj < n; jj++)
    {
    points->GetPoint(poly[jj], p2);
    v2[0] = p2[0] - p0[0];  v2[1] = p2[1] - p0[1];  v2[2] = p2[2] - p0[2];
    vtkMath::Cross(v1, v2, v);
    pnormal[0] += v[0]; pnormal[1] += v[1]; pnormal[2] += v[2];
    p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
    }

  // Check the normal
  double d = vtkMath::Dot(pnormal, normal);

  sense = (d > 0);

  return (d != 0);
}

// ---------------------------------------------------
// Check whether innerPoly is inside outerPoly.
// The normal is needed to verify the polygon orientation.
// The values of pp, bounds, and tol2 must be precomputed
// by calling vtkCTPFPrepareForPolyInPoly() on outerPoly.

int vtkCTPFPolyInPoly(
  const vtkCTPFPoly &outerPoly, const vtkCTPFPoly &innerPoly,
  vtkPoints *points, const double normal[3],
  const double *pp, const double bounds[6],
  double tol2)
{
  // Find a vertex of poly "j" that isn't on the edge of poly "i".
  // This is necessary or the PointInPolygon might return "true"
  // based only on roundoff error.

  size_t n = outerPoly.size();
  size_t m = innerPoly.size();

  for (size_t jj = 0; jj < m; jj++)
    {
    // Semi-randomize the point order
    size_t kk = (jj>>1) + (jj&1)*((m+1)>>1);
    double p[3];
    points->GetPoint(innerPoly[kk], p);

    if (vtkPolygon::PointInPolygon(
        p, static_cast<int>(n), const_cast<double *>(pp),
        const_cast<double *>(bounds), const_cast<double *>(normal)))
      {
      int pointOnEdge = 0;
      double q1[3], q2[3];
      points->GetPoint(outerPoly[n-1], q1);

      for (size_t ii = 0; ii < n; ii++)
        {
        points->GetPoint(outerPoly[ii], q2);
        double t, dummy[3];
        // This method returns distance squared
        if (vtkLine::DistanceToLine(p, q1, q2, t, dummy) < tol2)
          {
          pointOnEdge = 1;
          break;
          }
        q1[0] = q2[0]; q1[1] = q2[1]; q1[2] = q2[2];
        }

      if (!pointOnEdge)
        {
        // Good result, point is in polygon
        return 1;
        }
      }
    }

  // No matches found
  return 0;
}

// ---------------------------------------------------
// Precompute values needed for the PolyInPoly check.
// The values that are returned are as follows:
// pp: an array of the polygon vertices
// bounds: the polygon bounds
// tol2: a tolerance value based on the size of the polygon
// (note: pp must be pre-allocated to the 3*outerPoly.size())

void vtkCTPFPrepareForPolyInPoly(
  const vtkCTPFPoly &outerPoly, vtkPoints *points,
  double *pp, double bounds[6], double &tol2)
{
  size_t n = outerPoly.size();

  if (n == 0)
    {
    tol2=0.0; // to avoid false positive warning about uninitialized value.
    return;
    }

  // Pull out the points
  for (size_t k = 0; k < n; k++)
    {
    double *p = &pp[3*k];
    points->GetPoint(outerPoly[k], p);
    }

  // Find the bounding box and tolerance for the polygon
  tol2 = (vtkCTPFPolygonBounds(outerPoly, points, bounds)*
          (VTK_CCS_POLYGON_TOLERANCE * VTK_CCS_POLYGON_TOLERANCE));
}

// ---------------------------------------------------
// Check for polygons within polygons.  Group the polygons
// if they are within each other.  Reverse the sense of
// the interior "hole" polygons.  A hole within a hole
// will be reversed twice and will become its own group.

void vtkCTPFMakeHoleyPolys(
  std::vector<vtkCTPFPoly> &newPolys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  const double normal[3])
{
  size_t numNewPolys = newPolys.size();
  if (numNewPolys <= 1)
    {
    return;
    }

  // Use bit arrays to keep track of inner polys
  vtkCTPFBitArray polyReversed;
  vtkCTPFBitArray innerPolys;

  // Find the maximum poly size
  size_t nmax = 1;
  for (size_t kk = 0; kk < numNewPolys; kk++)
    {
    size_t n = newPolys[kk].size();
    if (n > nmax) { nmax = n; }
    }

  // These are some values needed for poly-in-poly checks
  double *pp = new double[3*nmax];
  double bounds[6];
  double tol2;

  // Go through all polys
  for (size_t i = 0; i < numNewPolys; i++)
    {
    size_t n = newPolys[i].size();

    if (n < 3) { continue; }

    // Check if poly is reversed
    int sense = 0;
    if (vtkCTPFCheckPolygonSense(newPolys[i], points, normal, sense))
      {
      polyReversed.set(i, sense);
      }

    // Precompute some values needed for poly-in-poly checks
    vtkCTPFPrepareForPolyInPoly(newPolys[i], points, pp, bounds, tol2);

    // Look for polygons inside of this one
    for (size_t j = 0; j < numNewPolys; j++)
      {
      size_t m = newPolys[j].size();
      if (j == i || m < 3) { continue; }

      // Make sure polygon i is not in polygon j
      int isInteriorPoly = 0;
      for (size_t k = 1; k < polyGroups[j].size(); k++)
        {
        if (polyGroups[j][k] == i)
          {
          isInteriorPoly = 1;
          break;
          }
        }

      if (isInteriorPoly)
        {
        continue;
        }

      if (vtkCTPFPolyInPoly(newPolys[i], newPolys[j], points,
                           normal, pp, bounds, tol2))
        {
        // Add to group
        polyGroups[i].push_back(j);
        }
      }
    }

  delete [] pp;

  for (size_t j = 0; j < numNewPolys; j++)
    {
    // Remove the groups for reversed polys
    if (polyReversed.get(j))
      {
      polyGroups[j].clear();
      }
    // Polys inside the interior polys have their own groups, so remove
    // them from this group
    else if (polyGroups[j].size() > 1)
      {
      // Convert the group into a bit array, to make manipulation easier
      innerPolys.clear();
      for (size_t k = 1; k < polyGroups[j].size(); k++)
        {
        innerPolys.set(polyGroups[j][k], 1);
        }

      // Look for non-reversed polys inside this one
      for (size_t kk = 1; kk < polyGroups[j].size(); kk++)
        {
        // jj is the index of the inner poly
        size_t jj = polyGroups[j][kk];
        // If inner poly is not reversed then
        if (!polyReversed.get(jj))
          {
          // Remove that poly and all polys inside of it from the group
          for (size_t ii = 0; ii < polyGroups[jj].size(); ii++)
            {
            innerPolys.set(polyGroups[jj][ii], 0);
            }
          }
        }

      // Use the bit array to recreate the polyGroup
      polyGroups[j].clear();
      polyGroups[j].push_back(j);
      for (size_t jj = 0; jj < numNewPolys; jj++)
        {
        if (innerPolys.get(jj) != 0)
          {
          polyGroups[j].push_back(jj);
          }
        }
      }
    }
}

// ---------------------------------------------------
// Check line segment with point Ids (i, j) to make sure that it
// doesn't cut through the edges of any polys in the group.
// Return value of zero means check failed and the cut is not
// usable.

int vtkCTPFCheckCut(
  const std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  const double normal[3], const vtkCTPFPolyGroup &polyGroup,
  size_t outerPolyId, size_t innerPolyId,
  vtkIdType outerIdx, vtkIdType innerIdx)
{
  vtkIdType ptId1 = polys[outerPolyId][outerIdx];
  vtkIdType ptId2 = polys[innerPolyId][innerIdx];

  const double tol = VTK_CCS_POLYGON_TOLERANCE;

  double p1[3], p2[3];
  points->GetPoint(ptId1, p1);
  points->GetPoint(ptId2, p2);

  double w[3];
  w[0] = p2[0] - p1[0]; w[1] = p2[1] - p1[1]; w[2] = p2[2] - p1[2];
  double l = vtkMath::Normalize(w);

  // Cuts between coincident points are good
  if (l == 0)
    {
    return 1;
    }

  // Define a tolerance with units of distance squared
  double tol2 = l*l*tol*tol;

  // Check the sense of the cut: it must be pointing "in" for both polys.
  size_t polyId = outerPolyId;
  size_t polyIdx = outerIdx;
  double *r = p1;
  double *r2 = p2;

  for (int ii= 0; ii < 2; ii++)
    {
    const vtkCTPFPoly &poly = polys[polyId];
    size_t n = poly.size();
    size_t prevIdx = n - polyIdx - 1;
    size_t nextIdx = polyIdx + 1;
    if (prevIdx >= n) { prevIdx -= n; }
    if (nextIdx >= n) { nextIdx -= n; }

    double r1[3], r3[3];

    points->GetPoint(poly[prevIdx], r1);
    points->GetPoint(poly[nextIdx], r3);

    if (vtkCTPFVectorProgression(r, r1, r2, r3, normal) < 0)
      {
      return 0;
      }

    polyId = innerPolyId;
    polyIdx = innerIdx;
    r = p2;
    r2 = p1;
    }

  // Check for intersections of the cut with polygon edges.
  // First, create a cut plane that divides space at the cut line.
  double pc[4];
  vtkMath::Cross(normal, w, pc);
  pc[3] = -vtkMath::Dot(pc, p1);

  for (size_t i = 0; i < polyGroup.size(); i++)
    {
    const vtkCTPFPoly &poly = polys[polyGroup[i]];
    size_t n = poly.size();

    double q1[3];
    vtkIdType qtId1 = poly[n-1];
    points->GetPoint(qtId1, q1);
    double v1 = pc[0]*q1[0] + pc[1]*q1[1] + pc[2]*q1[2] + pc[3];
    int c1 = (v1 > 0);

    for (size_t j = 0; j < n; j++)
      {
      double q2[3];
      vtkIdType qtId2 = poly[j];
      points->GetPoint(qtId2, q2);
      double v2 = pc[0]*q2[0] + pc[1]*q2[1] + pc[2]*q2[2] + pc[3];
      int c2 = (v2 > 0);

      // If lines share an endpoint, they can't intersect,
      // so don't bother with the check.
      if (ptId1 != qtId1 && ptId1 != qtId2 &&
          ptId2 != qtId1 && ptId2 != qtId2)
        {
        // Check for intersection
        if ( (c1 ^ c2) || v1*v1 < tol2 || v2*v2 < tol2)
          {
          w[0] = q2[0] - q1[0]; w[1] = q2[1] - q1[1]; w[2] = q2[2] - q1[2];
          if (vtkMath::Dot(w, w) > 0)
            {
            double qc[4];
            vtkMath::Cross(normal, w, qc);
            qc[3] = -vtkMath::Dot(qc, q1);

            double u1 = qc[0]*p1[0] + qc[1]*p1[1] + qc[2]*p1[2] + qc[3];
            double u2 = qc[0]*p2[0] + qc[1]*p2[1] + qc[2]*p2[2] + qc[3];
            int d1 = (u1 > 0);
            int d2 = (u2 > 0);

            if ( (d1 ^ d2) )
              {
              // One final check to make sure endpoints aren't coincident
              double *p = p1;
              double *q = q1;
              if (v2*v2 < v1*v1) { p = p2; }
              if (u2*u2 < u1*u1) { q = q2; }
              if (vtkMath::Distance2BetweenPoints(p, q) > tol2)
                {
                return 0;
                }
              }
            }
          }
        }

      qtId1 = qtId2;
      q1[0] = q2[0]; q1[1] = q2[1]; q1[2] = q2[2];
      v1 = v2;
      c1 = c2;
      }
    }

  return 1;
}

// ---------------------------------------------------
// Check the quality of a cut between an outer and inner polygon.
// An ideal cut is one that forms a 90 degree angle with each
// line segment that it joins to.  Smaller values indicate a
// higher quality cut.

double vtkCTPFCutQuality(
  const vtkCTPFPoly &outerPoly, const vtkCTPFPoly &innerPoly,
  size_t i, size_t j, vtkPoints *points)
{
  size_t n = outerPoly.size();
  size_t m = innerPoly.size();

  size_t a = ((i > 0) ? i-1 : n-1);
  size_t b = ((i < n-1) ? i+1 : 0);

  size_t c = ((j > 0) ? j-1 : m-1);
  size_t d = ((j < m-1) ? j+1 : 0);

  double p0[3], p1[3], p2[3];
  points->GetPoint(outerPoly[i], p1);
  points->GetPoint(innerPoly[j], p2);

  double v1[3], v2[3];
  v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];

  double l1 = vtkMath::Dot(v1, v1);
  double l2;
  double qmax = 0;
  double q;

  points->GetPoint(outerPoly[a], p0);
  v2[0] = p0[0] - p1[0]; v2[1] = p0[1] - p1[1]; v2[2] = p0[2] - p1[2];
  l2 = vtkMath::Dot(v2, v2);
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2);
    q *= q/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(outerPoly[b], p0);
  v2[0] = p0[0] - p1[0]; v2[1] = p0[1] - p1[1]; v2[2] = p0[2] - p1[2];
  l2 = vtkMath::Dot(v2, v2);
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2);
    q *= q/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(innerPoly[c], p0);
  v2[0] = p2[0] - p0[0]; v2[1] = p2[1] - p0[1]; v2[2] = p2[2] - p0[2];
  l2 = vtkMath::Dot(v2, v2);
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2);
    q *= q/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(innerPoly[d], p0);
  v2[0] = p2[0] - p0[0]; v2[1] = p2[1] - p0[1]; v2[2] = p2[2] - p0[2];
  l2 = vtkMath::Dot(v2, v2);
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2);
    q *= q/l2;
    if (q > qmax) { qmax = q; }
    }

  if (l1 > 0)
    {
    return qmax/l1; // also l1 + qmax, incorporates distance;
    }

  return VTK_DOUBLE_MAX;
}

// ---------------------------------------------------
// Find the two sharpest verts on an inner (i.e. inside-out) poly.

void vtkCTPFFindSharpestVerts(
  const vtkCTPFPoly &poly, vtkPoints *points, const double normal[3],
  size_t verts[2])
{
  double p1[3], p2[3];
  double v1[3], v2[3], v[3];
  double l1, l2;

  double minVal[2];
  minVal[0] = 0;
  minVal[1] = 0;

  verts[0] = 0;
  verts[1] = 0;

  size_t n = poly.size();
  points->GetPoint(poly[n-1], p2);
  points->GetPoint(poly[0], p1);

  v1[0] = p1[0] - p2[0];  v1[1] = p1[1] - p2[1];  v1[2] = p1[2] - p2[2];
  l1 = sqrt(vtkMath::Dot(v1, v1));

  for (size_t j = 0; j < n; j++)
    {
    size_t k = j+1;
    if (k == n) { k = 0; }

    points->GetPoint(poly[k], p2);
    v2[0] = p2[0] - p1[0];  v2[1] = p2[1] - p1[1];  v2[2] = p2[2] - p1[2];
    l2 = sqrt(vtkMath::Dot(v2, v2));

    vtkMath::Cross(v1, v2, v);
    double b = vtkMath::Dot(v, normal);

    if (b > 0 && l1*l2 > 0)
      {
      // Dot product is |v1||v2|cos(theta), range [-1, +1]
      double val = vtkMath::Dot(v1, v2)/(l1*l2);
      if (val < minVal[0])
        {
        minVal[1] = minVal[0];
        minVal[0] = val;
        verts[1] = verts[0];
        verts[0] = j;
        }
      }

    // Rotate to the next point
    p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
    l1 = l2;
    }
}

// ---------------------------------------------------
// Find two valid cuts between outerPoly and innerPoly.
// Used by vtkCTPFCutHoleyPolys.

int vtkCTPFFindCuts(
  const std::vector<vtkCTPFPoly> &polys,
  const vtkCTPFPolyGroup &polyGroup, size_t outerPolyId, size_t innerPolyId,
  vtkPoints *points, const double normal[3], size_t cuts[2][2],
  size_t exhaustive)
{
  const vtkCTPFPoly &outerPoly = polys[outerPolyId];
  const vtkCTPFPoly &innerPoly = polys[innerPolyId];
  size_t innerSize = innerPoly.size();
  // Find the two sharpest points on the inner poly
  size_t verts[2];
  vtkCTPFFindSharpestVerts(innerPoly, points, normal, verts);

  // A list of cut locations according to quality
  typedef std::pair<double, size_t> vtkCTPFCutLoc;
  std::vector<vtkCTPFCutLoc> cutlist(outerPoly.size());

  // Search for potential cuts (need to find two cuts)
  int cutId = 0;
  cuts[0][0] = cuts[0][1] = 0;
  cuts[1][0] = cuts[1][1] = 0;

  for (cutId = 0; cutId < 2; cutId++)
    {
    int foundCut = 0;

    size_t count = (exhaustive ? innerSize : 3);

    for (size_t i = 0; i < count && !foundCut; i++)
      {
      // Semi-randomize the search order
      size_t j = (i>>1) + (i&1)*((innerSize+1)>>1);
      // Start at the best first point
      j = (j + verts[cutId])%innerSize;

      for (size_t kk = 0; kk < outerPoly.size(); kk++)
        {
        double q = vtkCTPFCutQuality(outerPoly, innerPoly, kk, j, points);
        cutlist[kk].first = q;
        cutlist[kk].second = kk;
        }

      std::sort(cutlist.begin(), cutlist.end());

      for (size_t lid = 0; lid < cutlist.size(); lid++)
        {
        size_t k = cutlist[lid].second;

        // If this is the second cut, do extra checks
        if (cutId > 0)
          {
          // Make sure cuts don't share an endpoint
          if (j == cuts[0][1] || k == cuts[0][0])
            {
            continue;
            }

          // Make sure cuts don't intersect
          double p1[3], p2[3];
          points->GetPoint(outerPoly[cuts[0][0]], p1);
          points->GetPoint(innerPoly[cuts[0][1]], p2);

          double q1[3], q2[3];
          points->GetPoint(outerPoly[k], q1);
          points->GetPoint(innerPoly[j], q2);

          double u, v;
          if (vtkLine::Intersection(p1, p2, q1, q2, u, v) == 2)
            {
            continue;
            }
          }

        // This check is done for both cuts
        if (vtkCTPFCheckCut(polys, points, normal, polyGroup,
                           outerPolyId, innerPolyId, k, j))
          {
          cuts[cutId][0] = k;
          cuts[cutId][1] = j;
          foundCut = 1;
          break;
          }
        }
      }

    if (!foundCut)
      {
      return 0;
      }
    }

  return 1;
}

// ---------------------------------------------------
// Helper for vtkCTPFCutHoleyPolys.  Change a polygon and a hole
// into two separate polygons by making two cuts between them.

void vtkCTPFMakeCuts(
  std::vector<vtkCTPFPoly> &polys,
  std::vector<vtkCTPFPolyEdges> &polyEdges,
  size_t outerPolyId, size_t innerPolyId,
  vtkPoints *points, const size_t cuts[2][2])
{
  double q[3], r[3];
  for (size_t bb = 0; bb < 2; bb++)
    {
    vtkIdType ptId1 = polys[outerPolyId][cuts[bb][0]];
    vtkIdType ptId2 = polys[innerPolyId][cuts[bb][1]];
    points->GetPoint(ptId1, q);
    points->GetPoint(ptId2, r);
    }

  vtkCTPFPoly &outerPoly = polys[outerPolyId];
  vtkCTPFPoly &innerPoly = polys[innerPolyId];

  vtkCTPFPolyEdges &outerEdges = polyEdges[outerPolyId];
  vtkCTPFPolyEdges &innerEdges = polyEdges[innerPolyId];

  // Generate new polys from the cuts
  size_t n = outerPoly.size();
  size_t m = innerPoly.size();
  size_t idx;

  // Generate poly1
  size_t n1 = n*(cuts[1][0] < cuts[0][0]) + cuts[1][0] - cuts[0][0] + 1;
  size_t n2 = n1 + m*(cuts[0][1] < cuts[1][1]) + cuts[0][1] - cuts[1][1] + 1;

  vtkCTPFPoly poly1(n2);
  vtkCTPFPolyEdges edges1(n2);

  idx = cuts[0][0];
  for (size_t i1 = 0; i1 < n1; i1++)
    {
    size_t k = idx++;
    poly1[i1] = outerPoly[k];
    edges1[i1] = outerEdges[k];
    idx *= (idx != n);
    }
  edges1[n1-1] = -1;

  idx = cuts[1][1];
  for (size_t i2 = n1; i2 < n2; i2++)
    {
    size_t k = idx++;
    poly1[i2] = innerPoly[k];
    edges1[i2] = innerEdges[k];
    idx *= (idx != m);
    }
  edges1[n2-1] = -1;

  // Generate poly2
  size_t m1 = n*(cuts[0][0] < cuts[1][0]) + cuts[0][0] - cuts[1][0] + 1;
  size_t m2 = m1 + m*(cuts[1][1] < cuts[0][1]) + cuts[1][1] - cuts[0][1] + 1;

  vtkCTPFPoly poly2(m2);
  vtkCTPFPolyEdges edges2(m2);

  idx = cuts[1][0];
  for (size_t j1 = 0; j1 < m1; j1++)
    {
    size_t k = idx++;
    poly2[j1] = outerPoly[k];
    edges2[j1] = outerEdges[k];
    idx *= (idx != n);
    }
  edges2[m1-1] = -1;

  idx = cuts[0][1];
  for (size_t j2 = m1; j2 < m2; j2++)
    {
    size_t k = idx++;
    poly2[j2] = innerPoly[k];
    edges2[j2] = innerEdges[k];
    idx *= (idx != m);
    }
  edges2[m2-1] = -1;

  // Replace outerPoly and innerPoly with these new polys
  polys[outerPolyId] = poly1;
  polys[innerPolyId] = poly2;
  polyEdges[outerPolyId] = edges1;
  polyEdges[innerPolyId] = edges2;
}

// ---------------------------------------------------
// After the holes have been identified, make cuts between the
// outer poly and each hole.  Make two cuts per hole.  The only
// strict requirement is that the cut must not intersect any
// edges, but it's best to make sure that no really sharp angles
// are created.

int vtkCTPFCutHoleyPolys(
  std::vector<vtkCTPFPoly> &polys, vtkPoints *points,
  std::vector<vtkCTPFPolyGroup> &polyGroups,
  std::vector<vtkCTPFPolyEdges> &polyEdges,
  const double normal[3])
{
  int cutFailure = 0;

  // Go through all groups and cut out the first inner poly that is
  // found.  Every time an inner poly is cut out, the groupId counter
  // is reset because a cutting a poly creates a new group.
  size_t groupId = 0;
  while (groupId < polyGroups.size())
    {
    vtkCTPFPolyGroup &polyGroup = polyGroups[groupId];

    // Only need to make a cut if the group size is greater than 1
    if (polyGroup.size() > 1)
      {
      // The first member of the group is the outer poly
      size_t outerPolyId = polyGroup[0];

      // The second member of the group is the first inner poly
      size_t innerPolyId = polyGroup[1];

      // Sort the group by size, do largest holes first
      std::vector<std::pair<size_t, size_t> >
        innerBySize(polyGroup.size());

      for (size_t i = 1; i < polyGroup.size(); i++)
        {
        innerBySize[i].first = polys[polyGroup[i]].size();
        innerBySize[i].second = i;
        }

      std::sort(innerBySize.begin()+1, innerBySize.end());
      std::reverse(innerBySize.begin()+1, innerBySize.end());

      // Need to check all inner polys in sequence, until one succeeds.
      // Do a quick search first, then do an exhaustive search.
      int madeCut = 0;
      size_t inner = 0;
      for (int exhaustive = 0; exhaustive < 2 && !madeCut; exhaustive++)
        {
        for (size_t j = 1; j < polyGroup.size(); j++)
          {
          inner = innerBySize[j].second;
          innerPolyId = polyGroup[inner];

          size_t cuts[2][2];
          if (vtkCTPFFindCuts(polys, polyGroup, outerPolyId, innerPolyId,
                             points, normal, cuts, exhaustive))
            {
            vtkCTPFMakeCuts(polys, polyEdges, outerPolyId, innerPolyId,
                           points, cuts);
            madeCut = 1;
            break;
            }
          }
        }

      if (madeCut)
        {
        // Move successfuly cut innerPolyId into its own group
        polyGroup.erase(polyGroup.begin() + inner);
        polyGroups[innerPolyId].push_back(innerPolyId);
        }
      else
        {
        // Remove all failed inner polys from the group
        for (size_t k = 1; k < polyGroup.size(); k++)
          {
          innerPolyId = polyGroup[k];
          polyGroups[innerPolyId].push_back(innerPolyId);
          }
        polyGroup.resize(1);
        cutFailure = 1;
        }

      // If there are other interior polys in the group, find out whether
      // they are in poly1 or poly2
      if (polyGroup.size() > 1)
        {
        vtkCTPFPoly &poly1 = polys[outerPolyId];
        double *pp = new double[3*poly1.size()];
        double bounds[6];
        double tol2;
        vtkCTPFPrepareForPolyInPoly(poly1, points, pp, bounds, tol2);

        size_t ii = 1;
        while (ii < polyGroup.size())
          {
          if (vtkCTPFPolyInPoly(poly1, polys[polyGroup[ii]],
                               points, normal, pp, bounds, tol2))
            {
            // Keep this poly in polyGroup
            ii++;
            }
          else
            {
            // Move this poly to poly2 group
            polyGroups[innerPolyId].push_back(polyGroup[ii]);
            polyGroup.erase(polyGroup.begin()+ii);

            // Reduce the groupId to ensure that this new group
            // will get cut
            if (innerPolyId < groupId)
              {
              groupId = innerPolyId;
              }
            }
          }
        delete [] pp;

        // Continue without incrementing groupId
        continue;
        }
      }

    // Increment to the next group
    groupId++;
    }

  return !cutFailure;
}
