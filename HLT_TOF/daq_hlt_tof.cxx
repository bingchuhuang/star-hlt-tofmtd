#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include <rtsLog.h>
#include <iccp.h>
#include <cmds.h>
#include <SUNRT/ipcQLib.hh>
#include <SUNRT/shmLib.h>


#include <SFS/sfs_index.h>

#include <DAQ_READER/daqReader.h>
#include <DAQ_READER/daq_dta.h>
#include <DAQ_TOF/daq_tof.h>

#include "../HLT/daq_hlt.h"
#include "tof_hlt_sector.h"

extern const char *BuildDate ;

static int sector ;	// counts from 1 
static int my_id ;	// counts from 0
static int my_mem_id ;
static char *in_mem ;
static char *out_mem ;
static sem_t *in_mem_sem ;
static int use_gl3 ;

static ipcQClass *que ;
static int run_started ;
static u_int run_number ;

static tof_hlt_sector *sl3_tracker ;

static u_int sl3_evts ;
static int debug_mode ;

static int tracer	;	// used to trace the current state...



static int do_config(ic_msg *ic) ;
static int do_track(ic_msg *ic, char *in_mem, char *out_mem, int max_bytes) ;
static int do_send(char *mem, int bytes) ;


// used in emulation from file...
static void *sender_thread(void *arg) ;
static int sector_from_daq ;

static sfs_index sfs_debug ;
static sfs_index sfs_gl3[GL3_MAX_INSTANCES] ;
static int gl3_state[GL3_MAX_INSTANCES] ;
static int gl3_ip[GL3_MAX_INSTANCES] ;
static int gl3_num ;


static void sig_handler(int wha)
{
	LOG(ERR,"SL3 %d: signal %d, tracer at %d",my_id,wha,tracer) ;
	exit(-1) ;
}



static int do_run_start(int run)
{

	run_number = run ;
	tracer = 10 ;
	LOG(INFO,"SL3 %d: starting run %08d, using %d GL3 instances...",my_id,run_number, gl3_num) ;


	run_started = 1 ;
	sl3_evts = 0 ;

	return 0 ;
}

static int do_run_stop()
{
	tracer = 20 ;
	LOG(INFO,"SL3 %d: stopping run %08d, %d events",my_id,run_number,sl3_evts) ;

	run_started = 0 ;
//	sfs_debug.umount() ;


	for(int i=0;i<GL3_MAX_INSTANCES;i++) {
		if(gl3_state[i]) {
			sfs_gl3[i].umount() ;
			gl3_state[i] = 0 ;
		}
	}

	gl3_num = 0 ;

	return 0 ;
}

static int choose_gl3(int token)
{
	if(gl3_num==0) return -1 ;	// for now!

	int ix = token % gl3_num ;

	return ix ;
}

