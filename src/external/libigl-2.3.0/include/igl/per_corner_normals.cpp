// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2013 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
#include "per_corner_normals.h"

#include "vertex_triangle_adjacency.h"
#include "per_face_normals.h"
#include "PI.h"
#include "doublearea.h"

template <typename DerivedV, typename DerivedF, typename DerivedCN>
IGL_INLINE void igl::per_corner_normals(
  const Eigen::MatrixBase<DerivedV>& V,
  const Eigen::MatrixBase<DerivedF>& F,
  const double corner_threshold,
  Eigen::PlainObjectBase<DerivedCN> & CN)
{
  using namespace Eigen;
  using namespace std;
  Eigen::Matrix<typename DerivedV::Scalar,Eigen::Dynamic,3> FN;
  per_face_normals(V,F,FN);
  vector<vector<int> > VF,VFi;
  vertex_triangle_adjacency(V.rows(),F,VF,VFi);
  return per_corner_normals(V,F,FN,VF,corner_threshold,CN);
}

template <
  typename DerivedV,
  typename DerivedF,
  typename DerivedFN,
  typename DerivedCN>
IGL_INLINE void igl::per_corner_normals(
  const Eigen::MatrixBase<DerivedV>& V,
  const Eigen::MatrixBase<DerivedF>& F,
  const Eigen::MatrixBase<DerivedFN>& FN,
  const double corner_threshold,
  Eigen::PlainObjectBase<DerivedCN> & CN)
{
  using namespace Eigen;
  using namespace std;
  vector<vector<int> > VF,VFi;
  vertex_triangle_adjacency(V.rows(),F,VF,VFi);
  return per_corner_normals(V,F,FN,VF,corner_threshold,CN);
}

template <
  typename DerivedV,
  typename DerivedF,
  typename DerivedFN,
  typename IndexType,
  typename DerivedCN>
IGL_INLINE void igl::per_corner_normals(
  const Eigen::MatrixBase<DerivedV>& /*V*/,
  const Eigen::MatrixBase<DerivedF>& F,
  const Eigen::MatrixBase<DerivedFN>& FN,
  const std::vector<std::vector<IndexType> >& VF,
  const double corner_threshold,
  Eigen::PlainObjectBase<DerivedCN> & CN)
{
  using namespace Eigen;
  using namespace std;

  // number of faces
  const int m = F.rows();
  // valence of faces
  const int n = F.cols();

  // initialize output to ***zero***
  CN.setZero(m*n,3);

  // loop over faces
  for(size_t i = 0;int(i)<m;i++)
  {
    // Normal of this face
    Eigen::Matrix<typename DerivedV::Scalar,3,1> fn = FN.row(i);
    // loop over corners
    for(size_t j = 0;int(j)<n;j++)
    {
      const std::vector<IndexType> &incident_faces = VF[F(i,j)];
      // loop over faces sharing vertex of this corner
      for(int k = 0;k<(int)incident_faces.size();k++)
      {
        Eigen::Matrix<typename DerivedV::Scalar,3,1> ifn = FN.row(incident_faces[k]);
        // dot product between face's normal and other face's normal
        double dp = fn.dot(ifn);
        // if difference in normal is slight then add to average
        if(dp > cos(corner_threshold*PI/180))
        {
          // add to running sum
          CN.row(i*n+j) += ifn;
        // else ignore
        }else
        {
        }
      }
      // normalize to take average
      CN.row(i*n+j).normalize();
    }
  }
}


template <
  typename DerivedV, 
  typename DerivedI, 
  typename DerivedC, 
  typename DerivedN,
  typename DerivedVV,
  typename DerivedFF,
  typename DerivedJ,
  typename DerivedNN>
