#ifndef __GOV_REQUEST_HH__
#define __GOV_REQUEST_HH__

#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "mem/page_table.hh"
#include "mem/spm/api/spm_types.h"
#include "mem/spm/governor/GOVPktInfo.hh"
#include "mem/spm/pmmu.hh"
#include "sim/process.hh"

#define CEIL_DIV(X,Y) (X%Y)?(X/Y)+1:(X/Y)
#define FLOOR_DIV(X,Y) (X/Y)

class HostInfo
{
  private:
    ThreadContext *user_tc;
    PMMU *user_pmmu;
    PMMU *host_pmmu;
    MachineID host_m_id;
    bool valid_host_id;
    Addr start_p_spm_addr;
    int num_pages;

  public:
    AllocationModes alloc_mode;
    DeallocationModes dealloc_mode;

    GOVReqSignaling signaling;

  public:
    HostInfo(ThreadContext *_user_tc, PMMU *_user_pmmu, PMMU *_host_pmmu,
             Addr _start_p_spm_addr, int _num_pages)
    {
        user_tc = _user_tc;
        user_pmmu = _user_pmmu;
        host_pmmu = _host_pmmu;
        start_p_spm_addr = _start_p_spm_addr;
        num_pages = _num_pages;

        alloc_mode = COPY;
        signaling.wait_for_signal = false;
        signaling.signaler = 0;

        dealloc_mode = WRITE_BACK;
        signaling.needs_to_signal = false;
        signaling.signalee = 0;

        if (_host_pmmu) {
            valid_host_id = true;
            host_m_id = _host_pmmu->getMachineID();
        }
        else {
            valid_host_id = false;
        }
    }

    void setUserThreadContext(ThreadContext *_user_tc) {
        user_tc = _user_tc;
    }

    ThreadContext *getUserThreadContext() {
        return user_tc;
    }

    void setUserPMMU(PMMU * _user_pmmu) {
        user_pmmu = _user_pmmu;
    }

    PMMU *getUserPMMU() {
        return user_pmmu;
    }

    void setgetUserPMMU(PMMU * _user_pmmu) {
        user_pmmu = _user_pmmu;
    }

    PMMU *getHostPMMU() {
        return host_pmmu;
    }

    void setHostMachineID(MachineID _host_m_id) {
        valid_host_id = true;
        host_m_id = _host_m_id;
    }

    MachineID getHostMachineID() {
        assert(valid_host_id);
        return host_m_id;
    }

    void setHostPMMU(PMMU * _host_pmmu) {
        host_pmmu = _host_pmmu;
        if (_host_pmmu) {
            valid_host_id = true;
            host_m_id = _host_pmmu->getMachineID();
        }
        else {
            valid_host_id = false;
        }
    }

    Addr getSPMaddress() {
        return start_p_spm_addr;
    }

    void setSPMaddress(Addr _start_p_spm_addr) {
        start_p_spm_addr = _start_p_spm_addr;
    }

    int getNumPages() {
        return num_pages;
    }

    void setNumPages(int _num_pages) {
        num_pages = _num_pages;
    }

    void setAllocMode(AllocationModes _alloc_mode) {
        alloc_mode = _alloc_mode;
    }

    void setDeallocMode(DeallocationModes _dealloc_mode) {
        dealloc_mode = _dealloc_mode;
    }

    void setSignaler(NodeID _signaler) {
        signaling.signaler = _signaler;
        signaling.wait_for_signal = true;
    }

    void setSignalee(NodeID _signalee) {
        signaling.signalee = _signalee;
        signaling.needs_to_signal = true;
    }

    void setSignaling(GOVReqSignaling *_signaling) {
        signaling = *_signaling;
    }

    GOVReqSignaling *getSignaling() {
        return &signaling;
    }
};

typedef struct RequesterInfo
{
    ThreadContext *tc;
    BaseCPU *cpu;
    SPM *spm;
    PMMU *pmmu;
    FuncPageTable *pt;
} RequesterInfo;

typedef struct Annotations
{
    Approximation approximation;
    bool shared_data;
    AllocationModes alloc_mode;
    DeallocationModes dealloc_mode;
    DataImportance data_importance;
    ThreadPriority app_priority;
    uint32_t spm_addr;

    Annotations()
    {
        approximation = CRITICAL;
        shared_data = false;
        alloc_mode = COPY;
        dealloc_mode = WRITE_BACK;
        data_importance = MAX_IMPORTANCE;
        app_priority = HIGH_PRIORITY;
        spm_addr = 0;
    }

} Annotations;

static double pow_10[16] = {1, 1E-1, 1E-2, 1E-3, 1E-4, 1E-5, 1E-6, 1E-7, 1E-8, 1E-9, 1E-10, 1E-11, 1E-12, 1E-13, 1E-14, 1E-15};
inline double readBERfromAnnotations(Approximation approximation)
{
    return pow_10[(approximation >> 4) & 0x000000000000000F];
}

inline double writeBERfromAnnotations(Approximation approximation)
{
    return pow_10[(approximation)      & 0x000000000000000F];
}
/*
typedef struct AddressRange
{
    Addr start_addr;
    Addr end_addr;

    AddressRange() : AddressRange(0 , 0)
    {
    }

    AddressRange(Addr _start_addr, Addr _end_addr)
    {
        start_addr = _start_addr;
        end_addr = _end_addr;
    }
} AddressRange;
*/
enum VARangeMode
{
    Original,
    Original_Aligned,
    Unserved_Aligned
};

