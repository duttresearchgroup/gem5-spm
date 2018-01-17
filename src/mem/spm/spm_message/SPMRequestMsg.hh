#ifndef __SPMRequestMsg_HH__
#define __SPMRequestMsg_HH__

#include <iostream>

#include "mem/ruby/slicc_interface/RubySlicc_Util.hh"
//#include "mem/protocol/CoherenceRequestType.hh"
#include "mem/spm/spm_message/SPMMessageTypes.hh"
#include "mem/protocol/MachineID.hh"
#include "mem/protocol/NetDest.hh"
//#include "mem/protocol/DataBlock.hh"
#include "mem/spm/spm_class/SPMPage.hh"
#include "mem/protocol/MessageSizeType.hh"
#include "mem/protocol/Message.hh"
class SPMRequestMsg :  public Message
{
  public:
    SPMRequestMsg
(Tick curTime) : Message(curTime) {
        // m_Addr has no default
        m_Type = SPMRequestType_NUM; // default value of SPMRequestType
        // m_Requestor has no default
        // m_Destination has no default
        // m_DataBlk has no default
        m_MessageSize = MessageSizeType_NUM; // default value of MessageSizeType
    }
    SPMRequestMsg(const SPMRequestMsg&other)
        : Message(other)
    {
        m_Addr = other.m_Addr;
        m_Type = other.m_Type;
        m_Requestor = other.m_Requestor;
        m_Destination = other.m_Destination;
        m_DataBlk = other.m_DataBlk;
        m_MessageSize = other.m_MessageSize;
        m_PktPtr = other.m_PktPtr;
    }
    SPMRequestMsg(const Tick curTime, const Addr& local_addr, const SPMRequestType& local_Type, const MachineID& local_Requestor, const NetDest& local_Destination, const SPMPage& local_DataBlk, const MessageSizeType& local_MessageSize, const PacketPtr local_PktPtr)
        : Message(curTime)
    {
        m_Addr = local_addr;
        m_Type = local_Type;
        m_Requestor = local_Requestor;
        m_Destination = local_Destination;
        m_DataBlk = local_DataBlk;
        m_MessageSize = local_MessageSize;
        m_PktPtr = local_PktPtr;
    }
    MsgPtr
    clone() const
    {
         return std::shared_ptr<Message>(new SPMRequestMsg(*this));
    }
    // Const accessors methods for each field
    /** \brief Const accessor method for addr field.
     *  \return addr field
     */
    const Addr&
    getaddr() const
    {
        return m_Addr;
    }
    /** \brief Const accessor method for Type field.
     *  \return Type field
     */
    const SPMRequestType&
    getType() const
    {
        return m_Type;
    }
    /** \brief Const accessor method for Requestor field.
     *  \return Requestor field
     */
    const MachineID&
    getRequestor() const
    {
        return m_Requestor;
    }
    /** \brief Const accessor method for Destination field.
     *  \return Destination field
     */
    const NetDest&
    getDestination() const
    {
        return m_Destination;
    }
    /** \brief Const accessor method for DataBlk field.
     *  \return DataBlk field
     */
    const SPMPage&
    getDataBlk() const
    {
        return m_DataBlk;
    }
    /** \brief Const accessor method for MessageSize field.
     *  \return MessageSize field
     */
    const MessageSizeType&
    getMessageSize() const
    {
        return m_MessageSize;
    }
    // Non const Accessors methods for each field
    /** \brief Non-const accessor method for addr field.
     *  \return addr field
     */
    Addr&
    getaddr()
    {
        return m_Addr;
    }
    /** \brief Non-const accessor method for Type field.
     *  \return Type field
     */
    SPMRequestType&
    getType()
    {
        return m_Type;
    }
    /** \brief Non-const accessor method for Requestor field.
     *  \return Requestor field
     */
    MachineID&
    getRequestor()
    {
        return m_Requestor;
    }
    /** \brief Non-const accessor method for Destination field.
     *  \return Destination field
     */
    NetDest&
    getDestination()
    {
        return m_Destination;
    }
    /** \brief Non-const accessor method for DataBlk field.
     *  \return DataBlk field
     */
    SPMPage&
    getDataBlk()
    {
        return m_DataBlk;
    }
    /** \brief Non-const accessor method for MessageSize field.
     *  \return MessageSize field
     */
    MessageSizeType&
    getMessageSize()
    {
        return m_MessageSize;
    }
    /** \brief Non-const accessor method for PktPtr field.
     *  \return PktPtr field
     */
    PacketPtr
    getPktPtr() {
       return m_PktPtr;
    }
    // Mutator methods for each field
    /** \brief Mutator method for addr field */
    void
    setaddr(const Addr& local_addr)
    {
        m_Addr = local_addr;
    }
    /** \brief Mutator method for Type field */
    void
    setType(const SPMRequestType& local_Type)
    {
        m_Type = local_Type;
    }
    /** \brief Mutator method for Requestor field */
    void
    setRequestor(const MachineID& local_Requestor)
    {
        m_Requestor = local_Requestor;
    }
    /** \brief Mutator method for Destination field */
    void
    setDestination(const NetDest& local_Destination)
    {
        m_Destination = local_Destination;
    }
    /** \brief Mutator method for DataBlk field */
    void
    setDataBlk(const SPMPage& local_DataBlk)
    {
        m_DataBlk = local_DataBlk;
    }
    /** \brief Mutator method for MessageSize field */
    void
    setMessageSize(const MessageSizeType& local_MessageSize)
    {
        m_MessageSize = local_MessageSize;
    }
    /** \brief Mutator method for PktPtr field */
    void
    setPktPtr(const PacketPtr pktPtr)
    {
    	m_PktPtr = pktPtr;
    }
    void print(std::ostream& out) const;
  //private:
    /** Address for this request - can be virtual or local physical:
     * when in m_requestFromSPM_ptr queue, it is virtual
     * all other cases, it is local physical SPM address
     */
    Addr m_Addr;
    /** Type of request (GetS, GetX, PutX, etc) */
    SPMRequestType m_Type;
    /** Node who initiated the request */
    MachineID m_Requestor;
    /** Multicast destination mask */
    NetDest m_Destination;
    /** data for the spm page */
    SPMPage m_DataBlk;
    /** size category of the message */
    MessageSizeType m_MessageSize;
    /** pointer to original request packet */
    PacketPtr m_PktPtr;

    bool functionalWrite(Packet* param_pkt);
    bool functionalRead(Packet* param_pkt);
};
inline std::ostream&
operator<<(std::ostream& out, const SPMRequestMsg& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __SPMRequestMsg_HH__
