//:>------------------------------------------------------------------
//: FILE:       gl3Bischels.h
//: discription:   theretical dedx
//:<------------------------------------------------------------------

#ifndef GL3BISCHEL
#define GL3BISCHEL

class gl3Track;

class gl3Bischel {
private:
public:
  gl3Bischel(char* parametersDirectory = "./");
  void readTable(char* parametersDirectory = "./");
  double getDedxMean(double p, const char* particle);
};

#endif
