#include <iostream>
#include <rtsLog.h>
#include <ctime>
#include "gl3Track.h"
#include "FtfGeneral.h"

using namespace std;
#include "gl3MTD.h"

//================================================
const double gl3MTD::mtdRadius        = 403.60;
const double gl3MTD::innerSteelRadius = 303.29;
const double gl3MTD::outerSteelRadius = 364.25;
const double gl3MTD::stripLength      = 87.0;
const double gl3MTD::stripWidth       = 3.6;
const double gl3MTD::stripGap         = 0.6;
const double gl3MTD::bFieldInSteel    = 1.26;
const double gl3MTD::tesla            = 1e-13;
const double gl3MTD::pi               = M_PI;
const double gl3MTD::backlegPhiWidth  = 8./180.*pi;
const double gl3MTD::backlegPhiGap    = 4./180.*pi;
const double gl3MTD::firstBacklegPhi  = 90./180.*pi;
const double gl3MTD::vDrift           = 56.;   // ps/cm drifting velocity of electronic signal

//================================================
gl3MTD::gl3MTD()
{
  mtdEvent  = 0x0;
  evt_count = 0;
}

//================================================
gl3MTD::~gl3MTD()
{
}

//================================================
int gl3MTD::readFromMtdMachine(char *trgSend)
{
  mtdEvent = (Mtdsend*) trgSend;
  evt_count ++;
  return 0;
}

//================================================
int gl3MTD::findMatchedMtdHit(gl3Track *track, double &dz, double &dy)
{
  int index = -1;
  dz = -999.;
  dy = -999.;
  double projPhi, projZ, projPsi;
  int ret = -1;
  double radius = 0;
  double polarity = track->getPara()->bFieldPolarity;
  double bfield = track->getPara()->bField;

  LOG(DBG,"Before projection: (z, phi, psi) = (%f, %f, %f)",track->z0, track->phi0, track->psi);
  // Project to inner steel
  radius = innerSteelRadius;
  gl3Track *tmp = new gl3Track();
  tmp->q    = track->q;
  tmp->psi  = track->psi;
  tmp->r0   = track->r0;
  tmp->phi0 = track->phi0; 
  tmp->pt   = track->pt;
  tmp->tanl = track->tanl;
  tmp->z0   = track->z0;
  ret = extrapolateTrack(tmp, polarity, bfield, radius, projPhi, projZ, projPsi);
  LOG(DBG,"Projected to inner steel: (z, phi, psi) = (%f, %f, %f)",projZ, projPhi,projPsi);
  if(ret) return -1;
  
  // project to outer steel
  tmp->psi  = projPsi;
  tmp->r0   = radius;
  tmp->phi0 = projPhi;
  tmp->z0   = projZ;
  radius = outerSteelRadius;
  ret = extrapolateTrack(tmp, polarity*-1, bfield*bFieldInSteel/0.5, radius, projPhi, projZ, projPsi);
  LOG(DBG,"Projected to outer steel: (z, phi, psi) = (%f, %f, %f)",projZ, projPhi,projPsi);
  if(ret) return -1;

  // project to MTD with bfield = 0
  tmp->psi  = projPsi;
  tmp->r0   = radius;
  tmp->phi0 = projPhi;
  tmp->z0   = projZ;
  radius = mtdRadius;
  ret = extrapolateTrack(tmp, polarity, 1e-10, radius, projPhi, projZ, projPsi);
  LOG(DBG,"Projected to MTD radius: (z, phi, psi) = (%f, %f, %f)",projZ, projPhi,projPsi);
  if(ret) return -1;

  // Find the matched MTD hits  
  int nMtdHits = mtdEvent->nMtdHits;  
  double maxR = 999;
  const double mthWind = 50; 
  for (int i=0; i<nMtdHits; i++)
      {
	MtdHit hit = mtdEvent->mtdhits[i];
	int backleg = hit.backleg;
	int module  = hit.tray;
	int channel = hit.channel;
	double hitZ = getMtdHitGlobalZ(mtdEvent->mtdhits[i].leadingEdgeTime.first, 
				       mtdEvent->mtdhits[i].leadingEdgeTime.second, 
				       (int)mtdEvent->mtdhits[i].tray);
	double deltaZ = hitZ-projZ;
	double hitPhi = getMtdHitGlobalPhi(backleg, module, channel);
	double deltaY = mtdRadius*(hitPhi-projPhi);
	if(fabs(deltaY)>mthWind || fabs(deltaZ)>mthWind) continue;
	double deltaR = sqrt(deltaY*deltaY + deltaZ*deltaZ);
	if(deltaR < maxR )
	  {
	    maxR = deltaR;
	    dz = deltaZ;
	    dy = deltaY;
	    index = i;
	  }
      }
  delete tmp;

  return index;
}