int main(int argc, char *argv[])
{
	ic_msg ic ;
	int ret ;
	char fname[128] ;
	char sem_name[64] ;

	extern char *optarg ;
	int c ;

	rtsLogLevel(WARN) ;
	my_id = 0 ;
	sector_from_daq = 1 ;
	fname[0] = 0 ;



	while((c = getopt(argc, argv, "d:s:i:f:x:")) != EOF) {
		switch(c) {
		case 'd' :
			rtsLogLevel(optarg) ;
			break ;
		case 's' :
			sector_from_daq = atoi(optarg) ;
			break ;
		case 'i' :
			my_id = atoi(optarg) ;
			break ;
		case 'f' :
			strcpy(fname,optarg) ;
			break ;
		case 'x' :
			debug_mode = atoi(optarg) ;
			break ;
		default :
			fprintf(stderr,"Usage: %s -s <sector> -d <loglevel> -i <instance> -f <input_file>\n",argv[0]) ;
			return -1 ;
		}
	}

	
	tracer = 0 ;

	// setup
	signal(SIGPIPE,SIG_IGN) ;
	signal(SIGSEGV,sig_handler) ;
	signal(SIGBUS,sig_handler) ;
	signal(SIGILL,sig_handler) ;
	signal(SIGXFSZ,sig_handler) ;
	
	// if running from file, we create a DAQ emulator!
	if(fname[0]) {
		rtsLogOutput(RTS_LOG_STDERR) ;

		LOG(INFO,"Running through file %s -- creating TOF emulator",fname) ;
		pthread_t thr_id ;
		pthread_create(&thr_id,0,sender_thread,fname) ;
		sleep(2) ;	// it needs time to initialize
	}
	else {
		use_gl3 = 1 ;
		rtsLogAddDest(RTS_DAQMAN,RTS_LOG_PORT_L3) ;
	}

	if(debug_mode==1) use_gl3 = 1 ;

	LOG(NOTE,"SL3 %d: Starting.",my_id) ;

	// clear or set misc variables
	run_started = 0 ;
	run_number = 0 ;
	sl3_tracker = 0 ;


	// open queue for reading and NOT create
	errno = 0 ;
	que = new ipcQClass(SL3_TASK+my_id,0,0) ;
	if(errno) {
		LOG(ERR,"SL3 %3d: queue not found [%s]",my_id,strerror(errno)) ;
		return -1 ;
	}

	// get shared memory segment with my_id as well...
	errno = 0 ;
	my_mem_id = createShmSegment(SL3_SEGMENT_ID(my_id), SL3_SEG_BYTES) ;
	if(my_mem_id < 0) {
		LOG(ERR,"SL3 %3d: problem with memory segment[%s]",my_id,strerror(errno)) ;
		return -1 ;
	}

	// get to local virtual space
	errno = 0 ;
	in_mem = getShmPtr(SL3_SEGMENT_ID(my_id),0) ;
	if(!in_mem) {
		LOG(ERR,"SL3 %3d: can't connect to virt mem [%s]",my_id,strerror(errno)) ;
		return -1 ;
	}

	sprintf(sem_name,SL3_SEM_NAME,my_id) ;
	errno = 0 ;
	in_mem_sem = sem_open(sem_name,0,0666,0) ;	// this is an existing semaphore!
	if(in_mem_sem == SEM_FAILED) {
		LOG(ERR,"SL3 %d: sem %s create failed [%s]",my_id,sem_name,strerror(errno)) ;
	}

	// allocate output (track) memory
	errno = 0 ;
	out_mem = (char *)valloc(SL3_TRK_BYTES) ;
	if(!out_mem) {
		LOG(ERR,"SL3 %3d: can't valloc track memory [%s]",my_id,strerror(errno)) ;
		return -1 ;
	}


	// lock all memory (must be root for this to work)...
	errno = 0 ;
	ret = mlockall(MCL_CURRENT | MCL_FUTURE) ;
	if(ret) {
		LOG(NOTE,"SL3 %d: mlockall failed [%s]",my_id,strerror(errno)) ;
	}


	tracer = 1  ;
	LOG(INFO,"SL3 %d: shared memory of %d bytes, track memory of %d bytes, build %s",my_id,SL3_SEG_BYTES,SL3_TRK_BYTES,BuildDate) ;



	// and start listening for clusters...
	for(;;) {
		int cmd ;


		tracer = 100 ;
		ret = que->receive(&ic,sizeof(ic),1) ;	// blocking read...

		
		cmd = ic.head.daq_cmd ;

		LOG(DBG,"SL3 %3d: cmd %d received [ret %d]",my_id,cmd,ret) ;
		if(ret <= 0) {
			LOG(ERR,"Sl3 %d: queue returns %d -- ending!",my_id,ret) ;
			return -1 ;
		}

		switch(cmd) {
		case DAQ_SEND_CONFIG :
			do_config(&ic) ;
			break ;
		case DAQ_RUN_START :
			do_run_start(ic.ld.dword[0]) ;
			break ;
		case DAQ_RUN_STOP :
			do_run_stop() ;
			if(!use_gl3) return 0 ;	// only in emulation!
			break ;			
		case DET_START_SL3 :
			if(!run_started) {
				LOG(ERR,"Can't track, not running") ;
				break ;
			}
			if(ic.head.dst_task != my_id) {
				LOG(ERR,"SL3 %d: got task %d!?",my_id,ic.head.dst_task);
			}

			LOG(NOTE,"SL3 %3d: started token %d",my_id,ic.head.token) ;

			tracer = 1000 ;
			ret = do_track(&ic, in_mem, out_mem, SL3_TRK_BYTES) ;
			tracer = 1001 ;

			LOG(NOTE,"SL3 %3d: token %d done, %d bytes",my_id,ic.head.token,ret) ;
			
			do_send(out_mem, ret) ;
			tracer = 1002 ;

			break ;
		default :
			LOG(ERR,"SL3 %3d: unknown cmd %d",my_id,cmd) ;
			break ;
		}

	}

	return -1 ;
}


