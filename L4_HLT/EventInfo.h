#ifndef _EVENTINFO_H_
#define _EVENTINFO_H_

#include <ostream>
#include <iomanip>

#include "TObject.h"

class EventInfo : public TObject
{
public:
    EventInfo();
    virtual ~EventInfo();
    
    static EventInfo& Instance();

    void FillEventInfo(Int_t ec, UInt_t eId, UInt_t tok, UInt_t tcmd,
                       UInt_t qcmd, UInt_t eTime, UInt_t dec, Int_t sta,
                       ULong64_t l1, ULong64_t l2, ULong64_t daqbits);

    void FillTPCSectorMask(Int_t mask) { tpcSectorMask = mask; }
    void Reset();
    
    UInt_t    eventCount;
    UInt_t    eventId;
    UInt_t    token;
    UInt_t    trgcmd;
    UInt_t    daqcmd;
    UInt_t    evt_time;
    UInt_t    detectors;        // hex number
    Int_t     status;
    ULong64_t daqbits64_l1;     // hex
    ULong64_t daqbits64_l2;     // hex
    ULong64_t daqbits64;

    UInt_t    tpcSectorMask;    // lower 24 bits indicate fired TPC sectors
    Int_t     tpcNHits;         // number of TPC hits read in
    Int_t     tpcNTracks;
    Float_t   processTime;      // in second

    ClassDef(EventInfo, 1);
};

std::ostream& operator << (std::ostream& os, const EventInfo& evt);

#endif /* _EVENTINFO_H_ */