//================================================
// A similar function to FtfBaseTrack::extraRCyl() 
// with magnetic field as input argument
// It also returns psi after extrapolation
int gl3MTD::extrapolateTrack( gl3Track *track, int bFieldPolarity, double bfield, double r, double &phi, double &z, double &psic)
{
   double td  ;
   double fac1,sfac, fac2 ;
//--------------------------------------------------------
//     Get track parameters
//--------------------------------------------------------

   double psi   = track->psi;
   short  q     = track->q;
   double r0    = track->r0;
   double phi0  = track->phi0;
   double pt    = track->pt;
   double z0    = track->z0;
   double tanl  = track->tanl;

   double tPhi0 = psi + bFieldPolarity*double(q) * 0.5 * M_PI / fabs((double)q) ;
   double x0    = r0 * cos(phi0) ;
   double y0    = r0 * sin(phi0) ;
   double rc    = fabs(pt / ( bFactor * bfield ))  ;
   double xc    = x0 - rc * cos(tPhi0) ;
   double yc    = y0 - rc * sin(tPhi0) ;
   // cout << "pt = " << pt << ", psi = " << psi << endl;
   // cout << "Field polarity = " << bFieldPolarity << ", bFactor = " << bFactor << ", b-field = " << bfield << endl;
   // cout << "charge = " << double(q) << ", " << fabs((double)q) << endl;
   // cout << "M_PI = " << M_PI << endl;
   // cout << "tphi0 = " << tPhi0 << ", - psi = " << tPhi0-psi << endl;
   // cout << "r0 = " << r0 << ", phi0 = " << phi0 << endl;
   // cout << "rc = " << rc << endl;
 
   //    Check helix and cylinder intersect

   fac1 = xc*xc + yc*yc ;
   sfac = sqrt( fac1 ) ;
   //  
   //  If they don't intersect return
   //  Trick to solve equation of intersection of two circles
   //  rotate coordinates to have both circles with centers on x axis
   //  pretty simple system of equations, then rotate back
   //  
   if ( fabs(sfac-rc) > r || fabs(sfac+rc) < r ) {
     //    l3Log ( "particle does not intersect \n" ) ;
     return  1 ;
   }
   //  
   //     Find intersection
   //  
   fac2   = ( r*r + fac1 - rc*rc) / (2.00 * r * sfac ) ;
   phi    = atan2(yc,xc) + bFieldPolarity*float(q)*acos(fac2) ;
   if(phi<0) phi += 2*M_PI;
   td     = atan2(r*sin(phi) - yc,r*cos(phi) - xc) ;
   // cout << "acos(fac2) = " << acos(fac2) << ", phic = " << atan2(yc,xc) << ", phi = " << phi <<endl;
   // cout << "td = " << td << endl;
   //    Intersection in z
   
   if ( td < 0 ) td = td + 2. * M_PI ;
   double dangle = tPhi0 - td ;
   dangle = fmod ( dangle, 2.0 * M_PI ) ;
   if ( r < r0 ) dangle *= -1 ;
   // cout << "dangle = " << dangle << endl;
   // l3Log ( "dangle %f q %d \n", dangle, q ) ;
   if ( (bFieldPolarity*float(q) * dangle) < 0 ) 
     dangle = dangle + bFieldPolarity*float(q) * 2. * M_PI ;
   
   psic = psi - dangle;
   if(psic<0) psic += 2*M_PI;
   psic = fmod(psic,2.0*M_PI);
   double stot = fabs(dangle) * rc ;
   // cout << "stot = " << stot << ", dz = " << stot * tanl << endl;
   // l3Log ( "dangle %f z0 %f stot %f \n", dangle, z0, stot ) ;
   if ( r > r0 ) z = z0 + stot * tanl ;
   else          z = z0 - stot * tanl ;
   
   // That's it
   
   return 0 ;
}

