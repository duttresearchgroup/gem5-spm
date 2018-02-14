/*
 * Copyright (c) 2010-2015 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * Copyright (c) 2010,2015 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Erik Hallnor
 *          Dave Greene
 *          Nathan Binkert
 *          Steve Reinhardt
 *          Ron Dreslinski
 *          Andreas Sandberg
 */

/**
 * @file
 * SPM definitions.
 */

#include "base/types.hh"
#include "debug/SPM.hh"
#include "debug/SPMPort.hh"
#include "mem/spm/governor/GOVRequest.hh"
#include "mem/spm/spm_class/spm.hh"
#include "mem/spm/pmmu.hh"
#include "mem/cache/cache.hh"
#include "sim/sim_exit.hh"

SPM::SPM(const SPMParams *p)
    : BaseSPM(p, p->system->cacheLineSize()),
      read_ber(p->read_ber),
      write_ber(p->write_ber),
      ber_energy_file(p->ber_energy_file),
      pendingReqs(0)
{
    cpuSidePort = new CpuSidePort(p->name + ".cpu_side", this,
                                  "CpuSidePort");
    memSidePort = new MemSidePort(p->name + ".mem_side", this,
                                  "MemSidePort");

    pmmuSlavePort = new PMMUSideSlavePort(p->name + ".pmmu_s_side", this, 1);
    pmmuMasterPort = new PMMUSideMasterPort(p->name + ".pmmu_m_side", this, 0);
}

void
SPM::init()
{
    myPMMU = dynamic_cast<PMMU*>(pmmuSlavePort->getMasterPort().getOwner());
    assert(myPMMU);

    myPMMU->setSPM(this);
    pageSizeBytes = myPMMU->getPageSizeBytes();
    spmSlots = new SPMPage[size/pageSizeBytes];
    for(int i = 0; i < size/pageSizeBytes; i++) {
        spmSlots[i].readBERInfo(ber_energy_file);
        if (read_ber >= 0 || write_ber >= 0)  // if not their default values
            spmSlots[i].overrideActiveBERPoint(read_ber, write_ber);
    }
}

SPM::~SPM()
{
    delete cpuSidePort;
    delete memSidePort;

    delete [] spmSlots;
}

void
SPM::regStats()
{
    BaseSPM::regStats();
}

/////////////////////////////////////////////////////
//
// Access path: requests coming in from the CPU side
//
/////////////////////////////////////////////////////

class ForwardResponseRecord : public Packet::SenderState
{
  public:

    ForwardResponseRecord() {}
};

void
SPM::doWritebacks(PacketList& writebacks, Tick forward_time)
{
    panic("doWritebacks()");
}

void
SPM::doWritebacksAtomic(PacketList& writebacks)
{
    panic("doWritebacksAtomic()");
}


void
SPM::recvTimingSnoopResp(PacketPtr pkt)
{
    panic("recvTimingSnoopResp()");
}

Tick
SPM::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic()");
    return 0;
}


void
SPM::functionalAccess(PacketPtr pkt, bool fromCpuSide)
{
    if (system->bypassCaches()) {
        // Packets from the memory side are snoop request and
        // shouldn't happen in bypass mode.
        assert(fromCpuSide);

        // The spm should be flushed if we are in spm bypass mode,
        // so we don't need to check if we need to update anything.
        memSidePort->sendFunctional(pkt);
        return;
    }

    bool done = cpuSidePort->checkFunctional(pkt)
        || memSidePort->checkFunctional(pkt);

    // We're leaving the spm, so pop spm->name() label
    pkt->popLabel();

    if (done) {
        pkt->makeResponse();
    } else {
        // if it came as a request from the CPU side then make sure it
        // continues towards the memory side
        if (fromCpuSide) {
            memSidePort->sendFunctional(pkt);
        } else if (forwardSnoops && cpuSidePort->isSnooping()) {
            // if it came from the memory side, it must be a snoop request
            // and we should only forward it if we are forwarding snoops
        	panic("SPM::functionalAccess: we shouldn't be snooping");
            cpuSidePort->sendFunctionalSnoop(pkt);
        }
    }
}

/////////////////////////////////////////////////////
//
// Snoop path: requests coming in from the memory side
//
/////////////////////////////////////////////////////

void
SPM::recvTimingSnoopReq(PacketPtr pkt)
{
    return;
    panic("recvTimingSnoopReq()");
}

bool
SPM::CpuSidePort::recvTimingSnoopResp(PacketPtr pkt)
{
    // Express snoop responses from master to slave, e.g., from L1 to L2
    spm->recvTimingSnoopResp(pkt);
    return true;
}

