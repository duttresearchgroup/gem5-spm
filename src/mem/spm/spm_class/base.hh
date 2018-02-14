/*
 * Copyright (c) 2012-2013, 2015 ARM Limited
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
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 *          Steve Reinhardt
 *          Ron Dreslinski
 */

/**
 * @file
 * Declares a basic spm interface BaseSPM.
 */

#ifndef __MEM_SPM_BASE_HH__
#define __MEM_SPM_BASE_HH__

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/SPM.hh"
#include "debug/SPMPort.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"
#include "mem/request.hh"
#include "params/BaseSPM.hh"
#include "sim/eventq.hh"
#include "sim/full_system.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

/**
 * A basic spm interface. Implements some common functions for speed.
 */
class BaseSPM : public MemObject
{

  public:
    /**
     * Reasons for spms to be blocked.
     */
    enum BlockedCause {
        Blocked_NoMSHRs,
        Blocked_NoWBBuffers,
        Blocked_NoTargets,
        Blocked_Alloc_DeAlloc_Relocate,
        Blocked_MaxPendingReqs,
        NUM_BLOCKED_CAUSES
    };
  public:

    /**
     * A spm master port is used for the memory-side port of the
     * spm, and in addition to the basic timing port that only sends
     * response packets through a transmit list, it also offers the
     * ability to schedule and send request packets (requests &
     * writebacks). The send event is scheduled through schedSendEvent,
     * and the sendDeferredPacket of the timing port is modified to
     * consider both the transmit list and the requests from the MSHR.
     */
    class SPMMasterPort : public QueuedMasterPort
    {

      public:

        /**
         * Schedule a send of a request packet (from the MSHR). Note
         * that we could already have a retry outstanding.
         */
        void schedSendEvent(Tick time)
        {
            DPRINTF(SPMPort, "Scheduling send event at %llu\n", time);
            reqQueue.schedSendEvent(time);
        }

        MemObject* getOwner() {
        	return &owner;
        }

      protected:

        SPMMasterPort(const std::string &_name, BaseSPM *_spm,
                        ReqPacketQueue &_reqQueue,
                        SnoopRespPacketQueue &_snoopRespQueue) :
            QueuedMasterPort(_name, _spm, _reqQueue, _snoopRespQueue)
        { }

        /**
         * SPMs never require snooping.
         *
         * @return always true
         */
        virtual bool isSnooping() const { return false; }
    };

    /**
     * A spm slave port is used for the CPU-side port of the spm,
     * and it is basically a simple timing port that uses a transmit
     * list for responses to the CPU (or connected master). In
     * addition, it has the functionality to block the port for
     * incoming requests. If blocked, the port will issue a retry once
     * unblocked.
     */
    class SPMSlavePort : public QueuedSlavePort
    {

      public:

        /** Do not accept any new requests. */
        void setBlocked();

        /** Return to normal operation and accept new requests. */
        void clearBlocked();

        bool isBlocked() const { return blocked; }

        MemObject* getOwner() {
        	return &owner;
        }

      protected:

        SPMSlavePort(const std::string &_name, BaseSPM *_spm,
                       const std::string &_label);

        /** A normal packet queue used to store responses. */
        RespPacketQueue queue;

        bool blocked;

        bool mustSendRetry;

      private:

        void processSendRetry();

        EventWrapper<SPMSlavePort,
                     &SPMSlavePort::processSendRetry> sendRetryEvent;

    };

    SPMSlavePort *cpuSidePort;
    SPMMasterPort *memSidePort;

  protected:

    /**
     * Determine if an address is in the ranges covered by this
     * spm. This is useful to filter snoops.
     *
     * @param addr Address to check against
     *
     * @return If the address in question is in range
     */
    bool inRange(Addr addr) const;

    /** Block size of this spm */
    const unsigned blkSize;

    /** The size of the spm. */
    const unsigned size;


    /**
     * The latency of tag lookup of a spm. It occurs when there is
     * an access to the spm.
     */
    const Cycles lookupLatency;

    /**
     * This is the forward latency of the spm. It occurs when there
     * is a spm miss and a request is forwarded downstream, in
     * particular an outbound miss.
     */
    const Cycles forwardLatency;

    /** The latency to fill a spm block */
    const Cycles fillLatency;

    /**
     * The latency of sending reponse to its upper level spm/core on
     * a linefill. The responseLatency parameter captures this
     * latency.
     */
    const Cycles responseLatency;

    /** The number of targets for each MSHR. */
    const int numTarget;

    /** Do we forward snoops from mem side port through to cpu side port? */
    const bool forwardSnoops;

    /**
     * Is this spm read only, for example the instruction spm, or
     * table-walker spm. A spm that is read only should never see
     * any writes, and should never get any dirty data (and hence
     * never have to do any writebacks).
     */
    const bool isReadOnly;

    /**
     * Bit vector of the blocking reasons for the access path.
     * @sa #BlockedCause
     */
    uint8_t blocked;

