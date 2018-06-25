#ifndef _HLT_TPC_CA_H_
#define _HLT_TPC_CA_H_

#include "hlt_entity.h"
#include "online_TPC_CA_tracker.h"
#include "L4EventDataTypes.h"
#include <memory>

//#define TIMING_HLT
#ifdef TIMING_HLT
#include <time.h>
#endif


class hlt_tpc_ca : public hlt_entity {
public:
    hlt_tpc_ca() {
        name   = "tpc_ca" ;
        rts_id = TPX_ID ;       // keep TPX_ID
        tpc_ca = NULL ;
    }

    ~hlt_tpc_ca() {
        delete tpc_ca;
    }

    int run_config() {
        if (tpc_ca) delete tpc_ca ;

        LOG(NOTE, "Using config_directory %s, mag field %f, drift %f",
            hlt_config.config_directory,
            hlt_config.l3_sc.mag_field, hlt_config.l3_sc.drift_velocity_east) ;

        sprintf(hltParameters, "%s/HLTparameters", hlt_config.config_directory) ;
        sprintf(beamlineFile, "%s/beamline", hlt_config.config_directory);
        sprintf(GainParameters, "%s/GainParameters", hlt_config.config_directory);

        tpc_ca = new online_TPC_CA_tracker(hlt_config.l3_sc.mag_field,
                                           hlt_config.l3_sc.drift_velocity_east,
                                           hlt_config.config_directory,
                                           hlt_config.runnumber,
                                           hlt_config.myId);
        tpc_ca->setTPCMap();      // load tpc maps
        tpc_ca->makeCASettings(); // once per run?

#ifdef WITHSCIF
        tpc_ca->setKFPClient();
#endif // WITHSCIF
        return 0 ;
    }

    int run_start() {
        return 0 ;
    }

    int run_stop() {
#ifdef WITHSCIF
        tpc_ca->closeKFPConnection();
#endif // WITHSCIF

        return 0 ;
    }

    int event_start(void *data) {
        tpc_ca->reset();

        if (data) {
            LOG(NOTE, "Space charge = %i", *((int*)data));
            tpc_ca->setSpaceChargeFromScalerCounts((int*)data);
        } else {
            // no RICH scaler -- do nothing!
            LOG(WARN, "No RICH scaler data!") ;
        }

        return 0 ;
    }

    int event_data(int rts_id, int sector, void *start, int bytes, hlt_blob* blob = NULL) {
        // sector 1 - 24
        // this function needs to be called 24 times. looks stupid.
        detectors |= (1 << rts_id);

        int nhits = tpc_ca->readSectorFromESB(sector, (char *)start, bytes / 4) ;

        // tpc_ca->setHitsWeight();

        return nhits ;
    }

    int event_do(hlt_blob *out, int *blobs) {
        int ret = 0 ;   // assume 0 tracks

        if (! out->buff) {
            if (!output_store) {
                output_store.reset(new HLTTPCData_t);
            }

            out->buff  = (char*)output_store.get();
            out->bytes = sizeof(HLTTPCData_t); // pre-set to max
            out->name  = "tpx_sl3" ; // always
        }
        
        ret = tpc_ca->process((HLTTPCData_t*)out->buff, out->bytes);
        // ret = tpc_ca->process2((char*)output_store, bytes_used);
        
        *blobs = 1 ;    // always for SL3 tpx

        return ret ;
    }

    online_TPC_CA_tracker* tpc_ca;

    char beamlineFile[256];
    char GainParameters[256];
    char hltParameters[256];
    char mapName[256];

#ifdef TIMING_HLT
    double timeNow, timeOld;
    double timeStart, timeData, timeDo;
    int nTracks;
#endif

private:
    // char output_store[10 << 20] ; // internal buffer which keeps tracks 10 MB
    std::unique_ptr<HLTTPCData_t> output_store;
};
#endif
