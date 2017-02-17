// Copyright (C) 2014-2016 Anders Logg, August Johansson and Benjamin Kehlet
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// First added:  2014-02-03
// Last changed: 2017-02-17

#include <iomanip>
#include <dolfin/mesh/MeshEntity.h>
#include "predicates.h"
#include "GeometryPredicates.h"
#include "GeometryTools.h"
#include "GeometryDebugging.h"
#include "CollisionPredicates.h"
#include "IntersectionConstruction.h"

#ifdef augustdebug
#include "dolfin_simplex_tools.h"
#endif

using namespace dolfin;

//-----------------------------------------------------------------------------
// High-level intersection construction functions
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection(const MeshEntity& entity_0,
                                       const MeshEntity& entity_1)
{
  // Get data
  const MeshGeometry& g0 = entity_0.mesh().geometry();
  const MeshGeometry& g1 = entity_1.mesh().geometry();
  const unsigned int* v0 = entity_0.entities(0);
  const unsigned int* v1 = entity_1.entities(0);

  // Pack data as vectors of points
  std::vector<Point> points_0(entity_0.dim() + 1);
  std::vector<Point> points_1(entity_1.dim() + 1);
  for (std::size_t i = 0; i <= entity_0.dim(); i++)
    points_0[i] = g0.point(v0[i]);
  for (std::size_t i = 0; i <= entity_1.dim(); i++)
    points_1[i] = g1.point(v1[i]);

  // Only look at first entity to get geometric dimension
  std::size_t gdim = g0.dim();

  // Call common implementation
  return intersection(points_0, points_1, gdim);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection(const std::vector<Point>& p,
                                       const std::vector<Point>& q,
                                       std::size_t gdim)
{
  // Get topological dimensions
  const std::size_t d0 = p.size() - 1;
  const std::size_t d1 = q.size() - 1;

  // Pick correct specialized implementation

  // segment - segment
  if (d0 == 1 and d1 == 1)
  {
    std::vector<Point> points;
    switch (gdim)
    {
    case 1:
      // This case requires special handling to convert Point <--> double
      for (auto x : intersection_segment_segment_1d(p[0][0], p[1][0], q[0][0], q[1][0]))
	points.push_back(Point(x));
      return points;
    case 2: return intersection_segment_segment_2d(p[0], p[1], q[0], q[1]);
    case 3: return intersection_segment_segment_3d(p[0], p[1], q[0], q[1]);
    }
  }

  // segment - triangle
  else if (d0 == 1 and d1 == 2)
  {
    switch (gdim)
    {
    case 2: return intersection_triangle_segment_2d(q[0], q[1], q[2], p[0], p[1]);
    case 3: return intersection_triangle_segment_3d(q[0], q[1], q[2], p[0], p[1]);
    }
  }

  // triangle - segment
  else if (d0 == 2 and d1 == 1)
  {
    switch (gdim)
    {
    case 2: return intersection_triangle_segment_2d(p[0], p[1], p[2], q[0], q[1]);
    case 3: return intersection_triangle_segment_3d(p[0], p[1], p[2], q[0], q[1]);
    }
  }

  // triangle - triangle
  else if (d0 == 2 and d1 == 2)
  {
    switch (gdim)
    {
    case 2: return intersection_triangle_triangle_2d(p[0], p[1], p[2], q[0], q[1], q[2]);
    case 3: return intersection_triangle_triangle_3d(p[0], p[1], p[2], q[0], q[1], q[2]);
    }
  }

  // triangle - tetrahedron
  if (d0 == 2 and d1 == 3)
  {
    switch (gdim)
    {
    case 3: return intersection_tetrahedron_triangle_3d(q[0], q[1], q[2], q[3], p[0], p[1], p[2]);
    }
  }

  // tetrahedron - triangle
  if (d0 == 3 and d1 == 2)
  {
    switch (gdim)
    {
    case 3: return intersection_tetrahedron_triangle_3d(p[0], p[1], p[2], p[3], q[0], q[1], q[2]);
    }
  }

  // tetrahedron - tetrahedron
  if (d0 == 3 and d1 == 3)
  {
    switch (gdim)
    {
    case 3: return intersection_tetrahedron_tetrahedron_3d(p[0], p[1], p[2], p[3], q[0], q[1], q[2], q[3]);
    }
  }

  // We should not reach this point
  dolfin_error("IntersectionConstruction.cpp",
               "compute intersection",
               "Not implemented for dimensions %d / %d and geometric dimension %d", d0, d1, gdim);

  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
// Low-level intersection construction functions
//-----------------------------------------------------------------------------
std::vector<double>
IntersectionConstruction::intersection_point_point_1d(double p0,
                                                      double q0)
{
  if (p0 == q0)
    return std::vector<double>(1, p0);
  return std::vector<double>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_point_point_2d(const Point& p0,
                                                      const Point& q0)
{
  if (p0.x() == q0.x() && p0.y() == q0.y())
    return std::vector<Point>(1, p0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_point_point_3d(const Point& p0,
                                                      const Point& q0)
{
  if (p0.x() == q0.x() && p0.y() == q0.y() && p0.z() == q0.z())
    return std::vector<Point>(1, p0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<double>
IntersectionConstruction::intersection_segment_point_1d(double p0,
                                                        double p1,
                                                        double q0)
{
  if (CollisionPredicates::collides_segment_point_1d(p0, p1, q0))
    return std::vector<double>(1, q0);
  return std::vector<double>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_segment_point_2d(const Point& p0,
                                                        const Point& p1,
                                                        const Point& q0)
{
  if (CollisionPredicates::collides_segment_point_2d(p0, p1, q0))
    return std::vector<Point>(1, q0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_segment_point_3d(const Point& p0,
                                                        const Point& p1,
                                                        const Point& q0)
{
  if (CollisionPredicates::collides_segment_point_3d(p0, p1, q0))
    return std::vector<Point>(1, q0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_point_2d(const Point& p0,
                                                         const Point& p1,
                                                         const Point& p2,
                                                         const Point& q0)
{
  if (CollisionPredicates::collides_triangle_point_2d(p0, p1, p2, q0))
    return std::vector<Point>(1, q0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_point_3d(const Point& p0,
                                                         const Point& p1,
                                                         const Point& p2,
                                                         const Point& q0)
{
  if (CollisionPredicates::collides_triangle_point_3d(p0, p1, p2, q0))
    return std::vector<Point>(1, q0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_tetrahedron_point_3d(const Point& p0,
                                                            const Point& p1,
                                                            const Point& p2,
                                                            const Point& p3,
                                                            const Point& q0)
{
  if (CollisionPredicates::collides_tetrahedron_point_3d(p0, p1, p2, p3, q0))
    return std::vector<Point>(1, q0);
  return std::vector<Point>();
}
//-----------------------------------------------------------------------------
std::vector<double>
IntersectionConstruction::intersection_segment_segment_1d(double p0,
                                                          double p1,
                                                          double q0,
                                                          double q1)
{
  // The list of points (convex hull)
  std::vector<double> points;

  // Compute orientation of segment end points wrt other segment
  const double p0o = orient1d(q0, q1, p0);
  const double p1o = orient1d(q0, q1, p1);
  const double q0o = orient1d(p0, p1, q0);
  const double q1o = orient1d(p0, p1, q1);

  // Compute total orientation of segments wrt other segment
  const double po = p0o*p1o;
  const double qo = q0o*q1o;

  // Special case: no collision
  if (po > 0. or qo > 0.)
    return points;

  // Indicators to avoid duplicates
  bool p0i = false, p1i = false, q0i = false, q1i = false;

  // Special case: end point collision
  add_if_equal(points, p0, q0, p0i, q0i);
  add_if_equal(points, p0, q1, p0i, q1i);
  add_if_equal(points, p1, q0, p1i, q0i);
  add_if_equal(points, p1, q1, p1i, q1i);

  // Main case: interior collisions
  if (!p0i and p0o == 0.) points.push_back(p0);
  if (!p1i and p1o == 0.) points.push_back(p1);
  if (!q0i and q0o == 0.) points.push_back(q0);
  if (!q1i and q1o == 0.) points.push_back(q1);

  return points;
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_segment_segment_2d(const Point& p0,
                                                          const Point& p1,
                                                          const Point& q0,
                                                          const Point& q1)
{
  // FIXME: This function stil uses add_if_equal and tries to avoid
  // adding duplicates. Simplify by using add(points, intersection_foo) etc.

  // The list of points (convex hull)
  std::vector<Point> points;

  //GeometryDebugging::plot({p0, p1}, {q0, q1});

  // Compute orientation of segment end points wrt other segment
  const double p0o = orient2d(q0, q1, p0);
  const double p1o = orient2d(q0, q1, p1);
  const double q0o = orient2d(p0, p1, q0);
  const double q1o = orient2d(p0, p1, q1);

  // Compute total orientation of segments wrt other segment
  const double po = p0o*p1o;
  const double qo = q0o*q1o;

  // Special case: no collision
  if (po > 0. or qo > 0.)
    return points;

  // Special case: *possible* end point collision(s).
  // Note that segments may be collinear without colliding.
  if (po == 0. or qo == 0.)
  {
    // Indicators to avoid duplicates
    bool p0i = false, p1i = false, q0i = false, q1i = false;

    // Check point-point collisions
    add_if_equal(points, p0, q0, p0i, q0i);
    add_if_equal(points, p0, q1, p0i, q1i);
    add_if_equal(points, p1, q0, p1i, q0i);
    add_if_equal(points, p1, q1, p1i, q1i);

    // Check end points of first segment
    if (po == 0.)
    {
      // Project points to major axis of second segment
      const std::size_t major_axis = GeometryTools::major_axis_2d(q1 - q0);
      const double P0 = GeometryTools::project_to_axis_2d(p0, major_axis);
      const double P1 = GeometryTools::project_to_axis_2d(p1, major_axis);
      const double Q0 = GeometryTools::project_to_axis_2d(q0, major_axis);
      const double Q1 = GeometryTools::project_to_axis_2d(q1, major_axis);

      // Check collisions
      if (!p0i and p0o == 0. and CollisionPredicates::collides_segment_point_1d(Q0, Q1, P0))
        points.push_back(p0);
      if (!p1i and p1o == 0. and CollisionPredicates::collides_segment_point_1d(Q0, Q1, P1))
        points.push_back(p1);
    }

    // Check end points of second segment
    if (qo == 0.)
    {
      // Project points to major axis of first segment
      const std::size_t major_axis = GeometryTools::major_axis_2d(p1 - p0);
      const double P0 = GeometryTools::project_to_axis_2d(p0, major_axis);
      const double P1 = GeometryTools::project_to_axis_2d(p1, major_axis);
      const double Q0 = GeometryTools::project_to_axis_2d(q0, major_axis);
      const double Q1 = GeometryTools::project_to_axis_2d(q1, major_axis);

      // Check collisions
      if (!q0i and q0o == 0. and CollisionPredicates::collides_segment_point_1d(P0, P1, Q0))
        points.push_back(q0);
      if (!q1i and q1o == 0. and CollisionPredicates::collides_segment_point_1d(P0, P1, Q1))
        points.push_back(q1);
    }

    return points;
  }

  // At this point, we know that both po < 0 and q0 < 0 which means
  // that we have an intersection and it is internal to both segments.
  // This is the main case. The point is given by the formula
  //
  //   x = p0 + num / den * (p1 - p0)
  //
  // However, the computation may be unstable when the two segments
  // are nearly collinear (when den is small) so special hanndling is
  // needed when this happens. To improve the chance of the point
  // ending up inside both segments, we swap the points so that the
  // computation is based on the shortest segment.

  // Compute intersection point based on shortes segment
  double num = 0., den = 0.; Point x;
  if (p0.squared_distance(p1) < q0.squared_distance(q1))
  {
    num = p0o;
    den = (p1.x() - p0.x())*(q1.y() - q0.y())
        - (p1.y() - p0.y())*(q1.x() - q0.x());
    x = p0 + num / den * (p1 - p0);
  }
  else
  {
    num = q0o;
    den = (q1.x() - q0.x())*(p1.y() - p0.y())
        - (q1.y() - q0.y())*(p1.x() - p0.x());
    x = q0 + num / den * (q1 - q0);
  }

  // Special case: almost collinear segments. Intersection is very
  // hard to compute so just make sure we pick a sensible point which
  // we know (almost) belongs to both segments.
  if (std::abs(den*den) < DOLFIN_EPS_LARGE*std::abs(num))
  {
    // Compute major axis
    const std::size_t major_axis = GeometryTools::major_axis_2d(p1 - p0);

    // Sort the points along major axis
    std::array<Point, 4> _points = {p0, p1, q0, q1};
    std::sort(_points.begin(), _points.end(),
              [major_axis](const Point& a, const Point& b)
              { return a[major_axis] < b[major_axis]; });

    // Compute midpoint
    const Point x = 0.5*(_points[1] + _points[2]);
    points.push_back(x);

    dolfin_assert(points.size() == 1);
    return points;
  }

  // Main case: add intersection point
  points.push_back(x);

  dolfin_assert(points.size() == 1);
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_segment_segment_3d(const Point& p0,
                                                          const Point& p1,
                                                          const Point& q0,
                                                          const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Avoid some unnecessary computations
  if (!CollisionPredicates::collides_segment_segment_3d(p0, p1, q0, q1))
    return points;

  // FIXME: Can we reduce to 1d?

  points.reserve(4);

  // Check if the segment is actually a point
  if (p0 == p1)
  {
    if (CollisionPredicates::collides_segment_point_3d(q0, q1, p0))
    {
      points.push_back(p0);
      // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
      return points;
    }
  }

  if (q0 == q1)
  {
    if (CollisionPredicates::collides_segment_point_3d(p0, p1, q0))
    {
      points.push_back(q0);
      // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
      return points;
    }
  }

  // First test points to match procedure of
  // _collides_segment_segment_3d.
  if (CollisionPredicates::collides_segment_point_3d(q0, q1, p0))
  {
    points.push_back(p0);
  }
  if (CollisionPredicates::collides_segment_point_3d(q0, q1, p1))
  {
    points.push_back(p1);
  }
  if (CollisionPredicates::collides_segment_point_3d(p0, p1, q0))
  {
    points.push_back(q0);
  }
  if (CollisionPredicates::collides_segment_point_3d(p0, p1, q1))
  {
    points.push_back(q1);
  }

  // Now we may have found all the intersections
  if (points.size() == 1)
  {
    // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
    return points;
  }
  else if (points.size() > 1)
  {
    std::vector<Point> unique = _unique_points(points);
    dolfin_assert(points.size() == 2 ?
    		  (unique.size() == 1 or unique.size() == 2) :
    		  unique.size() == 2);
    // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
    return unique;
  }

  // Follow Shewchuk Lecture Notes on Geometric Robustness
  const Point w = p0 - p1;
  const Point v = q0 - q1;
  const Point u = p1 - q1;
  const Point wv = w.cross(v);
  const Point vu = v.cross(u);
  const double den = wv.squared_norm();
  const double num = wv.dot(vu);

  if (den == 0. and num == 0)
  {
    //PPause;
  }
  else if (den == 0 and num != 0)
  {
    // Parallel, disjoint
    //PPause;
  }
  else if (den != 0)
  {
    // Test Shewchuk

    // If fraction is close to 1, swap p0 and p1
    Point x0;

    if (std::abs(num / den - 1) < DOLFIN_EPS_LARGE)
    {
      const Point u_swapped = p0 - q1;
      const Point vu_swapped = v.cross(u_swapped);
      const double num_swapped = -wv.dot(vu_swapped);
      x0 = p0 + num_swapped / den * (p1 - p0);
      // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
    }
    else
    {
      // std::cout << __FUNCTION__<<' '<<__LINE__<<std::endl;
      x0 = p1 + num / den * (p0 - p1);
    }

    points.push_back(x0);
  }

  dolfin_assert(GeometryPredicates::is_finite(points));
  std::vector<Point> unique = _unique_points(points);
  return unique;
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_segment_2d(const Point& p0,
                                                           const Point& p1,
                                                           const Point& p2,
                                                           const Point& q0,
                                                           const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (2)
  _add(points, intersection_triangle_point_2d(p0, p1, p2, q0));
  _add(points, intersection_triangle_point_2d(p0, p1, p2, q1));

  // Add segment-segment intersections (3)
  _add(points, intersection_segment_segment_2d(p0, p1, q0, q1));
  _add(points, intersection_segment_segment_2d(p0, p2, q0, q1));
  _add(points, intersection_segment_segment_2d(p1, p2, q0, q1));

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_segment_3d(const Point& p0,
                                                           const Point& p1,
                                                           const Point& p2,
                                                           const Point& q0,
                                                           const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Compute orientation of segment end points wrt triangle plane
  const double q0o = orient3d(p0, p1, p2, q0);
  const double q1o = orient3d(p0, p1, p2, q1);

  // Compute total orientation of segment wrt triangle plane
  const double qo = q0o*q1o;

  // Special case: no collision
  if (qo > 0.)
    return points;

  // Compute triangle plane normal and major axis
  const Point n = GeometryTools::cross_product(p0, p1, p2);
  std::size_t major_axis = GeometryTools::major_axis_3d(n);

  // Project points to major axis plane
  const Point P0 = GeometryTools::project_to_plane_3d(p0, major_axis);
  const Point P1 = GeometryTools::project_to_plane_3d(p1, major_axis);
  const Point P2 = GeometryTools::project_to_plane_3d(p2, major_axis);
  const Point Q0 = GeometryTools::project_to_plane_3d(q0, major_axis);
  const Point Q1 = GeometryTools::project_to_plane_3d(q1, major_axis);

  /*

    Becomes very complicated. The segment points can be touching the triangle,
    but the triangle point can also be touching the segment... Need to think
    about maybe reusing lower dim functions?

   */



  /*
  // Special case: *possible* end point collision(s).
  // Note that segment may be in triangle plane without colliding.
  if (qo== 0.)
  {
    // Compute

    if (o0 == 0.)
    {
      if (CollisionPredicates::collides_triangle_point_2d(P0, P1, P2, Q0))
	points.push_back(q0);
    }

    if (o1 == 0.)
    {
      if (CollisionPredicates::collides_triangle_point_2d(P0, P1, P2, Q1))
	points.push_back(q1);
    }

    return points;
  }


  // FIXME: Edited to this point


  // FIXME: insert const on some variables here


  // Project points to major axis plane
  Point P0 = GeometryTools::project_to_plane_3d(p0, major_axis);
  Point P1 = GeometryTools::project_to_plane_3d(p1, major_axis);
  Point P2 = GeometryTools::project_to_plane_3d(p2, major_axis);
  Point Q0 = GeometryTools::project_to_plane_3d(q0, major_axis);
  Point Q1 = GeometryTools::project_to_plane_3d(q1, major_axis);

  // FIXME: We are missing the case when q0 and q1 are both in the
  // plane but outside

  // FIXME: Use collides_triangle_segment_2d

  // FIXME: Add function _unproject_point(p, major_axis)


  // At this point, we know that the two end points are on different
  // sides of the triangle plane (o0*o1 < 0).

  // FIXME: We don't check the size of |den|. If it is small then
  // the computation will be unstable and the point we compute may
  // happen to end up outside the triangle. On the other hand, we
  // check that the point is actually inside... So the result if
  // the plane and the segment are close to parallel is that we
  // return an empty list. Maybe that is ok?

  // Formula for intersection point: x = q0 + num / den * (q1 - q0)
  const double num = n.dot(p0 - q0);
  const double den = n.dot(q1 - q0);

  // Compute intersection point
  const Point x = q0 + num / den * (q1 - q0);

  // Check if intersection point is inside the triangle
  const Point X = GeometryTools::project_to_plane_3d(x, major_axis);
  if (CollisionPredicates::collides_triangle_point_2d(P0, P1, P2, X))
  {
    points.push_back(x);
  }

  dolfin_assert(GeometryPredicates::is_finite(points));

  */

  return points;
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_tetrahedron_segment_3d(const Point& p0,
                                                              const Point& p1,
                                                              const Point& p2,
                                                              const Point& p3,
                                                              const Point& q0,
                                                              const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (4 + 4 = 8)
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q0));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q1));

  // Add triangle-segment intersections (4)
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q1));

  return points;
}
//-----------------------------------------------------------------------------
// Intersections with triangles and tetrahedra: computed by delegation
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_triangle_2d(const Point& p0,
                                                            const Point& p1,
                                                            const Point& p2,
                                                            const Point& q0,
                                                            const Point& q1,
                                                            const Point& q2)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (3 + 3 = 6)
  _add(points, intersection_triangle_point_2d(p0, p1, p2, q0));
  _add(points, intersection_triangle_point_2d(p0, p1, p2, q1));
  _add(points, intersection_triangle_point_2d(p0, p1, p2, q2));
  _add(points, intersection_triangle_point_2d(q0, q1, q2, p0));
  _add(points, intersection_triangle_point_2d(q0, q1, q2, p1));
  _add(points, intersection_triangle_point_2d(q0, q1, q2, p2));

  // Add segment-segment intersections (3 x 3 = 9)
  _add(points, intersection_segment_segment_2d(p0, p1, q0, q1));
  _add(points, intersection_segment_segment_2d(p0, p1, q0, q2));
  _add(points, intersection_segment_segment_2d(p0, p1, q1, q2));
  _add(points, intersection_segment_segment_2d(p0, p2, q0, q1));
  _add(points, intersection_segment_segment_2d(p0, p2, q0, q2));
  _add(points, intersection_segment_segment_2d(p0, p2, q1, q2));
  _add(points, intersection_segment_segment_2d(p1, p2, q0, q1));
  _add(points, intersection_segment_segment_2d(p1, p2, q0, q2));
  _add(points, intersection_segment_segment_2d(p1, p2, q1, q2));

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_triangle_triangle_3d(const Point& p0,
                                                            const Point& p1,
                                                            const Point& p2,
                                                            const Point& q0,
                                                            const Point& q1,
                                                            const Point& q2)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (3 + 3 = 6)
  _add(points, intersection_triangle_point_3d(p0, p1, p2, q0));
  _add(points, intersection_triangle_point_3d(p0, p1, p2, q1));
  _add(points, intersection_triangle_point_3d(p0, p1, p2, q2));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p0));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p1));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p2));

  // Add triangle-segment intersections (3 + 3 = 6)
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q1, q2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p1));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p1, p2));

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_tetrahedron_triangle_3d(const Point& p0,
                                                               const Point& p1,
                                                               const Point& p2,
                                                               const Point& p3,
                                                               const Point& q0,
                                                               const Point& q1,
                                                               const Point& q2)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (3 + 4 = 7)
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q0));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q1));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q2));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p0));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p1));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p2));
  _add(points, intersection_triangle_point_3d(q0, q1, q2, p3));

  // Add triangle-segment intersections (4 x 3 + 1 x 6 = 18)
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q1, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p1));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p1, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p1, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p2, p3));

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::intersection_tetrahedron_tetrahedron_3d(const Point& p0,
                                                                  const Point& p1,
                                                                  const Point& p2,
                                                                  const Point& p3,
                                                                  const Point& q0,
                                                                  const Point& q1,
                                                                  const Point& q2,
                                                                  const Point& q3)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Add point intersections (4 + 4 = 8)
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q0));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q1));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q2));
  _add(points, intersection_tetrahedron_point_3d(p0, p1, p2, p3, q3));
  _add(points, intersection_tetrahedron_point_3d(q0, q1, q2, q3, p0));
  _add(points, intersection_tetrahedron_point_3d(q0, q1, q2, q3, p1));
  _add(points, intersection_tetrahedron_point_3d(q0, q1, q2, q3, p2));
  _add(points, intersection_tetrahedron_point_3d(q0, q1, q2, q3, p3));

  // Let's hope we got this right... :-)

  // Add triangle-segment intersections (4 x 6 + 4 x 6 = 48)
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q0, q3));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q1, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q1, q3));
  _add(points, intersection_triangle_segment_3d(p0, p1, p2, q2, q3));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q0, q3));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q1, q3));
  _add(points, intersection_triangle_segment_3d(p0, p1, p3, q2, q3));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q0, q3));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q1, q3));
  _add(points, intersection_triangle_segment_3d(p0, p2, p3, q2, q3));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q1));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q2));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q0, q3));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q1, q2));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q1, q3));
  _add(points, intersection_triangle_segment_3d(p1, p2, p3, q2, q3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p1));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p0, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p1, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p1, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q2, p2, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p0, p1));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p0, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p0, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p1, p2));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p1, p3));
  _add(points, intersection_triangle_segment_3d(q0, q1, q3, p2, p3));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p0, p1));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p0, p2));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p0, p3));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p1, p2));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p1, p3));
  _add(points, intersection_triangle_segment_3d(q0, q2, q3, p2, p3));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p0, p1));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p0, p2));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p0, p3));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p1, p2));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p1, p3));
  _add(points, intersection_triangle_segment_3d(q1, q2, q3, p2, p3));

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}
//-----------------------------------------------------------------------------
std::vector<Point>
IntersectionConstruction::_unique_points(const std::vector<Point>& input_points)
{
  std::vector<Point> unique;
  unique.reserve(input_points.size());

  for (std::size_t i = 0; i < input_points.size(); ++i)
  {
    bool found = false;
    for (std::size_t j = i+1; j < input_points.size(); ++j)
      if (input_points[i] == input_points[j])
      {
	found = true;
	break;
      }
    if (!found)
      unique.push_back(input_points[i]);
  }

  return unique;
}
//-----------------------------------------------------------------------------

