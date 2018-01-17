/** \file MachineType.hh
 *
 * Replaces auto-generated code to add PMMU MachineType support for SPM-based NoCs
 */

#include <cassert>
#include <iostream>
#include <string>

#include "base/logging.hh"
#include "mem/protocol/MachineType.hh"

using namespace std;

#include "mem/protocol/L1Cache_Controller.hh"
#include "mem/protocol/L2Cache_Controller.hh"
#include "mem/protocol/Directory_Controller.hh"
#include "mem/protocol/DMA_Controller.hh"
#include "mem/ruby/common/MachineID.hh"
#include "mem/spm/pmmu.hh"
// Code for output operator
ostream&
operator<<(ostream& out, const MachineType& obj)
{
    out << MachineType_to_string(obj);
    out << flush;
    return out;
}

// Code to convert state to a string
string
MachineType_to_string(const MachineType& obj)
{
    switch(obj) {
      case MachineType_L1Cache:
        return "L1Cache";
      case MachineType_L2Cache:
        return "L2Cache";
      case MachineType_L3Cache:
        return "L3Cache";
      case MachineType_Directory:
        return "Directory";
      case MachineType_DMA:
        return "DMA";
      case MachineType_PMMU:
        return "PMMU";
      case MachineType_Collector:
        return "Collector";
      case MachineType_L1Cache_wCC:
        return "L1Cache_wCC";
      case MachineType_L2Cache_wCC:
        return "L2Cache_wCC";
      case MachineType_CorePair:
        return "CorePair";
      case MachineType_TCP:
        return "TCP";
      case MachineType_TCC:
        return "TCC";
      case MachineType_TCCdir:
        return "TCCdir";
      case MachineType_SQC:
        return "SQC";
      case MachineType_RegionDir:
        return "RegionDir";
      case MachineType_RegionBuffer:
        return "RegionBuffer";
      case MachineType_NULL:
        return "NULL";
      default:
        panic("Invalid range for type MachineType");
    }
}

// Code to convert from a string to the enumeration
MachineType
string_to_MachineType(const string& str)
{
    if (str == "L1Cache") {
        return MachineType_L1Cache;
    } else if (str == "L2Cache") {
        return MachineType_L2Cache;
    } else if (str == "L3Cache") {
        return MachineType_L3Cache;
    } else if (str == "Directory") {
        return MachineType_Directory;
    } else if (str == "DMA") {
        return MachineType_DMA;
    } else if (str == "PMMU") {
        return MachineType_PMMU;
    } else if (str == "Collector") {
        return MachineType_Collector;
    } else if (str == "L1Cache_wCC") {
        return MachineType_L1Cache_wCC;
    } else if (str == "L2Cache_wCC") {
        return MachineType_L2Cache_wCC;
    } else if (str == "CorePair") {
        return MachineType_CorePair;
    } else if (str == "TCP") {
        return MachineType_TCP;
    } else if (str == "TCC") {
        return MachineType_TCC;
    } else if (str == "TCCdir") {
        return MachineType_TCCdir;
    } else if (str == "SQC") {
        return MachineType_SQC;
    } else if (str == "RegionDir") {
        return MachineType_RegionDir;
    } else if (str == "RegionBuffer") {
        return MachineType_RegionBuffer;
    } else if (str == "NULL") {
        return MachineType_NULL;
    } else {
        panic("Invalid string conversion for %s, type MachineType", str);
    }
}

// Code to increment an enumeration type
MachineType&
operator++(MachineType& e)
{
    assert(e < MachineType_NUM);
    return e = MachineType(e+1);
}
/** \brief returns the base vector index for each machine type to be
  * used by NetDest
  *
  * \return the base vector index for each machine type to be used by NetDest
  * \see NetDest.hh
  */
int
MachineType_base_level(const MachineType& obj)
{
    switch(obj) {
      case MachineType_L1Cache:
        return 0;
      case MachineType_L2Cache:
        return 1;
      case MachineType_L3Cache:
        return 2;
      case MachineType_Directory:
        return 3;
      case MachineType_DMA:
        return 4;
      case MachineType_PMMU:
        return 5;
      case MachineType_Collector:
        return 6;
      case MachineType_L1Cache_wCC:
        return 7;
      case MachineType_L2Cache_wCC:
        return 8;
      case MachineType_CorePair:
        return 9;
      case MachineType_TCP:
        return 10;
      case MachineType_TCC:
        return 11;
      case MachineType_TCCdir:
        return 12;
      case MachineType_SQC:
        return 13;
      case MachineType_RegionDir:
        return 14;
      case MachineType_RegionBuffer:
        return 15;
      case MachineType_NULL:
        return 16;
      case MachineType_NUM:
        return 17;

      default:
        panic("Invalid range for type MachineType");
    }
}

/** \brief returns the machine type for each base vector index used by NetDest
 *
 * \return the MachineType
 */