/*
	Sets up the configuration of this particular run.
	I.e. B field, drif velocity etc...
*/

int do_config(ic_msg *ic)
{
	if(sl3_tracker) {
		delete sl3_tracker ;
		sl3_tracker = 0 ;
	}

	sector = ic->head.src_task ;	// note the hack! -- starts from 1!!!

	// get field from file or something...
	// right now we hardcode!

	LOG(NOTE,"SL3 %d: Creating online_tracking_sector for sector %d",my_id,sector) ;

	sl3_tracker = new tof_hlt_sector() ;
	sl3_tracker->sector = sector ;

	memset(gl3_state,0,sizeof(gl3_state)) ;
	gl3_num = 0 ;
	int gl3_failed = 0 ;

	if(use_gl3) {
		// right now we only go to the demo...
		hostent *hent = gethostbyname(GL3_DEMO_IP) ;
		if(hent == 0) {
			LOG(ERR,"SL3 %d: unknown GL3 host %s -- will not send data!",my_id,GL3_DEMO_IP) ;
			goto err_end ;
		}

		u_int ip = ntohl(*(u_int *)*(hent->h_addr_list)) ;

		for(int i=0;i<GL3_MAX_TASKS;i++) {
			errno = 0 ;
			int ret = sfs_gl3[gl3_num].mount(ip,GL3_PORT_START+i) ;
			if(ret < 0) {
				LOG(WARN,"Sl3 %d: GL3 %d [ip 0x%08X:%d] skipped [%s]",my_id,i,ip,GL3_PORT_START+i,strerror(errno)) ;
				gl3_failed++ ;
			}
			else {
				gl3_state[gl3_num] = 1 ;	// OK
				gl3_ip[gl3_num] = ip ;
				gl3_num++ ;
			}
		}
	}

	err_end:;

	LOG(INFO,"SL3 %d: sector %d configured with %d/%d GL3s [%d failed]",my_id,sector,gl3_num,GL3_MAX_TASKS,gl3_failed) ;
	return 0 ;
}



