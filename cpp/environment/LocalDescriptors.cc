// Copyright (c) 2010-2019 The Regents of the University of Michigan
// This file is from the freud project, released under the BSD 3-Clause License.

#include <tbb/tbb.h>
#include <vector>

#include "LocalDescriptors.h"
#include "diagonalize.h"

/*! \file LocalDescriptors.cc
  \brief Computes local descriptors.
*/

namespace freud { namespace environment {

LocalDescriptors::LocalDescriptors(unsigned int l_max, bool negative_m)
    : m_l_max(l_max), m_negative_m(negative_m), m_n_points(0), m_nSphs(0)
{}

void LocalDescriptors::compute(const box::Box& box,
                               unsigned int num_neighbors, const vec3<float>* points, unsigned int n_points,
                               const vec3<float>* query_points, unsigned int n_query_points,
                               const quat<float>* orientations, LocalDescriptorOrientation orientation,
                               const freud::locality::NeighborList* nlist)
{
    nlist->validate(n_query_points, n_points);

    m_sphArray.prepare({nlist->getNumBonds(), getSphWidth()});

    tbb::parallel_for(tbb::blocked_range<size_t>(0, n_points), [=](const tbb::blocked_range<size_t>& br) {
        fsph::PointSPHEvaluator<float> sph_eval(m_l_max);

        for (size_t i = br.begin(); i != br.end(); ++i)
        {
            size_t bond(nlist->find_first_index(i));
            const vec3<float> r_i(points[i]);

            vec3<float> rotation_0, rotation_1, rotation_2;

            if (orientation == LocalNeighborhood)
            {
                util::ManagedArray<float> inertiaTensor = util::ManagedArray<float>({3, 3});

                for (size_t bond_copy(bond); bond_copy < nlist->getNumBonds()
                     && nlist->getNeighbors()(bond_copy, 0) == i && bond_copy < bond + num_neighbors;
                     ++bond_copy)
                {
                    const size_t j(nlist->getNeighbors()(bond_copy, 1));
                    const vec3<float> r_j(query_points[j]);
                    const vec3<float> r_ij(box.wrap(r_j - r_i));
                    const float r_sq(dot(r_ij, r_ij));

                    for (size_t ii(0); ii < 3; ++ii)
                    {
                        inertiaTensor(ii, ii) += r_sq;
                    }

                    inertiaTensor(0, 0) -= r_ij.x * r_ij.x;
                    inertiaTensor(0, 1) -= r_ij.x * r_ij.y;
                    inertiaTensor(0, 2) -= r_ij.x * r_ij.z;
                    inertiaTensor(1, 0) -= r_ij.x * r_ij.y;
                    inertiaTensor(1, 1) -= r_ij.y * r_ij.y;
                    inertiaTensor(1, 2) -= r_ij.y * r_ij.z;
                    inertiaTensor(2, 0) -= r_ij.x * r_ij.z;
                    inertiaTensor(2, 1) -= r_ij.y * r_ij.z;
                    inertiaTensor(2, 2) -= r_ij.z * r_ij.z;
                }

                util::ManagedArray<float> eigenvalues = util::ManagedArray<float>(3);
                util::ManagedArray<float> eigenvectors = util::ManagedArray<float>({3, 3});

                freud::util::diagonalize33SymmetricMatrix(inertiaTensor, eigenvalues, eigenvectors);

                rotation_0
                    = vec3<float>(eigenvectors(0, 0), eigenvectors(0, 1), eigenvectors(0, 2));
                rotation_1
                    = vec3<float>(eigenvectors(1, 0), eigenvectors(1, 1), eigenvectors(1, 2));
                rotation_2
                    = vec3<float>(eigenvectors(2, 0), eigenvectors(2, 1), eigenvectors(2, 2));
            }
            else if (orientation == ParticleLocal)
            {
                const rotmat3<float> rotmat(conj(orientations[i]));
                rotation_0 = rotmat.row0;
                rotation_1 = rotmat.row1;
                rotation_2 = rotmat.row2;
            }
            else if (orientation == Global)
            {
                rotation_0 = vec3<float>(1, 0, 0);
                rotation_1 = vec3<float>(0, 1, 0);
                rotation_2 = vec3<float>(0, 0, 1);
            }
            else
            {
                throw std::runtime_error("Uncaught orientation mode in LocalDescriptors::compute");
            }

            for (; bond < nlist->getNumBonds() && nlist->getNeighbors()(bond, 0) == i; ++bond)
            {
                const unsigned int sphCount(bond * getSphWidth());
                const size_t j(nlist->getNeighbors()(bond, 1));
                const vec3<float> r_j(query_points[j]);
                const vec3<float> r_ij(box.wrap(r_j - r_i));
                const float r_sq(dot(r_ij, r_ij));
                const vec3<float> bond_ij(dot(rotation_0, r_ij), dot(rotation_1, r_ij), dot(rotation_2, r_ij));

                const float magR(sqrt(r_sq));
                float theta(atan2(bond_ij.y, bond_ij.x)); // theta in [-pi..pi] initially
                if (theta < 0)
                    theta += float(2 * M_PI);             // move theta into [0..2*pi]
                float phi(acos(bond_ij.z / magR)); // phi in [0..pi]

                // catch cases where bond_ij.z/magR falls outside [-1, 1]
                // due to numerical issues
                if (std::isnan(phi))
                    phi = bond_ij.z > 0 ? 0 : M_PI;

                sph_eval.compute(phi, theta);

                std::copy(sph_eval.begin(m_negative_m), sph_eval.end(), &m_sphArray[sphCount]);
            }
        }
    });

    // save the last computed number of particles
    m_n_points = n_points;
    m_nSphs = nlist->getNumBonds();
}

}; }; // end namespace freud::environment
