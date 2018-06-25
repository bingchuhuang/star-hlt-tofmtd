//:>------------------------------------------------------------------
//: FILE:       gl3Tracks.h
//: HISTORY:
//:              6dec1999 version 1.00
//:              2feb2000 add sector to add methods
//:             27jul2000 add methods to drop hits
//:<------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "daqFormats.h"
#include "FtfTrack.h"
//#include "l3List.h"
#include "gl3Hit.h"

#ifndef GL3TRACK
#define GL3TRACK

class gl3Bischel;

class gl3Track: public FtfBaseTrack {
private:
   gl3Track* getNextTrack ( )    { return (gl3Track *)nextTrack ; }
public:
   void*     nextTrack ;
   int       sector ;
   float     dca;
   
   gl3Track(void* nt = NULL, int s = -1, float d = 0.0f)
    : nextTrack(nt), sector(s), dca(d) {}
   ~gl3Track() { nextTrack = NULL; }

   inline virtual   void nextHit ()
       { currentHit = ((gl3Hit *)currentHit)->nextHit; }

   int       addTrack ( gl3Track* ) ;
   void      dropHits ( int rest, int rowMin, int rowMax ) ;
   gl3Track* merge ( FtfContainer *trackArea ) ;
   void      Print ( int level ) ;
   void      setDca(Ftf3DHit vertex);
   double    nSigmaDedx(gl3Bischel* bischel, const char* particle, double sigma1);
   
   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   //
   //#######################################################################
   float getRealEta ( ) {
      float theta = atan2(1.,(double)tanl);
      float rEta  = -1. * log (tan(theta/2.)) ;
      return rEta ;
   }
   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   //
   //#######################################################################
   void print ( ) {
      float gl3ToDeg = 180./acos(-1.);
      printf ( "pt %f tanl %f psi %f r0 %f z0 %f phi0 %f nHits %d\n", 
                pt, tanl, psi, r0, z0, phi0*gl3ToDeg, nHits ) ;
   }
   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   //
   //#######################################################################
   void set ( short sectorIn, local_track* trk ) {
       //    id          = sectorIn * 10000 + abs(trk->id) ;
       nHits       = trk->nHits ;
       nDedx       = trk->ndedx ;
       chi2[0]     = float(trk->xy_chisq)/10. ;
       chi2[1]     = float(trk->sz_chisq)/10. ;
       dedx        = trk->dedx ; 
       pt          = fabs(trk->pt) ;
       psi         = trk->psi ;
       tanl        = trk->tanl ;
       eta         = getRealEta();
       z0          = trk->z0 ;
       length      = trk->trackLength ;
       innerMostRow= trk->innerMostRow ;
       outerMostRow= trk->outerMostRow ;
       r0          = trk->r0 ;
       q           = (short )(trk->pt/fabs(trk->pt)) ;
       phi0        = trk->phi0 ;
       dpt         = float(trk->dpt)/32768. * pt ; 
       dpsi        = float(trk->dpsi)/32768. * fabs(psi)  ;
       dtanl       = float(trk->dtanl)/32678.* fabs(tanl) ;
       dz0         = float(trk->dz0)/1024. ;
       nextTrack   = 0 ;
       firstHit    = 0 ;
       lastHit     = 0 ;

       dca = -1;
       //
       // Check errors are not zero
       //
       if ( dpt   == 0 ) dpt   = 1.e-5 * pt ;
       if ( dpsi  == 0 ) dpsi  = 1.e-5 ;
       if ( dtanl == 0 ) dtanl = 1.e-5 ;
       if ( dz0   == 0 ) dz0   = 1.e-3 ; 
       
   }
   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   //
   //#######################################################################
   void set ( FtfTrack* trk ) {
       id          = trk->id ;
       nHits       = trk->nHits;
       nDedx       = trk->nDedx ;
       chi2[0]     = trk->chi2[0] ;
       chi2[1]     = trk->chi2[1] ; 
       dedx        = trk->dedx ; 
       pt          = fabs(trk->pt) ;
       psi         = trk->psi ;
       tanl        = trk->tanl ;
       eta         = getRealEta();
       z0          = trk->z0 ;
       length      = trk->length ;
       innerMostRow= trk->innerMostRow ;
       outerMostRow= trk->outerMostRow ;
       r0          = trk->r0 ;
       phi0        = trk->phi0 ;
       q           = trk->q ;
       dpt         = trk->dpt ; 
       dpsi        = trk->dpsi ;
       dtanl       = trk->dtanl;
       dz0         = trk->dz0 ;
       dtanl = dpsi = dpt = 0. ;
       nextTrack   = 0 ;
       firstHit    = 0 ;
       lastHit     = 0 ;
              
       dca = -1;
       //
       // Check errors are not zero
       //
       if ( dpt   == 0 ) dpt   = 1.e-5 * pt ;
       if ( dpsi  == 0 ) dpsi  = 1.e-5 ;
       if ( dtanl == 0 ) dtanl = 1.e-5 ;
       if ( dz0   == 0 ) dz0   = 1.e-3 ; 
   }
};

#endif
