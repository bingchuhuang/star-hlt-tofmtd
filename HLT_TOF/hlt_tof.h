#ifndef _HLT_TOF_H_
#define _HLT_TOF_H_

#include "../GL3/hlt_entity.h"
#include "tof_hlt_sector.h"

#include <memory>

class hlt_tof : public hlt_entity {
public:
	hlt_tof() { 
		name = "tof" ;
		rts_id = TOF_ID ;
		tof_t = 0 ;
	}

	virtual ~hlt_tof() {
		if(tof_t) delete tof_t ;
	}


	int run_config() {
		if(tof_t) delete tof_t ;

		char pamName[256];

		sprintf(pamName,"%s/",hlt_config.config_directory);
		tof_t = new tof_hlt_sector(pamName) ;

		tof_t->sector = sector ;

		return 0 ;
	}

	int run_start() { 
		return 0 ; 
	}

	int run_stop() {
		return 0 ;
	}

	int event_start(void *data) {
            tof_t->new_event() ;
            return 0 ;
	}

	int event_data(int rts_id, int rdo, void *start, int bytes, hlt_blob* blob = NULL) {
            tof_t->read_rdo_event(rdo, (char *)start, bytes/4) ;
            return 0 ;
	}

        int event_do(hlt_blob* out, int* blobs) {
            if (!out->buff) {
                if (!output_store) {
                    output_store.reset(new TofSend);
                }
                out->buff = (char*)output_store.get();
                out->bytes = sizeof(TofSend);
                out->name = "sl3_tof"; // always
            }

            out->bytes = tof_t->do_event((TofSend*)out->buff, sizeof(TofSend));

            *blobs = 1; // always

            return 0;
        }

private:
    std::unique_ptr<TofSend> output_store;
    tof_hlt_sector *tof_t ;
};
#endif
