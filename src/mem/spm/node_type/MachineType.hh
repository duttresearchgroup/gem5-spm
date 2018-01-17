/** \file MachineType.hh
 *
 * Replaces auto-generated code to add PMMU MachineType support for SPM-based NoCs
 */

#ifndef __MachineType_HH__
#define __MachineType_HH__

#include <iostream>
#include <string>

#include <functional>
#include "base/logging.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/common/TypeDefines.hh"
struct MachineID;

// Class definition
/** \enum MachineType
 *  \brief ...
 */
enum MachineType {
    MachineType_FIRST,
    MachineType_L1Cache = MachineType_FIRST, /**< L1 Cache Mach */
    MachineType_L2Cache, /**< L2 Cache Mach */
    MachineType_L3Cache, /**< L3 Cache Mach */
    MachineType_Directory, /**< Directory Mach */
    MachineType_DMA, /**< DMA Mach */
    MachineType_PMMU, /**< PMMU (for SPM) Mach */
    MachineType_Collector, /**< Collector Mach */
    MachineType_L1Cache_wCC, /**< L1 Cache Mach to track cache-to-cache transfer (used for miss latency profile) */
    MachineType_L2Cache_wCC, /**< L2 Cache Mach to track cache-to-cache transfer (used for miss latency profile) */
    MachineType_CorePair, /**< Cache Mach (2 cores, Private L1Ds, Shared L1I & L2) */
    MachineType_TCP, /**< GPU L1 Data Cache (Texture Cache per Pipe) */
    MachineType_TCC, /**< GPU L2 Shared Cache (Texture Cache per Channel) */
    MachineType_TCCdir, /**< Directory at the GPU L2 Cache (TCC) */
    MachineType_SQC, /**< GPU L1 Instr Cache (Sequencer Cache) */
    MachineType_RegionDir, /**< Region-granular directory */
    MachineType_RegionBuffer, /**< Region buffer for CPU and GPU */
    MachineType_NULL, /**< null mach type */
    MachineType_NUM
};

// Code to convert from a string to the enumeration
MachineType string_to_MachineType(const std::string& str);

// Code to convert state to a string
std::string MachineType_to_string(const MachineType& obj);

// Code to increment an enumeration type
MachineType &operator++(MachineType &e);

// define a hash function for the MachineType class
namespace std {
template<>
struct hash<MachineType> {
    std::size_t operator()(const MachineType &mtype) const {
        return hash<size_t>()(static_cast<size_t>(mtype));
    }
};
}

int MachineType_base_level(const MachineType& obj);
MachineType MachineType_from_base_level(int);
int MachineType_base_number(const MachineType& obj);
int MachineType_base_count(const MachineType& obj);
/*
MachineID getL1CacheMachineID(NodeID RubyNode);

MachineID getL2CacheMachineID(NodeID RubyNode);

MachineID getL3CacheMachineID(NodeID RubyNode);

MachineID getDirectoryMachineID(NodeID RubyNode);
MachineID map_Address_to_DMA(const Addr &addr);

MachineID getDMAMachineID(NodeID RubyNode);

MachineID getCollectorMachineID(NodeID RubyNode);

MachineID getL1Cache_wCCMachineID(NodeID RubyNode);

MachineID getL2Cache_wCCMachineID(NodeID RubyNode);

MachineID getCorePairMachineID(NodeID RubyNode);

MachineID getTCPMachineID(NodeID RubyNode);

MachineID getTCCMachineID(NodeID RubyNode);

MachineID getTCCdirMachineID(NodeID RubyNode);

MachineID getSQCMachineID(NodeID RubyNode);

MachineID getRegionDirMachineID(NodeID RubyNode);

MachineID getRegionBufferMachineID(NodeID RubyNode);

MachineID getNULLMachineID(NodeID RubyNode);
*/
std::ostream& operator<<(std::ostream& out, const MachineType& obj);

#endif // __MachineType_HH__
