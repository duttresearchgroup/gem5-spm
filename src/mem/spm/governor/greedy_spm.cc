#include "mem/spm/governor/greedy_spm.hh"

#include <iostream>

GreedySPM *
GreedySPMParams::create()
{
    return new GreedySPM(this);
}

GreedySPM::GreedySPM(const Params *p)
    : RandomSPM(p)
{
    gov_type = "Greedy";
}

GreedySPM::~GreedySPM()
{

}

void
GreedySPM::init()
{

}

/*
Example of (col, row) ordering:
(2,0)  (1,0)  (0,0)
(2,1)  (1,1)  (0,1)
(2,2)  (1,2)  (0,2)
*/
void
GreedySPM::addPMMU(PMMU *p)
{
    vector<PMMU *>::iterator it = pmmus.begin();
    pmmus.insert(it,p);

    int col = p->getMachineID().num % num_column;
    int row = p->getMachineID().num / num_column;
    assert (coord_to_pmmu.find(make_pair(col,row)) == coord_to_pmmu.end());
    coord_to_pmmu[make_pair(col,row)] = p;
    assert (pmmu_to_coord.find(p) == pmmu_to_coord.end());
    pmmu_to_coord[p] = make_pair(col,row);
}

int
GreedySPM::findHopDistance (pair <int, int> pmmu_o, pair <int, int> pmmu_h)
{
    return abs(pmmu_o.first - pmmu_h.first) + abs(pmmu_o.second - pmmu_h.second);
}

void
GreedySPM::distanceBasedSort(pair <int, int> central_pmmu)
{
    bool have_swapped = true;
    for (unsigned j = 1; have_swapped && j < pmmus.size(); ++j) {
        have_swapped = false;
        for (unsigned i = 0; i < pmmus.size() - j; ++i) {
            if (findHopDistance(pmmu_to_coord.at(pmmus[i]),central_pmmu) >
                findHopDistance(pmmu_to_coord.at(pmmus[i+1]),central_pmmu)) {
                have_swapped = true;
                swap(pmmus[i], pmmus[i + 1]);
            }
        }
    }
}

int
GreedySPM::allocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    const int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    int remaining_pages = total_num_pages;

    if (!gov_type.compare("Greedy") && hybrid_mem) {
        cache_invalidator_helper(gov_request);
    }

    PMMU *requester_pmmu = gov_request->getPMMUPtr();

    distanceBasedSort(pmmu_to_coord.at(requester_pmmu));
    assert(requester_pmmu == pmmus[0]);

    vector<PMMU *>::size_type pmmu_it = 0;
    while (remaining_pages > 0 && pmmu_it != pmmus.size()) {
        PMMU *host_pmmu = pmmus[pmmu_it];
        HostInfo host_info (gov_request->getThreadContext(),
                            gov_request->getPMMUPtr(), host_pmmu, Addr(0), -1);
        host_info.setAllocMode(gov_request->getAnnotations()->alloc_mode);
        remaining_pages -= allocation_helper_on_free_pages(gov_request, &host_info);
        pmmu_it++;
    }

    if (!gov_type.compare("Greedy") && uncacheable_spm) {
        add_mapping_unallocated_pages(gov_request);
    }

    return total_num_pages - remaining_pages;
}
