#include "gl3Node.h"

gl3Node::gl3Node()
{
  globalTrack = 0;
  primaryTrack = 0;
  emcTower = 0;
  emcMatchPhiDiff = 0;
  emcMatchZEdge = 0;
  globaltofCell = 0;
  primarytofCell = 0;
  tof = 0;
  beta = 0;

  tofLocalY = 0;
  tofLocalZ = 0;
  pathlength = 0;

  tofProjectChannel = 0;
}
