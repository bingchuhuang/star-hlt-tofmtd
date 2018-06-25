#ifndef _HLT_TRG_H_
#define _HLT_TRG_H_

#include <L3_SUPPORT/l3_support.h>
#include "DAQ_TRG/daq_trg.h"
#include <trgDataDefs.h>
           
#include "hlt_entity.h"
#include "L4EventDataTypes.h"
#include <memory>

class hlt_trg : public hlt_entity {
public:
    hlt_trg() {
        name = "trg";
        rts_id = TRG_ID;
    }
    ~hlt_trg() {}

    int event_data(int rts_id, int rdo, void *start, int bytes, hlt_blob* blob = NULL) {
        HLTTrgData_t* trgData = blob ? (HLTTrgData_t*)blob->buff : NULL;

        if (!trgData) {
            if (!trg_data) {
                trg_data.reset(new HLTTrgData_t);
            }
            trgData = trg_data.get();
        }

        memset(trgData, 0, sizeof(HLTTrgData_t));
        // fill trigger data here with point trgData
        
        return 0;
    }

    int event_do(hlt_blob *out, int *blobs) {
        if (trg_data) {
            out->buff = (char*)trg_data.get();
            out->bytes = sizeof(trg_data) ;
            out->name = "trg_sl3" ;
            *blobs = 1 ;
            LOG(NOTE,"TRG: TrgId 0x%016llX",trg_data->daq_TrgId);
        } else {
            // if trg_data is not in use
            // then the output blob is alread set by event_data
            *blobs = 1;
        }
        
        return 0 ;
    }

private:
    std::unique_ptr<HLTTrgData_t> trg_data;
};

#endif
