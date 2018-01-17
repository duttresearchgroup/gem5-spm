#include "mem/spm/governor/random_spm.hh"

#include <iostream>
#include <algorithm>

RandomSPM *
RandomSPMParams::create()
{
    return new RandomSPM(this);
}

RandomSPM::RandomSPM(const Params *p)
    : LocalSPM(p)
{
    gov_type = "Random";
}

RandomSPM::~RandomSPM()
{

}

void
RandomSPM::init()
{

}

/*
Example of (col, row) ordering:
(2,0)  (1,0)  (0,0)
(2,1)  (1,1)  (0,1)
(2,2)  (1,2)  (0,2)
*/
void
RandomSPM::addPMMU(PMMU *p)
{
    vector<PMMU *>::iterator it = pmmus.begin();
    pmmus.insert(it,p);

    int col = p->getMachineID().num % num_column;
    int row = p->getMachineID().num / num_column;
    assert (coord_to_pmmu.find(make_pair(col,row)) == coord_to_pmmu.end());
    coord_to_pmmu[make_pair(col,row)] = p;
}

int
RandomSPM::allocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    const int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    int remaining_pages = total_num_pages;

    if (!gov_type.compare("Random") && hybrid_mem) {
        cache_invalidator_helper(gov_request);
    }

    // 1. Allocate on the local SPM first
    remaining_pages -= LocalSPM::allocate(gov_request);

    // 2. If we still need more SPM slots, target randomly selected remote SPMs
    if (remaining_pages > 0) {
        vector<int> indices;
        for (int i=0; i < pmmus.size(); i++) {
            indices.push_back(i);
        }
        random_shuffle(indices.begin(), indices.end());

        while (indices.size() > 0 && remaining_pages > 0) {
            int ind = *indices.begin();
            indices.erase(indices.begin());
            PMMU *host_pmmu = pmmus[ind];
            HostInfo host_info (gov_request->getThreadContext(),
                                gov_request->getPMMUPtr(), host_pmmu, Addr(0), -1);
            host_info.setAllocMode(gov_request->getAnnotations()->alloc_mode);
            remaining_pages -= allocation_helper_on_free_pages(gov_request, &host_info);
        }
    }

    if (!gov_type.compare("Random") && uncacheable_spm) {
        add_mapping_unallocated_pages(gov_request);
    }

    return total_num_pages - remaining_pages;
}
