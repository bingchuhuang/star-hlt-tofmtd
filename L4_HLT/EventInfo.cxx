#include "EventInfo.h"

ClassImp(EventInfo);

EventInfo::EventInfo()
{
    tpcSectorMask = 0xFFFFFF;
    tpcNHits = 0;
    tpcNTracks = 0;
    processTime = 0;
}

EventInfo::~EventInfo() {}

EventInfo& EventInfo::Instance()
{
    static EventInfo g;
    return g;
}

void EventInfo::FillEventInfo(Int_t ec, UInt_t eId, UInt_t tok, UInt_t tcmd,
                              UInt_t qcmd, UInt_t eTime, UInt_t dec, Int_t sta,
                              ULong64_t l1, ULong64_t l2, ULong64_t daqbits)
{
    eventCount   = ec;
    eventId      = eId;
    token        = tok;
    trgcmd       = tcmd;
    daqcmd       = qcmd;
    evt_time     = eTime;
    detectors    = dec;
    status       = sta;
    daqbits64_l1 = l1;
    daqbits64_l2 = l2;
    daqbits64    = daqbits;
}

void EventInfo::Reset() {
    eventCount   = 0;
    eventId      = 0;
    token        = 0;
    trgcmd       = 0;
    daqcmd       = 0;
    evt_time     = 0;
    detectors    = 0;
    status       = 0;
    daqbits64_l1 = 0;
    daqbits64_l2 = 0;
    daqbits64    = 0;

    tpcSectorMask = 0xFFFFFF;
    tpcNHits = 0;
    tpcNTracks = 0;
    processTime = 0;
}

// global function
std::ostream& operator << (std::ostream& os, const EventInfo& evt)
{
    os << std::setfill('0')
       << "0x" << std::hex << std::setw(16) << evt.daqbits64 << "\t"
       << "0x" << std::hex << std::setw(16) << evt.tpcSectorMask << "\t"
       << std::setfill(' ') << std::fixed
       << std::dec << std::setw(10) << evt.tpcNHits << "\t"
       << std::dec << std::setw(15) << evt.processTime << std::endl;

    return os;
}
