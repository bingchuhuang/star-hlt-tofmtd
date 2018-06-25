#!/bin/tcsh

setenv USE_64BITS 1
setenv ROOTSYS /RTS/hlt_home/software/gl3-1/root/5.34.02

if ( $?CPATH == 0 ) then
	setenv CPATH ""
endif
if ( $?LD_LIBRARY_PATH == 0 ) then
	setenv LD_LIBRARY_PATH ""
endif

# root
set ddd = `pwd`
cd /RTS/hlt_home/software/gl3-1/root/5.34.02/bin/
source thisroot.csh
cd $ddd
setenv PATH "/RTS/hlt_home/software/gl3-1/bin:${PATH}"
setenv LD_LIBRARY_PATH "/RTS/hlt_home/software/gl3-1/lib:/RTS/hlt_home/software/gl3-1/lib64:${LD_LIBRARY_PATH}"

# for gcc 4.4.5
setenv PATH "/RTS/hlt_home/software/gl3-1/gcc/4.4.5/bin:${PATH}"
setenv LD_LIBRARY_PATH "/RTS/hlt_home/software/gl3-1/gcc/4.4.5/lib:/RTS/hlt_home/software/gl3-1/gcc/4.4.5/lib64:${LD_LIBRARY_PATH}"

# for gcc 4.5.3
# export PATH="/RTS/hlt_home/software/gl3-1/gcc/4.5.3/bin:${PATH}"
# export LD_LIBRARY_PATH="/RTS/hlt_home/software/gl3-1/gcc/4.5.3/lib:/RTS/hlt_home/software/gl3-1/gcc/4.5.3/lib64:${LD_LIBRARY_PATH}"

# DAQ Group Project directory
setenv PROJDIR /RTS/exe/l4
setenv PATH "/RTS/exe/l4/bin/LINUX/x86_64:${PATH}"

