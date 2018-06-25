#ifndef GL3MTD_H
#define GL3MTD_H

#include "../HLT_MTD/mtd_defs.h"
#include <math.h>

class gl3Track;

class gl3MTD {
public:
    gl3MTD();
    ~gl3MTD();

    int    readFromMtdMachine(char *trgSend);
    int    findMatchedMtdHit(gl3Track *track, double &dz, double &dy);
    int    extrapolateTrack(gl3Track *track, int bFieldPolarity, double bfield, double r, double &phi, double &z, double &psic);
    int    getMtdBackleg(const double projPhi);
    int    getMtdModule(const double projZ);
    double getMtdHitGlobalZ(double leadingWestTime, double leadingEastTime, int module);
    double getMtdHitGlobalPhi(int backleg, int module, int channel);
    double rotatePhi(double phi);
    void   reset();
    
    struct vector3 {
        double x;
        double y;
        double z;
    };
    void   global2Local(vector3 global, int backleg, int module, vector3 &local);

    Mtdsend *mtdEvent;

    static const double mtdRadius;
    static const double innerSteelRadius;
    static const double outerSteelRadius;
    static const double stripLength;
    static const double stripWidth;
    static const double stripGap;
    static const double bFieldInSteel;
    static const double tesla;
    static const double pi;
    static const double backlegPhiWidth;
    static const double backlegPhiGap;
    static const double firstBacklegPhi;
    static const double vDrift;
    
private:
    int    evt_count;
    double bField;
};

#endif