// FIXME: Old versions kept around for reference, remove when working

std::vector<double>
IntersectionConstruction::_intersection_segment_segment_1d_old(double p0,
                                                               double p1,
                                                               double q0,
                                                               double q1)
{
  // FIXME: This function is not topologicaly correct in case where the two segments
  // overlap only in 1 point

  // The list of points (convex hull)
  std::vector<double> intersection;

  if (CollisionPredicates::collides_segment_segment_1d(p0, p1, q0, q1))
  {
    // Get range
    const double a0 = std::min(p0, p1);
    const double b0 = std::max(p0, p1);
    const double a1 = std::min(q0, q1);
    const double b1 = std::max(q0, q1);
    const double dx = std::min(b0 - a0, b1 - a1);

    intersection.resize(2);
    if (b0 - a1 < dx)
    {
      intersection[0] = a1;
      intersection[1] = b0;
    }
    else if (b1 - a0 < dx)
    {
      intersection[0] = a0;
      intersection[1] = b1;
    }
    else if (b0 - a0 < b1 - a1)
    {
      intersection[0] = a0;
      intersection[1] = b0;
    }
    else
    {
      intersection[0] = a1;
      intersection[1] = b1;
    }
  }

  return intersection;
}

std::vector<Point>
IntersectionConstruction::_intersection_segment_segment_2d_old(const Point& p0,
                                                               const Point& p1,
                                                               const Point& q0,
                                                               const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  // Avoid some unnecessary computations
  if (!CollisionPredicates::collides_segment_segment_2d(p0, p1, q0, q1))
    return points;

  // Can we reduce to axis-aligned 1d?
  for (std::size_t d = 0; d < 2; ++d)
  {
    // Check if coordinates in dimension d is the same
    const bool reduce = (p0[d] == p1[d] and p1[d] == q0[d] and q0[d] == q1[d]);
    if (reduce)
    {
      const std::size_t j = (d+1) % 2;
      const std::vector<double> intersection_1d =
	intersection_segment_segment_1d(p0[j], p1[j], q0[j], q1[j]);
      points.resize(intersection_1d.size());
      for (std::size_t k = 0; k < points.size(); ++k)
      {
	points[k][d] = p0[d];
	points[k][j] = intersection_1d[k];
      }
      return points;
    }
  }

  points.reserve(4);

  // Check if the segment is actually a point
  if (p0 == p1)
  {
    if (CollisionPredicates::collides_segment_point_2d(q0, q1, p0))
    {
      points.push_back(p0);
      return points;
    }
  }

  if (q0 == q1)
  {
    if (CollisionPredicates::collides_segment_point_2d(p0, p1, q0))
    {
      points.push_back(q0);
      return points;
    }
  }

  // First test points to match procedure of
  // _collides_segment_segment_2d.
  if (CollisionPredicates::collides_segment_point_2d(q0, q1, p0))
  {
    points.push_back(p0);
  }
  if (CollisionPredicates::collides_segment_point_2d(q0, q1, p1))
  {
    points.push_back(p1);
  }
  if (CollisionPredicates::collides_segment_point_2d(p0, p1, q0))
  {
    points.push_back(q0);
  }
  if (CollisionPredicates::collides_segment_point_2d(p0, p1, q1))
  {
    points.push_back(q1);
  }

#ifdef augustdebug
  std::cout << " after point collisions: " <<points.size()<<" points: ";
  for (const Point p: points)
    std::cout << tools::plot(p);
  std::cout << std::endl;
#endif

  // Now we may have found all the intersectiosn
  if (points.size() == 1)
  {
    return points;
  }
  else if (points.size() > 1)
  {
    std::vector<Point> unique = _unique_points(points);

    // assert that we only have one or two points
    dolfin_assert(points.size() == 2 ?
    		  (unique.size() == 1 or unique.size() == 2) :
    		  unique.size() == 2);
    return unique;
  }

  // The intersection is in principle given by p0 + num / den *
  // (p1 - p0), but we first identify certain cases where den==0 and
  // / or num == 0.

  // Use shortest distance as P0, P1
  const bool use_p = p0.squared_distance(p1) < q0.squared_distance(q1);
  Point P0, P1, Q0, Q1;
  if (use_p)
  {
    P0 = p0;
    P1 = p1;
    Q0 = q0;
    Q1 = q1;
  }
  else
  {
    P0 = q0;
    P1 = q1;
    Q0 = p0;
    Q1 = p1;
  }

  const double num = orient2d(Q0, Q1, P0);
  const double den = (P1.x()-P0.x())*(Q1.y()-Q0.y()) - (P1.y()-P0.y())*(Q1.x()-Q0.x());

#ifdef augustdebug
  std::cout << "num=" << num << " den="<<den << " (equal? "<<(num==den) <<")\n";
#endif

  // Case 0 (den = num = 0): segments are collinear
  if (den == 0. and num == 0.)
  {
    const Point r = Q1 - p0;
    const double r2 = r.squared_norm();
    const Point rn = r / std::sqrt(r2);

    // FIXME: what to do if small?
    dolfin_assert(r2 > DOLFIN_EPS);

    double t0 = (Q0 - P0).dot(r) / r2;
    double t1 = (Q1 - P0).dot(r) / r2;
    if (t0 > t1)
    {
#ifdef augustdebug
      std::cout << "  swapped t0 and t1\n";
#endif
      std::swap(t0, t1);
    }

    if (CollisionPredicates::collides_segment_segment_1d(t0, t1, 0, 1))
    {
      // Compute two intersection points
      const Point z0 = P0 + std::max(0.0, t0)*r;
      const Point z1 = P0 + std::min(1.0, (Q0 - P0).dot(r) / r2 )*r;

#ifdef augustdebug
      std::cout << "case 1a"<<std::endl;
      std::cout.precision(22);
      std::cout <<"t0="<< t0<<"; t1="<<t1<<"; "<<tools::plot(p0)<<tools::plot(p1)<<" gave:\n"
      		<<tools::plot(z0,"'r.'")<<tools::plot(z1,"'g.'")<<std::endl;
#endif
      points.push_back(z0);
      points.push_back(z1);
    }
    else // Disjoint: no intersection
    {
#ifdef augustdebug
      std::cout << "case 1b"<<std::endl;
#endif
    }
  }
  // Case 1 (den = 0, num != 0): segments parallel but disjoint
  else if (den == 0)
  {
    // FIXME: Check whether we need this, should never reach this code!
    // Do nothing
    // Parallel, disjoint
#ifdef augustdebug
    std::cout << "case 2"<<std::endl;
    std::cout << den<<' '<<num << std::endl;
#endif
  }
  // Case 2 (den != 0, num != 0): segments not nearly paralllel.
  // This is the main case, just use formula for computing intersection.
  else if (std::abs(den*den) > DOLFIN_EPS_LARGE*std::abs(num))
  {
    Point x0 = P0 + num / den * (P1 - P0);

#ifdef augustdebug
    std::cout << "robust_linear_combination gave " << tools::plot(x0)<<std::endl;
#endif

    points.push_back(x0);
  }
  // Case 3 (den! = 0, num != 0): segments nearly parallel.
  // Computation very unstable, so instead return something sensible (midpoint).
  else // den is small
  {
    // Now the line segments are almost collinear. Project on (1,0)
    // and (0,1) to find the largest projection. Note that Q0, Q1 is
    // longest.
    const std::size_t dim = (std::abs(Q0.x() - Q1.x()) > std::abs(Q0.y() - Q1.y())) ? 0 : 1;

    // Sort the points according to dim
    std::array<Point, 4> _points = { P0, P1, Q0, Q1 };
    std::sort(_points.begin(), _points.end(), [dim](Point a, Point b) { return a[dim] < b[dim]; });

    // Return midpoint
    Point xm = (_points[1] + _points[2]) / 2;

#ifdef augustdebug
    std::cout << "projection intersection with |den|="<<std::abs(den)<< " and difference "<< std::abs(std::abs(den)-std::abs(num)) <<" using P0, P1, Q0, Q1 " << tools::plot(P0)<<tools::plot(P1)<<tools::plot(Q0)<<tools::plot(Q1)<<"and dim = " << dim<< " gives\n" << tools::plot(xm)<<std::endl;
#endif
    points.push_back(xm);
  }

  dolfin_assert(GeometryPredicates::is_finite(points));
  const std::vector<Point> unique = _unique_points(points);
  return unique;
}