/*
	returns bytes used...
*/
int do_track(ic_msg *ic, char *in, char *out, int max_bytes)
{
	int bytes_used = 0 ;

	tracer = 10000 ;

	LOG(NOTE,"SL3 %3d: starting token %4d",my_id,ic->head.token) ;

	sl3_tracker->new_event() ;

	if(in==0) {
		LOG(ERR,"Critical!") ;
	}

	for(int rdo=0;rdo<6;rdo++) {
		u_int off = ic->ld.dword[2*rdo] ;	// in 32 bit words!
		u_int len = ic->ld.dword[2*rdo+1] ;	// in 32 bit words!



		if(len==0) continue ;	// RDO was masked....

		char *start = (char *)((u_int)in + off*4) ;

		if((off*4)>500000) {
			LOG(ERR,"Biiiig offset %d at RDO %d",off,rdo) ;
		}

		tracer = 10001 ;
		tracer = *start ;	// to force a bus error if necessary!

		//LOG(TERR,"SL3 %d: read: sector %2d: RDO %d: off %d, len %d, start %p",my_id,sector,rdo,off,len,start) ;
		
		tracer = 10010 + rdo ;
//#define HLT_BUG_SEARCH
#ifdef HLT_BUG_SEARCH
		int t = ic->head.token ;
		u_int expect = t + (10000 * (rdo+1)) ;
		u_int *i32 = (u_int *) start ;
		if(*i32 != expect) {
			LOG(ERR,"SL3 %d: expect %d, got %d",my_id,expect,*i32) ;
		}	
#else
		sl3_tracker->read_rdo_event(rdo+1, start, len) ;
#endif
		tracer = 10020 + rdo ;

		//LOG(TERR,"SL3 %3d: fromESB done, at hits %d",my_id,sl3_tracker->nHits) ;

	}

	tracer = 10100 ;
	
	int val = 123 ;
	sem_getvalue(in_mem_sem,&val) ;
	if(val != 0) {
		LOG(ERR,"SL3 %d: sem val %d",my_id,val) ;
	}

	errno = 0 ;
	if(sem_post(in_mem_sem)) {
		LOG(ERR,"SL3 %d: sem post [%s]",my_id,strerror(errno)) ;
	}

	tracer = 10101 ;
	HLT_SECTOR_HDR *hdr = (HLT_SECTOR_HDR *) out ;
	out += sizeof(HLT_SECTOR_HDR) ;

	// go!
	bytes_used = sl3_tracker->do_event(out, max_bytes - sizeof(HLT_SECTOR_HDR)) ;	//

	tracer = 10102 ;

	bytes_used += sizeof(HLT_SECTOR_HDR) ;

	if(bytes_used > max_bytes) {
		LOG(ERR,"OOps -- overwrote memory!") ;
	}

	tracer = 10103 ;

	hdr->token = ic->head.token ;	// important: add the correct token!
	hdr->run_number = run_number ;	// this is where we put the run number!
	hdr->det_id = TOF_ID ;
	hdr->sector_1 = sector ;
	hdr->tot_bytes = bytes_used ;
	memset(hdr->reserved,0,sizeof(hdr->reserved)) ;

	tracer = 10104 ;


	LOG(NOTE,"SL3 %3d: ending token %4d, bytes %d",my_id,ic->head.token, bytes_used) ;

	tracer = 10105 ;

	return bytes_used ;
}




/*
	Sends results to GL3!

*/
int do_send(char *mem, int bytes)
{
	iccp2k i2c ;
	int n_iovec ;
	int tot_bytes = 0 ;
	fs_iovec fsiovec[4] ;
//	char sfs_buff[256] ;
	sfs_index sfs ;
	int t ;
//	char name[128] ;
//	int len ;
	int ret ;

	tracer = 11000 ;

	sl3_evts++ ;

	if((sl3_evts % 1000)==0) {
		LOG(INFO,"SL3 %d: send %d events...",my_id,sl3_evts) ;
	}

	n_iovec = 0 ;
	tot_bytes = 0 ;
	
	HLT_SECTOR_HDR *l3 = (HLT_SECTOR_HDR *) mem ;

	t = l3->token ;

	tracer = 11001 ;

	LOG(NOTE,"SL3 %3d: token %d: written %d bytes",my_id, l3->token, bytes) ;

	i2c.cmd = CMD2_DATA ;
	i2c.token = t ;	
	i2c.srcTask = sector - 1 ;	// srcTask needs to count from 0!
	i2c.srcNode = TOF_NODE ;

	// if over ethernet, add the i2c header
	fsiovec[n_iovec].filename = 0 ;
	fsiovec[n_iovec].buff = &i2c ;
	fsiovec[n_iovec].len = sizeof(i2c) ;

	tot_bytes += fsiovec[n_iovec].len ;
	n_iovec++ ;


	// and finally the data
	fsiovec[n_iovec].filename = 0 ;
	fsiovec[n_iovec].buff = mem ;
	fsiovec[n_iovec].len = bytes ;

	tot_bytes += fsiovec[n_iovec].len ;
	n_iovec++ ;



	// do the word calc
	tracer = 11002 ;

	i2c.words = sfs_debug.getwritevsz(&fsiovec[1],n_iovec-1) / 4 ;	// without the eth header and in words...

	int ix ;
	if((ix=choose_gl3(t))>=0) {
		errno = 0 ;
		ret = sfs_gl3[ix].writev(fsiovec,n_iovec) ;
		if(ret != tot_bytes) {
			LOG(ERR,"SL3 %d: GL3 %d: [%s], wrote %d/%d bytes",my_id,ix,strerror(errno),ret,tot_bytes) ;

			// unmount
			tracer = 11003 ;
			sfs_gl3[ix].umount() ;

			// re-mount
			tracer = 11004 ;
			sfs_gl3[ix].mount(gl3_ip[ix], GL3_PORT_START+ix) ;

		}
		else {
			LOG(NOTE,"SL3 %d: GL3 ix %d: T %4d, seq %d: wrote %d bytes",my_id,ix,i2c.token, sl3_evts, i2c.words*4) ;
		}
	}
	else {
		LOG(NOTE,"SL3 %d: GL3 ix none: T %4d, seq %d: wrote %d bytes",my_id,i2c.token, sl3_evts, i2c.words*4) ;
	}

	tracer = 11005 ;

#if 0
	if(sl3_evts < 10) {	// only 100 becase I assume at least 6 guys working...
		tot_bytes = 0 ;
		n_iovec = 0 ;
		
		sprintf(name,"/#%u/tpx/sec%02d/l3_sectp",sl3_evts,sector) ;
		len = sfs.putfileheader(sfs_buff,name,bytes,SFS_ATTR_CD) ;

		fsiovec[n_iovec].filename = 0 ;
		fsiovec[n_iovec].buff = sfs_buff ;
		fsiovec[n_iovec].len = len ;

		tot_bytes += fsiovec[n_iovec].len ;
		n_iovec++ ;


		// and finally the data
		fsiovec[n_iovec].filename = 0 ;
		fsiovec[n_iovec].buff = mem ;
		fsiovec[n_iovec].len = bytes ;

		tot_bytes += fsiovec[n_iovec].len ;
		n_iovec++ ;


		ret = sfs_debug.writev(fsiovec,n_iovec) ;

		if(ret != tot_bytes) {
			LOG(ERR,"Local file [%s]",strerror(errno)) ;
		}

	}
#endif

	return 0 ;
}


	