Tick
SPM::recvAtomicSnoop(PacketPtr pkt)
{
    panic("recvAtomicSnoop()");
    return 0;
}

///////////////
//
// CpuSidePort
//
///////////////

AddrRangeList
SPM::CpuSidePort::getAddrRanges() const
{
    return spm->getAddrRanges();
}

bool
SPM::CpuSidePort::recvTimingReq(PacketPtr pkt)
{
    assert(!spm->system->bypassCaches());

    bool success = false;

    // always let inhibited requests through, even if blocked,
    // ultimately we should check if this is an express snoop, but at
    // the moment that flag is only set in the spm itself
    if (pkt->isExpressSnoop()) {
        panic("CpuSide::recvTimingReq() express snoop");
        // do not change the current retry state
        bool M5_VAR_USED bypass_success = spm->recvTimingReq(pkt);
        assert(bypass_success);
        return true;
    } else if (blocked || mustSendRetry) {
        // either already committed to send a retry, or blocked
        success = false;
    } else {
        // pass it on to the spm, and let the spm decide if we
        // have to retry or not
        success = spm->recvTimingReq(pkt);
    }

    // remember if we have to retry
    mustSendRetry = !success;
    return success;
}

Tick
SPM::CpuSidePort::recvAtomic(PacketPtr pkt)
{
    return spm->recvAtomic(pkt);
}

void
SPM::CpuSidePort::recvFunctional(PacketPtr pkt)
{
    // functional request
    spm->functionalAccess(pkt, true);
}

SPM::
CpuSidePort::CpuSidePort(const std::string &_name, SPM *_spm,
                         const std::string &_label)
    : BaseSPM::SPMSlavePort(_name, _spm, _label), spm(_spm)
{
}

SPM*
SPMParams::create()
{
    return new SPM(this);
}
///////////////
//
// MemSidePort
//
///////////////

bool
SPM::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    spm->recvTimingResp(pkt);
    return true;
}

// Express snooping requests to memside port
void
SPM::MemSidePort::recvTimingSnoopReq(PacketPtr pkt)
{
    // handle snooping requests
    spm->recvTimingSnoopReq(pkt);
}

Tick
SPM::MemSidePort::recvAtomicSnoop(PacketPtr pkt)
{
    return spm->recvAtomicSnoop(pkt);
}

void
SPM::MemSidePort::recvFunctionalSnoop(PacketPtr pkt)
{
    // functional snoop (note that in contrast to atomic we don't have
    // a specific functionalSnoop method, as they have the same
    // behaviour regardless)
	panic("SPM::MemSidePort::recvFunctionalSnoop we shouldn't get snoops");
    spm->functionalAccess(pkt, false);
}

SPM::
MemSidePort::MemSidePort(const std::string &_name, SPM *_spm,
                         const std::string &_label)
    : BaseSPM::SPMMasterPort(_name, _spm, _reqQueue, _snoopRespQueue),
      _reqQueue(*_spm, *this, _snoopRespQueue, _label),
      _snoopRespQueue(*_spm, *this, _label), spm(_spm)
{
}

/////////////////////////////////////////////////////
//
// Response handling: responses from the memory side
//
/////////////////////////////////////////////////////

void
SPM::recvTimingResp(PacketPtr pkt)
{
    if (pkt->govInfo.isAllocate()) {
        // unblock the cpu port
        conditionalUnblocking(Blocked_MaxPendingReqs);
        satisfyPageAllocation (pkt);
    }
    else if (pkt->govInfo.isDeallocate()) {
        // unblock the cpu port
        conditionalUnblocking(Blocked_MaxPendingReqs);
        satisfyPageDeallocation(pkt);
    }
    else if (!pkt->spmInfo.isOnChip()) {
        // unblock the cpu port
        conditionalUnblocking(Blocked_MaxPendingReqs);
        sendTimingRespToCPU(pkt, forwardLatency);
    }
    else {
        panic ("Invalid packet type received from memory.");
    }
}

/////////////////////
//
// SPM-related methods
//
/////////////////////

bool
SPM::recvTimingReq(PacketPtr pkt)
{
    //@TODO: Only forward CPU's pkts
    assert(pkt->isRequest());

    //Addr v_addr = pkt->req->getVaddr();
    //Addr p_addr = pkt->req->getPaddr();
    //DPRINTF(SPM, "SPM sends request to PMMU for \tVA = %x \tPA = %x\n", v_addr, p_addr);

    //Forward everything to main memory
    //memSidePort->schedTimingReq(pkt, curTick()+1);
    //return true;

    // Query PPMU for data holder or ask for performing a remote access
    pmmuMasterPort->schedTimingReq(pkt, curTick());

    return true;
}

