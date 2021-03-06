/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2017 University of Versailles UVSQ - Exascale Computing Research
* Copyright (c) 2017 Bull SAS
*
* TITUS  is a free software: you can redistribute it and/or modify  
* it under the terms of the GNU Lesser General Public License as   
* published by the Free Software Foundation, version 3.
*
* TITUS is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __GASPI_TOOLS_H__
#define __GASPI_TOOLS_H__

#include <GASPI.h>
#include <GASPI_Ext.h>
#include <stdlib.h>
#include <cstdint>
#include <execinfo.h>
#include <unistd.h>
#include <sys/time.h>

#include <iomanip>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>

#include <cxxabi.h>

#include <rdtsc.h>
#include <TITUS_DBG.hpp>



#define TITUS_DLB_GASPI_TIMEOUT GASPI_BLOCK
//#define TITUS_DLB_GASPI_TIMEOUT GASPI_BLOCK

#if GASPI_MAJOR_VERSION==1 && GASPI_MINOR_VERSION<3
#define GASPI_TOPOLOGY_NONE 0
#define GASPI_TOPOLOGY_STATIC 1
#define GASPI_TOPOLOGY_DYNAMIC 2
#endif

static inline const char * gaspi_topology_str(const gaspi_topology_t & arg){
	switch (arg){
		case GASPI_TOPOLOGY_NONE : return "GASPI_TOPOLOGY_NONE"; break;
		case GASPI_TOPOLOGY_STATIC : return "GASPI_TOPOLOGY_STATIC"; break;
		case GASPI_TOPOLOGY_DYNAMIC : return "GASPI_TOPOLOGY_DYNAMIC"; break;
		default : return "UNKNOWN_TOPOLOGY_VALUE"; break;
	}
}
static inline const char * gaspi_network_str(const gaspi_network_t & arg){
	switch(arg){
      case GASPI_IB : return "GASPI_IB";
      case GASPI_ROCE : return "GASPI_ROCE";
      case GASPI_ETHERNET : return "GASPI_ETHERNET";
      case GASPI_GEMINI : return "GASPI_GEMINI";
      case GASPI_ARIES : return "GASPI_ARIES";
      default : return "UNKNOWN gaspi_network_t VALUE";
  }
}


#define VAL_UNLOCKED 9999999L
#define TIMEOUT_TRY_LOCK 0

#define ADD_PTR(p,n) (void *)((char*)p + n)
#define DIFF_PTR(p,n) (size_t)(((size_t)p) - ((size_t)n))

#define typeofthis std::remove_reference<decltype(*this)>::type


#define UNW_LOCAL_ONLY
#include "libunwind-x86_64.h"
// backtrace with libunwind
extern void print_stacktrace (unw_addr_space_t * p_TITUS_DLB_unw_target_thread_addr_space,  unw_context_t * p_TITUS_DLB_unw_target_thread_context);

extern void print_stacktrace_and_quit(int sig);


class TITUS_DLB_sig_handler{
public:
	//static unw_addr_space_t TITUS_DLB_unw_main_thread_addr_space;	
	//static unw_context_t TITUS_DLB_unw_main_thread_context;	
	//TITUS_DLB_sig_handler * instance;
	TITUS_DLB_sig_handler();
	bool is_init() { return _is_init; }
private :
	bool _is_init;
};



