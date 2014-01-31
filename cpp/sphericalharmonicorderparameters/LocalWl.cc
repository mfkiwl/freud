#include "LocalWl.h"

#include <stdexcept>
#include <complex>
#include <boost/math/special_functions.hpp>
//#include <boost/math/special_functions/spheric_harmonic.hpp>

using namespace std;
using namespace boost::python;

/*! \file LocalWl.cc
    \brief Compute a Wl per particle
*/

namespace freud { namespace sphericalharmonicorderparameters {

LocalWl::LocalWl(const trajectory::Box& box, float rmax, unsigned int l)
    :m_box(box), m_rmax(rmax), m_lc(box, rmax), m_l(l)
    {
    if (m_rmax < 0.0f)
        throw invalid_argument("rmax must be positive!");
    if (m_l < 2)
        throw invalid_argument("l must be two or greater (and even)!");
    if (m_l%2 == 1)
        {
        fprintf(stderr,"Current value of m_l is %d\n",m_l);
        throw invalid_argument("This method requires even values of l!");
        }
    }

void LocalWl::Ylm(const double theta, const double phi, std::vector<std::complex<double> > &Y)
    {
    if(Y.size() != 2*m_l+1)
        Y.resize(2*m_l+1);

    for(int m = -m_l; m <=0; m++)
        //Doc for boost spherical harmonic
        //http://www.boost.org/doc/libs/1_53_0/libs/math/doc/sf_and_dist/html/math_toolkit/special/sf_poly/sph_harm.html
        // theta = colatitude = 0..Pi
        // phi = azimuthal (longitudinal) 0..2pi).
        Y[m+m_l]= boost::math::spherical_harmonic(m_l, m, theta, phi);

    for(unsigned int i = 1; i <= m_l; i++)
        Y[i+m_l] = Y[-i+m_l];
    }



void LocalWl::compute(const float3 *points, unsigned int Np)
    {

    //Set local data size
    m_Np = Np;

    //Initialize cell list
    m_lc.computeCellList(points,m_Np);

    double rmaxsq = m_rmax * m_rmax;

    //newmanrs:  For efficiency, if Np != m_Np, we could not reallocate these! Maybe.
    // for safety and debugging laziness, reallocate each time
    m_Qlmi = boost::shared_array<complex<double> >(new complex<double> [(2*m_l+1)*m_Np]);
    m_Wli = boost::shared_array<double>(new double[m_Np]);
    memset((void*)m_Qlmi.get(), 0, sizeof(complex<double>)*(2*m_l+1)*m_Np);
    memset((void*)m_Wli.get(), 0, sizeof(double)*m_Np);

    for (unsigned int i = 0; i<m_Np; i++)
        {
        //get cell point is in
        float3 ref = points[i];
        unsigned int ref_cell = m_lc.getCell(ref);
        unsigned int neighborcount=0;

        //loop over neighboring cells
        const std::vector<unsigned int>& neigh_cells = m_lc.getCellNeighbors(ref_cell);
        for (unsigned int neigh_idx = 0; neigh_idx < neigh_cells.size(); neigh_idx++)
            {
            unsigned int neigh_cell = neigh_cells[neigh_idx];

            //iterate over particles in neighboring cells
            locality::LinkCell::iteratorcell it = m_lc.itercell(neigh_cell);
            for (unsigned int j = it.next(); !it.atEnd(); j = it.next())
                {
                if (i == j) 
                {
                    continue;
                }
                // rij = rj - ri, from i pointing to j.
                float dx = float(points[j].x - ref.x);
                float dy = float(points[j].y - ref.y);
                float dz = float(points[j].z - ref.z);

                float3 delta = m_box.wrap(make_float3(dx, dy, dz));
                float rsq = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;

                if (rsq < rmaxsq)
                    {
                    double phi = atan2(delta.y,delta.x);      //0..2Pi
                    double theta = acos(delta.z / sqrt(rsq)); //0..Pi

                    std::vector<std::complex<double> > Y;
                    LocalWl::Ylm(theta, phi,Y);  //Fill up Ylm vector
                    for(unsigned int k = 0; k < (2*m_l+1); ++k)
                        {
                        m_Qlmi[(2*m_l+1)*i+k]+=Y[k];
                        }
                    neighborcount++;
                    }
                }
            } //End loop going over neighbor cells (and thus all neighboring particles);
            //Normalize!
            for(unsigned int k = 0; k < (2*m_l+1); ++k)
                {
                m_Qlmi[(2*m_l+1)*i+k]/= neighborcount;
                }
        } //Ends loop over particles i for Qlmi calcs
        
        //Need to compute Wli, for each particle, loop over all the wigner3j u1,u2,u3, calculate sum, normalize probably.
        
        
    }

void LocalWl::computePy(boost::python::numeric::array points)
    {
    //validate input type and rank
    num_util::check_type(points, PyArray_FLOAT);
    num_util::check_rank(points, 2);

    // validate that the 2nd dimension is only 3
    num_util::check_dim(points, 1, 3);
    unsigned int Np = num_util::shape(points)[0];

    // get the raw data pointers and compute the cell list
    float3* points_raw = (float3*) num_util::data(points);
    compute(points_raw, Np);
    }

void export_LocalWl()
    {
    class_<LocalWl>("LocalWl", init<trajectory::Box&, float, unsigned int>())
        .def("getBox", &LocalWl::getBox, return_internal_reference<>())
        .def("compute", &LocalWl::computePy)
        .def("getWl", &LocalWl::getWlPy)
        ;
    }

}; }; // end namespace freud::localwl


