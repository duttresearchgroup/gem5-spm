#include <cassert>
#include <iostream>
#include <string>

#include "base/logging.hh"
#include "mem/spm/spm_message/SPMMessageTypes.hh"

using namespace std;

// Code for output operator
ostream&
operator<<(ostream& out, const SPMRequestType& obj)
{
    out << SPMRequestType_to_string(obj);
    out << flush;
    return out;
}

// Code to convert state to a string
string
SPMRequestType_to_string(const SPMRequestType& obj)
{
    switch(obj) {
      case SPMRequestType_READ:
        return "ReadRequest";
      case SPMRequestType_WRITE:
        return "WriteRequest";
      case SPMRequestType_ALLOC:
        return "AllocRequest";
      case SPMRequestType_DEALLOC:
        return "DeAllocRequest";
      case SPMRequestType_RELOCATE_READ:
        return "RelocateRead";
      case SPMRequestType_RELOCATE_WRITE:
        return "RelocateWrite";
      default:
        panic("Invalid range for type SPMRequestType");
    }
}

// Code to convert from a string to the enumeration
SPMRequestType
string_to_SPMRequestType(const string& str)
{
    if (str == "ReadRequest") {
        return SPMRequestType_READ;
    } else if (str == "WriteRequest") {
        return SPMRequestType_WRITE;
    } else if (str == "AllocRequest") {
        return SPMRequestType_ALLOC;
    } else if (str == "DeAllocRequest") {
        return SPMRequestType_DEALLOC;
    } else if (str == "RelocateRead") {
        return SPMRequestType_RELOCATE_READ;
    } else if (str == "RelocateWrite") {
        return SPMRequestType_RELOCATE_WRITE;
    } else {
        panic("Invalid string conversion for %s, type SPMRequestType", str);
    }
}

// Code to increment an enumeration type
SPMRequestType&
operator++(SPMRequestType& e)
{
    assert(e < SPMRequestType_NUM);
    return e = SPMRequestType(e+1);
}


// Code for output operator
ostream&
operator<<(ostream& out, const SPMResponseType& obj)
{
    out << SPMResponseType_to_string(obj);
    out << flush;
    return out;
}

// Code to convert state to a string
string
SPMResponseType_to_string(const SPMResponseType& obj)
{
    switch(obj) {
      case SPMResponseType_DATA:
        return "ReadResponse";
      case SPMResponseType_WRITE_ACK:
        return "WriteResponse";
      case SPMResponseType_GOV_ACK:
        return "GovAck";
      case SPMResponseType_RELOCATION_HALFWAY:
        return "RelocationHalfway";
      case SPMResponseType_RELOCATION_DONE:
        return "RelocationDone";
      default:
        panic("Invalid range for type SPMResponseType");
    }
}

// Code to convert from a string to the enumeration
SPMResponseType
string_to_SPMResponseType(const string& str)
{
    if (str == "ReadResponse") {
        return SPMResponseType_DATA;
    } else if (str == "WriteResponse") {
        return SPMResponseType_WRITE_ACK;
    } else if (str == "GovAck") {
        return SPMResponseType_GOV_ACK;
    } else if (str == "RelocationHalfway") {
        return SPMResponseType_RELOCATION_HALFWAY;
    } else if (str == "RelocationDone") {
        return SPMResponseType_RELOCATION_DONE;
    } else {
        panic("Invalid string conversion for %s, type SPMResponseType", str);
    }
}

// Code to increment an enumeration type
SPMResponseType&
operator++(SPMResponseType& e)
{
    assert(e < SPMResponseType_NUM);
    return e = SPMResponseType(e+1);
}
