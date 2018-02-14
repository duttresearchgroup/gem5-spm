#include <cassert>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <typeinfo>
#include <unistd.h>

#include "base/compiler.hh"
#include "base/cprintf.hh"
#include "debug/PMMU.hh"
#include "debug/ATT.hh"
#include "mem/page_table.hh"
#include "mem/protocol/Types.hh"
#include "mem/ruby/network/Network.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "mem/ruby/slicc_interface/RubySlicc_includes.hh"
#include "mem/spm/pmmu.hh"
#include "mem/spm/att.hh"
#include "mem/spm/spm_message/SPMRequestMsg.hh"
#include "mem/spm/spm_message/SPMResponseMsg.hh"
#include "mem/spm/governor/local_spm.hh"
#include "mem/spm/governor/random_spm.hh"
#include "mem/spm/governor/greedy_spm.hh"
#include "mem/spm/governor/guaranteed_greedy_spm.hh"

int PMMU::m_num_controllers = 0;
int PMMU::m_page_size_bytes;

PMMU *
PMMUParams::create()
{
    return new PMMU(this);
}

PMMU::PMMU(const Params *p)
    : AbstractController(p),
      my_spm_ptr(nullptr),
      my_governor_ptr(p->governor),
      pending_gov_reqs(0)
{
    m_machineID.type = MachineType_PMMU;
    m_machineID.num = m_version;
    m_num_controllers++;

    spmSlavePort = new SPMSideSlavePort(p->name + ".spm_s_side", this, 0);
    spmMasterPort = new SPMSideMasterPort(p->name + ".spm_m_side", this, 1);

    m_page_size_bytes = p->page_size_bytes;
    assert(isPowerOf2(m_page_size_bytes));

    m_in_ports = 2; //@TODO: Should this be 2?
//    m_dma_sequencer_ptr->setController(this);
//    m_request_latency = p->request_latency;
    m_responseFromSPM_ptr = p->responseFromSPM;
    m_responseToSPM_ptr = p->responseToSPM;
    m_requestFromSPM_ptr = p->requestFromSPM;
    m_requestToSPM_ptr = p->requestToSPM;
    m_responseToNetwork_ptr = p->responseToNetwork;
    m_requestToNetwork_ptr = p->requestToNetwork;

    addToGovernor(p->gov_type);

    my_att = new ATT();
}

BaseMasterPort &
PMMU::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "spm_m_side") {
        return *spmMasterPort;
    }
    else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort &
PMMU::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "spm_s_side") {
        return *spmSlavePort;
    }
    else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

void
PMMU::addToGovernor(std::string gov_type)
{
    if (gov_type.compare("Random") == 0) {
        dynamic_cast<RandomSPM*>(my_governor_ptr)->addPMMU(this);
    }
    else if (gov_type.compare("Greedy") == 0) {
        dynamic_cast<GreedySPM*>(my_governor_ptr)->addPMMU(this);
    }
    else if (gov_type.compare("GuaranteedGreedy") == 0) {
        dynamic_cast<GuaranteedGreedySPM*>(my_governor_ptr)->addPMMU(this);
    }
}

void
PMMU::initNetQueues()
{
    MachineType machine_type = string_to_MachineType("PMMU");
    int base M5_VAR_USED = MachineType_base_number(machine_type);

    //assert(m_responseFromDir_ptr != NULL);
    //m_net_ptr->setFromNetQueue(m_version + base, m_responseFromDir_ptr->getOrdered(), 1,
    //                                 "response", m_responseFromDir_ptr);
    //assert(m_requestToDir_ptr != NULL);
    //m_net_ptr->setToNetQueue(m_version + base, m_requestToDir_ptr->getOrdered(), 0,
    //                                 "request", m_requestToDir_ptr);

    assert(m_responseToNetwork_ptr != NULL);
    m_net_ptr->setToNetQueue(m_version + base, m_responseToNetwork_ptr->getOrdered(), 0,
                                     "response", m_responseToNetwork_ptr);
    assert(m_requestToNetwork_ptr != NULL);
    m_net_ptr->setToNetQueue(m_version + base, m_requestToNetwork_ptr->getOrdered(), 1,
                                     "request", m_requestToNetwork_ptr);

    assert(m_requestToSPM_ptr != NULL);
    m_net_ptr->setFromNetQueue(m_version + base, m_requestToSPM_ptr->getOrdered(), 1,
                                     "request", m_requestToSPM_ptr);
    assert(m_responseToSPM_ptr!= NULL);
    m_net_ptr->setFromNetQueue(m_version + base, m_responseToSPM_ptr->getOrdered(), 0,
                                     "response", m_responseToSPM_ptr);
}

void
PMMU::init()
{
    //(*m_mandatoryQueue_ptr).setConsumer(this);
    //(*m_responseFromDir_ptr).setConsumer(this);

    (*m_responseFromSPM_ptr).setConsumer(this);
    (*m_requestFromSPM_ptr).setConsumer(this);
    (*m_responseToSPM_ptr).setConsumer(this);
    (*m_requestToSPM_ptr).setConsumer(this);

    AbstractController::init();
    resetStats();
}

void
PMMU::setSPM(SPM *_spm)
{
    assert(!my_spm_ptr);
    my_spm_ptr = _spm;
}

void
PMMU::regStats()
{
	AbstractController::regStats();
}

void
PMMU::collateStats()
{

}

int
PMMU::getNumControllers()
{
    return m_num_controllers;
}

MessageBuffer*
PMMU::getMandatoryQueue() const
{
    return NULL;
}

MessageBuffer*
PMMU::getMemoryQueue() const
{
    return NULL;
}

Sequencer*
PMMU::getCPUSequencer() const
{
    return NULL;
}

