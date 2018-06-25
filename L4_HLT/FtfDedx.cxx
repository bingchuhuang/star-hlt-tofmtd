/*:>-------------------------------------------------------------------
**: FILE:     FtfDedx.cxx
**: HISTORY:
**:<------------------------------------------------------------------*/
#include "FtfDedx.h"
#include <iostream>
#include <string.h>
#include <fstream>

#include <algorithm>

//***********************************************************************
// constructor
//***********************************************************************
FtfDedx::FtfDedx(l3CoordinateTransformer *trafo, float cutLow, float cutHigh, float driftLoss, const char* HLTparameters)
{
  fCoordTransformer = trafo;
  fCutLow = cutLow;
  fCutHigh = cutHigh;
  fNTruncate = 0;
  fDriftLoss = driftLoss;

  innerSectorGain = 7.9509856e-9;
  outerSectorGain = 1.4234702e-8;
  normInnerGain = 2.89098808e-08;
  normOuterGain = 3.94915375e-08;

  // ReadHLTparameters(HLTparameters);
  // initialize unit vector perpendicular to rows
  // for each sector
  for (int sector=0; sector<24; sector++) {
        // caution: sector>12 needs x->-x and y->y (east side!)
        fUnitVec[sector].x = fCoordTransformer->GetSectorSin(sector);
	if (sector+1>12) fUnitVec[sector].x = -1 * fUnitVec[sector].x;
	fUnitVec[sector].y = fCoordTransformer->GetSectorCos(sector);
  }
}

void FtfDedx::ReadHLTparameters(const char* HLTparameters)
{
	string parameterName;
	ifstream ifs(HLTparameters);
	if(!ifs.fail())
		while(!ifs.eof())
		{
			ifs>>parameterName;
			if(parameterName == "innerSectorGain") ifs>>innerSectorGain;
			if(parameterName == "outerSectorGain") ifs>>outerSectorGain;
		}
}

void FtfDedx::SetGainparameters(const double ig, const double og)
{
    innerSectorGain = ig;
    outerSectorGain = og;
}

void FtfDedx::ReadGainparameters(const char* GainParameters)
{
	int run_num[50]; double innerGain[50] , outerGain[50];
	int end_num = 0; int nlines = 0;
	string str;

	ifstream ifs(GainParameters);

	if(!ifs.fail()){
		while(!ifs.eof())
		{
			end_num = end_num%50;
			int run_bit=999; double tem_innergain=999.; double tem_outergain=999.;

			ifs>>str;
			run_bit = atoi(str.data());
			if(run_bit<10000000 || run_bit>500000000) continue;

			ifs>>str;
			tem_innergain = atof(str.data());
			if(fabs(tem_innergain-normInnerGain)/normInnerGain > 0.9) continue ;

			ifs>>str;
			tem_outergain = atof(str.data());
			if(fabs(tem_outergain-normOuterGain)/normOuterGain > 0.9 ) continue;

			run_num[end_num] = run_bit ;
			innerGain[end_num] = tem_innergain;
			outerGain[end_num] = tem_outergain;
			end_num++;
			if(nlines<50) nlines++ ;
		} 
	}
	else {
		LOG(ERR,"GainParameters not found: %s", GainParameters);
                return;
	}

	int pro_num = 0;
	int run_con = run_num[(end_num -1 + 50)%50];
	double innerGainSum = 0; double outerGainSum = 0;
	for(int i=1;i<=nlines;i++)
	{
		int index = (end_num - i + 50)%50;
		if(run_con == run_num[index]){
			innerGainSum = innerGainSum + innerGain[index] ;
			outerGainSum = outerGainSum + outerGain[index] ;
			pro_num++;
		}
		else break ;
	}
	innerSectorGain = innerGainSum/pro_num ;
	outerSectorGain = outerGainSum/pro_num ;

	if( fabs(innerSectorGain - normInnerGain)/normInnerGain > 0.9 || fabs(outerSectorGain - normOuterGain)/normOuterGain > 0.9 )
	{
		innerSectorGain = normInnerGain ;
		outerSectorGain = normOuterGain ;

		LOG(WARN, "innerSectorGain or outerSectorGain change from Calibrate value larger than 20% ... ");
	}

	ifs.close();

}

