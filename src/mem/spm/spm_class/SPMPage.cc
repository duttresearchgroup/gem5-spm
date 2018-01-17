#include "mem/spm/spm_class/SPMPage.hh"
#include "mem/spm/pmmu.hh"
#include "mem/spm/governor/GOVRequest.hh"
#include <random>
#include <iostream>

void
SPMPage::obtainPageSize()
{
    pageSize = PMMU::getPageSizeBytes();
}

void
SPMPage::alloc()
{
    m_data = new uint8_t[pageSize];
    m_alloc = true;
    clear();
}

void
SPMPage::clear()
{
    memset(m_data, 0, pageSize);
}

bool
SPMPage::equal(const SPMPage& obj) const
{
    return !memcmp(m_data, obj.m_data, pageSize);
}

void
SPMPage::print(std::ostream& out) const
{
    using namespace std;

    int size = pageSize;
    out << "[ ";
    for (int i = 0; i < size; i++) {
        out << setw(2) << setfill('0') << hex << "0x" << (int)m_data[i] << " ";
        out << setfill(' ');
    }
    out << dec << "]" << flush;
}

const uint8_t*
SPMPage::getData(int offset, int len) const
{
    assert(offset + len <= pageSize);
    return &m_data[offset];
}

void
SPMPage::setData(uint8_t *data, int offset, int len)
{
    assert(offset + len <= pageSize);
    memcpy(&m_data[offset], data, len);
}

void
SPMPage::performRead(PacketPtr pkt)
{
    assert(isValid());
    assert(annotations);
    assert(pkt->getOffset(pageSize) + pkt->getSize() <= pageSize);

    pkt->setDataFromBlock(m_data, pageSize);

    // this injects fault into the data after it is from SPM
    // but not the data stored in the SPM itself
    if (annotations->approximation != CRITICAL && getReadBER() != 0 && !pkt->govInfo.isGOVReq()) {
        if (injectFault(pkt->getPtr<uint8_t>(), pkt->getSize(), getReadBER())) {
        }
    }
}

void
SPMPage::performWrite(PacketPtr pkt)
{
    assert(annotations);
    assert(pkt->getOffset(pageSize) + pkt->getSize() <= pageSize);

    pkt->writeDataToBlock(m_data, pageSize);

    // this injects fault into data stored in the SPM
    if (annotations->approximation != CRITICAL && getWriteBER() != 0 && !pkt->govInfo.isGOVReq()) {
        if (injectFault(m_data + pkt->getOffset(pageSize), pkt->getSize(), getWriteBER())) {
            status |= PageFaulty;
        }
    }
}

SPMPage &
SPMPage::operator=(const SPMPage & obj)
{
//	assert (obj);
    assert (obj.m_data);
    memcpy(m_data, obj.m_data, pageSize);
    return *this;
}

bool
SPMPage::isValid() const
{
    return (status & PageValid) != 0;
}

void
SPMPage::validate()
{
    status |= PageValid;
}

void
SPMPage::invalidate(bool delete_annotations)
{
    status = 0;
    if (delete_annotations) {
        delete annotations;
        annotations = nullptr;
    }
    clearLoadLocks();
}

bool
SPMPage::isDirty() const
{
    return (status & PageDirty) != 0;
}

bool
SPMPage::isReferenced() const
{
    return (status & PageReferenced) != 0;
}

bool
SPMPage::isFaulty() const
{
    return (status & PageFaulty) != 0;
}

void
SPMPage::setOccupancy(SPMSlotOccupancyStatus _occupancy)
{
    occupancy = _occupancy;
}

void
SPMPage::setFree()
{
    occupancy = FREE_SLOT;
}

void
SPMPage::setUsed()
{
    occupancy = USED_SLOT;
}

SPMSlotOccupancyStatus
SPMPage::getOccupancy()
{
    return occupancy;
}

bool
SPMPage::isFree()
{
    return occupancy == FREE_SLOT;
}

void
SPMPage::setSpeed (Cycles _writeLatency, Cycles _readLatency)
{
    writeLatency = _writeLatency;
    readLatency = _readLatency;
}

Cycles
SPMPage::getReadSpeed()
{
    return readLatency;
}

Cycles
SPMPage::getWriteSpeed()
{
    return writeLatency;
}

float
SPMPage::getReadEnergy()
{
    return berEnergyData[operatingPoint].read_access_energy;
}

float
SPMPage::getWriteEnergy()
{
    return berEnergyData[operatingPoint].write_access_energy;
}

