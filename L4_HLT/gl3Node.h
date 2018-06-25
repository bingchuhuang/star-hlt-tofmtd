//:>------------------------------------------------------------------
//: FILE:       gl3Nodes.h
//: discription:   a link of matched global track, primary track, bemc
//:              hit and tof hit
//:<------------------------------------------------------------------

#ifndef GL3NODE
#define GL3NODE
#include "gl3TOF.h"
class gl3Track;
class gl3EmcTower;

class gl3Node {
private:
public:
  gl3Node();

  gl3Track* globalTrack;
  gl3Track* primaryTrack;
  gl3EmcTower* emcTower;
  double emcMatchPhiDiff, emcMatchZEdge;
  gl3TOF::TofCell* globaltofCell;
  gl3TOF::TofCell* primarytofCell;
  double tof;
  double beta;  // v/c
  double tofLocalY;
  double tofLocalZ;
  double pathlength;
  int tofProjectChannel;
  
};

#endif
