#ifndef __PMMU_HH__
#define __PMMU_HH__

#include <iostream>
#include <sstream>
#include <string>

#include "mem/protocol/Types.hh"
#include "mem/ruby/slicc_interface/AbstractController.hh"
#include "mem/spm/spm_class/spm.hh"
#include "params/PMMU.hh"

class SPMRequestMsg;
class SPMResponseMsg;
class ATT;
class BaseGovernor;
class GOVRequest;
class HostInfo;
class Annotations;

class PMMU : public AbstractController
{
    friend class BaseGovernor;

  public:

    class SPMSideSlavePort : public QueuedSlavePort
    {
      private:

      RespPacketQueue _respQueue;
      PMMU *my_pmmu;

      public:
        SPMSideSlavePort(const std::string &_name, PMMU *_pmmu, PortID id);
        bool recvTimingReq(PacketPtr pkt);

      protected:

        Tick recvAtomic(PacketPtr pkt)
        { panic("PMMU::SPMSideSlavePort::recvAtomic() not implemented!\n"); }

        void recvFunctional(PacketPtr pkt);

        AddrRangeList getAddrRanges() const
        { AddrRangeList ranges; return ranges; }

      private:
        bool isPhysMemAddress(Addr addr) const;
    };

    class SPMSideMasterPort : public QueuedMasterPort
    {
      private:

        ReqPacketQueue _reqQueue;
        SnoopRespPacketQueue _snoopRespQueue;
        PMMU *my_pmmu;

      public:
        SPMSideMasterPort(const std::string &_name, PMMU *_pmmu, PortID id);
        bool recvTimingResp(PacketPtr pkt);

      protected:

        Tick recvAtomic(PacketPtr pkt)
        { panic("PMMU::SPMSideMasterPort::recvAtomic() not implemented!\n"); }

        void recvFunctional(PacketPtr pkt);

        AddrRangeList getAddrRanges() const
        { AddrRangeList ranges; return ranges; }

      private:
        bool isPhysMemAddress(Addr addr) const;
    };

    typedef PMMUParams Params;
    PMMU(const Params *p);
    void init();
    void initNetQueues();

    SPM *my_spm_ptr;

    SPMSideSlavePort *spmSlavePort;     // port connected to local spm to respond to local requests
    SPMSideMasterPort *spmMasterPort;   // port connected to local spm to fulfill remote and alloc/dealloc/reloc requests

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                        PortID idx = InvalidPortID);

    void setSPM(SPM *_spm);

    BaseGovernor *getGovernor() { return my_governor_ptr; }

    // aligns the address to virtual page boundaries
    static Addr spmPageAlign(Addr v_addr)     { return spmPageAlignDown(v_addr); }
    static Addr spmPageAlignDown(Addr v_addr) { return (v_addr & ~(Addr(m_page_size_bytes - 1))); }
    static Addr spmPageAlignUP(Addr v_addr)   { return (v_addr + (m_page_size_bytes - 1)) &
                                                       ~(Addr(m_page_size_bytes - 1)); }

    static int getPageSizeBytes() { return m_page_size_bytes; }
    int getSPMSizePages() { return my_spm_ptr->getSize()/m_page_size_bytes; }

    NodeID getNodeID() const {return m_machineID.num;};

    /* alloc/dealloc/relocation request helper functions */
    int addATTMappingsVAddress(GOVRequest *gov_request, HostInfo *host_info);

    int removeATTMappingsVAddress(GOVRequest *gov_request, HostInfo *host_info);

    int removeATTMappingsSPMAddress(GOVRequest *gov_request, HostInfo *host_info);

    void changeATTMappingVAddress(GOVRequest *gov_request,
                                  HostInfo *current_host_info,
                                  HostInfo *future_host_info);
    
    void changeATTMappingSPMAddress(HostInfo *current_host_info,
                                    HostInfo *future_host_info,
                                    GOVRequest *gov_request = nullptr);

    void setUsedPages(Addr start_p_spm_addr, int num_pages,
                      NodeID owner);
    void setFreePages(Addr start_p_spm_addr, int num_pages);

    void wakeup();

  private:

    Cycles m_request_latency;
    //MessageBuffer* m_mandatoryQueue_ptr;
    MessageBuffer* m_responseFromSPM_ptr;
    MessageBuffer* m_responseToSPM_ptr;
    MessageBuffer* m_requestFromSPM_ptr;
    MessageBuffer* m_requestToSPM_ptr;
    MessageBuffer* m_responseToNetwork_ptr;
    MessageBuffer* m_requestToNetwork_ptr;

    static int m_page_size_bytes;

    BaseGovernor *my_governor_ptr;
    uint32_t pending_gov_reqs;

  public:
    ATT *my_att;

  private:

    void addToGovernor(std::string gov_type);

    bool recvSPMTimingReq(PacketPtr pkt);
    bool recvSPMTimingResp(PacketPtr pkt);

    void triggerPageAlloc(GOVRequest *gov_request,
                          HostInfo *host_info,
                          int page_index);

    void triggerPageDeAlloc(GOVRequest *gov_request,
                            HostInfo *host_info,
                            int page_index);

    void triggerPageRelocation(HostInfo *current_host_info,
                               HostInfo *future_host_info,
                               int page_index,
                               Annotations *att_annotation_ptr);

    void continuePageRelocationRequest(PacketPtr relocation_pkt);
    void finalizeLocalGOVReq(PacketPtr gov_pkt);

    // packet to message routines
    bool generateAccessReqMsg(PacketPtr pkt);
    bool generateAccessRespMsg(PacketPtr pkt);
    bool generateGovRespMsg(PacketPtr pkt);

    // message processing helpers
    bool processRemoteResponseMsg();
    bool processRemoteRequestMsg(bool *sending_mem_reqs_allowed);
    bool processLocalResponseMsg();
    bool processLocalRequestMsg(bool *sending_mem_reqs_allowed);

    // relocation handling
    bool sendRelocationDone(PacketPtr pkt);
    void fireupPendingAllocs(PacketPtr pkt);

    const SPMRequestMsg* getNextReadyRequestMsg(MessageBuffer *mb) const;

  public:

    void print(std::ostream& out) const;
    void resetStats();
    void regStats();
    void collateStats();

    static int getNumControllers();

    MessageBuffer *getMandatoryQueue() const;
    MessageBuffer *getMemoryQueue() const;

    void recordCacheTrace(int cntrl, CacheRecorder* tr);
    Sequencer* getCPUSequencer() const;
    GPUCoalescer* getGPUCoalescer() const;
    int functionalWriteBuffers(PacketPtr&);

  private:

    static int m_num_controllers;

    AccessPermission getAccessPermission(const Addr& param_addr);
    void functionalRead(const Addr& param_addr, Packet* param_pkt);
    int functionalWrite(const Addr& param_addr, Packet* param_pkt);
};
#endif // __PMMU_HH__