std::vector<Point>
IntersectionConstruction::_intersection_triangle_segment_2d_old(const Point& p0,
                                                                const Point& p1,
                                                                const Point& p2,
                                                                const Point& q0,
                                                                const Point& q1)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  if (CollisionPredicates::collides_triangle_point_2d(p0, p1, p2, q0))
    points.push_back(q0);
  if (CollisionPredicates::collides_triangle_point_2d(p0, p1, p2, q1))
    points.push_back(q1);

  if (CollisionPredicates::collides_segment_segment_2d(p0, p1, q0, q1))
  {
    const std::vector<Point> intersection = intersection_segment_segment_2d(p0, p1, q0, q1);
    // FIXME: Should we require consistency between collision and intersection
    //dolfin_assert(intersection.size());
    points.insert(points.end(), intersection.begin(), intersection.end());
  }

  if (CollisionPredicates::collides_segment_segment_2d(p0, p2, q0, q1))
  {
    const std::vector<Point> intersection = intersection_segment_segment_2d(p0, p2, q0, q1);
    //dolfin_assert(intersection.size());
    points.insert(points.end(), intersection.begin(), intersection.end());
  }

  if (CollisionPredicates::collides_segment_segment_2d(p1, p2, q0, q1))
  {
    const std::vector<Point> intersection = intersection_segment_segment_2d(p1, p2, q0, q1);
    //dolfin_assert(intersection.size());
    points.insert(points.end(), intersection.begin(), intersection.end());
  }

  // Remove strict duplictes. Use exact equality here. Approximate
  // equality is for ConvexTriangulation.
  // FIXME: This can be avoided if we use interior segment tests.
  dolfin_assert(GeometryPredicates::is_finite(points));
  const std::vector<Point> unique = _unique_points(points);
  return unique;
}

