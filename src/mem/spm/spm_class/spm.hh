/*
 * Copyright (c) 2012-2014 ARM Limited
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
 *          Steve Reinhardt
 *          Ron Dreslinski
 *          Andreas Hansson
 */

/**
 * @file
 * Describes a spm based on template policies.
 */

#ifndef __MEM_SPM_SPM_HH__
#define __MEM_SPM_SPM_HH__

#include "base/logging.hh" // fatal, panic, and warn
#include "mem/spm/spm_class/base.hh"
#include "mem/spm/spm_class/SPMPage.hh"
#include "params/SPM.hh"
#include "sim/eventq.hh"

#define MAX_PENDING_REQS 5

//Forward decleration
class PMMU;

/**
 * A template-policy based spm. The behavior of the spm can be altered by
 * supplying different template policies. TagStore handles all tag and data
 * storage @sa TagStore, \ref gem5MemorySystem "gem5 Memory System"
 */
class SPM : public BaseSPM
{
  friend class PMMU;

  protected:

    /**
     * The CPU-side port extends the base spm slave port with access
     * functions for functional, atomic and timing requests.
     */
    class CpuSidePort : public SPMSlavePort
    {
      private:

        // a pointer to our specific spm implementation
        SPM *spm;

      protected:

        virtual bool recvTimingSnoopResp(PacketPtr pkt);

        virtual bool recvTimingReq(PacketPtr pkt);

        virtual Tick recvAtomic(PacketPtr pkt);

        virtual void recvFunctional(PacketPtr pkt);

        virtual AddrRangeList getAddrRanges() const;

      public:

        CpuSidePort(const std::string &_name, SPM *_spm,
                    const std::string &_label);

    };

    /**
     * Override the default behaviour of sendDeferredPacket to enable
     * the memory-side spm port to also send requests based on the
     * current MSHR status. This queue has a pointer to our specific
     * spm implementation and is used by the MemSidePort.
     */
    class SPMReqPacketQueue : public ReqPacketQueue
    {

      protected:

        SPM &spm;
        SnoopRespPacketQueue &snoopRespQueue;

      public:

        SPMReqPacketQueue(SPM &spm, MasterPort &port,
                            SnoopRespPacketQueue &snoop_resp_queue,
                            const std::string &label) :
            ReqPacketQueue(spm, port, label), spm(spm),
            snoopRespQueue(snoop_resp_queue) { }

        /**
         * Override the normal sendDeferredPacket and do not only
         * consider the transmit list (used for responses), but also
         * requests.
         */
//        virtual void sendDeferredPacket();

    };

    /**
     * The memory-side port extends the base spm master port with
     * access functions for functional, atomic and timing snoops.
     */
    class MemSidePort : public SPMMasterPort
    {
      private:

        /** The spm-specific queue. */
        SPMReqPacketQueue _reqQueue;

        SnoopRespPacketQueue _snoopRespQueue;

        // a pointer to our specific spm implementation
        SPM *spm;

      protected:

        virtual void recvTimingSnoopReq(PacketPtr pkt);

        virtual bool recvTimingResp(PacketPtr pkt);

        virtual Tick recvAtomicSnoop(PacketPtr pkt);

        virtual void recvFunctionalSnoop(PacketPtr pkt);

      public:

        MemSidePort(const std::string &_name, SPM *_spm,
                    const std::string &_label);
    };

    /**
     * The pmmu-side ports
     */
    class PMMUSideSlavePort : public QueuedSlavePort
    {
      private:

        RespPacketQueue _respQueue;
        SPM *my_spm;

      public:
        PMMUSideSlavePort(const std::string &_name, SPM *_spm, PortID id);
        bool recvTimingReq(PacketPtr pkt);
        bool sendTimingResp(PacketPtr pkt);

        /** Do not accept any new requests. */
        void setBlocked();

        /** Return to normal operation and accept new requests. */
        void clearBlocked();

      protected:

        Tick recvAtomic(PacketPtr pkt)
        { panic("SPM::PMMUSideSlavePort::recvAtomic() not implemented!\n"); }

        void recvFunctional(PacketPtr pkt);

