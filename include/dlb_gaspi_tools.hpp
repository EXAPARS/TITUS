/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2016 University of Versailles UVSQ
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

#ifndef TITUS_PROC_FREQ
#define TITUS_PROC_FREQ          2.4e9
#endif

#define DLB_GASPI_TIMEOUT GASPI_BLOCK
//#define DLB_GASPI_TIMEOUT GASPI_BLOCK

#if GASPI_MAJOR_VERSION==1 && GASPI_MINOR_VERSION<3
#define GASPI_TOPOLOGY_NONE 0
#define GASPI_TOPOLOGY_STATIC 1
#define GASPI_TOPOLOGY_DYNAMIC 2
#endif


#define VAL_UNLOCKED 9999999L
#define TIMEOUT_TRY_LOCK 0

#define ADD_PTR(p,n) (void *)((char*)p + n)
#define DIFF_PTR(p,n) (size_t)(((size_t)p) - ((size_t)n))

#define typeofthis std::remove_reference<decltype(*this)>::type

static inline uint64_t rdtsc()
{
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}

static inline double now()
{
	struct timeval tp;
	if (gettimeofday (&tp, NULL) < 0)	{	perror ("gettimeofday failed");		}
	return (double) tp.tv_sec + ((double) tp.tv_usec) * 1.e-6;
}



class TITUS_gaspi_logger_simplistic{
	std::ostream * out;
	uint64_t timeref;
	static void add_proc_rank_str(std::ostream & out); // appends "rank [rankno] : " to output stream
public:
	// uses either gaspi_logger or cerr
	TITUS_gaspi_logger_simplistic(std::ostream & out = std::cerr) : out(&out){reset_timeref();}

	virtual ~TITUS_gaspi_logger_simplistic(){std::cerr << "deleting TITUS_gaspi_logger_simplistic" << std::endl;}

	void reset_timeref(){ timeref = rdtsc(); }
	void set_output(std::ostream * out){ this->out = out; }
	
	std::ostream & print_header ();
	
	void flush(){};
	
	operator std::ostream&(){return *out;}
	
	template <typename T>
	std::ostream & operator << (const T & arg){ return print_header() << arg;}
};


class TITUS_gaspi_logger : public std::stringstream{
	bool use_gaspi_logger;
	std::ostream * out;
	uint64_t timeref;
	static void add_proc_rank_str(std::ostream & out); // appends "rank [rankno] : " to output stream
public:
	// uses either gaspi_logger or cerr
	TITUS_gaspi_logger(bool use_gaspi_logger = false) : std::stringstream(""),use_gaspi_logger(use_gaspi_logger),out(&std::cerr){reset_timeref();}
	// uses custom ostream
	TITUS_gaspi_logger(std::ostream & out) : std::stringstream(""),use_gaspi_logger(false),out(&out){reset_timeref();}
	
	virtual ~TITUS_gaspi_logger(){flush(); std::cerr << "deleting TITUS_gaspi_logger" << std::endl;}

	void flush();

	void reset_timeref(){ timeref = rdtsc(); }
	void set_output(std::ostream & out){ this->out = &out; }
};

#define TITUS_DBG_SIMPLISTIC
//#define TITUS_DBG_WRAPPED

//#define TITUS_DBG (std::cerr)
#ifdef TITUS_DBG_SIMPLE
#define TITUS_DBG (std::cerr)
#endif // TITUS_DBG_SIMPLE

#ifdef TITUS_DBG_ORIG
static TITUS_gaspi_logger _TITUS_GASPI_LOGGER;
#define TITUS_DBG (_TITUS_GASPI_LOGGER)
#endif // TITUS_DBG_ORIG

#ifdef TITUS_DBG_WRAPPED
extern TITUS_gaspi_logger * get_TITUS_DBG();
#define TITUS_DBG (*get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED

#ifdef TITUS_DBG_SIMPLISTIC
extern TITUS_gaspi_logger_simplistic & get_TITUS_DBG();
extern TITUS_gaspi_logger_simplistic * m_TITUS_DBG;
#define TITUS_DBG (get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED_SIMPLISTIC

#ifdef TITUS_DBG_WRAPPED_SIMPLISTIC
extern std::ostream & get_TITUS_DBG();
extern TITUS_gaspi_logger_simplistic * m_TITUS_DBG;
#define TITUS_DBG (get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED_SIMPLISTIC


#define UNW_LOCAL_ONLY
#include "libunwind-x86_64.h"
// backtrace with libunwind
extern void print_stacktrace (unw_addr_space_t * p_dlb_unw_target_thread_addr_space,  unw_context_t * p_dlb_unw_target_thread_context);

extern void print_stacktrace_and_quit(int sig);


class dlb_sig_handler{
public:
	//static unw_addr_space_t dlb_unw_main_thread_addr_space;	
	//static unw_context_t dlb_unw_main_thread_context;	
	//dlb_sig_handler * instance;
	dlb_sig_handler();
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
    const gaspi_return_t __r__ = f;                                                                 \
    if (__r__ != GASPI_SUCCESS)                                                                     \
    {                                                                                               \
        printf("SUCCESS_OR_DIE failed at [%s:%i].\n",__FILE__,__LINE__);                            \
        gaspi_rank_t rank;                                                                          \
        gaspi_proc_rank(&rank);                                                                     \
        fprintf (stderr,"\nError rank %d: '%s' [%s:%i]: %i\n",rank, #f, __FILE__, __LINE__, __r__); \
        fprintf (stderr," \n Error %i : %s\n",__r__, gaspi_error_str(__r__));                       \
        print_stacktrace_and_quit(-1);                                                              \
        abort();                                                                        \
    }                                                                                               \
} while (0);

#define ASSERT(x...)                                                                    \
if (!(x))                                                                               \
{                                                                                       \
	gaspi_rank_t rank;                                                                  \
    gaspi_proc_rank(&rank);                                                             \
    fprintf (stderr, "Error rank %d : '%s' [%s:%i]\n", rank, #x, __FILE__, __LINE__);   \
    print_stacktrace_and_quit(-1);                                                      \
    abort();                                                                \
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