enum GOVCommand
{
    Allocation,
    Deallocation,
    Relocation,
    Unknown
};

inline const char* GOVCommandToString(GOVCommand v)
{
    switch (v)
    {
        case Allocation:   return "Allocation";
        case Relocation:   return "Relocation";
        case Deallocation: return "Deallocation";
        default:           return "[Unknown]";
    }
}

class GOVRequest
{
  public:

    ThreadContext *tc;
    GOVCommand cmd;
    AddrRange address_range;
    uint64_t metadata;
    Annotations *annotations;
    int pages_served;

    GOVRequest(ThreadContext *_tc, GOVCommand _cmd,
               Addr _start_addr, Addr _end_addr, uint64_t _metadata) :
        address_range(_start_addr,_end_addr)
    {
        tc = _tc;
        cmd = _cmd;

        metadata = _metadata;

        annotations = new Annotations();
        decodeMetadata();

        pages_served = 0;
    }

    ~GOVRequest()
    {
        delete annotations;
    }

    void decodeMetadata()
    {
        if (cmd == Allocation)
        {
            annotations->approximation   = static_cast<Approximation>((metadata >> 0)  & 0x00000000000000FF);
            annotations->shared_data     = ((metadata >> 8) & 0x000000000000000F) != 0;
            annotations->alloc_mode      = static_cast<AllocationModes>((metadata >> 12)  & 0x000000000000000F);
            annotations->data_importance = static_cast<DataImportance>((metadata >> 16) & 0x000000000000000F);
            annotations->app_priority    = static_cast<ThreadPriority>((metadata >> 20) & 0x000000000000000F);
            annotations->spm_addr        = static_cast<uint32_t>((metadata >> 32) & 0x00000000FFFFFFFF);
        }

        else if (cmd == Deallocation)
        {
            annotations->dealloc_mode    = static_cast<DeallocationModes>((metadata >> 0) & 0x000000000000000F);
        }
    }

    void incPagesServed(int num_pages)
    {
        pages_served += num_pages;
    }

    GOVCommand getCommand()
    {
        return cmd;
    }

    int getNumPagesServed()
    {
        return pages_served;
    }

    ThreadContext *getThreadContext()
    {
        assert(tc);
        return tc;
    }

    BaseCPU *getCPUPtr()
    {
        return tc->getCpuPtr();
    }

    SPM *getSPMPtr()
    {
        BaseSPM::SPMSlavePort& spm_port = dynamic_cast<BaseSPM::SPMSlavePort&>(getCPUPtr()->getMasterPort("dcache_port", 0).getSlavePort());
        return dynamic_cast<SPM*>(spm_port.getOwner());
    }

    PMMU *getPMMUPtr()
    {
        return getSPMPtr()->myPMMU;
    }

    FuncPageTable *getPageTablePtr()
    {
        return dynamic_cast<FuncPageTable*>(tc->getProcessPtr()->pTable);
    }

    RequesterInfo getRequesterInfo()
    {
        RequesterInfo requester_info;

        requester_info.tc = tc;
        requester_info.cpu = getCPUPtr();
        requester_info.spm = getSPMPtr();
        requester_info.pmmu = getPMMUPtr();
        requester_info.pt = getPageTablePtr();

        return requester_info;
    }

    NodeID getRequesterNodeID()
    {
        return getPMMUPtr()->getNodeID();
    }

    int getPageSizeBytes()
    {
        return getPMMUPtr()->getPageSizeBytes();
    }

    // TODO: page alignment needs to be fixed (inclusive or exclusive?)
    Addr getStartAddr(VARangeMode mode)
    {
        switch (mode) {
            case Original:
                return address_range.start();
                break;
            case Original_Aligned:
                return getPMMUPtr()->spmPageAlignUP(address_range.start());
                break;
            case Unserved_Aligned:
                return getPMMUPtr()->spmPageAlignUP(address_range.start() + pages_served * getPageSizeBytes());
                break;
            default:
                panic("Unknown VARageMode");
        }
    }

    Addr getEndAddr(VARangeMode mode)
    {
        switch (mode) {
            case Original:
                return address_range.end();
                break;
            case Original_Aligned:
                return getPMMUPtr()->spmPageAlignDown(address_range.end());
                break;
            case Unserved_Aligned:
                return getPMMUPtr()->spmPageAlignDown(address_range.end());
                break;
            default:
                panic("Unknown VARageMode");
        }
    }

    int getNumberOfPages(VARangeMode mode)
    {
        const int page_size = getPageSizeBytes();
        Addr start_addr = getStartAddr(mode);
        Addr end_addr = getEndAddr(mode);
        const Addr addr_range = end_addr - start_addr;
        const int total_num_pages = FLOOR_DIV(addr_range, page_size);
        return total_num_pages;
    }

    AddrRange getVAddressRange(VARangeMode mode)
    {
        AddrRange address_range (getStartAddr(mode), getEndAddr(mode));
        return address_range;
    }

    void setAnnotations(Annotations *_annotations)
    {
        assert(_annotations);
        annotations = _annotations;
    }

    Annotations *getAnnotations()
    {
        assert(cmd != Unknown);
        assert(annotations);
        return annotations;
    }

    Addr getNthPageStartAddr(VARangeMode mode, int n)
    {
        assert (n <= getNumberOfPages(mode));
        return getStartAddr(mode) + n * getPageSizeBytes();
    }
};

#endif // __GOV_REQUEST_HH__