GPUCoalescer*
PMMU::getGPUCoalescer() const
{
    return NULL;
}

void
PMMU::print(ostream& out) const
{
    out << "[PMMU " << m_version << "]";
}

void PMMU::resetStats()
{

}

void
PMMU::recordCacheTrace(int cntrl, CacheRecorder* tr)
{
}

AccessPermission
PMMU::getAccessPermission(const Addr& param_addr)
{
    return AccessPermission_NotPresent;
}

void
PMMU::functionalRead(const Addr& param_addr, Packet* param_pkt)
{
    panic("PMMU does not support functional read.");
}

int
PMMU::functionalWrite(const Addr& param_addr, Packet* param_pkt)
{
    panic("PMMU does not support functional write.");
}

int
PMMU::functionalWriteBuffers(PacketPtr& pkt)
{
    int num_functional_writes = 0;
    //num_functional_writes += m_responseFromDir_ptr->functionalWrite(pkt);
    //num_functional_writes += m_requestToDir_ptr->functionalWrite(pkt);
    //num_functional_writes += m_mandatoryQueue_ptr->functionalWrite(pkt);
    return num_functional_writes;
}

int
PMMU::addATTMappingsVAddress(GOVRequest *gov_request, HostInfo *host_info)
{
    PMMU *host_pmmu = host_info->getHostPMMU();
    int num_pages = host_info->getNumPages();
    Addr start_v_page_addr = gov_request->getStartAddr(Unserved_Aligned);
    Addr start_p_spm_addr = host_info->getSPMaddress();

    int added_pages = 0;
    for (int page_index = 0; page_index < num_pages; page_index++) {

        Addr v_page_addr = start_v_page_addr + page_index*getPageSizeBytes();
        Addr p_spm_addr = start_p_spm_addr + page_index*getPageSizeBytes();

        // update ATT
        if (my_att->addMapping(gov_request, host_info, page_index)) {

            DPRINTF(ATTMap, "Node %d: Adding ATT mapping for virtual address %x on node %d, spm address %d\n",
                    getNodeID(), v_page_addr, host_pmmu->getMachineID(), p_spm_addr);

            ATTEntry *mapping = my_att->getMapping(v_page_addr);
            if (host_info->alloc_mode != NUM_ALLOCATION_MODE) {
                // this hack is needed to pass the pointer to a private copy
                // of annotations kept in ATT
                Annotations *orignal_annotations = gov_request->getAnnotations();
                gov_request->setAnnotations(mapping->annotations);
                triggerPageAlloc(gov_request, host_info, page_index);
                gov_request->setAnnotations(orignal_annotations);
                if (!(host_info->signaling.wait_for_signal) && host_info->alloc_mode == UNINITIALIZE) {
                    my_att->validateATTEntry(mapping);
                }
            }
            added_pages++;
        }
        else {
            DPRINTF(ATTMap, "Node %d: Already Added: ATT mapping for virtual address %x\n",
                    getNodeID(), v_page_addr);
        }
    }
    return added_pages;
}

void
PMMU::triggerPageAlloc(GOVRequest *gov_request,
                       HostInfo *host_info,
                       int page_index)
{
    Addr v_page_addr = gov_request->getStartAddr(Unserved_Aligned) +
                       page_index * gov_request->getPMMUPtr()->getPageSizeBytes();
    Addr p_spm_addr = host_info->getSPMaddress() +
                      page_index * gov_request->getPMMUPtr()->getPageSizeBytes();

    Addr p_page_addr;
    bool translated = gov_request->getPageTablePtr()->translate(v_page_addr, p_page_addr);
    if (!translated)
        panic("Virtual page doesn't exist in page table!.\n");

    RequestPtr alloc_req = new Request(p_page_addr, getPageSizeBytes(), Request::PHYSICAL, Request::funcMasterId);
    PacketPtr alloc_pkt = new Packet(alloc_req, MemCmd::ReadReq, getPageSizeBytes());
    alloc_pkt->allocate();
    alloc_pkt->req->setFlags(Request::UNCACHEABLE | Request::STRICT_ORDER);

    alloc_pkt->govInfo.setAddresses(p_spm_addr, p_page_addr, v_page_addr);
    alloc_pkt->govInfo.markIncomplete();
    alloc_pkt->govInfo.makeAllocate();
    alloc_pkt->govInfo.setAnnotations(gov_request->getAnnotations());

    alloc_pkt->spmInfo.setOrigin(m_machineID.num);

    alloc_pkt->govInfo.setSignaling(&(host_info->signaling));

    alloc_pkt->govInfo.setStallStatus(alloc_pkt->govInfo.hasSignaler() ||
                                     (alloc_pkt->govInfo.getAnnotations()->alloc_mode == COPY));

    std::shared_ptr<SPMRequestMsg> msg = std::make_shared<SPMRequestMsg>(clockEdge());
    msg->m_Type = SPMRequestType_ALLOC;
    msg->m_PktPtr = alloc_pkt;
    (msg->m_Destination).add(host_info->getHostMachineID());
    msg->m_Requestor = m_machineID;
    msg->m_MessageSize = MessageSizeType_Control;

    if (host_info->getHostMachineID().num == m_machineID.num) {
        m_requestToSPM_ptr->enqueue(msg, clockEdge(), 1);
    }
    else {
        m_requestToNetwork_ptr->enqueue(msg, clockEdge(), 1);
    }

    if (alloc_pkt->govInfo.shouldStall()) {
        // block the spm's cpu-side port
        my_spm_ptr->setBlocked(BaseSPM::Blocked_Alloc_DeAlloc_Relocate);
        // to make sure we don't unblock the port unless all pages are allocated
        pending_gov_reqs++;
    }

    DPRINTF(PMMU, "Node %d: Allocating a page on node %d, spm address %d, should wait %d, for node %d\n",
                   getNodeID(), host_info->getHostMachineID().num, p_spm_addr,
                   host_info->signaling.wait_for_signal, host_info->signaling.signaler);
}