//_____________________________________________________________________________
int gl3MTD::getMtdBackleg(const double projPhi)
{
  double phi = rotatePhi(projPhi);
  int backleg = (int)(phi/(backlegPhiWidth+backlegPhiGap));
  backleg += 24;
  if(backleg>30) backleg -= 30;
  if(backleg>=1 && backleg<=30)
    return backleg;
  else
    return -1;
}

//_____________________________________________________________________________
int gl3MTD::getMtdModule(const double projZ)
{
  int module = -1;
  double temp = (projZ+2.5*stripLength)/stripLength;
  if(temp>0) module = (int)temp + 1;
  return module;
}

//_____________________________________________________________________________
double gl3MTD::getMtdHitGlobalZ(double leadingWestTime, double leadingEastTime, int module)
{
  double z = (module-3)*stripLength - 1e3*(leadingWestTime-leadingEastTime)/2./vDrift;
  return z;
}


//_____________________________________________________________________________
double gl3MTD::getMtdHitGlobalPhi(int backleg, int module, int channel)
{
  double backlegPhiCen = firstBacklegPhi + (backleg-1) * (backlegPhiWidth+backlegPhiGap);
  while(backlegPhiCen>2*pi) backlegPhiCen -= 2*pi;
  
  double stripPhiCen = 0;
  if(module>0 && module<4) stripPhiCen = backlegPhiCen - (MTD_NCHANNEL/4.-0.5-channel)*(stripWidth+stripGap)/mtdRadius;
  else stripPhiCen = backlegPhiCen + (MTD_NCHANNEL/4.-0.5-channel)*(stripWidth+stripGap)/mtdRadius;

  return rotatePhi(stripPhiCen);
}

//_____________________________________________________________________________
void gl3MTD::global2Local(vector3 global, int backleg, int module, vector3 &local)
{
  // convert the global coordinate to local coordinate
  // with the module center as the origin
  local.x = -999;
  local.y = -999;
  local.z = -999;

  if(backleg<1 || backleg>MTD_NBACKLEG) return;
  if(module<1  || module>MTD_NTRAY)     return;

  double r, theta, z;
  r = mtdRadius;
  z = (module-3) * stripLength;
  theta = (backleg-1) * (backlegPhiWidth + backlegPhiGap) + firstBacklegPhi;
  while(theta>2*pi) theta -= 2*pi;
  double medi_x = global.x - r*cos(theta);
  double medi_y = global.y - r*sin(theta);
  double local_x = medi_x*cos(-theta) - medi_y*sin(-theta);
  double local_y = medi_x*sin(-theta) + medi_y*cos(-theta);
  if(module>3) local_y = -1 * local_y;
  local.x = local_x;
  local.y = local_y;
  local.z = global.z - z;
}


//_____________________________________________________________________________
double gl3MTD::rotatePhi(double phi)
{
  double outPhi = phi;
  while(outPhi<0) outPhi += 2*pi;
  while(outPhi>2*pi) outPhi -= 2*pi;
  return outPhi;
}

//_____________________________________________________________________________
void gl3MTD::reset()
{
    // reset mtdEvent here is a bug. Do not do that.
    // if (mtdEvent) {
    //     mtdEvent->nMtdHits = 0;
    // }
    mtdEvent = NULL;
}
