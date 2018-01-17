#ifndef __SPM_UTIL_HH__
#define __SPM_UTIL_HH__

#include <cassert>

#include "debug/RubySlicc.hh"
#include "mem/packet.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/common/DataBlock.hh"
#include "mem/ruby/common/TypeDefines.hh"

//inline Cycles zero_time() { return Cycles(0); }
//
//inline NodeID
//intToID(int nodenum)
//{
//    NodeID id = nodenum;
//    return id;
//}
//
//inline int
//IDToInt(NodeID id)
//{
//    int nodenum = id;
//    return nodenum;
//}
//
//inline int
//addressToInt(Addr addr)
//{
//    assert(!(addr & 0xffffffff00000000));
//    return addr;
//}
//
//inline int
//mod(int val, int mod)
//{
//    return val % mod;
//}
//
//inline int max_tokens()
//{
//  return 1024;
//}

/**
 * This function accepts an address, a data block and a packet. If the address
 * range for the data block contains the address which the packet needs to
 * read, then the data from the data block is written to the packet. True is
 * returned if the data block was read, otherwise false is returned.
 */
inline bool
testAndRead(Addr addr, SPMPage& blk, Packet *pkt)
{
    Addr pktLineAddr = makeLineAddress(pkt->getAddr());
    Addr lineAddr = makeLineAddress(addr);

    if (pktLineAddr == lineAddr) {
        uint8_t *data = pkt->getPtr<uint8_t>();
        unsigned int size_in_bytes = pkt->getSize();
        unsigned startByte = pkt->getAddr() - lineAddr;

        for (unsigned i = 0; i < size_in_bytes; ++i) {
            data[i] = blk.getByte(i + startByte);
        }
        return true;
    }
    return false;
}

/**
 * This function accepts an address, a data block and a packet. If the address
 * range for the data block contains the address which the packet needs to
 * write, then the data from the packet is written to the data block. True is
 * returned if the data block was written, otherwise false is returned.
 */
inline bool
testAndWrite(Addr addr, SPMPage& blk, Packet *pkt)
{
    Addr pktLineAddr = makeLineAddress(pkt->getAddr());
    Addr lineAddr = makeLineAddress(addr);

    if (pktLineAddr == lineAddr) {
        const uint8_t *data = pkt->getConstPtr<uint8_t>();
        unsigned int size_in_bytes = pkt->getSize();
        unsigned startByte = pkt->getAddr() - lineAddr;

        for (unsigned i = 0; i < size_in_bytes; ++i) {
            blk.setByte(i + startByte, data[i]);
        }
        return true;
    }
    return false;
}

#endif // __SPM_UTIL_HH__
