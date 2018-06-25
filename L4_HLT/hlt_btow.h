#ifndef _HLT_BTOW_H_
#define _HLT_BTOW_H_

#include <DAQ_BTOW/daq_btow.h>
#include <L3_SUPPORT/l3_support.h>
#include "hlt_entity.h"

#include <memory>

class hlt_btow : public hlt_entity {
  public:
    hlt_btow() {
        name = "btow";
        rts_id = BTOW_ID;
    }
    ~hlt_btow() {}

    int event_data(int sector, int rdo, void* start, int bytes, hlt_blob* blob = NULL) {
        LOG(NOTE, "%d:%d: bytes %d, sizeof btow_t %d", sector, rdo, bytes, sizeof(btow_t));

        btow_t* btow_p = blob ? (btow_t*)blob->buff : NULL;

        if (!btow_p) {
            if (!btow_t_dta) {
                btow_t_dta.reset(new btow_t);
            }
            btow_p = btow_t_dta.get();
        }

        if (bytes != 9972) {
            LOG(ERR, "Get BTOW bad data, skip this event");
            return 1;
        }
        
        // unpack raw BTOW data (9972 bytes) to btow_t
        // unpack
        u_short* data = (u_short*)((char*)start + 4 + 128); // 4 byte dummy, 128 byte header

        for (int j = 0; j < BTOW_PRESIZE; j++) {
            for (int i = 0; i < BTOW_MAXFEE; i++) {
                btow_p->preamble[i][j] = (*data++);
            }
        }

        for (int j = 0; j < BTOW_DATSIZE; j++) {
            for (int i = 0; i < BTOW_MAXFEE; i++) {
                btow_p->adc[i][j] = (*data++);
            }
        }

        return 0;
    }

    int event_do(hlt_blob* out, int* blobs) {
        if (! out->buff) {
            out->buff = (char*)btow_t_dta.get();
            out->bytes = sizeof(btow_t);
            out->name = "btow_sl3";
        }

        *blobs = 1;

        return 0;
    }

  private:
    std::unique_ptr<btow_t> btow_t_dta;
};
#endif