int
PMMU::removeATTMappingsVAddress(GOVRequest *gov_request, HostInfo *host_info)
{
    int num_pages = host_info->getNumPages();
    Addr start_v_page_addr = gov_request->getStartAddr(Unserved_Aligned);

    int removed_pages = 0;
    for (int page_index = 0; page_index < num_pages; page_index++) {
        Addr v_page_addr = start_v_page_addr + page_index*getPageSizeBytes();

        ATTEntry* mapping =  my_att->getMapping(v_page_addr);

        // if we havn't allocated this page (=there was no space), then nothing needs to be removed
        if (mapping) {

            host_info->setHostMachineID(mapping->destination_node);
            host_info->setSPMaddress(mapping->spm_slot_addr);

            DPRINTF(ATTMap, "Node %d: Removing ATT mapping for virtual address %x on node %d, spm address %d\n",
                    getNodeID(), v_page_addr, mapping->destination_node, mapping->spm_slot_addr);

            Annotations *mapping_annotations = mapping->annotations;
            mapping_annotations->dealloc_mode = gov_request->getAnnotations()->dealloc_mode;

            if (my_att->removeMapping(gov_request, page_index)) {
                removed_pages++;
                Annotations *orignal_annotations = gov_request->getAnnotations();
                gov_request->setAnnotations(mapping_annotations);
                if (mapping_annotations->alloc_mode != NUM_ALLOCATION_MODE) // if it was actually allocated
                    triggerPageDeAlloc(gov_request, host_info, page_index);
                else
                    delete mapping_annotations;
                gov_request->setAnnotations(orignal_annotations);
            }
        }

        else {
            DPRINTF(ATTMap, "Node %d: Already Removed / Non-existent - ATT mapping for virtual address %x\n",
                    getNodeID(), v_page_addr);
        }
    }
    return removed_pages;
}

int
PMMU::removeATTMappingsSPMAddress(GOVRequest *gov_request, HostInfo *host_info)
{
    for (int i = 1; i <= host_info->getNumPages(); i++) {
      Addr start_v_page_addr = my_att->slot2Addr(host_info->getHostPMMU()->getMachineID(),
                               host_info->getSPMaddress());

      gov_request->setAnnotations(my_att->getMapping(start_v_page_addr)->annotations);

      gov_request->address_range = AddrRange(start_v_page_addr,
                                                start_v_page_addr + gov_request->getPageSizeBytes());

      removeATTMappingsVAddress(gov_request, host_info);
    }

    return 1;
}

void
PMMU::triggerPageDeAlloc(GOVRequest *gov_request,
                         HostInfo *host_info,
                         int page_index)
{
    Addr v_page_addr = gov_request->getStartAddr(Unserved_Aligned) +
                       page_index * gov_request->getPMMUPtr()->getPageSizeBytes();
    Addr p_spm_addr = host_info->getSPMaddress();

    Addr p_page_addr;
    bool translated = gov_request->getPageTablePtr()->translate(v_page_addr, p_page_addr);
    if (!translated)
        panic("Virtual page doesn't exist in page table!.\n");

    RequestPtr dealloc_req = new Request(p_page_addr, getPageSizeBytes(), Request::PHYSICAL, Request::funcMasterId);
    PacketPtr dealloc_pkt = new Packet(dealloc_req, MemCmd::WriteReq, getPageSizeBytes());
    dealloc_pkt->allocate();
    dealloc_pkt->req->setFlags(Request::UNCACHEABLE | Request::STRICT_ORDER);

    dealloc_pkt->spmInfo.setOrigin(m_machineID.num);

    dealloc_pkt->govInfo.setAddresses(p_spm_addr, p_page_addr, v_page_addr);
    dealloc_pkt->govInfo.markIncomplete();
    dealloc_pkt->govInfo.makeDeallocate();
    dealloc_pkt->govInfo.setAnnotations(gov_request->getAnnotations());

    dealloc_pkt->govInfo.setSignaling(&(host_info->signaling));

    dealloc_pkt->govInfo.setStallStatus(dealloc_pkt->govInfo.getAnnotations()->dealloc_mode == WRITE_BACK);

    std::shared_ptr<SPMRequestMsg> msg = std::make_shared<SPMRequestMsg>(clockEdge());
    msg->m_Type = SPMRequestType_DEALLOC;
    msg->m_PktPtr = dealloc_pkt;
    (msg->m_Destination).add(host_info->getHostMachineID());
    msg->m_Requestor = m_machineID;
    msg->m_MessageSize = MessageSizeType_Control;

    if (host_info->getHostMachineID().num == m_machineID.num) {
        m_requestToSPM_ptr->enqueue(msg, clockEdge(), 1);
    }
    else {
        m_requestToNetwork_ptr->enqueue(msg, clockEdge(), 1);
    }

    if (dealloc_pkt->govInfo.shouldStall()) {
        // block the spm's cpu-side port
        my_spm_ptr->setBlocked(BaseSPM::Blocked_Alloc_DeAlloc_Relocate);
        // to make sure we don't unblock the port unless all pages are deallocated
        pending_gov_reqs++;
    }

    DPRINTF(PMMU, "Node %d: Deallocating a page from node %d, spm address %d, should signal %d, node %d\n",
                   getNodeID(), host_info->getHostMachineID().num, p_spm_addr,
                   host_info->signaling.needs_to_signal, host_info->signaling.signalee);
}

