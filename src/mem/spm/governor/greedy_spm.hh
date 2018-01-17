/* This class implements an SPM governor which greedily maps the allocation
 * requests to the closest free on-chip SPM.
 *
 * Authors: Majid
 * */

#ifndef __GREEDY_SPM_HH__
#define __GREEDY_SPM_HH__

#include "params/GreedySPM.hh"
#include "mem/spm/governor/random_spm.hh"

class GreedySPM : public RandomSPM {

  public:
    GreedySPM(const Params *p);
    virtual ~GreedySPM();
    virtual void init();

    virtual int allocate(GOVRequest *gov_request);
    virtual void addPMMU(PMMU *p);

  protected:
    map<PMMU *, pair<int,int>> pmmu_to_coord;

    int findHopDistance (pair <int, int> pmmu_o, pair <int, int> pmmu_h);
    void distanceBasedSort(pair <int, int> central_pmmu);

};

#endif  /* __GREEDY_SPM_HH__ */
