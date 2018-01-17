#include "mem/spm/governor/local_spm.hh"

#include <iostream>

LocalSPM *
LocalSPMParams::create()
{
    return new LocalSPM(this);
}

LocalSPM::LocalSPM(const Params *p)
    : BaseGovernor(p)
{
    gov_type = "Local";
}

LocalSPM::~LocalSPM()
{

}

void
LocalSPM::init()
{

}

int
LocalSPM::allocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    const int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    int remaining_pages = total_num_pages;

    // just do this if we are not called by a child policy
    if (!gov_type.compare("Local") && hybrid_mem) {
        cache_invalidator_helper(gov_request);
    }

    // Allocate on local SPM
    PMMU *host_pmmu = gov_request->getPMMUPtr();
    HostInfo host_info (gov_request->getThreadContext(),
                        gov_request->getPMMUPtr(), host_pmmu, Addr(0), -1);
    host_info.setAllocMode(gov_request->getAnnotations()->alloc_mode);
    remaining_pages -= allocation_helper_on_free_pages(gov_request, &host_info);

    // just do this if we are not called by a child policy
    if (!gov_type.compare("Local") && uncacheable_spm) {
        add_mapping_unallocated_pages(gov_request);
    }

    return total_num_pages - remaining_pages;
}

int
LocalSPM::deAllocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    HostInfo host_info (gov_request->getThreadContext(), gov_request->getPMMUPtr(),
                        nullptr, Addr(0), total_num_pages);
    host_info.setDeallocMode(gov_request->getAnnotations()->dealloc_mode);
    int num_removed_pages = dallocation_helper_virtual_address(gov_request, &host_info);

    return num_removed_pages;
}
