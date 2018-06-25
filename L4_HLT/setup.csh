#!/bin/tcsh

source /afs/rhic.bnl.gov/star/packages/.DEV2/setupDEV2.csh
starver .DEV2
setup 64b
setup gcc451

setenv PATH ~kehw/software/cmake-2.8.7-Linux-i386/bin:${PATH}
setenv PATH "/star/u/kehw/software/binutils-2.19/bin:${PATH}"