    /** Increasing order number assigned to each incoming request. */
    uint64_t order;

    /** Stores time the spm blocked for statistics. */
    Cycles blockedCycle;

    /** The number of misses to trigger an exit event. */
    Counter missCount;

    /**
     * The address range to which the spm responds on the CPU side.
     * Normally this is all possible memory addresses. */
    const AddrRangeList addrRanges;

  public:
    /** System we are currently operating in. */
    System *system;

    // Statistics
    /**
     * @addtogroup SPMStatistics
     * @{
     */
    /** Access energy for each type of command. @sa Packet::Command */
    Stats::Vector dynamicEnergy[MemCmd::NUM_MEM_CMDS];
    /** Total enregy consumed in this SPM. */
    Stats::Formula totalDynamicEnergy;

    /** Number of local hits per thread for each type of command. @sa Packet::Command */
    Stats::Vector localHits[MemCmd::NUM_MEM_CMDS];
    /** Number of remote hits per thread for each type of command. @sa Packet::Command */
    Stats::Vector remoteHits[MemCmd::NUM_MEM_CMDS];
    /** Number of hits for demand accesses. */
    Stats::Formula demandHits;
    /** Number of hit for all accesses. */
    Stats::Formula overallHits;

    /** Number of misses per thread for each type of command. @sa Packet::Command */
    Stats::Vector misses[MemCmd::NUM_MEM_CMDS];
    /** Number of misses for demand accesses. */
    Stats::Formula demandMisses;
    /** Number of misses for all accesses. */
    Stats::Formula overallMisses;

    /**
     * Total number of cycles per thread/command spent waiting for a miss.
     * Used to calculate the average miss latency.
     */
    Stats::Vector missLatency[MemCmd::NUM_MEM_CMDS];
    /** Total number of cycles spent waiting for demand misses. */
    Stats::Formula demandMissLatency;
    /** Total number of cycles spent waiting for all misses. */
    Stats::Formula overallMissLatency;

    /** The number of accesses per command and thread. */
    Stats::Formula accesses[MemCmd::NUM_MEM_CMDS];
    /** The number of demand accesses. */
    Stats::Formula demandAccesses;
    /** The number of overall accesses. */
    Stats::Formula overallAccesses;

    /** The miss rate per command and thread. */
    Stats::Formula missRate[MemCmd::NUM_MEM_CMDS];
    /** The miss rate of all demand accesses. */
    Stats::Formula demandMissRate;
    /** The miss rate for all accesses. */
    Stats::Formula overallMissRate;

    /** The average miss latency per command and thread. */
    Stats::Formula avgMissLatency[MemCmd::NUM_MEM_CMDS];
    /** The average miss latency for demand misses. */
    Stats::Formula demandAvgMissLatency;
    /** The average miss latency for all misses. */
    Stats::Formula overallAvgMissLatency;

    /** The total number of cycles blocked for each blocked cause. */
    Stats::Vector blocked_cycles;
    /** The number of times this spm blocked for each blocked cause. */
    Stats::Vector blocked_causes;

    /** The average number of cycles blocked for each blocked cause. */
    Stats::Formula avg_blocked;

    /** The number of fast writes (WH64) performed. */
    Stats::Scalar fastWrites;

    /** The number of spm copies performed. */
    Stats::Scalar cacheCopies;

    /** Number of blocks written back per thread. */
    Stats::Vector writebacks;

    /** Number of misses that hit in the MSHRs per command and thread. */
    Stats::Vector mshr_hits[MemCmd::NUM_MEM_CMDS];
    /** Demand misses that hit in the MSHRs. */
    Stats::Formula demandMshrHits;
    /** Total number of misses that hit in the MSHRs. */
    Stats::Formula overallMshrHits;

    /** Number of misses that miss in the MSHRs, per command and thread. */
    Stats::Vector mshr_misses[MemCmd::NUM_MEM_CMDS];
    /** Demand misses that miss in the MSHRs. */
    Stats::Formula demandMshrMisses;
    /** Total number of misses that miss in the MSHRs. */
    Stats::Formula overallMshrMisses;

    /** Number of misses that miss in the MSHRs, per command and thread. */
    Stats::Vector mshr_uncacheable[MemCmd::NUM_MEM_CMDS];
    /** Total number of misses that miss in the MSHRs. */
    Stats::Formula overallMshrUncacheable;

    /** Total cycle latency of each MSHR miss, per command and thread. */
    Stats::Vector mshr_miss_latency[MemCmd::NUM_MEM_CMDS];
    /** Total cycle latency of demand MSHR misses. */
    Stats::Formula demandMshrMissLatency;
    /** Total cycle latency of overall MSHR misses. */
    Stats::Formula overallMshrMissLatency;

