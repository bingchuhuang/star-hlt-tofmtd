#ifndef _PROFILER_HH_
#define _PROFILER_HH_


#include <string.h>
#include <sys/time.h>


#include <rtsLog.h>
#include <I386/i386Lib.h>



#ifdef PROFILER_USE

#define PROFILER_DEFINE			class profiler PROFiler 
#define PROFILER_DECLARE		extern class profiler PROFiler

#define PROFILER_START() 		PROFiler.start_evt(__LINE__, __FILE__) 

#define PROFILER_DUMP()			PROFiler.dump()

#define PROFILER(X)  		PROFiler.mark(__LINE__, __FILE__,(X))

#define PROFILER_MHZ(X)		PROFiler.mhz((X)) ;

#else

// a bunch of dummies
#define PROFILER_DEFINE		volatile int PROFiler
#define PROFILER_DECLARE	extern volatile int PROFiler

#define PROFILER_START()
#define PROFILER_DUMP()
#define PROFILER(X)		PROFiler = (X) 
#define PROFILER_MHZ(X)

#endif

class profiler {
public:
	const static int PROFILER_MAX = 1000 ;

	profiler() { 
		dumped = 0 ;
		memset(prof,0,sizeof(prof)) ;

		stages = 1 ;	// stage 0 reserved for start

		prof[0].line = 0 ;
		prof[0].file = "START" ;
	

		cpu_mhz = 0 ;	// if 0 will do it in us!
	}

	struct {
		int line ;
		const char *file ;
		double dt ;
		double tick ;
		int cou ;
	} prof[PROFILER_MAX] ;


	~profiler() {
		dump() ;
	}



	void start_evt(int line, const char *file) {
		memset(prof,0,sizeof(prof)) ;

		prof[0].line = line ;
		prof[0].file = file ;
		prof[0].cou++ ;
		prof[0].dt = 0.0 ;



		if(cpu_mhz==0) {
			prof[0].tick = dtime() ;
		}
		else {
			prof[0].tick = (double)getfast_l() ;
		}
	}

	
	int mark(int line, const char *file, int previous) {

		double tick ;

		if(cpu_mhz==0) tick = dtime() ;
		else tick = (double) getfast_l() ;

		double delta = tick - prof[previous].tick ;

		for(int i=1;i<stages;i++) {
			if(prof[i].cou == 0) continue ;
			if(prof[i].line != line) continue ;
			if(strcmp(prof[i].file, file) != 0) continue ;

			prof[i].dt += delta ;
			prof[i].tick = tick ;
			prof[i].cou++ ;

			//LOG(DBG,"OLD %d: evt %d: %s[%d]: cou %d",i,prof[0].cou,file,line,prof[i].cou) ;

			return i ;
		}


		if(stages >= PROFILER_MAX) return 0 ;

		prof[stages].line = line ;
		prof[stages].file = file ;
		prof[stages].dt +=  delta ;
		prof[stages].cou++ ;
		prof[stages].tick = tick ;

		int ret = stages ;

		//LOG(DBG,"NEW %d: evt %d: %s[%d]: cou %d",stages,prof[0].cou,file,line,prof[stages].cou) ;
		
		stages++ ;

		return ret ;
	}

	void dump() {
		if(dumped) return ;
		char *c_type = "us" ;
		double div ;

		if(cpu_mhz) {
			div = (double) cpu_mhz ;
		}
		else {
			div = 1.0 ;
		}

		//dumped = 1 ;

		// zap to 0.0
		//prof[0].dt = 0.0 ;

		for(int i=0;i<PROFILER_MAX;i++) {
			if(prof[i].cou == 0) continue ;

			double mean_dt = prof[i].dt / prof[i].cou / div ;

			
			//LOG(TERR,"PROFILER %2d: %s[%d]: %d/%d hits: %.3f %s",
			LOG(INFO,"PROFILER %2d: %s[%d]: %d/%d hits: %.3f %s",
			//printf("PROFILER %2d: %s[%d]: %d/%d hits: %.3f %s\n",
				i,prof[i].file,prof[i].line,
				prof[i].cou,prof[0].cou,mean_dt,c_type) ;
		}
	}


	static int quick(int line, char *file, int prev) {
		extern class profiler PROFiler ;

		return PROFiler.mark(line,file,prev) ;
	}

	void mhz(unsigned int fre) {
		
		cpu_mhz = fre ;
	}

private:

	int stages ;

	int dumped ;

	unsigned int cpu_mhz ;

	static double dtime(void) {
		
		struct timeval tmval ;

		gettimeofday(&tmval, NULL) ;
		
		return ((double)tmval.tv_sec*1000000.0 + (double)tmval.tv_usec) ;
	}


} ;

#endif
