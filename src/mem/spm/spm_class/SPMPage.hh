#ifndef __MEM_SPMPAGE_HH__
#define __MEM_SPMPAGE_HH__

#include "base/types.hh"
#include "mem/packet.hh"
#include "mem/ruby/common/TypeDefines.hh"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <inttypes.h>

class Annotations;

//TODO: Unused for now -- just placeholder
enum SPMPageStatusBits : unsigned {
    /** valid, readable */
    PageValid =          0x01,
    /** write permission */
    PageWritable =       0x02,
    /** read permission (yes, page can be valid but not readable) */
    PageReadable =       0x04,
    /** dirty (modified) */
    PageDirty =          0x08,
    /** page was referenced */
    PageReferenced =     0x10,
    /** page was a hardware prefetch yet unaccessed*/
    PageHWPrefetched =   0x20,
    /** page holds data from the secure memory space */
    PageSecure =         0x40,
    /** page holds faulty data */
    PageFaulty =         0x80
};

enum SPMSlotOccupancyStatus {
    FREE_SLOT = 0,
    USED_SLOT = 1,
    NUM_SLOT_OCCUPANCY
};

typedef struct BEREnergyData
{
    double knob1;  // voltage or read current
    double knob2;  // none or write current
    double read_ber;
    double write_ber;
    double read_access_energy;
    double write_access_energy;
    double leakage_power;

    BEREnergyData()
      : knob1(0)
      , knob2(0)
      , read_ber(0)
      , write_ber(0)
      , read_access_energy(0)
      , write_access_energy(0)
      , leakage_power(0)
    {}
}BEREnergyData;

class SPMPage
{
  public:

    SPMSlotOccupancyStatus occupancy;

  public:
    SPMPage()
    {
        init();
    }

    ~SPMPage()
    {
        if (m_alloc)
            delete [] m_data;
    }

    void init()
    {
        obtainPageSize();
        alloc();
        status = 0;
        setOccupancy(FREE_SLOT);
        setSpeed(Cycles(0), Cycles(0));
        readRefCount = 0;
        writeRefCount = 0;
        tickInserted = 0;
        lastTickAccessed = 0;
        operatingPoint = 0;
    }

    void readBERInfo(std::string ber_energy_file)
    {
        if (ber_energy_file.compare("") == 0) {
            overrideActiveBERPoint(0,0);
            return;
        }

        std::ifstream file;
        file.open(ber_energy_file.c_str());
        if (file.fail())
           fatal("Failed to open this spm's ber file!\n");

        std::string element;
        getline(file, element); //throw out header row
        while (!file.eof()) {
            BEREnergyData info;
            if (getline(file,element,',')) {
                info.knob1 = atof(element.c_str());
                getline(file,element,',');
                info.knob2 = atof(element.c_str());
                getline(file,element,',');
                info.read_ber = atof(element.c_str());
                getline(file,element,',');
                info.write_ber = atof(element.c_str());
                getline(file,element,',');
                info.leakage_power = atof(element.c_str());
                getline(file,element,',');
                info.read_access_energy = atof(element.c_str());
                getline(file,element,'\n');
                info.write_access_energy = atof(element.c_str());
                berEnergyData.push_back(info);
            }
        }

        file.close();
    }

    void setOperatingPoint(int _operating_point)
    {
        assert (_operating_point < berEnergyData.size());
        operatingPoint = _operating_point;
    }

    SPMPage& operator=(const SPMPage& obj);

    void assign(uint8_t *data);

    void clear();
    uint8_t getByte(int whichByte) const;
    const uint8_t *getData(int offset, int len) const;
    void setByte(int whichByte, uint8_t data);
    void setData(uint8_t *data, int offset, int len);
    void copyPartial(const SPMPage & dblk, int offset, int len);
    bool equal(const SPMPage& obj) const;
    void print(std::ostream& out) const;

    void performRead(PacketPtr pkt);
    void performWrite(PacketPtr pkt);

