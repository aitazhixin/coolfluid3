
// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_Mesh_SF_Quad2DLagrangeP2_hpp
#define CF_Mesh_SF_Quad2DLagrangeP2_hpp

#include "Math/RealMatrix.hpp"
#include "Mesh/Quad2D.hpp"

#include "Mesh/SF/LibSF.hpp"

namespace CF {
namespace Mesh {
namespace SF {

/// This class provides the lagrangian shape function describing the
/// representation of the solution and/or the geometry in a P1 (bilinear)
/// quadrilateral element.
/// @author Andrea Lani
/// @author Geoffrey Deliege
/// @author Tiago Quintino
/// @author Bart Janssens
/// @author Willem Deconinck
struct SF_API Quad2DLagrangeP2  : public Quad2D {

/// typedef for the supporting geometry
typedef Quad2D Support;

/// Compute the shape functions corresponding to the given
/// mapped coordinates
/// @param mappedCoord The mapped coordinates
/// @param shapeFunc Vector storing the result
static void shape_function(const RealVector& mappedCoord, RealVector& shapeFunc)
{
  cf_assert(shapeFunc.size() == 9);
  cf_assert(mappedCoord.size() == 2);
  const Real xi  = mappedCoord[KSI];
  const Real eta = mappedCoord[ETA];

  const Real xi2 = xi*xi;
  const Real eta2 = eta*eta;
  const Real xiEta = xi*eta;

  shapeFunc[0] = 0.25 * (1.0 - xi)  * (1.0 - eta) * xiEta;
  shapeFunc[1] = -0.25 * (1.0 + xi)  * (1.0 - eta) * xiEta;
  shapeFunc[2] = 0.25 * (1.0 + xi)  * (1.0 + eta) * xiEta;
  shapeFunc[3] = -0.25 * (1.0 - xi)  * (1.0 + eta) * xiEta;
  shapeFunc[4] = -0.5  * (1.0 - xi2) * (1.0 - eta) * eta;
  shapeFunc[5] = 0.5  * (1.0 + xi)  * (1.0 - eta2) * xi;
  shapeFunc[6] = 0.5  * (1.0 - xi2) * (1.0 + eta) * eta;
  shapeFunc[7] = -0.5  * (1.0 - xi)  * (1.0 - eta2) * xi;
  shapeFunc[8] =        (1.0 - xi2) * (1.0 - eta2);
}

/// Compute Mapped Coordinates
/// @param coord contains the coordinates to be mapped
/// @param nodes contains the nodes
/// @param mappedCoord Store the output mapped coordinates
template<typename NodesT>
static void mapped_coordinates(const RealVector& coord, const NodesT& nodes, RealVector& mappedCoord)
{
  throw Common::NotImplemented( FromHere(), "" );
}

/// Compute the gradient with respect to mapped coordinates, i.e. partial derivatives are in terms of the
/// mapped coordinates. The result needs to be multiplied with the inverse jacobian to get the result in real
/// coordinates.
/// @param mappedCoord The mapped coordinates where the gradient should be calculated
/// @param result Storage for the resulting gradient matrix
static void mapped_gradient(const RealVector& mappedCoord, RealMatrix& result)
{
  cf_assert(result.nbCols() == nb_nodes);
  cf_assert(result.nbRows() == dimension);
  const Real xi =  mappedCoord[KSI];
  const Real eta = mappedCoord[ETA];
  const Real xi2 = xi*xi;
  const Real eta2 = eta*eta;
  const Real xiEta = xi*eta;
  const Real xi2Eta = xi2*eta;
  const Real xiEta2 = xi*eta2;

  result(KSI,0) =  0.25 * (eta - 2.*xiEta - eta2 + 2.*xiEta2);
  result(KSI,1) = -0.25 * (eta + 2.*xiEta - eta2 - 2.*xiEta2);
  result(KSI,2) =  0.25 * (eta + 2.*xiEta + eta2 + 2.*xiEta2);
  result(KSI,3) = -0.25 * (eta - 2.*xiEta + eta2 - 2.*xiEta2);
  result(KSI,4) = -0.5  * (-2.*xiEta + 2.*xiEta2);
  result(KSI,5) =  0.5  * (1. - eta2 + 2.*xi - 2.*xiEta2);
  result(KSI,6) =  0.5  * (-2.*xiEta - 2.*xiEta2);
  result(KSI,7) = -0.5  * (1. - eta2 - 2.*xi + 2.*xiEta2);
  result(KSI,8) =  2.*xiEta2 - 2.*xi;

  result(ETA,0) =  0.25 * (xi - xi2 - 2.*xiEta + 2.*xi2Eta);
  result(ETA,1) = -0.25 * (xi + xi2 - 2.*xiEta - 2.*xi2Eta);
  result(ETA,2) =  0.25 * (xi + xi2 + 2.*xiEta + 2.*xi2Eta);
  result(ETA,3) = -0.25 * (xi - xi2 + 2.*xiEta - 2.*xi2Eta);
  result(ETA,4) = -0.5 * (1. - xi2 - 2.*eta + 2.*xi2Eta);
  result(ETA,5) =  0.5 * (-2.*xiEta - 2.*xi2Eta);
  result(ETA,6) =  0.5 * (1. - xi2 + 2.*eta - 2.*xi2Eta);
  result(ETA,7) = -0.5 * (-2.*xiEta + 2.*xi2Eta);
  result(ETA,8) =  2.*xi2Eta - 2.*eta;
}

/// Compute the jacobian determinant at the given mapped coordinates
template<typename NodesT>
static Real jacobian_determinant(const RealVector& mappedCoord, const NodesT& nodes)
{
  cf_assert(mappedCoord.size() == dimension);
  cf_assert(nodes.size() == nb_nodes);

  throw Common::NotImplemented( FromHere(), "" );
}

/// Compute the Jacobian matrix
/// @param mappedCoord The mapped coordinates where the Jacobian should be calculated
/// @param result Storage for the resulting Jacobian matrix
template<typename NodesT>
static void jacobian(const RealVector& mappedCoord, const NodesT& nodes, RealMatrix& result)
{
  throw Common::NotImplemented( FromHere(), "" );
}

/// Compute the adjoint of Jacobian matrix
/// @param mappedCoord The mapped coordinates where the Jacobian should be calculated
/// @param result Storage for the resulting adjoint
template<typename NodesT>
static void jacobian_adjoint(const RealVector& mappedCoord, const NodesT& nodes, RealMatrix& result) {
  cf_assert(result.nbRows() == 2);
  cf_assert(result.isSquare());

  throw Common::NotImplemented( FromHere(), "" );
}

/// Volume of the cell
template<typename NodesT>
inline static Real volume(const NodesT& nodes) {

  const Real x0 = nodes[0][XX];
  const Real x1 = nodes[1][XX];
  const Real x2 = nodes[2][XX];
  const Real x3 = nodes[3][XX];
  const Real x4 = nodes[4][XX];
  const Real x5 = nodes[5][XX];
  const Real x6 = nodes[6][XX];
  const Real x7 = nodes[7][XX];
  const Real y0 = nodes[0][YY];
  const Real y1 = nodes[1][YY];
  const Real y2 = nodes[2][YY];
  const Real y3 = nodes[3][YY];
  const Real y4 = nodes[4][YY];
  const Real y5 = nodes[5][YY];
  const Real y6 = nodes[6][YY];
  const Real y7 = nodes[7][YY];

  return (4*((x7 - x4)*y0 + (x4 - x5)*y1 + (x5 - x6)*y2 + (x6 - x7)*y3) +
          x1*(y0 - y2 - 4*y4 + 4*y5) + x2*(y1 - y3 - 4*y5 + 4*y6) +
          x0*(y3 - y1 + 4*y4 - 4*y7) + x3*(y2 - y0 - 4*y6 + 4*y7)
         )/6.;
}

/// Number of nodes
static const Uint nb_nodes = 9;

/// Order of the shape function
static const Uint order = 2;

/// Connectivity info for the faces
static const FaceConnectivity& faces();

Quad2DLagrangeP2();
virtual std::string getElementTypeName() const;
virtual Real computeVolume(const NodesT& coord) const;
virtual bool is_coord_in_element(const RealVector& coord, const NodesT& nodes) const;
virtual const CF::Mesh::ElementType::FaceConnectivity& face_connectivity() const;
virtual const CF::Mesh::ElementType& face_type(const CF::Uint face) const;
virtual Real jacobian_determinantV ( const CF::RealVector& mapped_coord, const CF::Mesh::ElementType::NodesT& nodes ) const;

private:
/// Convenience struct to easily access the elements that make up the jacobian

/*
struct JacobianCoefficients
{
  Real ax, bx, cx, dx;
  Real ay, by, cy, dy;
  template<typename NodesT>
  JacobianCoefficients(const NodesT& nodes)
  {
    const Real x0 = nodes[0][XX];
    const Real y0 = nodes[0][YY];
    const Real x1 = nodes[1][XX];
    const Real y1 = nodes[1][YY];
    const Real x2 = nodes[2][XX];
    const Real y2 = nodes[2][YY];
    const Real x3 = nodes[3][XX];
    const Real y3 = nodes[3][YY];

    ax = 0.25*( x0 + x1 + x2 + x3);
    bx = 0.25*(-x0 + x1 + x2 - x3);
    cx = 0.25*(-x0 - x1 + x2 + x3);
    dx = 0.25*( x0 - x1 + x2 - x3);
    ay = 0.25*( y0 + y1 + y2 + y3);
    by = 0.25*(-y0 + y1 + y2 - y3);
    cy = 0.25*(-y0 - y1 + y2 + y3);
    dy = 0.25*( y0 - y1 + y2 - y3);
  }
};

*/

};

} // namespace SF
} // namespace Mesh
} // namespace CF

#endif /* CF_Mesh_SF_Quad2DLagrangeP2 */