IGL_INLINE void igl::per_corner_normals(
  const Eigen::MatrixBase<DerivedV> & V,
  const Eigen::MatrixBase<DerivedI> & I,
  const Eigen::MatrixBase<DerivedC> & C,
  const typename DerivedV::Scalar corner_threshold,
  Eigen::PlainObjectBase<DerivedN>  & N,
  Eigen::PlainObjectBase<DerivedVV> & VV,
  Eigen::PlainObjectBase<DerivedFF> & FF,
  Eigen::PlainObjectBase<DerivedJ>  & J,
  Eigen::PlainObjectBase<DerivedNN> & NN)
{
  const Eigen::Index m = C.size()-1;
  typedef Eigen::Index Index;
  Eigen::MatrixXd FN;
  per_face_normals(V,I,C,FN,VV,FF,J);
  typedef typename DerivedN::Scalar Scalar;
  Eigen::Matrix<Scalar,Eigen::Dynamic,1> AA;
  doublearea(VV,FF,AA);
  // VF[i](j) = p means p is the jth face incident on vertex i
  // to-do micro-optimization to avoid vector<vector>
  std::vector<std::vector<Eigen::Index>> VF(V.rows());
  for(Eigen::Index p = 0;p<m;p++)
  {
    // number of faces/vertices in this simple polygon
    const Index np = C(p+1)-C(p);
    for(Eigen::Index i = 0;i<np;i++)
    {
      VF[I(C(p)+i)].push_back(p);
    }
  }

  N.resize(I.rows(),3);
  for(Eigen::Index p = 0;p<m;p++)
  {
    // number of faces/vertices in this simple polygon
    const Index np = C(p+1)-C(p);
    Eigen::Matrix<Scalar,3,1> fn = FN.row(p);
    for(Eigen::Index i = 0;i<np;i++)
    {
      N.row(C(p)+i).setZero();
      // Loop over faces sharing this vertex
      for(const auto & n : VF[I(C(p)+i)])
      {
        Eigen::Matrix<Scalar,3,1> ifn = FN.row(n);
        // dot product between face's normal and other face's normal
        Scalar dp = fn.dot(ifn);
        if(dp > cos(corner_threshold*igl::PI/180))
        {
          // add to running sum
          N.row(C(p)+i) += AA(n) * ifn;
        }
      }
      N.row(C(p)+i).normalize();
    }
  }

  // Relies on order of FF 
  NN.resize(FF.rows()*3,3);
  {
    Eigen::Index k = 0;
    for(Eigen::Index p = 0;p<m;p++)
    {
      // number of faces/vertices in this simple polygon
      const Index np = C(p+1)-C(p);
      for(Eigen::Index i = 0;i<np;i++)
      {
        assert(FF(k,0) == I(C(p)+((i+0)%np)));
        assert(FF(k,1) == I(C(p)+((i+1)%np)));
        assert(FF(k,2) == V.rows()+p);
        NN.row(k*3+0) = N.row(C(p)+((i+0)%np));
        NN.row(k*3+1) = N.row(C(p)+((i+1)%np));
        NN.row(k*3+2) = FN.row(p);
        k++;
      }
    }
    assert(k == FF.rows());
  }
}

#ifdef IGL_STATIC_LIBRARY
// Explicit template instantiation
// generated by autoexplicit.sh
template void igl::per_corner_normals<Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<double, -1, -1, 0, -1, -1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> > const&, Eigen::Matrix<double, -1, -1, 0, -1, -1>::Scalar, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&);
// generated by autoexplicit.sh
template void igl::per_corner_normals<Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, -1, 0, -1, -1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> > const&, double, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&);
template void igl::per_corner_normals<Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, double, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&);
template void igl::per_corner_normals<Eigen::Matrix<double, -1, 3, 0, -1, 3>, Eigen::Matrix<int, -1, 3, 0, -1, 3> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 0, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 0, -1, 3> > const&, double, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 0, -1, 3> >&);
template void igl::per_corner_normals<Eigen::Matrix<double, -1, 3, 0, -1, 3>, Eigen::Matrix<int, -1, 3, 0, -1, 3>, Eigen::Matrix<double, -1, 3, 0, -1, 3>, Eigen::Matrix<double, -1, 3, 0, -1, 3> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 0, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 0, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 0, -1, 3> > const&, double, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 0, -1, 3> >&);
template void igl::per_corner_normals<Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<double, -1, -1, 0, -1, -1>, int, Eigen::Matrix<double, -1, -1, 0, -1, -1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, std::vector<std::vector<int, std::allocator<int> >, std::allocator<std::vector<int, std::allocator<int> > > > const&, double, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> >&);
#endif