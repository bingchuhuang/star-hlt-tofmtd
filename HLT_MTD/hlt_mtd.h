#ifndef _HLT_MTD_H_
#define _HLT_MTD_H_

#include "../L4_HLT/hlt_entity.h"
#include "../L4_HLT/EventInfo.h"
#include "mtd_hlt_reader.h"

#include <memory>

class hlt_mtd : public hlt_entity 
{
 public:
  hlt_mtd() {
    name = "mtd" ;
    rts_id = MTD_ID ;
    mtd_r  = 0;
  }


  virtual ~hlt_mtd() {
    if(mtd_r) delete mtd_r;
  }

		
  int run_config() {
    return 0;
  }


  int run_start() {
    if(mtd_r) delete mtd_r;
    mtd_r = new mtd_hlt_reader(run_number);
    return 0;
  }


  int run_stop() {
    return 0; 
  }
  
  
  int event_start(void *dta) {
    if (mtd_buffer) memset(mtd_buffer.get(), 0, sizeof(Mtdsend));
    mtd_r->new_event();
    return 0 ;
  }

  int event_data(int sector, int rdo, void *start, int bytes, hlt_blob* blob = NULL) {
    // read in raw data
      mtd_r->read_rdo_event(rdo, (char*)start, bytes/4);
      return 0 ;
  }

  int event_data_MC(McEvent* mcevt, hlt_blob* out, int* blobs)
  {
      if (!mtd_buffer) {
          mtd_buffer.reset(new Mtdsend);
      }
      
      int bytes_used = sizeof(Mtdsend); // pass the MAX size in and get the real size out
      int nMTDHits = mtd_r->readFromMcEvent(mcevt, (char*)mtd_buffer.get(), bytes_used);

      // prepare
      out->buff  = (char*)mtd_buffer.get();
      out->bytes = bytes_used ;
      out->name  = "mtd_sl3" ;
      *blobs = 1 ;	// MUST be 1

      return nMTDHits;
  }

  int trigger_data(void *start) {
    // read in trigger data
    mtd_r->read_mtd_trigger(start);
    return 0 ;
  }
    
  int event_do(hlt_blob *out, int *blobs) {
    if (!out->buff) {
        if (!mtd_buffer) {
            mtd_buffer.reset(new Mtdsend);            
        }
        out->buff  = (char*)mtd_buffer.get();
        out->bytes = sizeof(Mtdsend);
        out->name  = "mtd_sl3" ;
    }
      
    uint tpcMask = 0;
    out->bytes = mtd_r->do_event((Mtdsend*)out->buff, sizeof(Mtdsend), tpcMask);
    EventInfo::Instance().FillTPCSectorMask(tpcMask);

    *blobs = 1 ;	// MUST be 1
	       
    return 0 ;
  }

private:
    std::unique_ptr<Mtdsend> mtd_buffer;
    mtd_hlt_reader *mtd_r;
};

#endif