void
PMMU::changeATTMappingVAddress(GOVRequest *gov_request,
                               HostInfo *current_host_info,
                               HostInfo *future_host_info)
{
    changeATTMappingSPMAddress(current_host_info, future_host_info, gov_request);
}

void
PMMU::changeATTMappingSPMAddress(HostInfo *current_host_info,
                                 HostInfo *future_host_info,
                                 GOVRequest *gov_request)
{
    int num_pages = current_host_info->getNumPages();

    for (int page_index = 0; page_index < num_pages; page_index++) {

        // change ATT mapping first
        if (gov_request) { // if virtual address given not spm address
            Addr start_v_page_addr = gov_request->getStartAddr(Unserved_Aligned);
            Addr v_page_addr = start_v_page_addr + page_index*getPageSizeBytes();

            ATTEntry* current_mapping = my_att->getMapping(v_page_addr);
            current_host_info->setHostMachineID(current_mapping->destination_node);
            current_host_info->setSPMaddress(current_mapping->spm_slot_addr);
        }

        ATTEntry* mapping = my_att->changeMapping(current_host_info->getHostMachineID(),
                                                 current_host_info->getSPMaddress(),
                                                 future_host_info->getHostMachineID(),
                                                 future_host_info->getSPMaddress());
        assert (mapping);
        DPRINTF(ATTMap, "Node %d: Changing ATT mapping "
                     "from node %d, spm address %d to node %d, spm address %d\n",
                     getNodeID(),
                     current_host_info->getHostMachineID(),
                     current_host_info->getSPMaddress(),
                     future_host_info->getHostMachineID(),
                     future_host_info->getSPMaddress());
        // TODO: invalidate mapping during relocation and validate it again?

        triggerPageRelocation(current_host_info, future_host_info, page_index, mapping->annotations);
    }
}

void
PMMU::triggerPageRelocation(HostInfo *current_host_info,
                            HostInfo *future_host_info,
                            int page_index,
                            Annotations *att_annotation_ptr)
{
    RequestPtr relocation_req = new Request(0, getPageSizeBytes(), Request::PHYSICAL, Request::funcMasterId);
    PacketPtr relocate_pkt = new Packet(relocation_req, MemCmd::ReadReq, getPageSizeBytes());
    relocate_pkt->allocate();

    relocate_pkt->spmInfo.setSPMAddress(current_host_info->getSPMaddress());
    relocate_pkt->spmInfo.setOrigin(m_machineID.num);

    relocate_pkt->govInfo.setSPMAddress(current_host_info->getSPMaddress() + page_index*getPageSizeBytes());
    relocate_pkt->govInfo.setFutureHost(future_host_info->getHostMachineID().num);
    relocate_pkt->govInfo.setFutureSPMAddress(future_host_info->getSPMaddress() + page_index*getPageSizeBytes());

    relocate_pkt->govInfo.markIncomplete();
    relocate_pkt->govInfo.makeRelocationRead();
    relocate_pkt->govInfo.setAnnotations(att_annotation_ptr);

    relocate_pkt->govInfo.setSignaling(&(current_host_info->signaling));
    relocate_pkt->govInfo.setStallStatus(true);

    std::shared_ptr<SPMRequestMsg> msg = std::make_shared<SPMRequestMsg>(clockEdge());
    msg->m_Type = SPMRequestType_RELOCATE_READ;
    msg->m_PktPtr = relocate_pkt;
    (msg->m_Destination).add(current_host_info->getHostMachineID());
    msg->m_Requestor = m_machineID;
    msg->m_MessageSize = MessageSizeType_Control;

    if (current_host_info->getHostMachineID().num == m_machineID.num) {
        m_requestToSPM_ptr->enqueue(msg, clockEdge(), 1);
    }
    else {
        m_requestToNetwork_ptr->enqueue(msg, clockEdge(), 1);
    }

    if (relocate_pkt->govInfo.shouldStall()) {
        // block the spm's cpu-side port
        my_spm_ptr->setBlocked(BaseSPM::Blocked_Alloc_DeAlloc_Relocate);
        // to make sure we don't unblock the port unless all pages are relocated
        pending_gov_reqs++;
    }

    DPRINTF(PMMU, "Node %d: Relocating(read) a page from node %d, spm address %d, "
                  "to node %d, spm address %d, should signal %d, node %d\n",
                   getNodeID(),
                   current_host_info->getHostMachineID(),
                   current_host_info->getSPMaddress(),
                   future_host_info->getHostMachineID(),
                   future_host_info->getSPMaddress(),
                   current_host_info->signaling.needs_to_signal,
                   current_host_info->signaling.signalee);
}

void
PMMU::continuePageRelocationRequest(PacketPtr relocation_pkt)
{
    relocation_pkt->cmd = MemCmd::WriteReq;
    relocation_pkt->spmInfo.setSPMAddress(relocation_pkt->govInfo.getFutureSPMAddress());

    relocation_pkt->govInfo.markIncomplete();
    relocation_pkt->govInfo.makeRelocationWrite();

    relocation_pkt->govInfo.shouldNotWait();

    std::shared_ptr<SPMRequestMsg> msg = std::make_shared<SPMRequestMsg>(clockEdge());
    msg->m_Type = SPMRequestType_RELOCATE_WRITE;
    msg->m_PktPtr = relocation_pkt;
    MachineID future_host_node;
    future_host_node.num = relocation_pkt->govInfo.getFutureHost();
    future_host_node.type = MachineType_PMMU;
    (msg->m_Destination).add(future_host_node);
    msg->m_Requestor = m_machineID;
    msg->m_MessageSize = MessageSizeType_Data;

    if (future_host_node.num == m_machineID.num) {
        m_requestToSPM_ptr->enqueue(msg, clockEdge(), 1);
    }
    else {
        m_requestToNetwork_ptr->enqueue(msg, clockEdge(), 1);
    }

    DPRINTF(PMMU, "Node %d: Relocating(write) a page to node %d, spm address %d requested by node %d\n",
                   getNodeID(), relocation_pkt->govInfo.getFutureHost(),
                   relocation_pkt->govInfo.getFutureSPMAddress(), relocation_pkt->spmInfo.getOrigin());
}