//***********************************************************************
//   dEdx calculation using Truncated Mean algorithm
//   averaging method: dE/dx = (sum of q/s)/(# of points)
//
//   author: christof
//***********************************************************************
int FtfDedx::TruncatedMean(FtfTrack *track) {

  double padLength;
  int nPointsInArray;

  // dx calculation:
  //   straight-line approx. in x-y-plane
  //   assume perfect hit position on track
  //   :=> don't calculate intersection of circle and row
  //   1/cosl corr. factor for dip angle

//   printf("Track: id %i, pt %f, nHits %i, q %i, tanl %f\n",
// 	 fTrack->id, fTrack->pt, fTrack->nHits, fTrack->q, fTrack->tanl);

  // calculate dip angle correction factor 
  double cosl = 1 / (sqrt( 1 + (double) track->tanl*track->tanl));

//   printf("   cosl %f\n", cosl);
  if ( cosl==0 ) return 0;

  // calculate center of circle
  double tPhi0 = track->psi + track->q * track->getPara()->bFieldPolarity * pi/2;
  if ( tPhi0 > 2*pi ) tPhi0 = tPhi0 - 2*pi;
  else if ( tPhi0 < 0. ) tPhi0 = tPhi0 + 2*pi;

  double x0 = track->r0 * cos(track->phi0);
  double y0 = track->r0 * sin(track->phi0);
  double rr = track->pt / ( bFactor *track->getPara()->bField );
  double xc = x0 - rr * cos(tPhi0);
  double yc = y0 - rr * sin(tPhi0);

//   printf("   xc %f  yc %f\n", xc, yc);

  nPointsInArray = 0;

  //now loop over hits
  for ( FtfHit *ihit = (FtfHit *)track->firstHit ; 
	ihit != 0 ;
	ihit = (FtfHit *)ihit->nextTrackHit) {

        // discard hits with wrong charge information
        // i.e. hits of one-pad cluster or merged cluster
        if (ihit->flags) continue;

        // calculate direction of the tangent of circle at position
        // of given hit:
        //     - rotate radius vector to hit by -90 degrees
        //       matrix (  0   1 )
        //              ( -1   0 )
        //     - divide by length to get unity vector
        struct christofs_2d_vector tangent;
	tangent.x = (ihit->y - yc)/rr;
	tangent.y = -(ihit->x - xc)/rr;
	//printf("   tangent x %f y %f\n", tangent.x, tangent.y);
  
	// get crossing angle by calculating dot product of 
	// tanget and unity vector perpendicular to row (look-up table)
	double cosCrossing = fabs(tangent.x * fUnitVec[ihit->sector-1].x + tangent.y * fUnitVec[ihit->sector-1].y);

	// padlength
	if (ihit->row<14) padLength = padLengthInnerSector;
	else padLength = padLengthOuterSector;

	// ==> finally dx:
	if ( cosCrossing==0 ) continue;
	double dx = padLength / (cosCrossing * cosl);

	// drift length correctrion: d_loss [m^-1], l_drift [cm]
	// q_corr = q_meas / ( 1 - l_drift * d_loss/100 )
	double scaleDrift = 1 - fabs(ihit->z) * fDriftLoss/100;


	// first shot: de_scale as implemented in tph.F,
	// i.e. gas gain, wire-to-pad coupling, etc.
	// !! hard coded, has to come from the db somewhere sometime !!
	double deScale;
	if (ihit->row<14) deScale = innerSectorGain;
	else deScale = outerSectorGain;

	fDedxArray[nPointsInArray] = ihit->q * deScale / ( dx * scaleDrift );
	nPointsInArray++;
	
//         printf("  ihit row %i x %f y %f z %f  q %d  q_scaled %e scale %e flags %d\n",
// 	       ihit->row, ihit->x, ihit->y, ihit->z, ihit->q, ihit->q*deScale, deScale, ihit->flags);
// 	printf("    cosCros: %f, cosl: %f ===>> dx %f\n", cosCrossing, cosl, dx);

  }

  // sort dEdxArray
  sort( fDedxArray, fDedxArray+nPointsInArray );

  // calculate absolute cuts
  int cLow  = (int) floor(nPointsInArray * fCutLow);
  int cHigh = (int) floor(nPointsInArray * fCutHigh);

  track->nDedx = 0;
  track->dedx  = 0;

  for (int i=cLow; i<cHigh; i++) {
        track->nDedx++;
	track->dedx += fDedxArray[i];
  }
  
  if (track->nDedx>0) track->dedx = track->dedx/track->nDedx;

//   printf("  %e   %i\n", track->dedx, track->nDedx);

  return 0;
}

