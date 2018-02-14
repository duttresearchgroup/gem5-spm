#include "mem/spm/governor/explicit_local_spm.hh"

#include <iostream>

ExplicitLocalSPM *
ExplicitLocalSPMParams::create()
{
    return new ExplicitLocalSPM(this);
}

ExplicitLocalSPM::ExplicitLocalSPM(const Params *p)
    : BaseGovernor(p)
{
    gov_type = "ExplicitLocal";
}

ExplicitLocalSPM::~ExplicitLocalSPM()
{

}

void
ExplicitLocalSPM::init()
{

}

int
ExplicitLocalSPM::allocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    const int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    int remaining_pages = total_num_pages;

    // just do this if we are not called by a child policy
    if (!gov_type.compare("ExplicitLocal") && hybrid_mem) {
        cache_invalidator_helper(gov_request);
    }

    // Allocate on local SPM
    PMMU *host_pmmu = gov_request->getPMMUPtr();
    HostInfo host_info (gov_request->getThreadContext(),
                        gov_request->getPMMUPtr(),
                        host_pmmu,
                        (Addr)gov_request->annotations->spm_addr,
                        total_num_pages);
    host_info.setAllocMode(gov_request->getAnnotations()->alloc_mode);
    remaining_pages -= allocation_helper_on_free_pages(gov_request, &host_info);

    // just do this if we are not called by a child policy
    if (!gov_type.compare("ExplicitLocal") && uncacheable_spm) {
        add_mapping_unallocated_pages(gov_request);
    }
    assert (total_num_pages == remaining_pages);
    return total_num_pages - remaining_pages;
}

int
ExplicitLocalSPM::deAllocate(GOVRequest *gov_request)
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

int
ExplicitLocalSPM::allocation_helper_on_free_pages(GOVRequest *gov_request,
                                              HostInfo *host_info)
{
    PMMU *requester_pmmu = gov_request->getPMMUPtr();
    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    int remaining_pages = total_num_pages;

    // since we are allocating explicitly, we must ensure that end_spm_addr
    // is not greater than max_spm_addr
    if ((host_info->getSPMaddress()/host_info->getHostPMMU()->getPageSizeBytes()
    		+ total_num_pages) <= host_info->getHostPMMU()->getSPMSizePages()) {
        int num_added_pages = requester_pmmu->addATTMappingsVAddress(gov_request, host_info);
        host_info->getHostPMMU()->setUsedPages(host_info->getSPMaddress(), num_added_pages,
                gov_request->getRequesterNodeID());

        DPRINTF(GOV, "%s: Allocating %d/%d/%d free SPM slot(s) for node (%d,%d) on node (%d,%d) "
    	        "starting from slot address = %u\n",
                gov_type, num_added_pages, host_info->getNumPages(), total_num_pages,
                host_info->getUserPMMU()->getNodeID() / num_column,
                host_info->getUserPMMU()->getNodeID() % num_column,
                host_info->getHostPMMU()->getNodeID() / num_column,
                host_info->getHostPMMU()->getNodeID() % num_column,
                host_info->getSPMaddress());

        gov_request->incPagesServed(host_info->getNumPages());
        remaining_pages -= host_info->getNumPages();
    }
    else {
        // not enough space on this SPM, allocation too large
        DPRINTF(GOV, "%s: Couldn't allocate %d SPM slot(s) for node (%d,%d) on node (%d,%d)\n",
                gov_type, remaining_pages,
                host_info->getUserPMMU()->getNodeID() / num_column,
                host_info->getUserPMMU()->getNodeID() % num_column,
                host_info->getHostPMMU()->getNodeID() / num_column,
                host_info->getHostPMMU()->getNodeID() % num_column);
    }
    return total_num_pages - remaining_pages;
}
