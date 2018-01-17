#ifndef __ATT_HH__
#define __ATT_HH__

#include <limits>
#include <map>
#include "base/types.hh"
#include "debug/ATT.hh"
#include "mem/ruby/common/MachineID.hh"
#include "mem/spm/governor/GOVRequest.hh"

#define UINT_MAXIMUM std::numeric_limits<unsigned int>::max()

typedef struct ATTEntry
{
    MachineID destination_node;
    Addr spm_slot_addr;
    unsigned int num_owners;
    unsigned int num_accesses;
    bool data_valid;
    Annotations *annotations;

    ATTEntry()
    {
        annotations = new Annotations; // freed in SPMPage, if unallocated in PMMU
    }

    bool operator==(const ATTEntry &o) const
    {
        return (destination_node.num == o.destination_node.num) &&
               (spm_slot_addr == o.spm_slot_addr);
    }
} ATTEntry;

class ATT
{

  public:
    ATT();
    ~ATT();

    // checks if mapping exists for virtual page address
    // all other functions assume this is called before accessing table
    bool hasMapping(Addr v_page_addr);

    ATTEntry* getMapping(Addr v_page_addr);
    ATTEntry* getMapping(MachineID destination_node, Addr spm_slot_addr);

    bool isHit(Addr v_page_addr);

    bool addMapping(Addr v_page_addr, MachineID destination_node,
                    Addr spm_slot_addr, Annotations *annotations);
    bool addMapping(GOVRequest *gov_request, HostInfo *host_info, int page_index);

    bool removeMapping(Addr v_page_addr);
    bool removeMapping(GOVRequest *gov_request, int page_index);

    ATTEntry *changeMapping(Addr v_page_addr,
                            MachineID new_destination_node, Addr new_spm_slot_addr);
    ATTEntry *changeMapping(MachineID destination_node, Addr spm_slot_addr,
                            MachineID new_destination_node, Addr new_spm_slot_addr);
    Addr slot2Addr(MachineID destination_node, Addr spm_slot_addr);

    void validateATTEntry(ATTEntry *mapping);
    void invalidateATTEntry(ATTEntry *mapping);
    bool isATTEntryValid(ATTEntry *mapping);

    unsigned int getTableSize();
    bool isATTFull();

    void invalidateATT();
    void dump();

  public:
    std::map<Addr, ATTEntry*> translation_table;
    std::map<ATTEntry*, Addr> reverse_translation_table;

    static const unsigned int table_capacity = UINT_MAXIMUM; //TODO: make this a command line parameter
};

#endif /* __ATT_HH__ */
