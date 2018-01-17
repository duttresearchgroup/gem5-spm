#include "mem/spm/governor/guaranteed_greedy_spm.hh"

#include <iostream>

GuaranteedGreedySPM *
GuaranteedGreedySPMParams::create()
{
    return new GuaranteedGreedySPM(this);
}

GuaranteedGreedySPM::GuaranteedGreedySPM(const Params *p)
    : GreedySPM(p)
{
    gov_type = "GuaranteedGreedy";
    local_share = p->local_share;
    guest_slot_selection_policy = p->guest_slot_selection_policy;
    guest_slot_relocation_policy = p->guest_slot_relocation_policy;

}

GuaranteedGreedySPM::~GuaranteedGreedySPM()
{

}

void
GuaranteedGreedySPM::init()
{

}

/* What percentage of owner_pmmu's spm is used by user_pmmu? */
float
GuaranteedGreedySPM::spmShare(PMMU *owner_pmmu, PMMU *user_pmmu)
{
    int num_slots = owner_pmmu->getSPMSizePages();

    int slot_idx;
    int slots_used = 0;

    for (slot_idx = 0; slot_idx < num_slots; slot_idx++) {
        if (!owner_pmmu->my_spm_ptr->spmSlots[slot_idx].isFree()&&
            owner_pmmu->my_spm_ptr->spmSlots[slot_idx].getOwner() == user_pmmu->getNodeID())
            slots_used++;
    }
    return float(slots_used)/float(num_slots);
}

int
GuaranteedGreedySPM::findLeastIndexedGuestSlotToRelocate(HostInfo *current_host_info)
{
    PMMU *owner_pmmu = current_host_info->getHostPMMU();
    int candidate_slot_idx = 0;
    while (owner_pmmu->my_spm_ptr->spmSlots[candidate_slot_idx].getOwner() == owner_pmmu->getNodeID())
        candidate_slot_idx++;

    assert(candidate_slot_idx < owner_pmmu->getSPMSizePages());

    return candidate_slot_idx;
}

int
GuaranteedGreedySPM::findLeastRecentlyUsedGuestSlotToRelocate(HostInfo *current_host_info)
{
    PMMU *owner_pmmu = current_host_info->getHostPMMU();
    int candidate_slot_idx = 0;
    Tick least_recent_tick = UINT64_MAX;
    for (int i=0; i<owner_pmmu->getSPMSizePages(); i++) {
        if (!(owner_pmmu->my_spm_ptr->spmSlots[i].getOwner() == owner_pmmu->getNodeID())) {
            if (owner_pmmu->my_spm_ptr->spmSlots[i].getLastTickAccessed() < least_recent_tick) {
                least_recent_tick = owner_pmmu->my_spm_ptr->spmSlots[i].getLastTickAccessed();
                candidate_slot_idx = i;
            }
        }
    }
    return candidate_slot_idx;
}

int
GuaranteedGreedySPM::findLeastFrequentlyUsedGuestSlotToRelocate(HostInfo *current_host_info)
{
    PMMU *owner_pmmu = current_host_info->getHostPMMU();
    int candidate_slot_idx = 0;
    uint64_t least_access_count = UINT64_MAX;
    for (int i=0; i<owner_pmmu->getSPMSizePages(); i++) {
        if (!(owner_pmmu->my_spm_ptr->spmSlots[i].getOwner() == owner_pmmu->getNodeID())) {
            if (owner_pmmu->my_spm_ptr->spmSlots[i].getRefCount() < least_access_count) {
                least_access_count = owner_pmmu->my_spm_ptr->spmSlots[i].getRefCount();
                candidate_slot_idx = i;
            }
        }
    }
    return candidate_slot_idx;
}

void
GuaranteedGreedySPM::findGuestSlotToRelocate(HostInfo *current_host_info, string selection_policy)
{
    int candidate_slot_idx = -1;

    if (selection_policy.compare("LeastIndexed") == 0) {
        candidate_slot_idx = findLeastIndexedGuestSlotToRelocate(current_host_info);
    } else if (selection_policy.compare("LeastRecentlyUsed") == 0) {
        candidate_slot_idx = findLeastRecentlyUsedGuestSlotToRelocate(current_host_info);
    } else if (selection_policy.compare("LeastFrequentlyUsed") == 0) {
        candidate_slot_idx = findLeastFrequentlyUsedGuestSlotToRelocate(current_host_info);
    } else {
        panic("%s selection policy is not implemented", selection_policy);
    }

    NodeID user_id = current_host_info->getHostPMMU()->my_spm_ptr->spmSlots[candidate_slot_idx].getOwner();
    int user_col = user_id % num_column;
    int user_row = user_id / num_column;
    current_host_info->setUserPMMU(coord_to_pmmu[make_pair(user_col,user_row)]);
    BaseCPU *user_cpu = dynamic_cast<BaseCPU*>(((current_host_info->getUserPMMU())->
                        my_spm_ptr->getSlavePort("cpu_side", 0).getMasterPort()).getOwner());
    current_host_info->setUserThreadContext(user_cpu->getContext(0));

    Addr slot_to_be_evicted_p_addr = candidate_slot_idx * current_host_info->getUserPMMU()->getPageSizeBytes();
    current_host_info->setSPMaddress(slot_to_be_evicted_p_addr);
}

