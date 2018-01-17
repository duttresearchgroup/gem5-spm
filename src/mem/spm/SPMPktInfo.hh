#ifndef __SPMPktInfo_HH__
#define __SPMPktInfo_HH__

#include <limits>
#include "mem/ruby/common/TypeDefines.hh"

#define UINT_MAXIMUM std::numeric_limits<unsigned int>::max()

class SPMPktInfo
{
  friend class Packet;

  private:

    // List of all data locations associated with a request packet.
    enum PageLocation
    {
        Local_SPM = 0,
        Remote_SPM,
        Unallocated_SPM,
        Not_SPM,
        NUM_MEM_LOC
    };

    PageLocation loc;
    NodeID requesting_node;
    NodeID holder_node;
    Addr spm_p_addr;

  public:

    SPMPktInfo() : loc(NUM_MEM_LOC),
                   requesting_node(UINT_MAXIMUM),
                   holder_node(UINT_MAXIMUM),
                   spm_p_addr(Addr(0)){}

    const char *toString()
    {
        static const char * LocationStrings[] = { "Local SPM",
                                                  "Remote SPM",
                                                  "Unallocated SPM",
                                                  "Off-Chip",
                                                  "Uninitialized" };
        return LocationStrings[loc];
    }

    void makeLocal()       { loc = Local_SPM; }
    void makeRemote()      { loc = Remote_SPM; }
    void makeUnallocated() { loc = Unallocated_SPM; }
    void makeNotSPMSpace() { loc = Not_SPM; }

    bool isLocal() const                 { return getPageLocation() == Local_SPM; }
    bool isRemote() const                { return getPageLocation() == Remote_SPM; }
    bool isOnChip() const                { return (isLocal() || isRemote());}
    bool isUnallocated() const           { return getPageLocation() == Unallocated_SPM; }
    bool isNotSPMSpace() const           { return getPageLocation() == Not_SPM; }
    PageLocation getPageLocation() const { assert(loc != NUM_MEM_LOC); return loc; }

    void setOrigin (NodeID _requesting_node) { requesting_node = _requesting_node; }
    void setHolder (NodeID _holder_node)     { holder_node = _holder_node; }

    NodeID getOrigin ()
    {
        assert(requesting_node != UINT_MAXIMUM);
        return requesting_node;
    }

    NodeID getHolder ()
    {
        assert(holder_node != UINT_MAXIMUM);
        return holder_node;
    }

    void setSPMAddress(Addr _spm_p_addr)
    {
        spm_p_addr = _spm_p_addr;
    }

    Addr getSPMAddress()
    {
        return spm_p_addr;
    }

    bool operator==(SPMPktInfo c2) const
    {
        return (loc == c2.loc                         &&
                requesting_node == c2.requesting_node &&
                holder_node == c2.holder_node);
    }

    bool operator!=(SPMPktInfo c2) const
    {
        return (loc != c2.loc                         ||
                requesting_node != c2.requesting_node ||
                holder_node != c2.holder_node);
    }
};

#endif //__SPMPktInfo_HH__
