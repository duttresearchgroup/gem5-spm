#ifndef __GOV_REQ_INFO_HH__
#define __GOV_REQ_INFO_HH__

#include <limits>
#include "mem/ruby/common/TypeDefines.hh"

#define UINT_MAXIMUM std::numeric_limits<unsigned int>::max()

class Annotations;

typedef struct GOVReqSignaling
{
    // stall core while this request is being processed
    bool stall_core;

    // for allocations
    // These are used to wait for a signal from a node before
    // this request can proceed
    bool wait_for_signal;
    NodeID signaler;

    // for deallocations
    // these are used to signal a node that is waiting for us
    bool needs_to_signal;
    NodeID signalee;

    GOVReqSignaling() :
    stall_core(true),
    wait_for_signal(false),
    signaler(UINT_MAXIMUM),
    needs_to_signal(false),
    signalee(UINT_MAXIMUM) {}

} GOVReqSignaling;

class GOVPktInfo
{
  private:

    enum ReqType {
        Allocation = 0,
        Deallocation,
        RelocationRead,
        RelocationWrite,
        NUM_REQ_TYPES
    };

    enum ReqStatus {
        Complete = 0,
        Incomplete,
        NUM_REQ_STATUS
    };

    bool validInfo;

    Addr spm_p_addr;
    Addr mem_p_addr;
    Addr mem_v_addr;
    ReqType request_type;
    ReqStatus request_status;

    // Relocation-specific
    Addr future_spm_p_addr;
    NodeID future_host_node;

  private:
    GOVReqSignaling signaling;
    Annotations *annotations;

  public:

    GOVPktInfo() : validInfo (false),
                   request_type(NUM_REQ_TYPES),
                   request_status(NUM_REQ_STATUS),
                   future_spm_p_addr(0),
                   future_host_node(UINT_MAXIMUM),
                   annotations(nullptr)
    {
        setAddresses(0,0,0);
    }

    const char *toString()
    {
        static const char * ReqTypeStrings[] = { "Allocation",
                                                 "Deallocation",
                                                 "RelocationRead",
                                                 "RelocationWrite",
                                                 "Uninitialized" };
        return ReqTypeStrings[request_type];
    }

    void markComplete()   { request_status = Complete; }
    void markIncomplete() { request_status = Incomplete; }

    bool isComplete() { return (request_status == Complete); }

    void makeAllocate()        { request_type = Allocation; }
    void makeDeallocate()      { request_type = Deallocation; }
    void makeRelocationRead()  { request_type = RelocationRead; }
    void makeRelocationWrite() { request_type = RelocationWrite; }

    bool isDeallocate()      { return request_type == Deallocation; }
    bool isAllocate()        { return request_type == Allocation; }
    bool isRelocationRead()  { return request_type == RelocationRead; }
    bool isRelocationWrite() { return request_type == RelocationWrite; }

    bool isGOVReq()
    {
        return (isAllocate()         ||
                isDeallocate()       ||
                isRelocationRead()   ||
                isRelocationWrite());
    }

    void setStallStatus(bool _stall_core) { signaling.stall_core = _stall_core;}
    bool shouldStall()                    { return signaling.stall_core;}

    bool hasSignalee()                 { return signaling.needs_to_signal; }
    void shouldSignal()                { signaling.needs_to_signal = true; }
    void shouldNotSignal()             { signaling.needs_to_signal = false; }
    void setSignalee(NodeID _signalee) { signaling.signalee = _signalee;}
    NodeID getSignalee()               { return signaling.signalee;}

    bool hasSignaler()                 { return signaling.wait_for_signal; }
    void shouldWait()                  { signaling.wait_for_signal = true; }
    void shouldNotWait()               { signaling.wait_for_signal = false; }
    void setSignaler(NodeID _signaler) { signaling.signaler = _signaler;}
    NodeID getSignaler()               { return signaling.signaler;}

    void setAddresses(Addr _spm_p_addr, Addr _mem_p_addr, Addr _mem_v_addr)
    {
        spm_p_addr = _spm_p_addr;
        mem_p_addr = _mem_p_addr;
        mem_v_addr = _mem_v_addr;

        validInfo =  true;
    }

    void setSPMAddress(Addr _spm_p_addr)      { spm_p_addr = _spm_p_addr; }
    void setPhysicalAddress(Addr _mem_p_addr) { mem_p_addr = _mem_p_addr; }
    void setVirtualAddress(Addr _mem_v_addr)  { mem_v_addr = _mem_v_addr; }

    Addr getSPMAddress()      { return spm_p_addr; }
    Addr getPhysicalAddress() { return mem_p_addr; }
    Addr getVirtualAddress()  { return mem_v_addr; }

    void setFutureHost(NodeID _future_host_node)      {future_host_node = _future_host_node;}
    void setFutureSPMAddress(Addr _future_spm_p_addr) {future_spm_p_addr = _future_spm_p_addr;}

    NodeID getFutureHost()     { return future_host_node; }
    Addr getFutureSPMAddress() { return future_spm_p_addr; }

    void setAnnotations(Annotations *_annotations) { assert(_annotations); annotations = _annotations; }
    Annotations *getAnnotations() { assert(annotations); return annotations; }

    void setSignaling(GOVReqSignaling *_signaling) { signaling = *_signaling; }
    GOVReqSignaling *getSignaling() { return &signaling; }
};

#endif  /* __GOV_REQ_INFO_HH__ */
