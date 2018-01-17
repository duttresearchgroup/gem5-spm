#ifndef __SPMMessageTypes_HH__
#define __SPMMessageTypes_HH__

enum SPMRequestType {
    SPMRequestType_FIRST,
    SPMRequestType_READ = SPMRequestType_FIRST, /* Read access to the SPM's data array */
    SPMRequestType_WRITE,                       /* Write access to the SPM's data array */
    SPMRequestType_ALLOC,                       /* Alloc request to remote SPM */
    SPMRequestType_DEALLOC,                     /* DeAlloc request to remote SPM */
    SPMRequestType_RELOCATE_READ,               /* Relocate request to another SPM - read part*/
    SPMRequestType_RELOCATE_WRITE,              /* Relocate request to another SPM - write part*/
    SPMRequestType_NUM
};

enum SPMResponseType {
    SPMResponseType_FIRST,
    SPMResponseType_WRITE_ACK = SPMResponseType_FIRST, /* ACKnowledgement of write request */
    SPMResponseType_DATA,                              /* Data for read request */
    SPMResponseType_GOV_ACK,                           /* ACK for governor request */
    SPMResponseType_RELOCATION_HALFWAY,                /* Read part of relocation request is done */
    SPMResponseType_RELOCATION_DONE,                   /* Write part of relocation request is done */
    SPMResponseType_NUM
};

// Code to convert from a string to the enumeration
SPMRequestType string_to_SPMRequestType(const std::string& str);

// Code to convert state to a string
std::string SPMRequestType_to_string(const SPMRequestType& obj);

// Code to increment an enumeration type
SPMRequestType &operator++(SPMRequestType &e);
std::ostream& operator<<(std::ostream& out, const SPMRequestType& obj);

// Code to convert from a string to the enumeration
SPMResponseType string_to_SPMResponseType(const std::string& str);

// Code to convert state to a string
std::string SPMResponseType_to_string(const SPMResponseType& obj);

// Code to increment an enumeration type
SPMResponseType &operator++(SPMResponseType &e);
std::ostream& operator<<(std::ostream& out, const SPMResponseType& obj);

#endif /* __SPMMessageTypes_HH__ */