        AddrRangeList getAddrRanges() const
        { AddrRangeList ranges; return ranges; }

      private:
        bool isPhysMemAddress(Addr addr) const;
    };

    class PMMUSideMasterPort : public QueuedMasterPort
    {
      private:

        ReqPacketQueue _reqQueue;
        SnoopRespPacketQueue _snoopRespQueue;
        SPM *my_spm;

      public:
        PMMUSideMasterPort(const std::string &_name, SPM *_spm, PortID id);
        //bool sendTimingReq(PacketPtr pkt);
        bool recvTimingResp(PacketPtr pkt);

      protected:

        Tick recvAtomic(PacketPtr pkt)
        { panic("SPM::PMMUSideMasterPort::recvAtomic() not implemented!\n"); }

        void recvFunctional(PacketPtr pkt);

        AddrRangeList getAddrRanges() const
        { AddrRangeList ranges; return ranges; }

      private:
        bool isPhysMemAddress(Addr addr) const;
    };

  protected:
    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @return The result of the access.
     */
    bool recvTimingReq(PacketPtr pkt);

    /**
     * Insert writebacks into the write buffer
     */
    void doWritebacks(PacketList& writebacks, Tick forward_time);

    /**
     * Send writebacks down the memory hierarchy in atomic mode
     */
    void doWritebacksAtomic(PacketList& writebacks);

    /**
     * Handles a response (spm line fill/write ack) from the bus.
     * @param pkt The response packet
     */
    void recvTimingResp(PacketPtr pkt);

    /**
     * Snoops bus transactions to maintain coherence.
     * @param pkt The current bus transaction.
     */
    void recvTimingSnoopReq(PacketPtr pkt);

    /**
     * Handle a snoop response.
     * @param pkt Snoop response packet
     */
    void recvTimingSnoopResp(PacketPtr pkt);

    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @return The number of ticks required for the access.
     */
    Tick recvAtomic(PacketPtr pkt);

    /**
     * Snoop for the provided request in the spm and return the estimated
     * time taken.
     * @param pkt The memory request to snoop
     * @return The number of ticks required for the snoop.
     */
    Tick recvAtomicSnoop(PacketPtr pkt);

    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @param fromCpuSide from the CPU side port or the memory side port
     */
    void functionalAccess(PacketPtr pkt, bool fromCpuSide);

  public:
    /** Instantiates a basic spm object. */
    SPM(const SPMParams *p);
    /** Non-default destructor is needed to deallocate memory. */
    virtual ~SPM();

    PMMUSideSlavePort *pmmuSlavePort;       // port connected to local pmmu to respond to remote requests
    PMMUSideMasterPort *pmmuMasterPort;     // port connected to local pmmu to fill requests

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                        PortID idx = InvalidPortID);

    PMMU *myPMMU;            //PMMU connected to this SPM
    SPMPage *spmSlots;
    unsigned pageSizeBytes;  //Page size of this SPM

    double read_ber;
    double write_ber;
    std::string ber_energy_file;

    void init();
    void regStats();

    bool recvPMMUTimingReq(PacketPtr pkt);
    void recvPMMUTimingResp(PacketPtr pkt);
    void sendTimingRespToCPU(PacketPtr pkt, Cycles lat);
    bool satisfySPMAccess(PacketPtr pkt, Cycles *lat);

    void initializePageAllocation (PacketPtr pkt);
    void satisfyPageAllocation (PacketPtr pkt);

    void initializePageDeallocation (PacketPtr pkt);
    void satisfyPageDeallocation (PacketPtr pkt);

    /** considering total pending (cpu + pmmu) reqs,
     * if blocked, unblock and if unblocked, block
     * We use them to prevent packet queue overflow on the memory side
     **/
    void conditionalBlocking (BlockedCause cause);
    void conditionalUnblocking(BlockedCause cause);

    bool acceptingMemReqs();
    int pendingReqs;

    void writebackCacheCopies(Addr p_page_addr);
    unsigned getCacheBlkSize();

    unsigned int pAddress2PageIndex(Addr addr) const
    {
        return (addr / pageSizeBytes);
    }
};
#endif // __MEM_SPM_SPM_HH__