bool
SPM::recvPMMUTimingReq(PacketPtr pkt)
{
    assert(pkt->isRequest());

    //case 1: page allocation
    if (pkt->govInfo.isAllocate()) {
        // forward it to main memory,
        // once response returned from memory we will fill the SPM
        if (pkt->govInfo.getAnnotations()->alloc_mode == COPY) {
            initializePageAllocation(pkt);
        }

        else {
            assert(pkt->govInfo.getAnnotations()->alloc_mode == UNINITIALIZE);
            pkt->makeResponse();
            conditionalUnblocking(Blocked_MaxPendingReqs);
            satisfyPageAllocation(pkt);
        }

        return true;
    }
    //case 2: page deallocation
    else if (pkt->govInfo.isDeallocate()) {
        // page number should be in the pkt
        // i) find the page
        // ii) call a pageDeallocation function
        if (pkt->govInfo.getAnnotations()->dealloc_mode == WRITE_BACK) {
            initializePageDeallocation(pkt);
        }

        else {
            assert(pkt->govInfo.getAnnotations()->dealloc_mode == DISCARD);
            initializePageDeallocation(pkt);
            pkt->makeResponse();
            conditionalUnblocking(Blocked_MaxPendingReqs);
            satisfyPageDeallocation(pkt);
        }

        return true;
    }
    //case 3: relocations
    //TODO: Consolidate this with the next case?
    else if (pkt->govInfo.isRelocationRead() || pkt->govInfo.isRelocationWrite()) {
        Cycles lat;
        satisfySPMAccess(pkt, &lat);

        pkt->saveCmd();
        pkt->makeResponse();
        pmmuSlavePort->schedTimingResp(pkt, clockEdge(lat));

        return true;
    }
    //case 4: remote requests
    else if (pkt->spmInfo.isRemote()) {
        Cycles lat;
        satisfySPMAccess(pkt, &lat);

        pkt->saveCmd();
        pkt->makeResponse();
        pmmuSlavePort->schedTimingResp(pkt, clockEdge(lat));

        return true;
    }
    else {
        panic("Invalid request type received from PMMU");
    }
}

void
SPM::recvPMMUTimingResp(PacketPtr pkt)
{
    // response for a CPU request
    assert(pkt->isResponse());

    DPRINTF(SPM, "SPM <== PMMU - VA = %x\t - Data holder = %s\n",
            pkt->req->getVaddr(), (pkt->spmInfo.toString()));

    bool is_error = pkt->isError();

    if (is_error) {
        DPRINTF(SPM, "SPM received packet with error for address %x (%s), "
                "cmd: %s\n", pkt->getAddr(), pkt->isSecure() ? "s" : "ns",
                pkt->cmdString());

        panic("Error packet received! We don't handle it.");
    }

    if (system->bypassCaches()) {
        memSidePort->sendTimingReq(pkt);
        return;
    }

    if (pkt->spmInfo.isLocal()) {

        DPRINTF(SPM, "CPU <== SPM - VA = %x\n", pkt->req->getVaddr());

        // @TODO: temporary work-around for stats as hits should be counted for a req not resp
        pkt->retrieveCmd();
        incLocalHitCount(pkt);

        Cycles lat;
        satisfySPMAccess (pkt, &lat);

        sendTimingRespToCPU(pkt, lat);

    } else if (pkt->spmInfo.isRemote()){

        DPRINTF(SPM, "CPU <== SPM - VA = %x\n", pkt->req->getVaddr());

        pkt->retrieveCmd();
        incRemoteHitCount(pkt);

        sendTimingRespToCPU(pkt, forwardLatency);

    } else if (pkt->spmInfo.isUnallocated()) {
        assert (pendingReqs <= MAX_PENDING_REQS);

        pkt->retrieveCmd();

        incMissCount(pkt);

        memSidePort->schedTimingReq(pkt, curTick());

    } else if (pkt->spmInfo.isNotSPMSpace()) {
        assert (pendingReqs <= MAX_PENDING_REQS);

        pkt->retrieveCmd();

        memSidePort->schedTimingReq(pkt, curTick());

    } else{
        panic("Invalid data holder!");
    }
}

