/* This class implements an SPM governor which maps the allocation
 * requests to a random on-chip SPM after all local SPM slots are taken.
 * It first finds the highest number of free contiguous slots on the
 * randomly chosen remote SPM and then maps as many pages as possible to
 * that SPM, repeats this process until no free slot is available anywhere on-chip,
 * and the rest would go off chip.
 *
 * Authors: Majid, Donny
 * */

#ifndef __RANDOM_SPM_HH__
#define __RANDOM_SPM_HH__

#include "params/RandomSPM.hh"
#include "mem/spm/governor/local_spm.hh"

using namespace std;

class RandomSPM : public LocalSPM {

  public:
    RandomSPM(const Params *p);
    virtual ~RandomSPM();
    virtual void init();

    virtual int allocate(GOVRequest *gov_request);
    virtual void addPMMU(PMMU *p);

  protected:
    vector<PMMU *> pmmus;
    map<pair<int,int>, PMMU *> coord_to_pmmu;

};

#endif  /* __RANDOM_SPM_HH__ */