/* //backtrace without libunwind (quite unreadable, mangled and with binary offsets rather than lineno)
inline static void print_stacktrace(){
	void *array[50];
	size_t size;
	// get void*'s for all entries on the stack
	size = backtrace(array, 50);
	
	// print out all the frames to stderr
	backtrace_symbols_fd(array, size, STDERR_FILENO);
}
*/
#define SUCCESS_OR_DIE(f...)                                                                        \
do                                                                                                  \
{                                                                                                   \
    uint64_t f_called = rdtsc();                                                                    \
    const gaspi_return_t __r__ = f;                                                                 \
    uint64_t f_returned = rdtsc();                                                                  \
    double time_ms = (double)(f_returned - f_called) / (((double)TITUS_PROC_FREQ) / 1e3);           \
    if (time_ms > 10.0){                                                                            \
		char str[512];                                                                              \
		sprintf (str, "Warning : %f ms in '%s' [%s:%i]\n", time_ms, #f, __FILE__, __LINE__);        \
		TITUS_DBG << str;                                                                           \
	}                                                                                               \
	if (__r__ != GASPI_SUCCESS)                                                                     \
    {                                                                                               \
        printf("SUCCESS_OR_DIE failed at [%s:%i].\n",__FILE__,__LINE__);                            \
        gaspi_rank_t rank;                                                                          \
        gaspi_proc_rank(&rank);                                                                     \
        fprintf (stderr,"\nError rank %d: '%s' [%s:%i]: %i\n",rank, #f, __FILE__, __LINE__, __r__); \
        fprintf (stderr," \n Error %i : %s\n",__r__, gaspi_error_str(__r__));                       \
        print_stacktrace_and_quit(-1);                                                              \
        abort();                                                                                    \
    }                                                                                               \
} while (0);

#define ASSERT(x...)                                                                    \
if (!(x))                                                                               \
{                                                                                       \
	gaspi_rank_t rank;                                                                  \
    gaspi_proc_rank(&rank);                                                             \
    fprintf (stderr, "Error rank %d : '%s' [%s:%i]\n", rank, #x, __FILE__, __LINE__);   \
    fflush (stderr);                                                                    \
    print_stacktrace_and_quit(-1);                                                      \
    abort();                                                                            \
}

static void wait_if_queue_full(const gaspi_queue_id_t queue_id, gaspi_number_t request_size){
	// wait until request_size fits in queue
	gaspi_number_t queue_size_max; SUCCESS_OR_DIE(gaspi_queue_size_max(&queue_size_max));
	gaspi_number_t queue_size; SUCCESS_OR_DIE(gaspi_queue_size(queue_id, &queue_size));
	if (request_size == 0) request_size = queue_size_max - queue_size_max/2;
	const uint64_t msg_interval = 1000;//ms
	uint64_t wait_start = rdtsc(); uint64_t next_msg = msg_interval;
	while (queue_size + request_size >= queue_size_max){

		gaspi_return_t e = gaspi_wait(queue_id,2);
		if (e == GASPI_SUCCESS) break;
		if (e != GASPI_TIMEOUT) SUCCESS_OR_DIE(e);

		uint64_t wait_time = rdtsc() - wait_start;
		if (wait_time / (TITUS_PROC_FREQ/1000) > next_msg){
			next_msg += msg_interval;
			TITUS_DBG << "wait_if_queue_full : WARNING : waiting for queue to flush for " << (rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) << "ms (" << queue_size << " / " << queue_size_max << " elts in queue)" << std::endl;
		}
		
		gaspi_number_t queue_size_max; SUCCESS_OR_DIE(gaspi_queue_size_max(&queue_size_max));
		gaspi_number_t queue_size; SUCCESS_OR_DIE(gaspi_queue_size(queue_id, &queue_size));
	}
}


// from CFD-PROXY
// more information and copyright licence available at https://github.com/PGAS-community-benchmarks/CFD-Proxy
static void wait_for_queue_max_half (gaspi_queue_id_t* queue)
{
  gaspi_number_t queue_size_max;
  gaspi_number_t queue_size;

  SUCCESS_OR_DIE (gaspi_queue_size_max (&queue_size_max));
  SUCCESS_OR_DIE (gaspi_queue_size (*queue, &queue_size));

  if (queue_size >= queue_size_max/2)
    {
      SUCCESS_OR_DIE (gaspi_wait (*queue, GASPI_BLOCK));
    }

}
//~ class debug_msg{
	//~ std::stringstream msg;
//~ public :
	//~ debug_msg():msg(""){}
	//~ ~debug_msg(){gaspi_printf(msg.str().c_str());}
	//~ template <typename T>
	//~ operator <<(const T &arg){msg << arg;}
//~ };


std::ostream & operator<< (std::ostream & out, std::stringstream const& msg);
void print_gaspi_config(std::ostream & out = std::cout);
void print_gaspi_config(char ** out);


void wait_for_queue_entries (gaspi_queue_id_t* queue, unsigned long wanted_entries);
void wait_for_queue_entries_for_write_notify (gaspi_queue_id_t* queue_id);
void wait_for_queue_entries_for_notify (gaspi_queue_id_t* queue_id);
void wait_for_flush_queues ();
void wait_or_die( gaspi_segment_id_t segment_id, gaspi_notification_id_t notification_id, gaspi_notification_t expected    );
gaspi_return_t global_try_lock(const gaspi_segment_id_t seg, const gaspi_offset_t off, const gaspi_rank_t rank_loc, const gaspi_timeout_t timeout );
gaspi_return_t global_unlock ( const gaspi_segment_id_t seg, const gaspi_offset_t off, const gaspi_rank_t rank_loc, const gaspi_timeout_t timeout);


#endif