bool
SPM::satisfySPMAccess(PacketPtr pkt, Cycles* lat)
{
    DPRINTF(SPM, "%s for %s VA = %x size %d\n", __func__,
            pkt->cmdString(), pkt->req->getVaddr(), pkt->getSize());

    unsigned int spm_page_index = pAddress2PageIndex(pkt->spmInfo.getSPMAddress());
    assert (spm_page_index < size/pageSizeBytes);

    SPMPage *page =  &spmSlots[spm_page_index]; // TODO: we need to do some bookkeeping for accesses, latencies, etc

    if (pkt->isWrite()) {
        if (page->checkWrite(pkt)) {
            *lat = page->getWriteSpeed() + lookupLatency;
            if (pkt->govInfo.isRelocationWrite()) {
                page->resetStatsForNewlyAddedPage();
                page->setAnnotations(pkt->govInfo.getAnnotations());
                page->performWrite(pkt);
                incDynamicEnergy(pkt, page->getWriteEnergy()*(pageSizeBytes));
            }
            else {
                pkt->setAddr(pkt->req->getVaddr()); // because offset calculation requires virtual address
                page->performWrite(pkt);
                page->writeRefOccurred();
                incDynamicEnergy(pkt, page->getWriteEnergy()*(pkt->getSize()));
            }
        }

    } else if (pkt->isRead()) {
        if (pkt->isLLSC()) {
            page->trackLoadLocked(pkt);
        }
        *lat = page->getReadSpeed() + lookupLatency;
        if (pkt->govInfo.isRelocationRead()) {
            page->performRead(pkt);
            page->invalidate(false);
            incDynamicEnergy(pkt, page->getReadEnergy()*(pageSizeBytes));
        }
        else {
            pkt->setAddr(pkt->req->getVaddr()); // because offset calculation requires virtual address
            page->performRead(pkt);
            page->readRefOccurred();
            incDynamicEnergy(pkt, page->getReadEnergy()*(pkt->getSize()));
        }

    }

    return true;
}

void
SPM::sendTimingRespToCPU(PacketPtr pkt, Cycles lat)
{
//    assert(pkt->isResponse());

    // if lat is 1 cycle, this ensures that the response
    // will be returned to CPU, one cycle after a response
    // has been received from PMMU
    Tick completion_time = clockEdge(lat - Cycles(1));

    if (pkt->spmInfo.isLocal()) {
    }

    else if (pkt->spmInfo.isRemote()) {
    }

    else if (pkt->spmInfo.isUnallocated()) {
        missLatency[pkt->cmdToIndex()][pkt->req->masterId()] +=
            completion_time - pkt->req->time();
    }

    else if (pkt->spmInfo.isNotSPMSpace()) {
    }

    // path for local/remote spm is already a response
    // path for off_chip is a request
    if (!pkt->isResponse())
        pkt->makeResponse();

    cpuSidePort->schedTimingResp(pkt, completion_time);
}

///////////////
//
// Alloc/Dealloc helper functions
//
///////////////

void
SPM::initializePageAllocation(PacketPtr pkt)
{
    //conditionalBlocking(Blocked_MaxPendingReqs);
    assert (pendingReqs <= MAX_PENDING_REQS);

    memSidePort->schedTimingReq(pkt, curTick());
}

void
SPM::satisfyPageAllocation(PacketPtr pkt)
{
    unsigned int spm_page_index = pAddress2PageIndex(pkt->govInfo.getSPMAddress());

    DPRINTF(SPM, "satisfyPageAllocation on SPM Page = %d\n", spm_page_index);

    SPMPage* page = &spmSlots[spm_page_index];
//  page->setOwner(pkt->origin); // we do it before allocation in governor
    page->resetStatsForNewlyAddedPage();
    page->setAnnotations(pkt->govInfo.getAnnotations());
    if (pkt->govInfo.getAnnotations()->alloc_mode == COPY) {
        assert(pkt->getOffset(pageSizeBytes) == 0);
        page->performWrite(pkt);
        incDynamicEnergy(pkt, page->getWriteEnergy()*(pageSizeBytes));
    }

    Cycles page_write_latency = page->getWriteSpeed() + fillLatency;

    // return a response to PMMU
    pkt->govInfo.markComplete();
    pmmuSlavePort->schedTimingResp(pkt, clockEdge(page_write_latency));
}

void
SPM::initializePageDeallocation (PacketPtr pkt)
{
    // conditionalBlocking(Blocked_MaxPendingReqs);
    assert (pendingReqs <= MAX_PENDING_REQS);

    unsigned int spm_page_index = pAddress2PageIndex(pkt->govInfo.getSPMAddress());

    DPRINTF(SPM, "initializePageDeallocation on SPM Page = %d\n", spm_page_index);

    // read the page copy it to the pkt
    SPMPage* page = &spmSlots[spm_page_index];
    if (pkt->govInfo.getAnnotations()->dealloc_mode == WRITE_BACK) {
        assert(pkt->getOffset(pageSizeBytes) == 0);
        page->performRead(pkt);
    }
    page->invalidate(true);
    if (pkt->govInfo.getAnnotations()->dealloc_mode == WRITE_BACK) {
        incDynamicEnergy(pkt, page->getReadEnergy()*(pageSizeBytes));
        Cycles page_read_latency = page->getReadSpeed() + fillLatency;
        memSidePort->schedTimingReq(pkt, clockEdge(page_read_latency));
    }
}