void
PMMU::setFreePages(Addr start_p_spm_addr, int num_pages)
{
    int ind = start_p_spm_addr / getPageSizeBytes();
    int ctr;
    for (ctr = 0; ctr < num_pages; ctr++) {
        assert (ctr+ind < getSPMSizePages());
        my_spm_ptr->spmSlots[ctr+ind].setFree();
    }
}

void
PMMU::setUsedPages(Addr start_p_spm_addr, int num_pages,
                   NodeID owner)
{
    int ind = start_p_spm_addr / getPageSizeBytes();
    int ctr;
    for (ctr = 0; ctr < num_pages; ctr++) {
        assert (ctr+ind < getSPMSizePages());
        my_spm_ptr->spmSlots[ctr+ind].setUsed();
        my_spm_ptr->spmSlots[ctr+ind].setOwner(owner);
    }
}

bool
PMMU::sendRelocationDone(PacketPtr pkt)
{
    bool signal_required = pkt->govInfo.hasSignalee();

    MachineID dest;
    dest.num = pkt->govInfo.getSignalee();
    dest.type = MachineType_PMMU;

    if (signal_required){

        DPRINTF(PMMU, "Node %d: SPMResponseType_RELOCATION_DONE to node %d \n",
                getNodeID(), pkt->govInfo.getSignalee());

        //SPMResponseMsg *msg = new SPMResponseMsg(clockEdge());
        std::shared_ptr<SPMResponseMsg> msg = std::make_shared<SPMResponseMsg>(clockEdge());

        pkt->spmInfo.setOrigin(getNodeID());
        msg->m_PktPtr = pkt;
        (msg->m_Destination).add(dest);
        msg->m_Sender = m_machineID;
        msg->m_Type = SPMResponseType_RELOCATION_DONE;
        msg->m_MessageSize = MessageSizeType_Control;

        m_responseToNetwork_ptr->enqueue(msg, clockEdge(), 1);
    }

    return signal_required;
}

void
PMMU::fireupPendingAllocs(PacketPtr pkt)
{
    // for all of the local alloc reqs, if their current_user, matches previous_owner, then make them active

    NodeID signalee = pkt->spmInfo.getOrigin();
    Addr p_spm_addr = pkt->govInfo.getSPMAddress();

    unsigned int size = m_requestToSPM_ptr->getSize(curTick());
    unsigned int it = 0;

    unsigned int success = 0;
    for (it=0; (it < size); it++){

        SPMRequestMsg* msg = const_cast<SPMRequestMsg *>(dynamic_cast<const SPMRequestMsg *>(m_requestToSPM_ptr->peekNthMsg(it)));

        if (msg->m_PktPtr->govInfo.getSignaler() == signalee &&
            msg->m_PktPtr->govInfo.getSPMAddress() == p_spm_addr) {
            msg->m_PktPtr->govInfo.shouldNotWait();
            success++;
        }
    }

    DPRINTF(PMMU, "Node %d: Firing up %d pending allocate(s) triggered by node %d\n", getNodeID(), success, signalee);

    delete pkt->req;
    delete pkt;
}

const SPMRequestMsg*
PMMU::getNextReadyRequestMsg(MessageBuffer *mb) const
{

    unsigned int size = mb->getSize(curTick());
    unsigned int it = 0;
    const SPMRequestMsg* msg = NULL;

    for (it=0; (it < size) && mb->isReady(curTick()); it++){

        msg = (dynamic_cast<const SPMRequestMsg *>(((*mb)).peek()));
        MsgPtr msg_ptr = mb->peekMsgPtr();

        assert(msg->m_PktPtr->isRequest());

        if (!msg->m_PktPtr->govInfo.hasSignaler()) {
            return (msg);
        }

//      Tick enq_time = 0;
//      enq_time = msg_ptr->getLastEnqueueTime();

        mb->dequeue(curTick());
        mb->enqueue(msg_ptr, clockEdge(), 1);

//      msg_ptr->setLastEnqueueTime(enq_time);

    }

    DPRINTF(PMMU, "Node %d: Ready request not found in %s\n", getNodeID(), mb->name());
    return nullptr;
}

void
PMMU::finalizeLocalGOVReq(PacketPtr gov_pkt)
{

    if (gov_pkt->govInfo.shouldStall()) {
        pending_gov_reqs--;
    }
    if (pending_gov_reqs == 0 && my_spm_ptr->isBlocked()) {
        my_spm_ptr->clearBlocked(BaseSPM::Blocked_Alloc_DeAlloc_Relocate);
    }

    Addr start_v_page_addr = gov_pkt->govInfo.getVirtualAddress();

    if (gov_pkt->govInfo.isAllocate()){
        // if we've not been kicked out by the owner while we were busy allocating the slot on-chip
        if (my_att->hasMapping(start_v_page_addr)){

            DPRINTF(PMMU, "Node %d: Finalizing allocation\n", getNodeID());

            ATTEntry *translation = my_att->getMapping(start_v_page_addr);
            assert (translation);

            my_att->validateATTEntry(translation);
        }
    }
    else if (gov_pkt->govInfo.isDeallocate()) {
        DPRINTF(PMMU, "Node %d: Finalizing deallocation\n", getNodeID());
    }
    else if (gov_pkt->govInfo.isRelocationWrite()) {
        DPRINTF(PMMU, "Node %d: Finalizing relocation\n", getNodeID());
    }
    else {
        panic("Unknown packet type received!");
    }

    bool signal_required = sendRelocationDone(gov_pkt);

    if (!signal_required){
//      pkt->deleteData();
        delete gov_pkt->req;
        delete gov_pkt;
    }
}