std::vector<Point>
IntersectionConstruction::_intersection_triangle_triangle_2d_old(const Point& p0,
                                                                 const Point& p1,
                                                                 const Point& p2,
                                                                 const Point& q0,
                                                                 const Point& q1,
                                                                 const Point& q2)
{
  // The list of points (convex hull)
  std::vector<Point> points;

  if (CollisionPredicates::collides_triangle_triangle_2d(p0, p1, p2,
  							 q0, q1, q2))
  {
    // Pack points as vectors
    std::array<Point, 3> tri_0({p0, p1, p2});
    std::array<Point, 3> tri_1({q0, q1, q2});

    // Find all vertex-vertex collision
    for (std::size_t i = 0; i < 3; i++)
    {
      for (std::size_t j = 0; j < 3; j++)
      {
  	if (tri_0[i] == tri_1[j])
  	  points.push_back(tri_0[i]);
      }
    }

#ifdef augustdebug
    std::cout << " after vertex--vertex collisions: total " << points.size() <<" points: ";
    for (const Point p: points) std::cout << tools::plot3(p);
    std::cout << std::endl;
#endif

    // Find all vertex-"edge interior" intersections
    for (std::size_t i = 0; i < 3; i++)
    {
      for (std::size_t j = 0; j < 3; j++)
      {
  	if (tri_0[i] != tri_1[j] and tri_0[(i+1)%3] != tri_1[j] and
  	    CollisionPredicates::collides_segment_point_2d(tri_0[i], tri_0[(i+1)%3], tri_1[j]))
  	  points.push_back(tri_1[j]);

  	if (tri_1[i] != tri_0[j] and tri_1[(i+1)%3] != tri_0[j] and
  	    CollisionPredicates::collides_segment_point_2d(tri_1[i], tri_1[(i+1)%3], tri_0[j]))
  	  points.push_back(tri_0[j]);
      }
    }

#ifdef augustdebug
    std::cout << " after vertex edge interior collisions: total " << points.size() <<" points: ";
    for (const Point p: points) std::cout << tools::plot3(p);
    std::cout << std::endl;
#endif

    // Find all "edge interior"-"edge interior" intersections
    for (std::size_t i = 0; i < 3; i++)
    {
      for (std::size_t j = 0; j < 3; j++)
      {
  	{
  	  std::vector<Point> triangulation =
  	    intersection_segment_segment_2d(tri_0[i],
					    tri_0[(i+1)%3],
					    tri_1[j],
					    tri_1[(j+1)%3]);

  	  points.insert(points.end(), triangulation.begin(), triangulation.end());
  	}
      }
    }

#ifdef augustdebug
    std::cout << " after edge interior -- edge interior collisions: total " << points.size() <<" points: ";
    for (const Point p: points) std::cout << tools::plot(p);
    std::cout << std::endl;
#endif

    // Find all vertex-"triangle interior" intersections
    const int s0 = std::signbit(orient2d(tri_0[0], tri_0[1], tri_0[2])) == true ? -1 : 1;
    const int s1 = std::signbit(orient2d(tri_1[0], tri_1[1], tri_1[2])) == true ? -1 : 1;

    for (std::size_t i = 0; i < 3; ++i)
    {
      const double q0_q1_pi = s1*orient2d(tri_1[0], tri_1[1], tri_0[i]);
      const double q1_q2_pi = s1*orient2d(tri_1[1], tri_1[2], tri_0[i]);
      const double q2_q0_pi = s1*orient2d(tri_1[2], tri_1[0], tri_0[i]);

      if (q0_q1_pi > 0. and
  	  q1_q2_pi > 0. and
  	  q2_q0_pi > 0.)
      {
  	points.push_back(tri_0[i]);
      }

      const double p0_p1_qi = s0*orient2d(tri_0[0], tri_0[1], tri_1[i]);
      const double p1_p2_qi = s0*orient2d(tri_0[1], tri_0[2], tri_1[i]);
      const double p2_p0_qi = s0*orient2d(tri_0[2], tri_0[0], tri_1[i]);

      if (p0_p1_qi > 0. and
  	  p1_p2_qi > 0. and
  	  p2_p0_qi > 0.)
      {
  	points.push_back(tri_1[i]);
      }
    }

#ifdef augustdebug
    std::cout << " after vertex -- triangle collisions: total " << points.size() <<" points: ";
    for (const Point p: points) std::cout << tools::plot(p);
    std::cout << std::endl;
#endif
  }

  dolfin_assert(GeometryPredicates::is_finite(points));
  return _unique_points(points);
}

