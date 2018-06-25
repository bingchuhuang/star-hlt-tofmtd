#ifndef _HLT_ETOW_H_
#define _HLT_ETOW_H_

#include <DAQ_ETOW/daq_etow.h>
#include <L3_SUPPORT/l3_support.h>
#include "hlt_entity.h"

#include <memory>

class hlt_etow : public hlt_entity {
public:
    hlt_etow() {
        name = "etow";
        rts_id = ETOW_ID;
    }
    ~hlt_etow() {}

    int event_data(int sector, int rdo, void* start, int bytes, void* dst = NULL, int dst_sz = 0) {
        LOG(NOTE, "%d:%d: bytes %d, sizeof etow_t %d", sector, rdo, bytes, sizeof(etow_t));

        etow_t* etow_p = (etow_t*)dst ? dst : NULL;

        if (!etow_p) {
            if (!etow_t_dta) {
                etow_t_dta.reset(new etow_t);
            }
            etow_p = etow_t_dta.get();
        }

        // unpack raw ETOW data (9972 bytes) to etow_t
        // unpack
        u_short* data = (u_short*)((char*)start + 4 + 128); // 4 byte dummy, 128 byte header

        for (int j = 0; j < ETOW_PRESIZE; j++) {
            for (int i = 0; i < ETOW_MAXFEE; i++) {
                etow_p->preamble[i][j] = (*data++);
            }
        }

        for (int j = 0; j < ETOW_DATSIZE; j++) {
            for (int i = 0; i < ETOW_MAXFEE; i++) {
                etow_p->adc[i][j] = (*data++);
            }
        }

        return 0;
    }

    int event_do(hlt_blob* out, int* blobs) {
        if (! out->buff) {
            out->buff = (char*)etow_t_dta.get();
            out->bytes = sizeof(etow_t_dta);
            out->name = "etow_sl3";
        }

        *blobs = 1;

        return 0;
    }

private:
    std::unique_ptr<etow_t> etow_t_dta;
};

#endif
