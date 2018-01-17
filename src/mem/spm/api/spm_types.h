#ifndef SPM_TYPES_H_
#define SPM_TYPES_H_

#ifdef ARM32_ARCH
#define SPM_ALLOC SPM_ALLOC32
#define SPM_FREE SPM_FREE32
#endif

#ifdef ARM64_ARCH
#define SPM_ALLOC SPM_ALLOC64
#define SPM_FREE SPM_FREE64
#endif

#ifdef ALPHA_ARCH
#define SPM_ALLOC SPM_ALLOC64
#define SPM_FREE SPM_FREE64
#endif

#ifdef X86_ARCH
#define SPM_ALLOC SPM_ALLOC64
#define SPM_FREE SPM_FREE64
#endif

#define SHARED_DATA 1
#define PRIVATE_DATA 0

typedef enum
{
    CRITICAL = 0x00,
    APPROXIMATE = 0xFF,
    NUM_APPROXIMATION
} Approximation;

typedef enum
{
    MAX_IMPORTANCE,
    AVG_IMPORTANCE,
    MIN_IMPORTANCE,
    NUM_DATA_IMPORTANCE
} DataImportance;

typedef enum
{
    HIGH_PRIORITY,
    MID_PRIORITY,
    LOW_PRIORITY,
    NUM_APP_PRIORITY
} ThreadPriority;

typedef enum
{
    COPY,
    UNINITIALIZE,
    NUM_ALLOCATION_MODE
} AllocationModes;

typedef enum
{
    WRITE_BACK,
    DISCARD,
    NUM_DEALLOCATION_MODE
} DeallocationModes;


#endif /* SPM_TYPES_H_ */
