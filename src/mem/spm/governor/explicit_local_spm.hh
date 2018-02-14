/* This class implements an SPM governor which maps the allocation
 * requests to the local SPMs only. It first finds the highest number
 * of free contiguous slots on the local SPM and then maps as many pages
 * as possible to that region and repeats this process until no page is free
 * and then the rest would go off chip.
 *
 * Authors: Majid, Donny
 * */

#ifndef __EXPLICIT_LOCAL_SPM_HH__
#define __EXPLICIT_LOCAL_SPM_HH__

#include "params/ExplicitLocalSPM.hh"
#include "mem/spm/governor/base.hh"

using namespace std;

class ExplicitLocalSPM : public BaseGovernor {

  public:
	ExplicitLocalSPM(const Params *p);
    virtual ~ExplicitLocalSPM();
    virtual void init();

    // SPM APIs
    virtual int allocate(GOVRequest *gov_request);
    virtual int deAllocate(GOVRequest *gov_request);

  protected:
    virtual int allocation_helper_on_free_pages(GOVRequest *gov_request,
                                                HostInfo *host_info) override;

};

#endif  /* __EXPLICIT_LOCAL_SPM_HH__ */