////////////////////
//
// Message Processing helpers
//
////////////////////

bool
PMMU::processRemoteRequestMsg(bool *sending_mem_reqs_allowed)
{
    bool msg_processed = false;

    const SPMRequestMsg* in_msg_ptr = getNextReadyRequestMsg (m_requestToSPM_ptr);
    if(in_msg_ptr){

        PacketPtr reqPacket = in_msg_ptr->m_PktPtr;
        assert(reqPacket->isRequest());

        if ((in_msg_ptr->getType() == SPMRequestType_ALLOC) && (in_msg_ptr->m_PktPtr->govInfo.hasSignaler())) {
            // do nothing
            DPRINTF(PMMU, "Node %d: SPMRequestType_ALLOC should wait for node %d\n",
                    getNodeID(), in_msg_ptr->m_PktPtr->govInfo.getSignaler());
        }
        else if (in_msg_ptr->getType() == SPMRequestType_ALLOC) {
            DPRINTF(PMMU, "Node %d: SPMRequestType_ALLOC from node %d\n",
                    getNodeID(), reqPacket->spmInfo.getOrigin());
            if (*sending_mem_reqs_allowed){
                // we shouldn't forward any other memory request to spm
                my_spm_ptr->conditionalBlocking(BaseSPM::Blocked_MaxPendingReqs);
                *sending_mem_reqs_allowed = false;
                spmMasterPort->schedTimingReq(reqPacket, curTick());
                msg_processed = true;
            }
        }
        else if (in_msg_ptr->getType() == SPMRequestType_DEALLOC) {
            DPRINTF(PMMU, "Node %d: SPMRequestType_DEALLOC from node %d\n",
                    getNodeID(), reqPacket->spmInfo.getOrigin());
            if (*sending_mem_reqs_allowed){
                // we shouldn't forward any other memory request to spm
                my_spm_ptr->conditionalBlocking(BaseSPM::Blocked_MaxPendingReqs);
                *sending_mem_reqs_allowed = false;
                spmMasterPort->schedTimingReq(reqPacket, curTick());
                msg_processed = true;
            }
        }
        else if (in_msg_ptr->getType() == SPMRequestType_RELOCATE_READ) {
            DPRINTF(PMMU, "Node %d: SPMRequestType_RELOCATE_READ from node %d\n",
                    getNodeID(), reqPacket->spmInfo.getOrigin());
            spmMasterPort->schedTimingReq(reqPacket, curTick());
            msg_processed = true;
        }
        else if (in_msg_ptr->getType() == SPMRequestType_RELOCATE_WRITE) {
            DPRINTF(PMMU, "Node %d: SPMRequestType_RELOCATE_WRITE from node %d\n",
                    getNodeID(), reqPacket->spmInfo.getOrigin());
            spmMasterPort->schedTimingReq(reqPacket, curTick());
            msg_processed = true;
        }
        else {
            DPRINTF(PMMU, "Node %d: SPMRequestType_READ/WRITE from node %d\n",
                    getNodeID(), reqPacket->spmInfo.getOrigin());
            spmMasterPort->schedTimingReq(reqPacket, curTick());
            msg_processed = true;
        }
    }

    if (msg_processed) {
        m_requestToSPM_ptr->dequeue(clockEdge());
    }

    return msg_processed;
}

bool
PMMU::processLocalRequestMsg(bool *sending_mem_reqs_allowed)
{
    const SPMRequestMsg* in_msg_ptr = dynamic_cast<const SPMRequestMsg *>(((*m_requestFromSPM_ptr)).peek());
    assert(in_msg_ptr != NULL);
    MsgPtr msg_ptr = m_requestFromSPM_ptr->peekMsgPtr();

    PacketPtr req_packet = in_msg_ptr->m_PktPtr;

    if (in_msg_ptr->m_PktPtr->spmInfo.isLocal()) {
        assert(req_packet->needsResponse());
        req_packet->saveCmd();
        req_packet->makeResponse(); // This should be makeResponse because we don't need to fwd it to main memory

        spmSlavePort->schedTimingResp(req_packet, curTick());
        m_requestFromSPM_ptr->dequeue(clockEdge());
    }
    else if (in_msg_ptr->m_PktPtr->spmInfo.isRemote()){
        m_requestFromSPM_ptr->dequeue(clockEdge());
        m_requestToNetwork_ptr->enqueue(msg_ptr, curTick(), 1);
    }
    else if (!in_msg_ptr->m_PktPtr->spmInfo.isOnChip() && *sending_mem_reqs_allowed){
        my_spm_ptr->conditionalBlocking(BaseSPM::Blocked_MaxPendingReqs);
        *sending_mem_reqs_allowed = false;

        assert(req_packet->needsResponse());
        req_packet->saveCmd();
        req_packet->makeResponse(); // This should be makeShallowResponse because we need to fwd it to main memory

        spmSlavePort->schedTimingResp(req_packet, curTick());
        m_requestFromSPM_ptr->dequeue(clockEdge());
    }
    return true;
}

