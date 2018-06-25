//:>------------------------------------------------------------------
//:<------------------------------------------------------------------
#include <stdio.h>
#include <math.h>

#ifndef GL3HISTO 
#define GL3HISTO 

class gl3HistoHeader {
 public:
   char      id[64]; 
   char      title[128]; 
   double    sum ;
   double    vMin ;
   double    vMax ;
   double    xMin ;
   double    xMax ;
   double    xStep ;
   // JB 08312K
   int       nEntries ;
   int       nBins;
   int      maxBin;
   int      padding; // needed for 64-bit alignment
};

class gl3Histo {
public:
   gl3HistoHeader header ;
   double*   info ;

public:
   gl3Histo ( char iId[10]="id", char iTitle[100]="name", 
              int iNBins=100,  double iXMin=0., double iXMax=100. ) ;
   ~gl3Histo ( ) ;
   int    Fill   (double x, double weight=1) ;
   double GetValue   (int iBin ) ;
   int    Print  (short Level=1 ) ;
   int    Read   (char* input  ) ; 
   int    Read   (FILE* f  ) ; 
   int    Reset  (  ) ;
   int    Write  ( unsigned int maxBytes, char* output ) ; 
   int    Write  ( FILE* f ) ; 
   size_t Size();

   // JB 08/15/2K added some methods
   // --------------------------------------------------------------------
   double GetMaximum();
   int GetMaximumBin();
   double GetBinCenter(int Bin);
   double Integral(int minBin, int maxBin);
   double getWeightedMean(double sigmaWidthBins=4);
};

class gl3Histo1D: public gl3Histo {
 public:
  gl3Histo1D ( char iId[10]="id", char iTitle[100]="name", 
	       int iNBins=100,  double iXMin=0., double iXMax=100. ) ;
};

class gl3Histo2DHeader {
 public:
   char      id[64]; 
   char      title[128]; 
   double    sum ;
   double    vMin ;
   double    vMax ;
   double    xMin ;
   double    xMax ;
   double    yMin ;
   double    yMax ;
   double    xStep ;
   double    yStep ;
   // JB 08312K
   int       nEntries ;
   int       nBinsX, nBinsY;
   int      maxBinX, maxBinY;
   int      padding; // needed for 64-bit alignment
};

class gl3Histo2D {
public:
   gl3Histo2DHeader header ;
   double*   info ;

public:
   gl3Histo2D ( char iId[10]="id", char iTitle[100]="name", 
                int iNBinsX=100,  double iXMin=0., double iXMax=100.,
		int iNBinsY=100,  double iYMin=0., double iYMax=100.) ;
   ~gl3Histo2D ( ) ;
   int   Fill   (double x, double y, double weight=1) ;
   double GetValue   (int iBinX, int iBinY ) ;
   int   Print  (short Level=1 ) ;
   int   Read   (char* input  ) ; 
   int   Read   (FILE* f  ) ; 
   int   Reset  (  ) ;
   int   Write  ( unsigned int maxBytes, char* output ) ; 
   int   Write  ( FILE* f ) ; 
   size_t Size();

};

class gl3Histo3DHeader {
 public:
   char      id[64]; 
   char      title[128]; 
   double    sum ;
   double    vMin ;
   double    vMax ;
   double    xMin ;
   double    xMax ;
   double    yMin ;
   double    yMax ;
   double    zMin ;
   double    zMax ;
   double    xStep ;
   double    yStep ;
   double    zStep ;
   // JB 08312K
   int       nEntries ;
   int       nBinsX, nBinsY, nBinsZ;
   int      maxBinX, maxBinY, maxBinZ;
   int      padding; // needed for 64-bit alignment
};

class gl3Histo3D {
public:
   gl3Histo3DHeader header ;
   double*   info ;

public:
   gl3Histo3D ( char iId[10]="id", char iTitle[100]="name", 
		int iNBinsX=100,  double iXMin=0., double iXMax=100.,
		int iNBinsY=100,  double iYMin=0., double iYMax=100.,
		int iNBinsZ=100,  double iZMin=0., double iZMax=100.) ;
   ~gl3Histo3D ( ) ;
   int   Fill   (double x, double y, double z, double weight=1) ;
   double GetValue   (int iBinX, int iBinY, int iBinZ ) ;
   int   Print  (short Level=1 ) ;
   int   Read   (char* input  ) ; 
   int   Read   (FILE* f  ) ; 
   int   Reset  (  ) ;
   int   Write  ( unsigned int maxBytes, char* output ) ; 
   int   Write  ( FILE* f ) ; 
   size_t Size();

};

#endif
