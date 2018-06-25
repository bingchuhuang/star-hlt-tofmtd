#ifndef _HLT_ENTITY_H_
#define _HLT_ENTITY_H_

#include <rtsSystems.h>
#include <rtsLog.h>
#include <L3_SUPPORT/l3_support.h>

#include "hltNetworkDataStruct.h"

#include "L4_SUPPORT/l4Interface.h"

// used in run_config() for any detector
// Some values make no sense for misc detectors so they
// are probably wrong but are ignored i.e. drift_velocity for TOF...
struct hlt_config_t {
    u_int version;      // normally 0 for FY11
    u_int seeding_mask; // mostly for SL3s...
    l3_sc_t l3_sc;

    int standalone;
    char* config_directory; // where misc HLTparameters reside:
    //  normally daqman:/RTS/conf/tpx when in realtime
    //  current directory when standalone but this can be overriden
    char* trgsetupname;
    int runnumber;
    int myId;                // HLT instance id on the current machine
    l3_algos_t hlt_algo[32]; // for GL3, TBD

    hltInitialCalib hltInitCaliData; // initial values of beamline xy and DeDx gain
};

class hlt_entity {
  public:
    hlt_entity() {
        sector = 0;
        run_number = 0;
        evt_counter = 0;

        detectors = 0;
        name = "dummy";

        memset(&hlt_config, 0, sizeof(hlt_config));

        rts_id = 0;
    }

    virtual ~hlt_entity() {
        LOG(NOTE, "hlt_entity destructor"); // normally doesn't happen!
    }

    // used to return data from the daughter classed
    //  struct blob_desc {  // blob descriptor
    //      char *buffer ;
    //      int bytes ;
    //      char *name ;
    //  } ;

    // SHOULD exist in the detector classes
    virtual int run_config() { return 0; }
    virtual int run_start() { return 0; }
    virtual int run_stop() { return 0; }
    virtual int event_start(void* data = 0) { return 0; }
    virtual int event_data(int rts_id, int sub, void* start, int bytes, hlt_blob* blob = NULL) = 0;
    virtual int event_do(hlt_blob* out, int* blob_count) = 0;

    // Prepare seeding data for TPX sector "sector"
    // It's up to the user to do it right!
    virtual int seed_to_sl3(int sector, hlt_blob* out) {
        if (out)
            return 0; // nothing to do

        out->bytes = 0; // assume we have nothing for SL3

        return 0;
    }

    // specific HLT classes should not touch this!
    int Run_config(int sect = 1, hlt_config_t* data = 0) {
        int ret;

        sector = sect;

        if (data)
            Get_config(data);

        ret = run_config();

        LOG(NOTE, "Run_config %s: sector %d, ret %d", name, sector, ret);
        return ret;
    }

    int Run_start(u_int r_number) {
        int ret;

        run_number = r_number;
        evt_counter = 0;

        ret = run_start();

        LOG(NOTE, "Run_start %s: run %u, ret %d", name, run_number, ret);
        return ret;
    }

    int Run_stop() {
        int ret;

        ret = run_stop();

        LOG(NOTE, "Run_stop %s: run %u, events %d, ret %d", name, run_number, evt_counter, ret);
        return ret;
    }

    // data is an kind of void pointer that might be required by a
    // specific instance. TBD.
    int Event_start(void* data = 0) { // called before an event i.e. for some cleanup
        int ret;

        detectors = 0; // clear them all...

        ret = event_start(data);

        LOG(NOTE, "Event_start %s: %d", name, ret);
        return ret;
    }

    // This is how we feed DAQ data to the processor
    // in SL3 sub is the RDO (counting from 1)
    // in GL3 sub is the sector (counting from 1)
    int Event_data(int rts_id, int sub, void* start, int bytes, hlt_blob* blob = NULL) {
        int ret;

        // OK, I have this guy: useful for GL3 only but hey...
        detectors |= 1 << rts_id;

        ret = event_data(rts_id, sub, start, bytes, blob);

        LOG(NOTE, "Event_data %s: rts_id %d, sub %d, bytes %d: ret %d", name, rts_id, sub, bytes,
            ret);

        return ret;
    }

    // this is what starts the processing
    int Event_do(hlt_blob* out, int* blobs) { // called for processing...
        int ret;
        int bytes = 0;

        evt_counter++;

        ret = event_do(out, blobs);

        for (int i = 0; i < (*blobs); i++) {
            bytes += (out + i)->bytes;
        }
        LOG(NOTE, "Event_do %s: evt %d, blobs %d, bytes %d", name, evt_counter, *blobs, bytes);
        return ret;
    }

    // this runs in TOF & GL3/BTOW
    int Seed_to_SL3(int sl3_sector, hlt_blob* out_data) {

        if (hlt_config.seeding_mask & (1 << rts_id))
            ; // want to send seeds
        else {
            out_data->bytes = 0;
            return 0;
        }

        int ret = seed_to_sl3(sl3_sector, out_data);

        return ret;
    }

    // this runs in SL3's
    int Seed_from(int rts_id, hlt_blob* in_data) {
        if (hlt_config.seeding_mask & (1 << rts_id))
            ;
        else
            return 0;

        if (in_data->bytes == 0)
            return 0; // no seed present...

        return 0;
    }

    // set by main class:
    void Get_config(hlt_config_t* cfg) { memcpy(&hlt_config, cfg, sizeof(hlt_config)); }

    hlt_config_t hlt_config;

    u_int detectors; // detector mask; if 0 the detector is not needed
    // the index is the rts_id as defined in rtsSystems.h i.e.
    // TPX_ID or TOF_ID or TRG_ID...
    int rts_id;

    char* name;
    u_int run_number;
    u_int evt_counter;

  protected:
    int sector;
};
#endif