bool
PMMU::processRemoteResponseMsg()
{
    const SPMResponseMsg* in_msg_ptr = (dynamic_cast<const SPMResponseMsg *>(((*m_responseToSPM_ptr)).peek()));
    assert(in_msg_ptr != NULL);

    if (in_msg_ptr->getType() == SPMResponseType_RELOCATION_DONE) {
        fireupPendingAllocs(in_msg_ptr->m_PktPtr);
    }
    else {
        PacketPtr resp_packet = in_msg_ptr->m_PktPtr;
        assert(resp_packet->isResponse());

        if (in_msg_ptr->getType() == SPMResponseType_GOV_ACK) {
            finalizeLocalGOVReq(resp_packet);
        }
        else {
            // the packet is already complete and can be sent to spm
            DPRINTF(PMMU, "Node %d: SPMResponseType_DATA/WRITE_ACK from node %d\n",
                    getNodeID(), in_msg_ptr->getSender().num);
            spmSlavePort->schedTimingResp(resp_packet, curTick());
        }
    }

    m_responseToSPM_ptr->dequeue(clockEdge());
    return true;
}

bool
PMMU::processLocalResponseMsg()
{
    const SPMResponseMsg* in_msg_ptr = dynamic_cast<const SPMResponseMsg *>(((*m_responseFromSPM_ptr)).peek()); // making a copy because peek returns const message but we need non-const to modify and enqueue
    assert(in_msg_ptr != NULL);
    MsgPtr msg_ptr = m_responseFromSPM_ptr->peekMsgPtr();

    PacketPtr resp_packet = in_msg_ptr->m_PktPtr;

    // local gov_ack
    if (in_msg_ptr->getType() == SPMResponseType_GOV_ACK && in_msg_ptr->m_PktPtr->spmInfo.getOrigin() == getNodeID()) {
        DPRINTF(PMMU, "Node %d: SPMResponseType_GOV_ACK\n", getNodeID());

        finalizeLocalGOVReq(resp_packet);
        m_responseFromSPM_ptr->dequeue(clockEdge());
    }
    else if (in_msg_ptr->getType() == SPMResponseType_RELOCATION_HALFWAY) { // first part of relocation
        DPRINTF(PMMU, "Node %d: SPMResponseType_RELOCATION_HALFWAY for node %d\n",
                getNodeID(), resp_packet->spmInfo.getOrigin());

        continuePageRelocationRequest(resp_packet);
        m_responseFromSPM_ptr->dequeue(clockEdge());
    }
    else { // remote gov_ack or remote access
        m_responseFromSPM_ptr->dequeue(clockEdge());
        m_responseToNetwork_ptr->enqueue(msg_ptr, curTick(), 1);
    }

    return true;
}

////////////////////
//
// Message Processing
//
////////////////////

void
PMMU::wakeup()
{
    // TODO: memory leak?

    int counter = 0;

    bool sending_mem_reqs_allowed = my_spm_ptr->acceptingMemReqs();
    bool remote_response_msg_processed = false;
    bool remote_request_msg_stopped = false;
    bool local_request_msg_processed = false;
    bool local_response_msg_processed = false;

    // case I: we have remote responses incoming from the network waiting
    counter = 0;
    while (counter < m_transitions_per_cycle) {
        if ((*m_responseToSPM_ptr).isReady(curTick())) {
            remote_response_msg_processed = processRemoteResponseMsg();
        }
        counter++;
    }

    // case II: we have remote requests incoming from the network waiting
    // send packet to spm to read/write via sendTimingReq()
    counter = 0;
    while (counter < m_transitions_per_cycle) {
        if ((*m_requestToSPM_ptr).isReady(curTick())) {
            remote_request_msg_stopped = processRemoteRequestMsg(&sending_mem_reqs_allowed);
        }
        counter++;
    }

    // case III: we have requests from the spm (cpu) waiting
    counter = 0;
    while (counter < m_transitions_per_cycle) {
        if ((*m_requestFromSPM_ptr).isReady(curTick()) ) {
            local_request_msg_processed = processLocalRequestMsg(&sending_mem_reqs_allowed);
        }
        counter++;
    }

    // case IV: we have response from the local spm waiting to be forwarded to remote nodes
    // simply forward this to original requester node on network via m_responseToNetwork_ptr
    counter = 0;
    while (counter < m_transitions_per_cycle) {
        if ((*m_responseFromSPM_ptr).isReady(curTick())) {
            local_response_msg_processed = processLocalResponseMsg();
        }
        counter++;
    }

    // reschedule myself when i can't process all messages
    if ( (m_requestToSPM_ptr->isReady(curTick())    && !remote_request_msg_stopped)  ||
         (m_responseToSPM_ptr->isReady(curTick())   && remote_response_msg_processed) ||
         (m_requestFromSPM_ptr->isReady(curTick())  && local_request_msg_processed)   ||
         (m_responseFromSPM_ptr->isReady(curTick()) && local_response_msg_processed)
        ) {

        scheduleEvent(Cycles(1));
    }

}

////////////////////
//
// SPM-Side Req/Resp Handlers
//
////////////////////

