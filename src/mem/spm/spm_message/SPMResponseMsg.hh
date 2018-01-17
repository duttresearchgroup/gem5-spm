#ifndef __SPMResponseMsg_HH__
#define __SPMResponseMsg_HH__

#include <iostream>

#include "mem/ruby/slicc_interface/RubySlicc_Util.hh"
//#include "mem/protocol/CoherenceResponseType.hh"
#include "mem/spm/spm_message/SPMMessageTypes.hh"
#include "mem/protocol/MachineID.hh"
#include "mem/protocol/NetDest.hh"
//#include "mem/protocol/DataBlock.hh"
#include "mem/spm/spm_class/SPMPage.hh"
#include "mem/protocol/MessageSizeType.hh"
#include "mem/protocol/Message.hh"
class SPMResponseMsg :  public Message
{
  public:
    SPMResponseMsg
(Tick curTime) : Message(curTime) {
        // m_Addr has no default
        m_Type = SPMResponseType_NUM; // default value of SPMResponseType
        // m_Sender has no default
        // m_Destination has no default
        // m_DataBlk has no default
//        m_Dirty = false; // default value of bool
        m_MessageSize = MessageSizeType_NUM; // default value of MessageSizeType
    }
    SPMResponseMsg(const SPMResponseMsg&other)
        : Message(other)
    {
        m_Addr = other.m_Addr;
        m_Type = other.m_Type;
        m_Sender = other.m_Sender;
        m_Destination = other.m_Destination;
        m_DataBlk = other.m_DataBlk;
//        m_Dirty = other.m_Dirty;
        m_MessageSize = other.m_MessageSize;
        m_PktPtr = other.m_PktPtr;
    }
    SPMResponseMsg(const Tick curTime, const Addr& local_addr, const SPMResponseType& local_Type, const MachineID& local_Sender, const NetDest& local_Destination, const SPMPage& local_DataBlk, const bool& local_Dirty, const MessageSizeType& local_MessageSize, const PacketPtr local_PktPtr)
        : Message(curTime)
    {
        m_Addr = local_addr;
        m_Type = local_Type;
        m_Sender = local_Sender;
        m_Destination = local_Destination;
        m_DataBlk = local_DataBlk;
//        m_Dirty = local_Dirty;
        m_MessageSize = local_MessageSize;
        m_PktPtr = local_PktPtr;
    }
    MsgPtr
    clone() const
    {
         return std::shared_ptr<Message>(new SPMResponseMsg(*this));
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
    const SPMResponseType&
    getType() const
    {
        return m_Type;
    }
    /** \brief Const accessor method for Sender field.
     *  \return Sender field
     */
    const MachineID&
    getSender() const
    {
        return m_Sender;
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
//    /** \brief Const accessor method for Dirty field.
//     *  \return Dirty field
//     */
//    const bool&
//    getDirty() const
//    {
//        return m_Dirty;
//    }
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
    SPMResponseType&
    getType()
    {
        return m_Type;
    }
    /** \brief Non-const accessor method for Sender field.
     *  \return Sender field
     */
    MachineID&
    getSender()
    {
        return m_Sender;
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
//    /** \brief Non-const accessor method for Dirty field.
//     *  \return Dirty field
//     */
//    bool&
//    getDirty()
//    {
//        return m_Dirty;
//    }
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
    setType(const SPMResponseType& local_Type)
    {
        m_Type = local_Type;
    }
    /** \brief Mutator method for Sender field */
    void
    setSender(const MachineID& local_Sender)
    {
        m_Sender = local_Sender;
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
//    /** \brief Mutator method for Dirty field */
//    void
//    setDirty(const bool& local_Dirty)
//    {
//        m_Dirty = local_Dirty;
//    }
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
    /** Physical address for this request */
    Addr m_Addr;
    /** Type of response (Ack, Data, etc) */
    SPMResponseType m_Type;
    /** Node who sent the data */
    MachineID m_Sender;
    /** Node to whom the data is sent */
    NetDest m_Destination;
    /** data for the spm page*/
    SPMPage m_DataBlk;
//    /** Is the data dirty (different than memory)? */
//    bool m_Dirty;
    /** size category of the message */
    MessageSizeType m_MessageSize;
    /** pointer to original request packet */
    PacketPtr m_PktPtr;
    bool functionalWrite(Packet* param_pkt);
    bool functionalRead(Packet* param_pkt);
};
inline std::ostream&
operator<<(std::ostream& out, const SPMResponseMsg& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __SPMResponseMsg_HH__
