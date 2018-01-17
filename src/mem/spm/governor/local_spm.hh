/* This class implements an SPM governor which maps the allocation
 * requests to the local SPMs only. It first finds the highest number
 * of free contiguous slots on the local SPM and then maps as many pages
 * as possible to that region and repeats this process until no page is free
 * and then the rest would go off chip.
 *
 * Authors: Majid, Donny
 * */

#ifndef __LOCAL_SPM_HH__
#define __LOCAL_SPM_HH__

#include "params/LocalSPM.hh"
#include "mem/spm/governor/base.hh"

using namespace std;

class LocalSPM : public BaseGovernor {

  public:
    LocalSPM(const Params *p);
    virtual ~LocalSPM();
    virtual void init();

    // SPM APIs
    virtual int allocate(GOVRequest *gov_request);
    virtual int deAllocate(GOVRequest *gov_request);
};

#endif  /* __LOCAL_SPM_HH__ */