bool
PMMU::generateAccessReqMsg(PacketPtr pkt)
{
    assert(pkt->isRequest());

    // enhance packet with spm stuff
    pkt->spmInfo.setOrigin(getNodeID());
    Addr req_v_page_addr = spmPageAlign(pkt->req->getVaddr());
    bool att_hit = my_att->isHit(req_v_page_addr);

    ATTEntry *translation = NULL;
    Addr spm_p_addr = 0;

    if (att_hit) {
        DPRINTF (ATTLookup, "Node %d, ATT hit for virtual page: %x\n", getNodeID(), pkt->req->getVaddr());

        translation = my_att->getMapping(req_v_page_addr);
        spm_p_addr = translation->spm_slot_addr | pkt->getOffset(getPageSizeBytes());  //TODO:  needs to be block size
        pkt->spmInfo.setSPMAddress(spm_p_addr);

        // if on local spm
        if (translation->destination_node.num == getNodeID()) {
            pkt->spmInfo.makeLocal();
        }
        else {  // else it's remote
            pkt->spmInfo.makeRemote();
        }
    }
    else {
        DPRINTF (ATTLookup, "Node %d, ATT miss for virtual page: %x\n", getNodeID(), pkt->req->getVaddr());

        // it's not allocated on SPM, but there was an allocation request for it
        if (my_att->hasMapping(req_v_page_addr)) {
            pkt->req->setFlags(Request::UNCACHEABLE | Request::STRICT_ORDER);
            pkt->spmInfo.makeUnallocated();
        }
        else { // otherwise it should be accessed from cache hierarchy or off-chip mem
            pkt->spmInfo.makeNotSPMSpace();
        }
    }

    // create message
    std::shared_ptr<SPMRequestMsg> msg = std::make_shared<SPMRequestMsg>(curTick());
    msg->m_PktPtr = pkt;
    msg->m_Requestor = getMachineID();
    msg->m_Type = pkt->isRead() ? SPMRequestType_READ : SPMRequestType_WRITE;
    msg->m_MessageSize = MessageSizeType_Access;
    (msg->m_Destination).add(att_hit ? translation->destination_node : getMachineID());
    msg->m_Addr = !pkt->spmInfo.isOnChip() ? pkt->req->getVaddr() : spm_p_addr;

    m_requestFromSPM_ptr->enqueue(msg, curTick(), 1);
    return true;
}

bool
PMMU::generateAccessRespMsg(PacketPtr pkt)
{
    // enhance packet with spm stuff
    // none!

    // create message
    std::shared_ptr<SPMResponseMsg> msg = std::make_shared<SPMResponseMsg>(curTick());
    msg->m_PktPtr = pkt; // pkt should already be a response packet from spm
    msg->m_Sender = getMachineID();
    MachineID req_owner;
    req_owner.num = pkt->spmInfo.getOrigin();
    req_owner.type = MachineType_PMMU;
    (msg->m_Destination).add(req_owner);
    msg->m_Type = pkt->isRead() ? SPMResponseType_DATA : SPMResponseType_WRITE_ACK;
    msg->m_MessageSize = (msg->m_Type == SPMResponseType_DATA) ?
            MessageSizeType_Access : MessageSizeType_Control;
    m_responseFromSPM_ptr->enqueue(msg, curTick(), 1);
    return true;
}

bool
PMMU::generateGovRespMsg(PacketPtr pkt)
{
    // enhance packet with spm stuff
    // none!

    // create message
    std::shared_ptr<SPMResponseMsg> msg = std::make_shared<SPMResponseMsg>(clockEdge());
    msg->m_PktPtr = pkt; // pkt should already be a response packet from spm
    msg->m_Sender = getMachineID();
    MachineID req_owner;
    req_owner.num = pkt->spmInfo.getOrigin();
    req_owner.type = MachineType_PMMU;
    (msg->m_Destination).add(req_owner);

    if (pkt->govInfo.isComplete()) {
        msg->m_Type = SPMResponseType_GOV_ACK;
        if (pkt->govInfo.isDeallocate() && !pkt->govInfo.hasSignalee()) {
            setFreePages(pkt->govInfo.getSPMAddress(), 1); // TODO: Always one slot?
        }
    }
    else if (pkt->govInfo.isRelocationRead()) {
        msg->m_Type = SPMResponseType_RELOCATION_HALFWAY;
        // TODO: making the page free overrides the fact that already set it to occupied for the next user in the gov
        if (!pkt->govInfo.hasSignalee()) {
            setFreePages(pkt->govInfo.getSPMAddress(), 1); // TODO: Always one slot?
        }
    }
    else if (pkt->govInfo.isRelocationWrite()) {
        msg->m_Type = SPMResponseType_GOV_ACK;
    }
    else {
        panic("SPM governor request returned incomplete");
    }

    msg->m_MessageSize = MessageSizeType_Control;
    m_responseFromSPM_ptr->enqueue(msg, clockEdge(), 1);
    return true;
}

bool
PMMU::recvSPMTimingReq(PacketPtr pkt)
{
    return generateAccessReqMsg(pkt);
}


bool
PMMU::recvSPMTimingResp(PacketPtr pkt)
{
    if (pkt->govInfo.isGOVReq()) {
        return generateGovRespMsg(pkt);
    }
    else {
        return generateAccessRespMsg(pkt);
    }
}

////////////////////
//
// SPMSideSlavePort
//
////////////////////

PMMU::SPMSideSlavePort::SPMSideSlavePort(const std::string &_name, PMMU *_pmmu, PortID id)
    : QueuedSlavePort(_name, NULL, _respQueue, id),
      _respQueue(*_pmmu, *this),
      my_pmmu(_pmmu)
{
}

bool
PMMU::SPMSideSlavePort::recvTimingReq(PacketPtr pkt)
{
    /*
     * This is called by local spm when there is a new mem request from local cpu
     */
    return my_pmmu->recvSPMTimingReq(pkt);
}

void
PMMU::SPMSideSlavePort::recvFunctional(PacketPtr pkt)
{
    //@TODO:  implement - see rubyport.cc version
}

////////////////////
//
// SPMSideMasterPort
//
////////////////////

PMMU::SPMSideMasterPort::SPMSideMasterPort(const std::string &_name, PMMU *_pmmu, PortID id)
    : QueuedMasterPort(_name, _pmmu, _reqQueue, _snoopRespQueue, id),
      _reqQueue(*_pmmu, *this),
      _snoopRespQueue(*my_pmmu, *this, ""),
      my_pmmu(_pmmu)
{
}

bool
PMMU::SPMSideMasterPort::recvTimingResp(PacketPtr pkt)
{
    /*
     * This is called by local spm in response to a request made by a remote pmmu
     */
    return my_pmmu->recvSPMTimingResp(pkt);
}