std::vector<Point>
IntersectionConstruction::_intersection_tetrahedron_triangle_3d_old(const Point& p0,
                                                                    const Point& p1,
                                                                    const Point& p2,
                                                                    const Point& p3,
                                                                    const Point& q0,
                                                                    const Point& q1,
                                                                    const Point& q2)
{
  // This code mimics the triangulate_tetrahedron_tetrahedron and the
  // triangulate_tetrahedron_tetrahedron_triangle_codes: we first
  // identify triangle nodes in the tetrahedra. Them we continue with
  // edge-face detection for the four faces of the tetrahedron and the
  // triangle. The points found are used to form a triangulation by
  // first sorting them using a Graham scan.

  // Pack points as vectors
  std::array<Point, 4> tet = {{p0, p1, p2, p3}};
  std::array<Point, 3> tri = {{q0, q1, q2}};

  // Tolerance for duplicate points (p and q are the same if
  // (p-q).norm() < same_point_tol)
  const double same_point_tol = DOLFIN_EPS;

  // Tolerance for small triangle (could be improved by identifying
  // sliver and small triangles)
  const double tri_det_tol = DOLFIN_EPS;

  std::vector<Point> points;

  // Triangle node in tetrahedron intersection
  for (std::size_t i = 0; i < 3; ++i)
    if (CollisionPredicates::collides_tetrahedron_point_3d(tet[0],
                                                           tet[1],
                                                           tet[2],
                                                           tet[3],
                                                           tri[i]))
      points.push_back(tri[i]);

  // Check if a tetrahedron edge intersects the triangle
  const std::array<std::array<std::size_t, 2>, 6> tet_edges = {{ {2, 3},
								 {1, 3},
								 {1, 2},
								 {0, 3},
								 {0, 2},
								 {0, 1} }};
  for (std::size_t e = 0; e < 6; ++e)
    if (CollisionPredicates::collides_triangle_segment_3d(tri[0], tri[1], tri[2],
							  tet[tet_edges[e][0]],
							  tet[tet_edges[e][1]]))
    {
      const std::vector<Point> ii = intersection_triangle_segment_3d(tri[0], tri[1], tri[2],
                                                                     tet[tet_edges[e][0]],
                                                                     tet[tet_edges[e][1]]);
#ifdef augustdebug
      std::cout << __FUNCTION__<<'\n'
		<< tools::drawtriangle({p0,p1,p2,p3})<<tools::drawtriangle({q0,q1,q2})<<'\n'
		<< tools::drawtriangle({tri[0], tri[1], tri[2]})<<tools::drawtriangle({tet[tet_edges[e][0]],tet[tet_edges[e][1]]})<<'\n';
#endif

      dolfin_assert(ii.size());
      points.insert(points.end(), ii.begin(), ii.end());
    }

  // check if a triangle edge intersects a tetrahedron face
  const std::array<std::array<std::size_t, 3>, 4> tet_faces = {{ {1, 2, 3},
								 {0, 2, 3},
								 {0, 1, 3},
								 {0, 1, 2} }};
  const std::array<std::array<std::size_t, 2>, 3> tri_edges = {{ {0, 1},
								 {0, 2},
								 {1, 2} }};
  for (std::size_t f = 0; f < 4; ++f)
    for (std::size_t e = 0; e < 3; ++e)
      if (CollisionPredicates::collides_triangle_segment_3d(tet[tet_faces[f][0]],
							    tet[tet_faces[f][1]],
							    tet[tet_faces[f][2]],
							    tri[tri_edges[e][0]],
							    tri[tri_edges[e][1]]))
      {
	const std::vector<Point> ii = intersection_triangle_segment_3d(tet[tet_faces[f][0]],
                                                                       tet[tet_faces[f][1]],
                                                                       tet[tet_faces[f][2]],
                                                                       tri[tri_edges[e][0]],
                                                                       tri[tri_edges[e][1]]);
	dolfin_assert(ii.size());
	points.insert(points.end(), ii.begin(), ii.end());
      }

  // FIXME: segment-segment intersection should not be needed if
  // triangle-segment intersection doesn't miss this

  // Remove duplicate nodes
  // FIXME: If this is necessary, reuse code from ConvexTriangulation
  std::vector<Point> tmp;
  tmp.reserve(points.size());

  for (std::size_t i = 0; i < points.size(); ++i)
  {
    bool different = true;
    for (std::size_t j = i+1; j < points.size(); ++j)
      if ((points[i] - points[j]).norm() < same_point_tol)
      {
	different = false;
	break;
      }
    if (different)
      tmp.push_back(points[i]);
  }
  points = tmp;

  return points;
}