bool
GuaranteedGreedySPM::findClosestFreeSlots(HostInfo *future_host_info)
{
    PMMU *previous_center = pmmus[0];
    bool found = false;
    future_host_info->setHostPMMU(nullptr);

    distanceBasedSort(pmmu_to_coord.at(future_host_info->getUserPMMU()));
    assert(future_host_info->getUserPMMU() == pmmus[0]);

    vector<PMMU *>::size_type pmmu_it = 0;
    while (pmmu_it != pmmus.size()) {
        future_host_info->setHostPMMU(pmmus[pmmu_it]);

        int pages_needed = future_host_info->getNumPages();

        found = getMaxContiguousFreePages(future_host_info, pages_needed);

        DPRINTF(GOV, "%s: Examining node (%d , %d) for %d free slot(s) "
                "to be used by (%d , %d). Found = %d\n",
                gov_type,
                future_host_info->getHostPMMU()->getNodeID() / num_column,
                future_host_info->getHostPMMU()->getNodeID() % num_column,
                pages_needed,
                future_host_info->getUserPMMU()->getNodeID() / num_column,
                future_host_info->getUserPMMU()->getNodeID() % num_column,
                future_host_info->getNumPages());

        if (found) {
            break;
        }

        // getMaxContiguousFreePages() reassigns the value
        future_host_info->setNumPages(pages_needed);

        pmmu_it++;
    }

    distanceBasedSort(pmmu_to_coord.at(previous_center));

    if (!found) {
        future_host_info->setHostPMMU(nullptr);
    }

    return found;
}

void
GuaranteedGreedySPM::relocateSlots(HostInfo *current_host_info,
                                   string relocation_policy)
{
    // relocate the slot
    // option 1:  (relocate == evict)
    if (relocation_policy.compare("MoveOffChip") == 0) {
        current_host_info->setDeallocMode(WRITE_BACK);
        dallocation_helper_spm_address(current_host_info);
    // option 2:  (relocate == migrate if possible)
    } else if (relocation_policy.compare("ClosestFreeSlot") == 0) {
        // try migrating to the closest free slot
        HostInfo future_host_info (nullptr, current_host_info->getUserPMMU(), nullptr,
                                   Addr(0), current_host_info->getNumPages());
        if (findClosestFreeSlots(&future_host_info)) {
            relocation_helper_spm_address(current_host_info, &future_host_info);
        }
        else { // if not available, deallocate
            current_host_info->setDeallocMode(WRITE_BACK);
            dallocation_helper_spm_address(current_host_info);
        }
    } else {
        panic("%s relocation policy is not implemented", relocation_policy);
    }
}

int
GuaranteedGreedySPM::allocate(GOVRequest *gov_request)
{
    printRequestStatus(gov_request);

    const int total_num_pages = gov_request->getNumberOfPages(Unserved_Aligned);
    if (total_num_pages <= 0) {
        return 0;
    }

    int remaining_pages = total_num_pages;

    if (!gov_type.compare("GuaranteedGreedy") && hybrid_mem) {
        cache_invalidator_helper(gov_request);
    }

    // 1. Allocate on the local SPM first
    remaining_pages -= LocalSPM::allocate(gov_request);

    PMMU *requester_pmmu = gov_request->getPMMUPtr();

    // 2. Check if the requester_pmmu has NOT used its minimum share of its local SPM yet
    float current_local_spm_share = spmShare(requester_pmmu, requester_pmmu);
    while (remaining_pages > 0 && (current_local_spm_share < local_share)) {
        DPRINTF(GOV, "%s: Current local share for node (%d , %d) = %f\n",
                gov_type,
                requester_pmmu->getNodeID() / num_column,
                requester_pmmu->getNodeID() % num_column,
                current_local_spm_share);

        // find a slot to be relocated
        int num_slots_to_relocate = 1;
        HostInfo candidate_host_info = HostInfo(nullptr, nullptr, requester_pmmu,
                                                Addr(0), num_slots_to_relocate);
        candidate_host_info.setSignalee(requester_pmmu->getNodeID());
        findGuestSlotToRelocate(&candidate_host_info, guest_slot_selection_policy);

        relocateSlots(&candidate_host_info, guest_slot_relocation_policy);

        // allocate one slot for the requested page on the local SPM
        HostInfo host_info = HostInfo(gov_request->getThreadContext(),
                                      gov_request->getPMMUPtr(),
                                      candidate_host_info.getHostPMMU(),
                                      candidate_host_info.getSPMaddress(),
                                      candidate_host_info.getNumPages());
        host_info.setAllocMode(gov_request->getAnnotations()->alloc_mode);
        host_info.setSignaler(candidate_host_info.getUserPMMU()->getNodeID());
        remaining_pages -= allocation_helper_on_occupied_page(gov_request, &host_info);

        current_local_spm_share += float(num_slots_to_relocate) /
                                   float(requester_pmmu->getSPMSizePages());
    }

    // 3. If we still need more SPM slots, look at the nearby SPMs in the order of hop distance
    if (remaining_pages > 0) {
        DPRINTF(GOV, "%s: Node (%d , %d) has used all its local share = %f\n",
                gov_type,
                requester_pmmu->getNodeID() / num_column,
                requester_pmmu->getNodeID() % num_column,
                current_local_spm_share);

        remaining_pages -= GreedySPM::allocate(gov_request);
    }

    if (!gov_type.compare("GuaranteedGreedy") && uncacheable_spm) {
        add_mapping_unallocated_pages(gov_request);
    }

    return total_num_pages - remaining_pages;
}