void
SPMPage::overrideActiveBERPoint(double read_ber, double write_ber)
{
    BEREnergyData info;
    info.read_ber = read_ber;
    info.write_ber = write_ber;
    berEnergyData.push_back(info);
    setOperatingPoint(berEnergyData.size() - 1);
}

double
SPMPage::getReadBER()
{
    return berEnergyData[operatingPoint].read_ber;
}

double
SPMPage::getWriteBER()
{
    return berEnergyData[operatingPoint].write_ber;
}


bool
SPMPage::injectFault(uint8_t *data, int len, double ber)
{
    std::mt19937_64 rng;
    //std::default_random_engine generator;
    // initialize the random number generator with time-dependent seed
    uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    rng.seed(ss);
    std::uniform_real_distribution<double> distribution(0.0,1.0);

    bool fault_injected = false;
    for (int byte = 0; byte < len; byte++) {
        uint8_t bit_mask = 0x80;
        for (int bit = 0; bit < 8; bit++) {
            double random_probability = distribution(rng);
            if(random_probability < ber) {
                uint8_t fault_mask = bit_mask >> bit;
                if (fault_mask != 0) {
                    data[byte] = data[byte] ^ fault_mask;
                    fault_injected = true;
                }
            }
        }
    }
    return fault_injected;
}

void
SPMPage::setOwner (NodeID owner)
{
    ownerNode = owner;
}

NodeID
SPMPage::getOwner()
{
    return ownerNode;
}

void
SPMPage::setAnnotations(Annotations *_annotations)
{
    assert(_annotations);
    annotations = _annotations;
}

Annotations*
SPMPage::getAnnotations()
{
    assert(annotations);
    return annotations;
}

void
SPMPage::resetStatsForNewlyAddedPage()
{
    status = 0;
    validate();
    tickInserted = curTick();
    readRefCount = 0;
    writeRefCount = 0;
    lastTickAccessed = 0;
}

void
SPMPage::readRefOccurred()
{
    isValid();
    status |= PageReferenced;
    readRefCount += 1;
    lastTickAccessed = curTick();
}

void
SPMPage::writeRefOccurred()
{
    isValid();
    status |= PageReferenced;
    status |= PageDirty;
    writeRefCount += 1;
    lastTickAccessed = curTick();
}

uint64_t
SPMPage::getRefCount()
{
    return readRefCount + writeRefCount;
}

Tick
SPMPage::getLastTickAccessed()
{
    return lastTickAccessed;
}

float
SPMPage::pageUtlization()
{
    Tick age = curTick() - tickInserted;
    if (age == 0)
        return 0;
    else
        return ((float)getRefCount())/((float)age);
}

/////////////////////
//
// LLSC stuff
//
/////////////////////

/**
 * Track the fact that a local locked was issued to the page.  If
 * multiple LLs get issued from the same context we could have
 * redundant records on the list, but that's OK, as they'll all
 * get blown away at the next store.
 */
void
SPMPage::trackLoadLocked(PacketPtr pkt)
{
    assert(pkt->isLLSC());
    lockList.emplace_front(pkt->req);
}

/**
 * Clear the list of valid load pages.  Should be called whenever
 * page is written to or invalidated.
 */
void
SPMPage::clearLoadLocks(RequestPtr req)
{
    if (!req) {
        // No request, invaldate all locks to this line
        lockList.clear();
    } else {
        // Only invalidate locks that overlap with this request
        auto lock_itr = lockList.begin();
        while (lock_itr != lockList.end()) {
            if (lock_itr->overlapping(req)) {
                lock_itr = lockList.erase(lock_itr);
            } else {
                ++lock_itr;
            }
        }
    }
}

bool
SPMPage::checkWrite(PacketPtr pkt)
{
    // common case
    if (!pkt->isLLSC() && lockList.empty())
        return true;

    RequestPtr req = pkt->req;

    if (pkt->isLLSC()) {
        // it's a store conditional... have to check for matching
        // load locked.
        bool success = false;
        for (const auto& l : lockList) {
            if (l.matchesContext(req)) {
                // it's a store conditional, and as far as the memory
                // system can tell, the requesting context's lock is
                // still valid.
                success = true;
                break;
            }
        }

        req->setExtraData(success ? 1 : 0);
        clearLoadLocks(req);
        return success;
    } else {
        // for *all* stores (conditional or otherwise) we have to
        // clear the list of load-locks as they're all invalid now.
        clearLoadLocks(req);
        return true;
    }
}
