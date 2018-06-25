//:>------------------------------------------------------------------
//: FILE:       gl3Tracks.h
//: HISTORY:
//:              6dec1999 version 1.00
//:<------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "gl3Histo.h"
#include <stdio.h>



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo::gl3Histo ( char iId[10], char iTitle[100], 
      int iNBins,  double iXMin, double iXMax ) {
   info = 0 ;
   if ( iNBins < 0 ) {
      fprintf ( stderr, " %d bins, not valid value \n", (int)iNBins );
      return ;
   }
   header.nBins = iNBins ;
   if ( iXMin > iXMax  ) {
      fprintf ( stderr, " xMin %e xMax %e, not valid values \n", iXMin, iXMax );
      return ;
   }
   strcpy ( header.id, iId ) ;
   strcpy ( header.title, iTitle ) ;
   header.nBins    = iNBins ;
   header.xMin     = iXMin ;
   header.xMax     = iXMax ;
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.vMin     = header.vMax = 0. ;
   header.xStep    = (header.xMax-header.xMin)/float(header.nBins);
   header.maxBin   = 0;
   
   info     = new double[header.nBins+2]; 
   memset ( info, 0, (header.nBins+2)*sizeof(double) ) ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo::~gl3Histo ( ) {
   if ( info ) delete[] info ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo::Fill ( double x, double weight ) {
    int iBin = (int)((x-header.xMin)/header.xStep+1.) ;
    if ( iBin < 1 ) iBin = 0 ;
    else if ( iBin > header.nBins ) iBin = header.nBins + 1 ; 

    info[iBin] += weight ;
    if ( iBin > 1 && iBin < header.nBins + 1 ) {
	if ( info[iBin] > header.vMax ) {
	    header.vMax = info[iBin] ;
	    header.maxBin= iBin ;       
	}
	if ( info[iBin] < header.vMin ) header.vMin = info[iBin] ;
    }
    header.sum += weight ;
    header.nEntries++ ;
    return 0 ;
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo::GetValue ( int iBin ) {
   if ( iBin < 0 || iBin > header.nBins+2 ) {
       printf("out of range\n");
       return 0 ;
   }
   return info[iBin] ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo::Print ( short level  ) {
   printf ( "gl3Histo::Print: id \"%s\" title \"%s\" nBins %d nEntries %d\n", 
             header.id, header.title, 
	    (int)header.nBins, (int)header.nEntries ) ;
   return 0 ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo::Read ( char* input ) {

//     printf("%x / %x / %x\n",
// 	   sizeof(gl3HistoHeader),
// 	   ((gl3HistoHeader*)input)->nBins,
// 	   ( sizeof(gl3HistoHeader) + 
// 	     sizeof(double) * (((gl3HistoHeader*)input)->nBins+2)));
	   
    memcpy ( &header, input, sizeof(gl3HistoHeader) ) ;
    
    if ( info ) delete info ;
    info     = new double[header.nBins+2]; 
    
    memcpy ( info, input+sizeof(gl3HistoHeader), 
	     (header.nBins+2)*sizeof(double) ) ;
    
    // printf ( "length 2 %d \n", length_2 ) ;
    
//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }
    
    return sizeof(gl3HistoHeader) + (header.nBins+2)*sizeof(double);
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo::Read ( FILE* f ) {
    fread ( &header, sizeof(gl3HistoHeader), 1, f ) ;
    
    if ( info ) delete info ;
    info     = new double[header.nBins+2]; 
    
    fread ( info, (header.nBins+2)*sizeof(double), 1, f ) ;
    return Size();
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//##########################################################################
int gl3Histo::Reset (  ) {
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.maxBin   = 0;
   header.vMin     = 0;
   header.vMax     = 0;
   memset ( info, 0, (header.nBins+2)*sizeof(double) ) ;
   return 0 ;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo::Write ( unsigned int maxBytes, char* output ) {

    if ( Size() >= maxBytes ) {
        printf("gl3Histo::Write %u bytes in buffer not enough \n", maxBytes);
	return 0 ;
    }
   
    memcpy ( output, &header, sizeof(gl3HistoHeader) ) ;
    memcpy ( output + sizeof(gl3HistoHeader), info,
	     (header.nBins+2)*sizeof(double) );

//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }

    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo::Write ( FILE* f ) {
    fwrite ( &header, sizeof(gl3HistoHeader), 1, f ) ;
    fwrite ( info, (header.nBins+2)*sizeof(double), 1, f ) ;
    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
size_t gl3Histo::Size () {
    return sizeof(gl3HistoHeader) + (header.nBins+2)*sizeof(double);
}

// JB 08/15/2K added some methods
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo::GetMaximum()
{
 return header.vMax;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo::GetMaximumBin()
{
 return header.maxBin;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo::GetBinCenter(int Bin)
{
 return(header.xMin+(float(Bin-0.5)*header.xStep)) ;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo::Integral(int minBin, int maxBin)
{
      double sum=0;

      //if (minBin < 0) minBin = 0;
      //if (maxBin >= header.nBins+2) maxBin=header.nBins+1;

      if(minBin>=0 && maxBin<header.nBins) {
	    for(int cnt=minBin; cnt<=maxBin; cnt++) sum += GetValue(cnt);
	    return sum;
      }
      else return 0;
}



//####################################################################
// calculates (fast) the fuckin weighted mean of a gl3Histo
//####################################################################
double gl3Histo::getWeightedMean(double sigmaWidthBins)
{
  // Weighted mean of Histo
  
  // suggestion
  // sigmaWidthBins = 4; 
  double HistoMax = GetMaximum();
  int HistoMaxBin = GetMaximumBin();
  double HistoMaxCenter = GetBinCenter(HistoMaxBin);
  
  double SigmaHalfL = Integral((int)(HistoMaxBin-sigmaWidthBins),HistoMaxBin-1);
  double SigmaHalfR = Integral(HistoMaxBin+1,(int)(HistoMaxBin+sigmaWidthBins));
  double SigmaHalfLCenter=0;
  
  for(int cnt=HistoMaxBin-1; cnt>=(HistoMaxBin-sigmaWidthBins); cnt--) {
    SigmaHalfLCenter = SigmaHalfLCenter + GetBinCenter(cnt);
  }
  SigmaHalfLCenter=SigmaHalfLCenter/sigmaWidthBins;
  
  double SigmaHalfRCenter=0;
  for(int cnt=HistoMaxBin+1; cnt<=(HistoMaxBin+sigmaWidthBins); cnt++) {
    SigmaHalfRCenter = SigmaHalfRCenter+GetBinCenter(cnt);
  }
  SigmaHalfRCenter=SigmaHalfRCenter/sigmaWidthBins;
  
  double weightedMean;
  if((SigmaHalfL+HistoMax+SigmaHalfR)>0) {
    weightedMean = ( (SigmaHalfL * SigmaHalfLCenter) + 
		     (HistoMax * HistoMaxCenter) + 
		     (SigmaHalfR * SigmaHalfRCenter) ) 
      / (SigmaHalfL+HistoMax+SigmaHalfR);
  }
  else weightedMean=0.0;  // CAUTION METHOD RETURNS 0.0 IF IT FAILS
  
  return(weightedMean);
}
gl3Histo1D::gl3Histo1D ( char iId[10], char iTitle[100], 
      int iNBins,  double iXMin, double iXMax ) {
   info = 0 ;
   if ( iNBins < 0 ) {
      fprintf ( stderr, " %d bins, not valid value \n", (int)iNBins );
      return ;
   }
   header.nBins = iNBins ;
   if ( iXMin > iXMax  ) {
      fprintf ( stderr, " xMin %e xMax %e, not valid values \n", iXMin, iXMax );
      return ;
   }
   strcpy ( header.id, iId ) ;
   strcpy ( header.title, iTitle ) ;
   header.nBins    = iNBins ;
   header.xMin     = iXMin ;
   header.xMax     = iXMax ;
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.vMin     = header.vMax = 0. ;
   header.xStep    = (header.xMax-header.xMin)/float(header.nBins);
   header.maxBin   = 0;
   
   info     = new double[header.nBins+2]; 
   memset ( info, 0, (header.nBins+2)*sizeof(double) ) ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo2D::gl3Histo2D ( char iId[10], char iTitle[100], 
			 int iNBinsX,  double iXMin, double iXMax,
			 int iNBinsY,  double iYMin, double iYMax) {
   info = 0 ;
   if ( iNBinsX < 0 ) {
      fprintf ( stderr, " %d x bins, not valid value \n", (int)iNBinsX );
      return ;
   }
   if ( iNBinsY < 0 ) {
      fprintf ( stderr, " %d y bins, not valid value \n", (int)iNBinsY );
      return ;
   }
   header.nBinsX = iNBinsX ;
   header.nBinsY = iNBinsY ;
   if ( iXMin > iXMax  ) {
      fprintf ( stderr, " xMin %e xMax %e, not valid values \n", iXMin, iXMax );
      return ;
   }
   if ( iYMin > iYMax  ) {
      fprintf ( stderr, " yMin %e yMax %e, not valid values \n", iYMin, iYMax );
      return ;
   }
   strcpy ( header.id, iId ) ;
   strcpy ( header.title, iTitle ) ;
   header.nBinsX    = iNBinsX ;
   header.nBinsY    = iNBinsY ;
   header.xMin     = iXMin ;
   header.xMax     = iXMax ;
   header.yMin     = iYMin ;
   header.yMax     = iYMax ;
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.vMin     = header.vMax = 0. ;
   header.xStep    = (header.xMax-header.xMin)/float(header.nBinsX);
   header.yStep    = (header.yMax-header.yMin)/float(header.nBinsY);
   header.maxBinX   = 0;
   header.maxBinY   = 0;
   
   info     = new double[(header.nBinsX+2)*(header.nBinsY+2)]; 
   memset ( info, 0, (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double) ) ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo2D::~gl3Histo2D ( ) {
   if ( info ) delete[] info ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo2D::Fill ( double x, double y, double weight ) {
    int iBinX = (int)((x-header.xMin)/header.xStep+1.) ;
    int iBinY = (int)((y-header.yMin)/header.yStep+1.) ;
    if ( iBinX < 1 ) iBinX = 0 ;
    else if ( iBinX > header.nBinsX ) iBinX = header.nBinsX + 1 ; 
    if ( iBinY < 1 ) iBinY = 0 ;
    else if ( iBinY > header.nBinsY ) iBinY = header.nBinsY + 1 ; 

    info[iBinX*(header.nBinsY+2)+iBinY] += weight ;
    if ( iBinX > 1 && iBinX < header.nBinsX + 1 && iBinY > 1 && iBinY < header.nBinsY + 1) {
	if ( GetValue ( iBinX, iBinY ) > header.vMax ) {
	    header.vMax = GetValue ( iBinX, iBinY ) ;
	    header.maxBinX= iBinX ;       
	    header.maxBinY= iBinY ;       
	}
	if (GetValue ( iBinX, iBinY ) < header.vMin ) header.vMin = GetValue ( iBinX, iBinY );
    }
    header.sum += weight ;
    header.nEntries++ ;
    return 0 ;
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo2D::GetValue ( int iBinX, int iBinY ) {
   if ( iBinX < 0 || iBinX > header.nBinsX+2 || iBinY < 0 || iBinY > header.nBinsY+2 ) {
       printf("out of range\n");
       return 0 ;
   }
   return info[iBinX*(header.nBinsY+2)+iBinY] ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo2D::Print ( short level  ) {
   printf ( "gl3Histo2D::Print: id \"%s\" title \"%s\" nBinsX %d nBinsY %d nEntries %d\n", 
             header.id, header.title, 
	    (int)header.nBinsX, (int)header.nBinsY, (int)header.nEntries ) ;
   return 0 ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo2D::Read ( char* input ) {

//     printf("%x / %x / %x\n",
// 	   sizeof(gl3Histo2DHeader),
// 	   ((gl3Histo2DHeader*)input)->nBins,
// 	   ( sizeof(gl3Histo2DHeader) + 
// 	     sizeof(double) * (((gl3Histo2DHeader*)input)->nBins+2)));
	   
    memcpy ( &header, input, sizeof(gl3Histo2DHeader) ) ;
    
    if ( info ) delete info ;
    info     = new double[(header.nBinsX+2)*(header.nBinsY+2)]; 
    
    memcpy ( info, input+sizeof(gl3Histo2DHeader), 
	     (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double) ) ;
    
    // printf ( "length 2 %d \n", length_2 ) ;
    
//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }
    
    return sizeof(gl3Histo2DHeader) + (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double);
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo2D::Read ( FILE* f ) {
    fread ( &header, sizeof(gl3Histo2DHeader), 1, f ) ;
    
    if ( info ) delete info ;
    info     = new double[(header.nBinsX+2)*(header.nBinsY+2)]; 
    
    fread ( info, (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double), 1, f ) ;
    return Size();
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//##########################################################################
int gl3Histo2D::Reset (  ) {
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.maxBinX   = 0;
   header.maxBinY   = 0;
   header.vMin     = 0;
   header.vMax     = 0;
   memset ( info, 0, (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double) ) ;
   return 0 ;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo2D::Write ( unsigned int maxBytes, char* output ) {

    if ( Size() >= maxBytes ) {
	
	printf("gl3Histo2D::Write %d bytes in buffer not enough \n", maxBytes);
	return 0 ;
    }
   
    memcpy ( output, &header, sizeof(gl3Histo2DHeader) ) ;
    memcpy ( output + sizeof(gl3Histo2DHeader), info,
	     (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double) );

//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }

    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo2D::Write ( FILE* f ) {
    fwrite ( &header, sizeof(gl3Histo2DHeader), 1, f ) ;
    fwrite ( info, (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double), 1, f ) ;
    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
size_t gl3Histo2D::Size () {
    return sizeof(gl3Histo2DHeader) + (header.nBinsX+2)*(header.nBinsY+2)*sizeof(double);
}




//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo3D::gl3Histo3D ( char iId[10], char iTitle[100], 
			 int iNBinsX,  double iXMin, double iXMax,
			 int iNBinsY,  double iYMin, double iYMax,
			 int iNBinsZ,  double iZMin, double iZMax) {
   info = 0 ;
   if ( iNBinsX < 0 ) {
      fprintf ( stderr, " %d x bins, not valid value \n", (int)iNBinsX );
      return ;
   }
   if ( iNBinsY < 0 ) {
      fprintf ( stderr, " %d y bins, not valid value \n", (int)iNBinsY );
      return ;
   }
   if ( iNBinsZ < 0 ) {
      fprintf ( stderr, " %d z bins, not valid value \n", (int)iNBinsZ );
      return ;
   }
   header.nBinsX = iNBinsX ;
   header.nBinsY = iNBinsY ;
   header.nBinsZ = iNBinsZ ;
   if ( iXMin > iXMax  ) {
      fprintf ( stderr, " xMin %e xMax %e, not valid values \n", iXMin, iXMax );
      return ;
   }
   if ( iYMin > iYMax  ) {
      fprintf ( stderr, " yMin %e yMax %e, not valid values \n", iYMin, iYMax );
      return ;
   }
   if ( iZMin > iZMax  ) {
      fprintf ( stderr, " zMin %e zMax %e, not valid values \n", iZMin, iZMax );
      return ;
   }
   strcpy ( header.id, iId ) ;
   strcpy ( header.title, iTitle ) ;
   header.nBinsX    = iNBinsX ;
   header.nBinsY    = iNBinsY ;
   header.nBinsZ    = iNBinsZ ;
   header.xMin     = iXMin ;
   header.xMax     = iXMax ;
   header.yMin     = iYMin ;
   header.yMax     = iYMax ;
   header.zMin     = iZMin ;
   header.zMax     = iZMax ;
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.vMin     = header.vMax = 0. ;
   header.xStep    = (header.xMax-header.xMin)/float(header.nBinsX);
   header.yStep    = (header.yMax-header.yMin)/float(header.nBinsY);
   header.zStep    = (header.zMax-header.zMin)/float(header.nBinsZ);
   header.maxBinX   = 0;
   header.maxBinY   = 0;
   header.maxBinZ   = 0;
   
   info     = new double[(header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)]; 
   memset ( info, 0, (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double) ) ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
gl3Histo3D::~gl3Histo3D ( ) {
   if ( info ) delete[] info ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo3D::Fill ( double x, double y, double z, double weight ) {
    int iBinX = (int)((x-header.xMin)/header.xStep+1.) ;
    int iBinY = (int)((y-header.yMin)/header.yStep+1.) ;
    int iBinZ = (int)((z-header.zMin)/header.zStep+1.) ;
    if ( iBinX < 1 ) iBinX = 0 ;
    else if ( iBinX > header.nBinsX ) iBinX = header.nBinsX + 1 ; 
    if ( iBinY < 1 ) iBinY = 0 ;
    else if ( iBinY > header.nBinsY ) iBinY = header.nBinsY + 1 ; 
    if ( iBinZ < 1 ) iBinZ = 0 ;
    else if ( iBinZ > header.nBinsZ ) iBinZ = header.nBinsZ + 1 ; 

    info[(iBinX*(header.nBinsY+2)+iBinY)*(header.nBinsZ+2)+iBinZ] += weight ;
    if ( iBinX > 1 && iBinX < header.nBinsX + 1 && iBinY > 1 && iBinY < header.nBinsY + 1 && iBinZ > 1 && iBinZ < header.nBinsZ + 1) {
	if ( GetValue ( iBinX, iBinY, iBinZ ) > header.vMax ) {
	    header.vMax = GetValue ( iBinX, iBinY, iBinZ ) ;
	    header.maxBinX= iBinX ;       
	    header.maxBinY= iBinY ;       
	    header.maxBinZ= iBinZ ;       
	}
	if (GetValue ( iBinX, iBinY, iBinZ ) < header.vMin ) header.vMin = GetValue ( iBinX, iBinY, iBinZ );
    }
    header.sum += weight ;
    header.nEntries++ ;
    return 0 ;
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
double gl3Histo3D::GetValue ( int iBinX, int iBinY, int iBinZ ) {
   if ( iBinX < 0 || iBinX > header.nBinsX+2 || iBinY < 0 || iBinY > header.nBinsY+2 || iBinZ < 0 || iBinZ > header.nBinsZ+2 ) {
       printf("out of range\n");
       return 0 ;
   }
   return info[(iBinX*(header.nBinsY+2)+iBinY)*(header.nBinsZ+2)+iBinZ] ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//####################################################################################
int gl3Histo3D::Print ( short level  ) {
   printf ( "gl3Histo3D::Print: id \"%s\" title \"%s\" nBinsX %d nBinsY %d nBinsZ %d nEntries %d\n", 
             header.id, header.title, 
	    (int)header.nBinsX, (int)header.nBinsY, (int)header.nBinsZ, (int)header.nEntries ) ;
   return 0 ;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo3D::Read ( char* input ) {

//     printf("%x / %x / %x\n",
// 	   sizeof(gl3Histo3DHeader),
// 	   ((gl3Histo3DHeader*)input)->nBins,
// 	   ( sizeof(gl3Histo3DHeader) + 
// 	     sizeof(double) * (((gl3Histo3DHeader*)input)->nBins+2)));
	   
    memcpy ( &header, input, sizeof(gl3Histo3DHeader) ) ;
    
    if ( info ) delete info ;
    info     = new double[(header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)]; 
    
    memcpy ( info, input+sizeof(gl3Histo3DHeader), 
	     (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double) ) ;
    
    // printf ( "length 2 %d \n", length_2 ) ;
    
//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }
    
    return sizeof(gl3Histo3DHeader) + (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double);
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#####################################################################
int gl3Histo3D::Read ( FILE* f ) {
    fread ( &header, sizeof(gl3Histo3DHeader), 1, f ) ;
    
    if ( info ) delete info ;
    info     = new double[(header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)]; 
    
    fread ( info, (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double), 1, f ) ;
    return Size();
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//##########################################################################
int gl3Histo3D::Reset (  ) {
   header.nEntries = 0 ;
   header.sum      = 0 ;
   header.maxBinX   = 0;
   header.maxBinY   = 0;
   header.maxBinZ   = 0;
   header.vMin     = 0;
   header.vMax     = 0;
   memset ( info, 0, (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double) ) ;
   return 0 ;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo3D::Write ( unsigned int maxBytes, char* output ) {

    if ( Size() >= maxBytes ) {
	
	printf("gl3Histo3D::Write %d bytes in buffer not enough \n", maxBytes);
	return 0 ;
    }
   
    memcpy ( output, &header, sizeof(gl3Histo3DHeader) ) ;
    memcpy ( output + sizeof(gl3Histo3DHeader), info,
	     (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double) );

//     for (int i=0; i<header.nBins+2; i++) {
// 	printf("%d: %f\n", i, info[i]);
//     }

    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
int gl3Histo3D::Write ( FILE* f ) {
    fwrite ( &header, sizeof(gl3Histo3DHeader), 1, f ) ;
    fwrite ( info, (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double), 1, f ) ;
    return Size();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//#########################################################################
size_t gl3Histo3D::Size () {
    return sizeof(gl3Histo3DHeader) + (header.nBinsX+2)*(header.nBinsY+2)*(header.nBinsZ+2)*sizeof(double);
}
