#include "mem/spm/governor/base.hh"
#include "mem/spm/governor/local_spm.hh"
#include "mem/spm/governor/random_spm.hh"
#include "mem/spm/governor/greedy_spm.hh"
#include "mem/spm/governor/guaranteed_greedy_spm.hh"

#include <iostream>
using namespace std;

BaseGovernor *
BaseGovernorParams::create()
{
    if (gov_type.compare("Local") == 0)
        return new LocalSPM(this);
    else if (gov_type.compare("Random") == 0)
        return new RandomSPM(this);
    else if (gov_type.compare("Greedy") == 0)
        return new GreedySPM(this);
    else if (gov_type.compare("GuaranteedGreedy") == 0)
        return new GuaranteedGreedySPM(this);
    else
        panic ("undefined governor type");
}

BaseGovernor::BaseGovernor(const Params *p)
    : SimObject(p), num_column(p->col),
      num_row(p->row), gov_type("BaseGovernor"),
      hybrid_mem(p->hybrid_mem),
      uncacheable_spm(p->uncacheable_spm)
{
}

void
BaseGovernor::printRequestStatus(GOVRequest *gov_request)
{
    DPRINTF(GOV, "%s: %s Request - Node %d - [%u %u %d] , %d , [%u %u %d] \n",
                 gov_type,
                 GOVCommandToString(gov_request->getCommand()),
                 gov_request->getRequesterNodeID(),
                 gov_request->getStartAddr(Original),
                 gov_request->getEndAddr(Original),
                 gov_request->getNumberOfPages(Original),
                 gov_request->getNumPagesServed(),
                 gov_request->getStartAddr(Unserved_Aligned),
                 gov_request->getEndAddr(Unserved_Aligned),
                 gov_request->getNumberOfPages(Unserved_Aligned));
}

bool
BaseGovernor::getMaxContiguousFreePages(HostInfo *host_info, int max_num_pages_needed, int start_page_index)
{
    bool satisfied = false;
    int ctr;
    int current_max = 0;
    int max_num_pages_found = 0;
    Addr current_base_found = Addr(0);
    Addr base_max_free_pages_found = Addr(0);

    for (ctr = start_page_index; ctr < host_info->getHostPMMU()->getSPMSizePages(); ctr++) {
        if (!host_info->getHostPMMU()->my_spm_ptr->spmSlots[ctr].isFree()){
            current_max = 0;
            current_base_found = Addr((ctr + 1) * host_info->getHostPMMU()->getPageSizeBytes());
        } else {
            current_max++;
        }

        if (current_max > max_num_pages_found) {
            max_num_pages_found = current_max;
            base_max_free_pages_found = current_base_found;
        }

        if (max_num_pages_found >= max_num_pages_needed) {
            satisfied = true;
            break;
        }
    }

    host_info->setNumPages(max_num_pages_found);
    host_info->setSPMaddress(base_max_free_pages_found);

    return satisfied;
}

int
BaseGovernor::allocation_helper_on_free_pages(GOVRequest *gov_request,
                                              HostInfo *host_info)
{
    PMMU *requester_pmmu = gov_request->getPMMUPtr();
    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    int remaining_pages = total_num_pages;

    while (remaining_pages > 0) {

        getMaxContiguousFreePages(host_info, remaining_pages);

        if (host_info->getNumPages() > 0) {
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
            // no more space on this spm OR all pages are already mapped on chip
            DPRINTF(GOV, "%s: Couldn't allocate %d SPM slot(s) for node (%d,%d) on node (%d,%d)\n",
                    gov_type, remaining_pages,
                    host_info->getUserPMMU()->getNodeID() / num_column,
                    host_info->getUserPMMU()->getNodeID() % num_column,
                    host_info->getHostPMMU()->getNodeID() / num_column,
                    host_info->getHostPMMU()->getNodeID() % num_column);
            break;
        }
    }
    return total_num_pages - remaining_pages;
}

int
BaseGovernor::allocation_helper_on_occupied_page(GOVRequest *gov_request,
                                                 HostInfo *host_info)
{
    PMMU *requester_pmmu = gov_request->getPMMUPtr();
    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);

    if (total_num_pages > 0) { // if there is anything to add
        int num_added_pages = requester_pmmu->addATTMappingsVAddress(gov_request, host_info);
        host_info->getHostPMMU()->setUsedPages(host_info->getSPMaddress(), num_added_pages,
                                               gov_request->getRequesterNodeID());

        DPRINTF(GOV, "%s: Allocating %d/%d/%d SPM slot(s) for node (%d,%d) on node (%d,%d)\n",
                gov_type, num_added_pages, host_info->getNumPages(), total_num_pages,
                host_info->getUserPMMU()->getNodeID() / num_column,
                host_info->getUserPMMU()->getNodeID() % num_column,
                host_info->getHostPMMU()->getNodeID() / num_column,
                host_info->getHostPMMU()->getNodeID() % num_column);

        // host_info->num_pages tells us how many pages we are allowed to allocate
        gov_request->incPagesServed(host_info->getNumPages());
    }
    return host_info->getNumPages();
}