/*
	This EMULATES what the Sector Broker (DAQ's ESB)
	will actually do. But also reads from a file.
*/
void *sender_thread(void *arg)
{
	ic_msg ic ;
	char *fname = (char *) arg ;
	char *buff_s ;
	int evt_counter = 0 ;
	int ret ;
	int sl3_num = 1 ;

	struct sl3_struct {
		char *shared_mem ;
		sem_t *sem ;
		ipcQClass *que ;
	} sl3[SL3_MAX_TASKS] ;

	for(int i=0;i<sl3_num;i++) {
		char sem_name[64] ;

		sl3[i].que = new ipcQClass(SL3_TASK+i,1,0) ;
		LOG(NOTE,"Created queue 0x%X",SL3_TASK+i) ;

		// get shared memory segment with my_id as well...
		errno = 0 ;
		ret = createShmSegment(SL3_SEGMENT_ID(i), SL3_SEG_BYTES) ;
		if(ret < 0) {
			LOG(ERR,"SL3 %3d: problem with memory segment[%s]",i,strerror(errno)) ;
			exit(-1) ;
		}

		// get to local virtual space
		sl3[i].shared_mem = getShmPtr(SL3_SEGMENT_ID(i),0) ;
		if(!sl3[i].shared_mem) {
			LOG(ERR,"SL3 %3d: can't connect to virt mem [%s]",i,strerror(errno)) ;
			exit(-1) ;
		}
		LOG(NOTE,"Created shared memory 0x%X",SL3_SEGMENT_ID(i)) ;

		sprintf(sem_name,SL3_SEM_NAME,i) ;
		sl3[i].sem = sem_open(sem_name,O_CREAT,0666,1) ;
		if(sl3[i].sem == SEM_FAILED) {
			LOG(ERR,"Can't create semaphore %s [%s]",sem_name,strerror(errno)) ;
		}
		int val ;
		sem_getvalue(sl3[i].sem,&val) ;
		if(val != 1) {
			LOG(WARN,"SL3 %d: semaphore %s has %d != 1?",i,sem_name,val) ;
			sem_post(sl3[i].sem) ;
			sem_getvalue(sl3[i].sem,&val) ;
		}
		LOG(NOTE,"Created semaphore %s [full %d]",sem_name,val) ;
	
	}


	sleep(2) ;	// emulate some wait...

	ic.head.src_task = sector_from_daq ;	// sector!
	ic.head.daq_cmd = DAQ_SEND_CONFIG ;

	for(int i=0;i<sl3_num;i++) {
		ic.head.dst_task = i ;		// instance ID!
		ret = sl3[i].que->send(&ic,sizeof(ic),0) ;
		if(ret) {	// this should be 120
			LOG(NOTE,"ret is %d",ret) ;
		}
	}


	ic.head.daq_cmd = DAQ_RUN_START ;
	ic.ld.dword[0] = 11111 ;	// dummy run

	for(int i=0;i<sl3_num;i++) {
		ret = sl3[i].que->send(&ic,sizeof(ic),0) ;
	}



	ic.head.token = 1 ;	// don't know so I start from 1...
	ic.head.daq_cmd = DET_START_SL3 ;

	daqReader *dr = new daqReader(fname) ;

	while(dr->get(0,0)) {
		daq_dta *dd ;
		int got_some = 0 ;
		int sl3_id ;

		evt_counter++ ;

		LOG(NOTE,"DAQ: got event evt_counter %d",evt_counter) ;

		// choose SL3
		sl3_id = evt_counter % sl3_num ;


		memset(ic.ld.dword,0,sizeof(ic.ld.dword)) ;
		buff_s = sl3[sl3_id].shared_mem ;

		LOG(NOTE,"DAQ: SL3 %d: wating on semaphore...",sl3_id) ;

		// if I took the semaphore I _must_ send it to SL3!
		got_some = 1 ;
		errno = 0 ;
//		ret = semTake(sl3[sl3_id].sem,5) ;
		ret = sem_wait(sl3[sl3_id].sem) ;
		if(ret) {
			LOG(ERR,"DAQ: SL3 %d, T %4d: sem rets %d [%s]",sl3_id,ic.head.token,ret,strerror(errno)) ;			
		}
		else {
			LOG(NOTE,"DAQ: SL3 %d, T %4d: sem rets %d",sl3_id,ic.head.token,ret) ;
		}

		for(int rdo=0;rdo<4;rdo++) {
			dd = dr->det("tof")->get("raw",sector_from_daq,rdo+1) ;
			while(dd && dd->iterate()) {
				got_some = 1 ;
				

				memcpy(buff_s, dd->Byte, dd->ncontent) ;

				ic.ld.dword[2*rdo] = ((u_int)(buff_s - sl3[sl3_id].shared_mem))/4 ;	// offset
				ic.ld.dword[2*rdo+1] = dd->ncontent/4 ;	// words, by usual DAQ standard

				LOG(DBG,"DAQ: rdo %d: offset %d, len %d",rdo,ic.ld.dword[2*rdo],ic.ld.dword[2*rdo+1]) ;

				buff_s += dd->ncontent ;
			}
		}

		if(got_some) {

			ret = sl3[sl3_id].que->send(&ic, sizeof(ic), 0) ;

			if(ret < 0) {
				LOG(ERR,"Error in queue send") ;
			}
			else {
				LOG(TERR,"DAQ: Sent event %d, token %d, tot bytes %d",evt_counter, ic.head.token,buff_s - sl3[sl3_id].shared_mem) ;
			}

			int token = ic.head.token ;
			token++ ;
			if(token >= 4096) token = 1 ;
			ic.head.token = token ;
			
			//LOG(DBG,"DAQ: Taking sem...") ;
			//semTake(sl3[sl3_id].sem, WAIT_FOREVER) ;
			//LOG(DBG,"DAQ: Took sem.") ;
		}
		


	}

	LOG(INFO,"Done with file!") ;
	ic.head.daq_cmd = DAQ_RUN_STOP ;

	for(int i=0;i<sl3_num;i++) {
		sl3[i].que->send(&ic, sizeof(ic), 0) ;
	}

	for(;;) sleep(10) ;	// wait for SL3 to exit...
}


