/* This class implements an SPM governor which greedily maps the allocation
 * requests to the closest free SPM, while guaranteeing a each thread gets
 * defined percentage of its local SPM should it needs that space.
 *
 * Authors: Majid, Donny
 * */

#ifndef __GUARANTEED_GREEDY_SPM_HH__
#define __GUARANTEED_GREEDY_SPM_HH__

#include "params/GuaranteedGreedySPM.hh"
#include "mem/spm/governor/greedy_spm.hh"

class GuaranteedGreedySPM : public GreedySPM {

  public:
    GuaranteedGreedySPM(const Params *p);
    virtual ~GuaranteedGreedySPM();
    virtual void init();

    virtual int allocate(GOVRequest *gov_request);

  protected:
    float local_share;
    string guest_slot_selection_policy;
    string guest_slot_relocation_policy;

    virtual float spmShare(PMMU *owner_pmmu, PMMU *user_pmmu);

    virtual void findGuestSlotToRelocate(HostInfo *current_host_info,
                                         string selection_policy = "LeastIndexed");
    virtual int findLeastIndexedGuestSlotToRelocate(HostInfo *current_host_info);
    virtual int findLeastFrequentlyUsedGuestSlotToRelocate(HostInfo *current_host_info);
    virtual int findLeastRecentlyUsedGuestSlotToRelocate(HostInfo *current_host_info);

    virtual bool findClosestFreeSlots(HostInfo *future_host_info);

    virtual void relocateSlots(HostInfo *current_host_info,
                               string relocation_policy = "ClosestFreeSlot");
};

#endif  /* __GUARANTEED_GREEDY_SPM_HH__ */