std::vector<Point>
IntersectionConstruction::_intersection_tetrahedron_tetrahedron_3d_old(const Point& p0,
                                                                       const Point& p1,
                                                                       const Point& p2,
                                                                       const Point& p3,
                                                                       const Point& q0,
                                                                       const Point& q1,
                                                                       const Point& q2,
                                                                       const Point& q3)
{
  // This algorithm computes the intersection of cell_0 and cell_1 by
  // returning a vector<double> with points describing a tetrahedral
  // mesh of the intersection. We will use the fact that the
  // intersection is a convex polyhedron. The algorithm works by first
  // identifying intersection points: vertex points inside a cell,
  // edge-face collision points and edge-edge collision points (the
  // edge-edge is a rare occurance). Having the intersection points,
  // we identify points that are coplanar and thus form a facet of the
  // polyhedron. These points are then used to form a tessellation of
  // triangles, which are used to form tetrahedra by the use of the
  // center point of the polyhedron. This center point is thus an
  // additional point not found on the polyhedron facets.

  std::cout << "p0 = " << p0 << std::endl;
  std::cout << "p1 = " << p1 << std::endl;
  std::cout << "p2 = " << p2 << std::endl;
  std::cout << "p3 = " << p3 << std::endl;
  std::cout << "q0 = " << q0 << std::endl;
  std::cout << "q1 = " << q1 << std::endl;
  std::cout << "q2 = " << q2 << std::endl;
  std::cout << "q3 = " << q3 << std::endl;

  // Pack points as vectors
  const std::array<Point, 4> tet_0 = {p0, p1, p2, p3};
  const std::array<Point, 4> tet_1 = {q0, q1, q2, q3};

  // Points in the triangulation (unique)
  std::vector<Point> points;

  // Node intersection
  for (int i = 0; i < 4; ++i)
  {
    if (CollisionPredicates::collides_tetrahedron_point_3d(tet_0[0],
                                                           tet_0[1],
                                                           tet_0[2],
                                                           tet_0[3],
                                                           tet_1[i]))
      points.push_back(tet_1[i]);

    if (CollisionPredicates::collides_tetrahedron_point_3d(tet_1[0],
                                                           tet_1[1],
                                                           tet_1[2],
                                                           tet_1[3],
                                                           tet_0[i]))
      points.push_back(tet_0[i]);

    dolfin_assert(GeometryPredicates::is_finite(points));
  }

  // Edge face intersections
  const std::array<std::array<std::size_t, 2>, 6> edges = {{ {2, 3},
							     {1, 3},
							     {1, 2},
							     {0, 3},
							     {0, 2},
							     {0, 1} }};
  const std::array<std::array<std::size_t, 3>, 4> faces = {{ {1, 2, 3},
							     {0, 2, 3},
							     {0, 1, 3},
							     {0, 1, 2} }};

  // Loop over edges e and faces f
  for (std::size_t e = 0; e < 6; ++e)
  {
    for (std::size_t f = 0; f < 4; ++f)
    {
      if (CollisionPredicates::collides_triangle_segment_3d(tet_0[faces[f][0]],
							    tet_0[faces[f][1]],
							    tet_0[faces[f][2]],
							    tet_1[edges[e][0]],
							    tet_1[edges[e][1]]))
      {
	const std::vector<Point> intersection
	  = intersection_triangle_segment_3d(tet_0[faces[f][0]],
					     tet_0[faces[f][1]],
					     tet_0[faces[f][2]],
					     tet_1[edges[e][0]],
					     tet_1[edges[e][1]]);
	points.insert(points.end(), intersection.begin(), intersection.end());
        dolfin_assert(GeometryPredicates::is_finite(points));
      }

      if (CollisionPredicates::collides_triangle_segment_3d(tet_1[faces[f][0]],
							    tet_1[faces[f][1]],
							    tet_1[faces[f][2]],
							    tet_0[edges[e][0]],
							    tet_0[edges[e][1]]))
      {
	const std::vector<Point> intersection
	  = intersection_triangle_segment_3d(tet_1[faces[f][0]],
					     tet_1[faces[f][1]],
					     tet_1[faces[f][2]],
					     tet_0[edges[e][0]],
					     tet_0[edges[e][1]]);
	points.insert(points.end(), intersection.begin(), intersection.end());
        dolfin_assert(GeometryPredicates::is_finite(points));
      }
    }
  }

  // Edge edge intersection
  Point pt;
  for (int i = 0; i < 6; ++i)
  {
    for (int j = 0; j < 6; ++j)
    {
      if (CollisionPredicates::collides_segment_segment_3d(tet_0[edges[i][0]],
							   tet_0[edges[i][1]],
							   tet_1[edges[j][0]],
							   tet_1[edges[j][1]]))
      {
	const std::vector<Point> intersection
	  = intersection_segment_segment_3d(tet_0[edges[i][0]],
					    tet_0[edges[i][1]],
					    tet_1[edges[j][0]],
					    tet_1[edges[j][1]]);
	points.insert(points.end(), intersection.begin(), intersection.end());
        dolfin_assert(GeometryPredicates::is_finite(points));
      }
    }
  }

  dolfin_assert(GeometryPredicates::is_finite(points));
  std::vector<Point> unique = _unique_points(points);
  return unique;
}