    void setOccupancy (SPMSlotOccupancyStatus occupancy);
    void setUsed ();
    void setFree ();
    bool isFree();
    SPMSlotOccupancyStatus getOccupancy();

    void setSpeed (Cycles writeLatency, Cycles readLatency);
    Cycles getReadSpeed();
    Cycles getWriteSpeed();

    float getReadEnergy();
    float getWriteEnergy();

    std::vector<BEREnergyData> berEnergyData;
    int operatingPoint;
    void overrideActiveBERPoint(double read_ber, double write_ber);
    double getReadBER();
    double getWriteBER();
    bool injectFault(uint8_t *data, int len, double ber);

    void setOwner (NodeID owner);
    NodeID getOwner();

    void setAnnotations(Annotations *_annotation);
    Annotations *getAnnotations();

    uint8_t *m_data; // TODO: this needs to be made private

    /** block state: OR of SPMPageStatusBits */
    typedef unsigned State;
    /** The current status of this block. @sa SPMPageStatusBits */
    State status;

    bool isValid() const;
    void validate();
    void invalidate(bool delete_annotations);
    bool isDirty() const;
    bool isReferenced() const;
    bool isFaulty() const;

  private:
    Cycles writeLatency;
    Cycles readLatency;

    Annotations *annotations;
    NodeID ownerNode;

    int pageSize;
    void obtainPageSize();

    /** Number of references to this page since it was brought in. */
    uint64_t readRefCount;
    uint64_t writeRefCount;
    /** Last time this page was accessed */
    Tick lastTickAccessed;
    /** When this page was inserted */
    Tick tickInserted;

  public:
    void resetStatsForNewlyAddedPage();
    void readRefOccurred();
    void writeRefOccurred();
    uint64_t getRefCount();
    Tick getLastTickAccessed();
    float pageUtlization();

  private:
    /**
     * Represents that the indicated thread context has a "lock" on
     * the page, in the LL/SC sense. Copied form blk.hh
     */
    class Lock {
      public:
        ContextID contextId;     // locking context
        Addr lowAddr;      // low address of lock range
        Addr highAddr;     // high address of lock range

        // check for matching execution context
        bool matchesContext(const RequestPtr req) const
        {
            Addr req_low = req->getPaddr();
            Addr req_high = req_low + req->getSize() -1;
            return (contextId == req->contextId()) &&
                   (req_low >= lowAddr) && (req_high <= highAddr);
        }

        bool overlapping(const RequestPtr req) const
        {
            Addr req_low = req->getPaddr();
            Addr req_high = req_low + req->getSize() - 1;

            return (req_low <= highAddr) && (req_high >= lowAddr);
        }

        Lock(const RequestPtr req)
            : contextId(req->contextId()),
              lowAddr(req->getPaddr()),
              highAddr(lowAddr + req->getSize() - 1)
        {
        }
    };

    /** List of thread contexts that have performed a load-locked (LL)
     * on the block since the last store. */
    std::list<Lock> lockList;

  public:
    void trackLoadLocked(PacketPtr pkt);
    void clearLoadLocks(RequestPtr req = nullptr);
    bool checkWrite(PacketPtr pkt);

  private:
    void alloc();
    bool m_alloc;
};

inline void
SPMPage::assign(uint8_t *data)
{
    assert(data != NULL);
    if (m_alloc) {
        delete [] m_data;
    }
    m_data = data;
    m_alloc = false;
}

inline uint8_t
SPMPage::getByte(int whichByte) const
{
    return m_data[whichByte];
}

inline void
SPMPage::setByte(int whichByte, uint8_t data)
{
    m_data[whichByte] = data;
}

inline void
SPMPage::copyPartial(const SPMPage & dblk, int offset, int len)
{
    setData(&dblk.m_data[offset], offset, len);
}

inline std::ostream&
operator<<(std::ostream& out, const SPMPage& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

inline bool
operator==(const SPMPage& obj1,const SPMPage& obj2)
{
    return obj1.equal(obj2);
}

#endif // __MEM_SPMPAGE_HH__