MachineType
MachineType_from_base_level(int type)
{
    switch(type) {
      case 0:
        return MachineType_L1Cache;
      case 1:
        return MachineType_L2Cache;
      case 2:
        return MachineType_L3Cache;
      case 3:
        return MachineType_Directory;
      case 4:
        return MachineType_DMA;
      case 5:
        return MachineType_PMMU;
      case 6:
        return MachineType_Collector;
      case 7:
        return MachineType_L1Cache_wCC;
      case 8:
        return MachineType_L2Cache_wCC;
      case 9:
        return MachineType_CorePair;
      case 10:
        return MachineType_TCP;
      case 11:
        return MachineType_TCC;
      case 12:
        return MachineType_TCCdir;
      case 13:
        return MachineType_SQC;
      case 14:
        return MachineType_RegionDir;
      case 15:
        return MachineType_RegionBuffer;
      case 16:
        return MachineType_NULL;
      default:
        panic("Invalid range for type MachineType");
    }
}

/** \brief The return value indicates the number of components created
 * before a particular machine's components
 *
 * \return the base number of components for each machine
 */
int
MachineType_base_number(const MachineType& obj)
{
    int base = 0;
    switch(obj) {
      case MachineType_NUM:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_NULL:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_RegionBuffer:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_RegionDir:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_SQC:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_TCCdir:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_TCC:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_TCP:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_CorePair:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_L2Cache_wCC:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_L1Cache_wCC:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_Collector:
        base += PMMU::getNumControllers();
        M5_FALLTHROUGH;
      case MachineType_PMMU:
        base += DMA_Controller::getNumControllers();
        M5_FALLTHROUGH;
      case MachineType_DMA:
        base += Directory_Controller::getNumControllers();
        M5_FALLTHROUGH;
      case MachineType_Directory:
        base += 0;
        M5_FALLTHROUGH;
      case MachineType_L3Cache:
        base += L2Cache_Controller::getNumControllers();
        M5_FALLTHROUGH;
      case MachineType_L2Cache:
        base += L1Cache_Controller::getNumControllers();
        M5_FALLTHROUGH;
      case MachineType_L1Cache:
        break;
      default:
        panic("Invalid range for type MachineType");
    }

    return base;
}

/** \brief returns the total number of components for each machine
 * \return the total number of components for each machine
 */
int
MachineType_base_count(const MachineType& obj)
{
    switch(obj) {
case MachineType_L1Cache:
return L1Cache_Controller::getNumControllers();
case MachineType_L2Cache:
return L2Cache_Controller::getNumControllers();
case MachineType_L3Cache:
return 0;
case MachineType_Directory:
return Directory_Controller::getNumControllers();
case MachineType_DMA:
return DMA_Controller::getNumControllers();
case MachineType_PMMU:
return PMMU::getNumControllers();
case MachineType_Collector:
return 0;
case MachineType_L1Cache_wCC:
return 0;
case MachineType_L2Cache_wCC:
return 0;
case MachineType_CorePair:
return 0;
case MachineType_TCP:
return 0;
case MachineType_TCC:
return 0;
case MachineType_TCCdir:
return 0;
case MachineType_SQC:
return 0;
case MachineType_RegionDir:
return 0;
case MachineType_RegionBuffer:
return 0;
case MachineType_NULL:
return 0;
      case MachineType_NUM:
      default:
        panic("Invalid range for type MachineType");
    }
}
/*
MachineID
getL1CacheMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_L1Cache, RubyNode};
      return mach;
}

MachineID
getL2CacheMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_L2Cache, RubyNode};
      return mach;
}

MachineID
getL3CacheMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_L3Cache, RubyNode};
      return mach;
}

MachineID
getDirectoryMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_Directory, RubyNode};
      return mach;
}
MachineID
map_Address_to_DMA(const Addr &addr)
{
      MachineID dma = {MachineType_DMA, 0};
      return dma;
}

MachineID
getDMAMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_DMA, RubyNode};
      return mach;
}

MachineID
getCollectorMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_Collector, RubyNode};
      return mach;
}

MachineID
getL1Cache_wCCMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_L1Cache_wCC, RubyNode};
      return mach;
}

MachineID
getL2Cache_wCCMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_L2Cache_wCC, RubyNode};
      return mach;
}

MachineID
getCorePairMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_CorePair, RubyNode};
      return mach;
}

MachineID
getTCPMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_TCP, RubyNode};
      return mach;
}

MachineID
getTCCMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_TCC, RubyNode};
      return mach;
}

MachineID
getTCCdirMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_TCCdir, RubyNode};
      return mach;
}

MachineID
getSQCMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_SQC, RubyNode};
      return mach;
}

MachineID
getRegionDirMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_RegionDir, RubyNode};
      return mach;
}

MachineID
getRegionBufferMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_RegionBuffer, RubyNode};
      return mach;
}

MachineID
getNULLMachineID(NodeID RubyNode)
{
      MachineID mach = {MachineType_NULL, RubyNode};
      return mach;
}
*/
