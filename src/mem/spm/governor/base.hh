#ifndef __BASE_GOVERNOR_HH__
#define __BASE_GOVERNOR_HH__

#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/GOV.hh"
#include "mem/spm/api/spm_types.h"
#include "mem/spm/governor/GOVRequest.hh"
#include "mem/spm/pmmu.hh"
#include "mem/spm/spm_class/spm.hh"
#include "mem/spm/spm_class/SPMPage.hh"
#include "params/BaseGovernor.hh"
#include "sim/process.hh"
#include "sim/sim_object.hh"

#include <string>

class BaseGovernor : public SimObject {

  public:

    typedef BaseGovernorParams Params;
    BaseGovernor(const Params *p);

    virtual ~BaseGovernor() {}
    virtual void init() {}

    // SPM APIs
    virtual int allocate(GOVRequest *gov_request) = 0;
    virtual int deAllocate(GOVRequest *gov_request) = 0;

  protected:
    // dimensions of the mesh
    int num_column;
    int num_row;

    std::string gov_type;
    bool hybrid_mem;
    bool uncacheable_spm;

    void printRequestStatus(GOVRequest *gov_request);

    // SPM allocation/deallocation utilities
    bool getMaxContiguousFreePages(HostInfo *host_info,
                                   int max_num_pages_needed,
                                   int start_page_index = 0);

    // SPM allocation/deallocation/relocation primitives
    virtual int allocation_helper_on_free_pages(GOVRequest *gov_request,
                                                HostInfo *host_info);

    virtual int allocation_helper_on_occupied_page(GOVRequest *gov_request,
                                                   HostInfo *host_info);

    virtual int dallocation_helper_virtual_address(GOVRequest *gov_request,
                                                   HostInfo *host_info);

    virtual int dallocation_helper_spm_address(HostInfo *current_host_info);

    virtual void relocation_helper_virtual_address(GOVRequest *gov_request,
                                                   HostInfo *current_host_info,
                                                   HostInfo *future_host_info);

    virtual void relocation_helper_spm_address(HostInfo *current_host_info,
                                               HostInfo *future_host_info);

    virtual void add_mapping_unallocated_pages(GOVRequest *gov_request);

    virtual void cache_invalidator_helper(GOVRequest *gov_request);
};

#endif  /* __BASE_GOVERNOR_HH__ */