int
BaseGovernor::dallocation_helper_virtual_address(GOVRequest *gov_request, HostInfo *host_info)
{
    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    int num_removed_pages = host_info->getUserPMMU()->removeATTMappingsVAddress(gov_request, host_info);

    DPRINTF(GOV, "%s: Deallocating %d/%d SPM slot(s) for node (%d,%d)\n",
            gov_type, num_removed_pages, total_num_pages,
            host_info->getUserPMMU()->getNodeID() / num_column,
            host_info->getUserPMMU()->getNodeID() % num_column);

    gov_request->incPagesServed(total_num_pages);
    return num_removed_pages;
}

int
BaseGovernor::dallocation_helper_spm_address(HostInfo *current_host_info)
{
    GOVRequest gov_request = GOVRequest(current_host_info->getUserThreadContext(), Deallocation,
                                        Addr(0), Addr(0), 0);
    current_host_info->getUserPMMU()->removeATTMappingsSPMAddress(&gov_request, current_host_info);

    DPRINTF(GOV, "%s: Evicting %d SPM slot(s) on node (%d,%d) used by node (%d,%d)\n",
            gov_type, current_host_info->getNumPages(),
            current_host_info->getHostPMMU()->getNodeID() / num_column,
            current_host_info->getHostPMMU()->getNodeID() % num_column,
            current_host_info->getUserPMMU()->getNodeID() / num_column,
            current_host_info->getUserPMMU()->getNodeID() % num_column);

    return current_host_info->getNumPages();
}

void
BaseGovernor::relocation_helper_virtual_address(GOVRequest *gov_request,
                                                HostInfo *current_host_info,
                                                HostInfo *future_host_info)
{
}

void
BaseGovernor::relocation_helper_spm_address(HostInfo *current_host_info,
                                            HostInfo *future_host_info)
{

    current_host_info->getUserPMMU()->changeATTMappingSPMAddress(current_host_info,
                                                                 future_host_info);

    future_host_info->getHostPMMU()->setUsedPages(future_host_info->getSPMaddress(),
                                                  future_host_info->getNumPages(),
                                                  future_host_info->getUserPMMU()->getNodeID());

    DPRINTF(GOV, "%s: Relocating %d SPM slot(s) belong to node (%d,%d) from node (%d,%d) to node (%d,%d)\n",
            gov_type, future_host_info->getNumPages(),
            current_host_info->getUserPMMU()->getNodeID() / num_column,
            current_host_info->getUserPMMU()->getNodeID() % num_column,
            current_host_info->getHostPMMU()->getNodeID() / num_column,
            current_host_info->getUserPMMU()->getNodeID() % num_column,
            future_host_info->getHostPMMU()->getNodeID() / num_column,
            future_host_info->getHostPMMU()->getNodeID() % num_column);
}

void
BaseGovernor::add_mapping_unallocated_pages(GOVRequest *gov_request)
{
    PMMU *requester_pmmu = gov_request->getPMMUPtr();
    int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);

    HostInfo host_info (gov_request->getThreadContext(), gov_request->getPMMUPtr(),
                        gov_request->getPMMUPtr(), Addr(0), total_num_pages);
    host_info.setAllocMode(NUM_ALLOCATION_MODE);
    gov_request->getAnnotations()->alloc_mode = NUM_ALLOCATION_MODE;

    int num_added_pages = requester_pmmu->addATTMappingsVAddress(gov_request, &host_info);

    DPRINTF(GOV, "%s: Adding %d/%d unallocated SPM pages for node (%d,%d) \n",
            gov_type, num_added_pages, total_num_pages,
            requester_pmmu->getNodeID() / num_column,
            requester_pmmu->getNodeID() % num_column);

    gov_request->incPagesServed(total_num_pages);

    assert(num_added_pages == total_num_pages);
}

void
BaseGovernor::cache_invalidator_helper(GOVRequest *gov_request)
{
    SPM *requester_spm = gov_request->getSPMPtr();
    FuncPageTable *requester_pt = gov_request->getPageTablePtr();
    Addr aligned_start_addr = gov_request->getStartAddr(Unserved_Aligned);
    Addr aligned_end_addr = gov_request->getEndAddr(Unserved_Aligned);

    const Addr addr_range = aligned_end_addr - aligned_start_addr;

    const unsigned blkSize = requester_spm->getCacheBlkSize();
    for (int i=0; i < addr_range / blkSize; i++ ) {

        Addr p_page_addr;
        bool translated = requester_pt->translate(aligned_start_addr + i*blkSize, p_page_addr);
        assert(translated);

        requester_spm->writebackCacheCopies(p_page_addr);
    }

//    cache_ptr->memWriteback();
//    cache_ptr->memInvalidate();
}
