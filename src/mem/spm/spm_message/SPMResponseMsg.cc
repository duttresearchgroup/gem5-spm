#include <iostream>
#include <memory>

#include "mem/spm/spm_message/SPMResponseMsg.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "mem/spm/spm_message/SPM_Util.hh"

using namespace std;
/** \brief Print the state of this object */
void
SPMResponseMsg::print(ostream& out) const
{
    out << "[SPMResponseMsg: ";
    out << "addr = " << printAddress(m_Addr) << " ";
    out << "Type = " << m_Type << " ";
    out << "Sender = " << m_Sender << " ";
    out << "Destination = " << m_Destination << " ";
    out << "DataBlk = " << m_DataBlk << " ";
//    out << "Dirty = " << m_Dirty << " ";
    out << "MessageSize = " << m_MessageSize << " ";
    out << "]";
}

bool
SPMResponseMsg::functionalWrite(Packet* param_pkt)
{
return (testAndWrite(m_Addr, m_DataBlk, param_pkt));
}

bool
SPMResponseMsg::functionalRead(Packet* param_pkt)
{
return (testAndRead(m_Addr, m_DataBlk, param_pkt));

}
