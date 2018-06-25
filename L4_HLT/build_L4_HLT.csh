#!/bin/tcsh -f

if ( -e Vc ) then
    cd Vc
    ./build.csh
    cd ..
else
    echo "Error: Cannot find Vc package"
endif

if ( -e TPCCATracker ) then
    cd TPCCATracker
    ./build.csh
    cd ..
else
    echo "Error: Cannot find CA tracker"
endif

make -B -j