    /** Total cycle latency of each MSHR miss, per command and thread. */
    Stats::Vector mshr_uncacheable_lat[MemCmd::NUM_MEM_CMDS];
    /** Total cycle latency of overall MSHR misses. */
    Stats::Formula overallMshrUncacheableLatency;

#if 0
    /** The total number of MSHR accesses per command and thread. */
    Stats::Formula mshrAccesses[MemCmd::NUM_MEM_CMDS];
    /** The total number of demand MSHR accesses. */
    Stats::Formula demandMshrAccesses;
    /** The total number of MSHR accesses. */
    Stats::Formula overallMshrAccesses;
#endif

    /** The miss rate in the MSHRs pre command and thread. */
    Stats::Formula mshrMissRate[MemCmd::NUM_MEM_CMDS];
    /** The demand miss rate in the MSHRs. */
    Stats::Formula demandMshrMissRate;
    /** The overall miss rate in the MSHRs. */
    Stats::Formula overallMshrMissRate;

    /** The average latency of an MSHR miss, per command and thread. */
    Stats::Formula avgMshrMissLatency[MemCmd::NUM_MEM_CMDS];
    /** The average latency of a demand MSHR miss. */
    Stats::Formula demandAvgMshrMissLatency;
    /** The average overall latency of an MSHR miss. */
    Stats::Formula overallAvgMshrMissLatency;

    /** The average latency of an MSHR miss, per command and thread. */
    Stats::Formula avgMshrUncacheableLatency[MemCmd::NUM_MEM_CMDS];
    /** The average overall latency of an MSHR miss. */
    Stats::Formula overallAvgMshrUncacheableLatency;

    /** The number of times a thread hit its MSHR cap. */
    Stats::Vector mshr_cap_events;
    /** The number of times software prefetches caused the MSHR to block. */
    Stats::Vector soft_prefetch_mshr_full;

    Stats::Scalar mshr_no_allocate_misses;

    /**
     * @}
     */

    /**
     * Register stats for this object.
     */
    virtual void regStats();

  public:
    BaseSPM(const BaseSPMParams *p, unsigned blk_size);
    ~BaseSPM() {}

    virtual void init();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                        PortID idx = InvalidPortID);

    /**
     * Query block size of a spm.
     * @return  The block size
     */
    unsigned
    getBlockSize() const
    {
        return blkSize;
    }

    unsigned
    getSize() const
    {
        return size;
    }


    Addr blockAlign(Addr addr) const { return (addr & ~(Addr(blkSize - 1))); }


    const AddrRangeList &getAddrRanges() const { return addrRanges; }

    /**
     * Returns true if the spm is blocked for accesses.
     */
    bool isBlocked() const
    {
        return blocked != 0;
    }

    /**
     * Marks the access path of the spm as blocked for the given cause. This
     * also sets the blocked flag in the slave interface.
     * @param cause The reason for the spm blocking.
     */
    void setBlocked(BlockedCause cause)
    {
        uint8_t flag = 1 << cause;
        if (blocked == 0) {
            blocked_causes[cause]++;
            blockedCycle = curCycle();
            cpuSidePort->setBlocked();
        }
        blocked |= flag;
        DPRINTF(SPM,"Blocking for cause %d, mask=%d\n", cause, blocked);
    }

    /**
     * Marks the spm as unblocked for the given cause. This also clears the
     * blocked flags in the appropriate interfaces.
     * @param cause The newly unblocked cause.
     * @warning Calling this function can cause a blocked request on the bus to
     * access the spm. The spm must be in a state to handle that request.
     */
    void clearBlocked(BlockedCause cause)
    {
        uint8_t flag = 1 << cause;
        blocked &= ~flag;
        DPRINTF(SPM,"Unblocking for cause %d, mask=%d\n", cause, blocked);
        if (blocked == 0) {
            blocked_cycles[cause] += curCycle() - blockedCycle;
            cpuSidePort->clearBlocked();
        }
    }

    /**
     * Schedule a send event for the memory-side port. If already
     * scheduled, this may reschedule the event at an earlier
     * time. When the specified time is reached, the port is free to
     * send either a response, a request, or a prefetch request.
     *
     * @param time The time when to attempt sending a packet.
     */
    void schedMemSideSendEvent(Tick time)
    {
        memSidePort->schedSendEvent(time);
    }

    void incMissCount(PacketPtr pkt)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        misses[pkt->cmdToIndex()][pkt->req->masterId()]++;
        pkt->req->incAccessDepth();
        if (missCount) {
            --missCount;
            if (missCount == 0)
                exitSimLoop("A spm reached the maximum miss count");
        }
    }

    void incLocalHitCount(PacketPtr pkt)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        localHits[pkt->cmdToIndex()][pkt->req->masterId()]++;

    }

    void incRemoteHitCount(PacketPtr pkt)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        remoteHits[pkt->cmdToIndex()][pkt->req->masterId()]++;

    }

    void incDynamicEnergy(PacketPtr pkt, float per_byte_energy)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        dynamicEnergy[pkt->cmdToIndex()][pkt->req->masterId()] += per_byte_energy;

    }

};

#endif //__MEM_SPM_BASE_HH__
