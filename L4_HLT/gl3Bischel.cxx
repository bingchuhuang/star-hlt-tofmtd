#include "gl3Bischel.h"
#include "FtfGeneral.h"
#include "gl3Track.h"
#include <iostream>
#include <string.h>
#include <fstream>

const int nParticles = 8;
const int nP = 11900;
const double pMin = 0.1;
const double pMax = 12.;
double dedxMean[nParticles][nP+1];
char* Names[nParticles] = {"Pion", "Kaon", "Proton", "Electron", "Deuteron", "Triton", "He3", "He4"};
double Masses[nParticles] = {0.13957, 0.49368, 0.93827, 0.000510999, 1.875613, 2.80925, 2.80923, 3.727417};

gl3Bischel::gl3Bischel(char* parametersDirectory)
{
  readTable(parametersDirectory);
}

void gl3Bischel::readTable(char* parametersDirectory)
{
  char fileName[1024];
  string tem;
  for(int i=0; i<nParticles; i++)
    {
      sprintf(fileName, "%sdedx_mean_%s", parametersDirectory, Names[i]);
      ifstream ifs(fileName);
      getline(ifs, tem);
      for(int j=0; j<nP+1; j++)
	{
	  ifs>>tem>>dedxMean[i][j];
	  dedxMean[i][j] *= 1.e-6;
	}
    }

}

double gl3Bischel::getDedxMean(double p, const char* particle)
{
  int particleIndex = -1;
  for(int i=0; i<nParticles; i++)
    if(strcmp(particle, Names[i]) == 0)
      particleIndex = i;
  if(particleIndex == -1) return 0;
  if(p<0.1) return 0;
  if(p>12.) return dedxMean[particleIndex][nP];
  double pBin = (pMax-pMin)/nP;
  int pIndex = (int)((p-pMin)/pBin);
  double dp = p-pMin-pIndex*pBin;
  return (1.-dp/pBin)*dedxMean[particleIndex][pIndex] + dp/pBin*dedxMean[particleIndex][pIndex+1];
}

