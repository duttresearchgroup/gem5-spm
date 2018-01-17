#include <iostream>
#include <memory>

#include "mem/spm/spm_message/SPMRequestMsg.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "mem/spm/spm_message/SPM_Util.hh"


using namespace std;
/** \brief Print the state of this object */
void
SPMRequestMsg::print(ostream& out) const
{
    out << "[SPMRequestMsg: ";
    out << "addr = " << printAddress(m_Addr) << " ";
    out << "Type = " << m_Type << " ";
    out << "Requestor = " << m_Requestor << " ";
    out << "Destination = " << m_Destination << " ";
    out << "DataBlk = " << m_DataBlk << " ";
    out << "MessageSize = " << m_MessageSize << " ";
    out << "]";
}

bool
SPMRequestMsg::functionalWrite(Packet* param_pkt)
{
return (testAndWrite(m_Addr, m_DataBlk, param_pkt));
}

bool
SPMRequestMsg::functionalRead(Packet* param_pkt)
{
    if ((m_Type == SPMRequestType_READ)) {
        return (testAndRead(m_Addr, m_DataBlk, param_pkt));
    }
    return (false);

}
