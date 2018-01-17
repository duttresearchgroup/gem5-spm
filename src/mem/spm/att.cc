#include "mem/spm/att.hh"
#include "debug/PMMU.hh"
#include "sim/system.hh"

ATT::ATT()
{
}

ATT::~ATT()
{
    std::map<Addr,ATTEntry*>::iterator it;
    for (it = translation_table.begin(); it != translation_table.end(); it++) {
        delete(it->second);
    }
    translation_table.clear();
}

bool
ATT::hasMapping(Addr v_page_addr)
{
    return (translation_table.find(v_page_addr) != translation_table.end());
}

ATTEntry*
ATT::getMapping(Addr v_page_addr)
{
    if (hasMapping (v_page_addr))
        return translation_table.at(v_page_addr);
    else
        return NULL;
}

ATTEntry*
ATT::getMapping(MachineID destination_node, Addr spm_slot_addr)
{
    return getMapping(slot2Addr(destination_node, spm_slot_addr));
}

bool
ATT::isHit(Addr v_page_addr)
{
    if (!hasMapping(v_page_addr)) {
        return false;
    }
    else {
        ATTEntry *mapping = getMapping(v_page_addr);
        bool hit = mapping->data_valid;
        if (hit) {
            mapping->num_accesses++;
        }
        return hit;
    }
}

bool
ATT::addMapping(Addr v_page_addr, MachineID destination_node, Addr spm_slot_addr, Annotations *annotations)
{
    ATTEntry *new_entry;

    if (hasMapping(v_page_addr)) {
        new_entry = getMapping(v_page_addr);
        new_entry->num_owners++;
        return false;
    }
    else if (!isATTFull()) {
        new_entry = new ATTEntry;
        new_entry->destination_node = destination_node;
        new_entry->spm_slot_addr = spm_slot_addr;
        new_entry->num_owners = 1;
        new_entry->num_accesses = 0;
        new_entry->data_valid = false;
        *new_entry->annotations = *annotations;
        translation_table[v_page_addr] = new_entry;
        reverse_translation_table[new_entry] = v_page_addr;
        return true;
    }
    else {
        return false;
    }
}

bool
ATT::addMapping(GOVRequest *gov_request, HostInfo *host_info, int page_index)
{
    return addMapping(gov_request->getNthPageStartAddr(Unserved_Aligned, page_index),
                      host_info->getHostMachineID(),
                      host_info->getSPMaddress() + page_index * gov_request->getPageSizeBytes(),
                      gov_request->getAnnotations());
}

bool
ATT::removeMapping(Addr v_page_addr)
{
    ATTEntry *ret = getMapping(v_page_addr);

    if (ret->num_owners > 1) {
        ret->num_owners--;
        return false;
    }
    else {
        assert (ret->num_owners == 1);
        reverse_translation_table.erase(ret);
        translation_table.erase(v_page_addr);
        delete ret;
        return true;
    }
}

bool
ATT::removeMapping(GOVRequest *gov_request, int page_index)
{
    return removeMapping(gov_request->getNthPageStartAddr(Unserved_Aligned, page_index));
}

Addr
ATT::slot2Addr(MachineID destination_node, Addr spm_slot_addr)
{
    ATTEntry search_entry;
    search_entry.destination_node = destination_node;
    search_entry.spm_slot_addr = spm_slot_addr;

    for(std::map<ATTEntry*, Addr >::const_iterator it = reverse_translation_table.begin();
        it != reverse_translation_table.end(); ++it) {
        if (*it->first == search_entry) {
            return it->second;
        }
    }

    panic("ATT entry can not be mapped to a virtual address\n");
}

void
ATT::validateATTEntry(ATTEntry *mapping)
{
    mapping->data_valid = true;
}

void
ATT::invalidateATTEntry(ATTEntry *mapping)
{
    mapping->data_valid = false;
}

bool
ATT::isATTEntryValid(ATTEntry *mapping)
{
    return mapping->data_valid;
}

ATTEntry*
ATT::changeMapping(Addr v_page_addr, MachineID new_destination_node, Addr new_spm_slot_addr)
{
    if (hasMapping(v_page_addr)) {
        ATTEntry* mapping = getMapping(v_page_addr);
        mapping->destination_node = new_destination_node;
        mapping->spm_slot_addr = new_spm_slot_addr;
        return mapping;
    }
    else {
        return NULL;
    }
}

ATTEntry*
ATT::changeMapping(MachineID destination_node, Addr spm_slot_addr,
                   MachineID new_destination_node, Addr new_spm_slot_addr)
{
    return changeMapping(slot2Addr(destination_node, spm_slot_addr),
                         new_destination_node, new_spm_slot_addr);
}

unsigned int
ATT::getTableSize()
{
    return translation_table.size();
}

bool
ATT::isATTFull()
{
    return getTableSize() >= table_capacity;
}

void
ATT::invalidateATT()
{
    translation_table.clear();
    reverse_translation_table.clear();
}

void
ATT::dump()
{
    DPRINTF(ATTMap,"\n");
    for(std::map<Addr, ATTEntry*>::const_iterator it = translation_table.begin();
        it != translation_table.end(); ++it) {
        DPRINTF(ATTMap, "0x%16x %d %d\n",
                it->first,
                (*it->second).destination_node,
                (*it->second).spm_slot_addr);
    }
}