void
SPM::satisfyPageDeallocation (PacketPtr pkt)
{
    // returning a response to PPMU
    pkt->govInfo.markComplete();
    pmmuSlavePort->schedTimingResp(pkt, curTick());
}

///////////////
//
// Blocking/Unblocking helper functions
//
///////////////

void
SPM::conditionalBlocking(BlockedCause cause)
{
    assert (pendingReqs < MAX_PENDING_REQS);
    pendingReqs++;
    if (pendingReqs == MAX_PENDING_REQS)
        setBlocked(cause);
}

void
SPM::conditionalUnblocking(BlockedCause cause)
{
    assert (pendingReqs <= MAX_PENDING_REQS);
    pendingReqs--;
    if (pendingReqs <= MAX_PENDING_REQS && isBlocked())
        clearBlocked(cause);
}

bool
SPM::acceptingMemReqs()
{
    assert(pendingReqs <= MAX_PENDING_REQS);
    return  (pendingReqs < MAX_PENDING_REQS);
}

/////////////////////
//
// Cache-related
//
/////////////////////

void
SPM::writebackCacheCopies(Addr p_page_addr) {
    Cache *cache_ptr = dynamic_cast<Cache*>(memSidePort->getSlavePort().getOwner());
    assert(cache_ptr);

    CacheBlk *blk = cache_ptr->findBlock(p_page_addr,false);
    if (blk) {
        cache_ptr->writebackVisitor(*blk);
        cache_ptr->invalidateVisitor(*blk);
    }
}

unsigned
SPM::getCacheBlkSize() {
    Cache *cache_ptr = dynamic_cast<Cache*>(memSidePort->getSlavePort().getOwner());
    assert(cache_ptr);

    return cache_ptr->getBlockSize();
}

/////////////////////
//
// PMMUSideSlavePort
//
/////////////////////

BaseSlavePort &
SPM::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "pmmu_s_side") {
        return *pmmuSlavePort;
    } else {
        return BaseSPM::getSlavePort(if_name, idx);
    }
}

SPM::PMMUSideSlavePort::PMMUSideSlavePort(const std::string &_name, SPM *_spm, PortID id)
    : QueuedSlavePort(_name, NULL, _respQueue, id),
      _respQueue(*_spm, *this),
      my_spm(_spm)
{
    DPRINTF(SPM, "Created slave pmmuport on spm %s\n", _name);
}

bool
SPM::PMMUSideSlavePort::recvTimingReq(PacketPtr pkt)
{
    // this is called by the pmmu when SPM needs to fill a remote request or alloc/dealloc
    my_spm->recvPMMUTimingReq(pkt);
    return true;
}

void
SPM::PMMUSideSlavePort::recvFunctional(PacketPtr pkt)
{
    //@TODO:  implement - see rubyport.cc version
    panic("recvFunctional not implemented for PMMUSideSlavePort");
}

void
SPM::PMMUSideSlavePort::setBlocked()
{
    //@TODO:  implement
    panic("setBlocked not implemented for PMMUSideSlavePort");
}

void
SPM::PMMUSideSlavePort::clearBlocked()
{
    //@TODO:  implement
    panic("clearBlocked not implemented for PMMUSideSlavePort");
}

/////////////////////
//
// PMMUSideMasterPort
//
/////////////////////

BaseMasterPort &
SPM::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "pmmu_m_side") {
        return *pmmuMasterPort;
    } else {
        return BaseSPM::getMasterPort(if_name, idx);
    }
}

SPM::PMMUSideMasterPort::PMMUSideMasterPort(const std::string &_name, SPM *_spm, PortID id)
    : QueuedMasterPort(_name, _spm, _reqQueue, _snoopRespQueue, id),
      _reqQueue(*_spm, *this),
      _snoopRespQueue(*_spm, *this, ""),
      my_spm(_spm)
{
    DPRINTF(SPM, "Created master pmmuport on spm %s\n", _name);
}

bool
SPM::PMMUSideMasterPort::recvTimingResp(PacketPtr pkt)
{
    // this is where SPM gets a reply from PMMU
    // regarding a request by its local CPU
    my_spm->recvPMMUTimingResp(pkt);
    return true;
}