//=================================================
//   dEdx calculation using Truncated Mean algorithm
//   averaging method: dE/dx = (sum of q/s)/(# of points)
// 	 calculate dEdx base on CAGBTrack and FtfHits.
//   
//   added by Yi Guo for CATracker
//=================================================
int FtfDedx::TruncatedMean(AliHLTTPCCAGBTrack *CATrack,
                           const AliHLTTPCCAGBTracker *fCATracker,
                           const FtfHit* ftfHits, const int nFtfHits,
                           const double bField)
{
    double padLength;
    int nPointsInArray;
    const AliHLTTPCCATrackParam&  firstParam = CATrack->InnerParam();

    // convert CATrack param to HLTTrackParam
    // dx calculation:
    // straight-line approx. in x-y-plane
    // assume perfect hit position on track
    // :=> don't calculate intersection of circle and row
    // 1/cosl corr. factor for dip angle

    // calculate dip angle correction factor
    double tanl = 1.*firstParam.DzDs(); // CA local Z direction to STAR z direction...orz
    double cosl = 1 / (sqrt(1 + tanl * tanl));

    //   printf("   cosl %f\n", cosl);
    if(cosl == 0) return 0;

    // calculate center of circle
    // short h = firstParam.GetQPt() > 0 ? 1 : -1; // h = sign(-qB) QPt~signed inversed pt
    double localx0 = firstParam.GetX();
    double localy0 = firstParam.GetY();
    //double r0 = sqrt(x0*x0+y0*y0);
    double sinphi = firstParam.GetSinPhi();
    // double cosphi = sqrt(1 - sinphi * sinphi);
    double cosphi = firstParam.GetCosPhi(); // kehw
    
    double rr = 1. / firstParam.GetQPt() / (bFactor * -bField);
    double localxc = localx0 - rr * sinphi; 
    double localyc = localy0 + rr * cosphi;

    // basic idea (╯-_-)╯┴—┴
    // star global to CA local:
    // 1. rotate to sector 12
    // 2. (x,y,z) -> (y,x,-z)
    // change to star global here
    // local to global
    std::swap(localxc, localyc);

    // std::cout << ">>> " << CATrack->Alpha() / (3.14159/6.0) << ", "
    //           << fCATracker->Hit(fCATracker->TrackHit(CATrack->FirstHitRef())).ISlice() << std::endl;
    // std::cout << fCATracker->Hit(fCATracker->TrackHit(CATrack->FirstHitRef())).X() << ", "
    //           << fCATracker->Hit(fCATracker->TrackHit(CATrack->FirstHitRef())).Y() << ", "
    //           << fCATracker->Hit(fCATracker->TrackHit(CATrack->FirstHitRef())).Z() << std::endl;
    // std::cout << firstParam.GetX() << ", "
    //           << firstParam.GetY() << ", "
    //           << firstParam.GetZ() << std::endl << std::endl;
    
    // rotate
    const double alpha = CATrack->Alpha();
    const double ca = cos(alpha), sa = sin(alpha);
    double xc =  localxc*ca+localyc*sa;
    double yc = -localxc*sa+localyc*ca;

    //   printf("   xc %f  yc %f\n", xc, yc);

    nPointsInArray = 0;

    //now loop over hits
    const int NHits = CATrack->NHits();
    // printf("\n>>> %d\n", NHits);
    //    static int trackId = 0;
    // printf("\n\n# Track ID = %d\n", trackId++);
    // printf("%12.6f %12.6f\n", localyc, localxc);
    // LOG(INFO, ">>> %i\t%f\t%f", fCutLow, fCutHigh);
    for(int i = 0; i < NHits && nPointsInArray < 45; i++) {
        int index_CA = fCATracker->TrackHit(CATrack->FirstHitRef() + i);
        int hid = fCATracker->Hit(index_CA).ID();

        // std::cout << ">>> " << (fCATracker->Hit(index_CA).X() - localyc)/rr * (fCATracker->Hit(index_CA).X() - localyc)/rr +
        //     (fCATracker->Hit(index_CA).Y() - localxc)/rr * (fCATracker->Hit(index_CA).Y() - localxc)/rr << std::endl;
        //        printf(" %f %f\n", fCATracker->Hit(index_CA).X(), fCATracker->Hit(index_CA).Y());
        
        int index = id2index(hid); // kehw: ID == index of FtfHits/CAGBHits array
        // LOG(INFO, "%i\t%i", index, nFtfHits);
        //CA hits are in local coordinates
        //const AliHLTTPCCAGBHit *ihit = fcaTracker->Hit(index_CA);
        assert(index >= 0 && index < nFtfHits);
        const FtfHit *ihit = &(ftfHits[index]); // global coordinates

        //        printf("%12.6f %12.6f\n", ihit->x, ihit->y);
        // printf("%f %f %f\n", ihit->x, ihit->y, ihit->z);
        
        // discard hits with wrong charge information
        // i.e. hits of one-pad cluster or merged cluster
        if(ihit->flags) continue;

        // calculate direction of the tangent of circle at position
        // of given hit:
        //     - rotate radius vector to hit by -90 degrees
        //       matrix (  0   1 )
        //              ( -1   0 )
        //     - divide by length to get unity vector
        //printf("   tangent x %f y %f\n", tangent.x, tangent.y);

        struct christofs_2d_vector tangent;
        tangent.x = (ihit->y - yc) / rr;
        tangent.y = -(ihit->x - xc) / rr;
        // printf(">>> tangent x %f y %f, %f\n", tangent.x, tangent.y, tangent.x*tangent.x + tangent.y*tangent.y);

        // get crossing angle by calculating dot product of
        // tanget and unity vector perpendicular to row (look-up table)
        double cosCrossing = fabs(tangent.x * fUnitVec[ihit->sector - 1].x + tangent.y * fUnitVec[ihit->sector - 1].y);

        // padlength
        if(ihit->row < 14) padLength = padLengthInnerSector;
        else padLength = padLengthOuterSector;

        // ==> finally dx:
        if(cosCrossing == 0) continue;
        double dx = padLength / (cosCrossing * cosl);

        // drift length correctrion: d_loss [m^-1], l_drift [cm]
        // q_corr = q_meas / ( 1 - l_drift * d_loss/100 )
        double scaleDrift = 1 - fabs(ihit->z) * fDriftLoss / 100;

        // first shot: de_scale as implemented in tph.F,
        // i.e. gas gain, wire-to-pad coupling, etc.
        // !! hard coded, has to come from the db somewhere sometime !!
        double deScale;
        if(ihit->row < 14) deScale = innerSectorGain;
        else deScale = outerSectorGain;

        fDedxArray[nPointsInArray] = ihit->q * deScale / (dx * scaleDrift);
        nPointsInArray++;

        // LOG(INFO, ">>> %i\t%f\t%f", nPointsInArray, fCutLow, fCutHigh);
    
        //  printf("  ihit row %i x %f y %f z %f  q %d  q_scaled %e scale %e flags %d\n",
        //  ihit->row, ihit->x, ihit->y, ihit->z, ihit->q, ihit->q*deScale, deScale, ihit->flags);
        //  printf("    cosCros: %f, cosl: %f ===>> dx %f\n", cosCrossing, cosl, dx);

    }

    // sort dEdxArray
    sort(fDedxArray, fDedxArray + nPointsInArray);

    // LOG(INFO, ">>> %f\t%f", fCutLow, fCutHigh);
    
    // calculate absolute cuts
    int cLow  = (int) floor(nPointsInArray * fCutLow);
    int cHigh = (int) floor(nPointsInArray * fCutHigh);

    int    nDedx = 0;
    double dedx  = 0;

    for(int i = cLow; i < cHigh; i++) {
        nDedx++;
        dedx += fDedxArray[i];
    }

    // if(nDedx > 0) dedx = dedx / nDedx;
    dedx = nDedx > 0 ? dedx/nDedx : 0;
    
    CATrack->SetNDeDx(nDedx);
    CATrack->SetDeDx(dedx);
    
    // printf(">>> %d %f\n", nDedx, dedx*1e6);

    return 0;
}

int FtfDedx::id2index(int hid)
{
    // from CA/Ftf Hit id to index of Ftf hit array
    // for now hit id = hit index
    return hid;
}
