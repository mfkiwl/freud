#ifndef __FILTERSANN_H__
#define __FILTERSANN_H__

#include "Filter.h"

#include "NeighborList.h"
#include "NeighborQuery.h"

namespace freud { namespace locality {

/* Class for the SANN method of filtering a neighborlist.
 * */
class FilterSANN : public Filter
{
public:
    //<! Construct a FilterSANN
    /*
     * \param allow_incomplete_shell whether incomplete neighbor shells should
     *                               result in a warning or error
     * */
    explicit FilterSANN(bool allow_incomplete_shell)
        : Filter(), m_allow_incomplete_shell(allow_incomplete_shell)
    {}

    void compute(const NeighborQuery* nq, const vec3<float>* query_points, unsigned int num_query_points,
                 const NeighborList* nlist, const QueryArgs& qargs) override;

private:
    //! warn/raise exception about unfilled shells
    void warnAboutUnfilledNeighborShells(const std::vector<unsigned int>& unfilled_qps) const;

    //! whether incomplete neighbor shell should result in a warning or error
    bool m_allow_incomplete_shell;
};

}; }; // namespace freud::locality

#endif // __FILTERSANN_H__