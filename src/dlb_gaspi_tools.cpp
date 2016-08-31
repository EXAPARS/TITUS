/* 
* This file is part of the TITUS software.
* https://github.com/exapars/
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


//====================================================================================
//
//  AUTHOR  :   UEMURA Seijilo, FONTNAILLE Clément, PETIT Eric 
//  FILE	: dlb_gaspi_tools.cpp
//  CONTENT	:
//
//====================================================================================


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <GASPI.h>
#include <cstdint>
#include <DLB.hpp>
#include <cassert>
#include <dlb_gaspi_tools.hpp>
#include "dlb_logger.hpp"


pthread_t dlb_main_thread_id;

void TITUS_gaspi_logger::add_proc_rank_str(std::ostream & out){

	static int rank=0;

	gaspi_number_t gaspi_is_init; assert(gaspi_initialized(&gaspi_is_init) == GASPI_SUCCESS);
	if (gaspi_is_init){ // if gaspi is init, use gaspi rank
		gaspi_rank_t gaspi_rank; gaspi_proc_rank(&gaspi_rank);
		rank = gaspi_rank;
	}
	
//! TODO : figure out a way to handle printing mpi rank if available without forcing mpi inclusion in TITUS, move this feature in DLB or DLB_Context or comm layer management
//~ #ifdef MPI // mpi interoperability mode
	//~ else if {
		//~ int mpi_is_init; MPI_Initialized(&mpi_is_init);
		//~ if (mpi_is_init) // if mpi is init, use mpi rank instead
			//~ MPI_Comm_rank(MPI_COMM_WORLD, rank);
	//~ }
//~ #endif
	else return; // no communication library initialized, add_proc_rank_str does not output anything.
	
	out << "rank " << rank << " : ";

}

void TITUS_gaspi_logger::flush(){
	double now = ((double)(rdtsc() - timeref)) / TITUS_PROC_FREQ; // time, in seconds, elapsed since last call to reset_timeref()
	if (str().empty()) {
		return;
	}

	if (use_gaspi_logger){
		gaspi_printf(str().c_str());
	}
	else {
		std::stringstream msg("");
		msg << std::fixed << std::setfill('0') << std::setw(14) << std::setprecision(8) << now << " ";
		add_proc_rank_str(msg); msg << str();
		*out << msg;
	}
	str("");
}


void TITUS_gaspi_logger_simplistic::add_proc_rank_str(std::ostream & out){

	int rank;

	gaspi_number_t gaspi_is_init; assert(gaspi_initialized(&gaspi_is_init) == GASPI_SUCCESS);
	if (gaspi_is_init){ // if gaspi is init, use gaspi rank
		gaspi_rank_t gaspi_rank; gaspi_proc_rank(&gaspi_rank);
		rank = gaspi_rank;
	}
	else return; // no communication library initialized, add_proc_rank_str does not output anything.
	
	out << "[rank " << rank << "] : ";

}

std::ostream & TITUS_gaspi_logger_simplistic::print_header(){
	std::stringstream header("");
	double now = ((double)(rdtsc() - timeref)) / TITUS_PROC_FREQ; // time, in seconds, elapsed since last call to reset_timeref()
	header << std::fixed << std::setfill('0') << std::setw(11) << std::setprecision(5) << now << " ";
	add_proc_rank_str(header);
	return *out << header;
}


#ifdef TITUS_DBG_SIMPLISTIC
TITUS_gaspi_logger_simplistic * m_TITUS_DBG = nullptr;
TITUS_gaspi_logger_simplistic & get_TITUS_DBG(){
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger_simplistic();
	return *m_TITUS_DBG;
}
#endif //TITUS_DBG_SIMPLISTIC

#ifdef TITUS_DBG_WRAPPED_SIMPLISTIC
TITUS_gaspi_logger_simplistic * m_TITUS_DBG = nullptr;
std::ostream * get_TITUS_DBG(){
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger_simplistic();
	m_TITUS_DBG->print_header();
	return m_TITUS_DBG;
}
#endif //TITUS_DBG_WRAPPED_SIMPLISTIC
#ifdef TITUS_DBG_WRAPPED
TITUS_gaspi_logger * get_TITUS_DBG(){
	static TITUS_gaspi_logger * m_TITUS_DBG = nullptr;
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger();
	return m_TITUS_DBG;
}
#endif //TITUS_DBG_WRAPPED

dlb_sig_handler::dlb_sig_handler(){
	_is_init = true;
	dlb_main_thread_id = pthread_self();
	//std::cout << "found main tid = " << dlb_main_thread_id << std::endl;
	//signal(SIGKILL,print_stacktrace_and_quit); // cannot be caught
	signal(SIGABRT,print_stacktrace_and_quit);
	signal(SIGTERM,print_stacktrace_and_quit);
	signal(SIGSEGV,print_stacktrace_and_quit);
	signal(SIGQUIT,print_stacktrace_and_quit);
	signal(SIGINT,print_stacktrace_and_quit);
	signal(SIGUSR1,print_stacktrace_and_quit);
	signal(SIGUSR2,print_stacktrace_and_quit);
	//std::cout << "dlb_sig_handler initialized !" << std::endl;
}

#define UNW_LOCAL_ONLY
#include "libunwind-x86_64.h"
// backtrace with libunwind
//void print_stacktrace (unw_addr_space_t dlb_unw_target_thread_addr_space,  unw_context_t * p_dlb_unw_target_thread_context) {
void print_stacktrace () {

	unw_cursor_t cursor; 
	unw_word_t ip, sp, offp;
	char mangled_name[512];
	//  size_t unmangled_name_size = 256;
	//  char unmangled_name[unmangled_name_size];
	int status;

	unw_context_t uc;
	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);
	//int unw_init_remote(&cursor , dlb_unw_target_thread_addr_space, p_dlb_unw_target_thread_context);

	gaspi_rank_t rank; gaspi_proc_rank(&rank);
	int ignored_steps = 0;
	int i = 0;
	do {
		if (ignored_steps > i){ i++ ; continue;}
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		mangled_name[0] = '\0';
		unw_get_proc_name (&cursor, mangled_name, 512, &offp);
		char* unmangled_name = abi::__cxa_demangle( mangled_name, nullptr, nullptr, &status ); // doesnt seem to work :/
		if (status == 0)
			TITUS_DBG << "bt[" << i++ << "] " << unmangled_name << " : ip=" << ip << ", sp=" << sp << std::endl;
		else
			TITUS_DBG << "bt[" << i++ << "] " << mangled_name << " : ip=" << ip << ", sp=" << sp << std::endl;
		free(unmangled_name);
	}
	while (unw_step(&cursor) > 0);
}

bool gaspi_is_init(){ // non-standard (from GASPI_Ext.h)
	gaspi_number_t r;
	gaspi_initialized (&r);
	return (r != 0);
}

void print_stacktrace_and_quit(int sig){
	if (dlb_main_thread_id == 0){
		TITUS_DBG << "WARNING : dlb_main_thread_id was not set" << std::endl;
	}
	else {
		if (pthread_self() != dlb_main_thread_id){
			TITUS_DBG << "print_stacktrace_and_quit : got sig " << sig << " in thread " << pthread_self() << ", killing main thread (id = " << dlb_main_thread_id << ")" << std::endl;
			pthread_kill(dlb_main_thread_id, sig);
			usleep(60000000); // 1min
			exit(sig);
		}
	TITUS_DBG << "print_stacktrace_and_quit : got sig " << sig << " in main thread " << pthread_self() << std::endl;
	}
	//print_stacktrace(dlb_sig_handler::dlb_unw_main_thread_addr_space, & dlb_sig_handler::dlb_unw_main_thread_context);
	print_stacktrace();
	
	gaspi_rank_t rank = (gaspi_rank_t)-1;
	if (gaspi_is_init())  gaspi_proc_rank(&rank);
	std::stringstream outname("");
	outname << "cleanup_logs_" << std::setfill('0') << std::setw(4) << ((rank == (gaspi_rank_t)-1)?(gaspi_rank_t)rdtsc()&0xFFFF : rank) << ".out";
	std::ostream * out = new std::ofstream(outname.str().c_str());
	DLB::get_context()->get_logger()->print_all_sessions(*out);

	DLB_Context * dlb_ctx = DLB::get_context();
	dlb_ctx->get_logger()->m_impl->signal_end_parallel_work_session();
	if (dlb_ctx != nullptr)  TITUS_DBG << *dlb_ctx;
	//TITUS_DBG << std::endl;
	TITUS_DBG.flush();
	exit(sig);
}

std::ostream & operator<< (std::ostream & out, std::stringstream const& msg){
	return out << msg.str();
}

void print_gaspi_config(std::ostream & out){
	gaspi_config_t config; gaspi_config_get(&config);
	out << "logger=" << config.logger << std::endl;
	out << "sn_port=" << config.sn_port << std::endl;
	out << "net_info=" << config.net_info << std::endl;
	out << "netdev_id=" << config.netdev_id << std::endl;
	out << "mtu=" << config.mtu << std::endl;
	out << "port_check=" << config.port_check << std::endl;
	out << "user_net=" << config.user_net << std::endl;
	out << "network=" << config.network << std::endl;
	out << "queue_depth=" << config.queue_depth << std::endl;
	out << "queue_num=" << config.queue_num << std::endl;
	out << "group_max=" << config.group_max << std::endl;
	out << "segment_max=" << config.segment_max << std::endl;
	out << "transfer_size_max=" << config.transfer_size_max << std::endl;
	out << "notification_num=" << config.notification_num << std::endl;
	out << "passive_queue_size_max=" << config.passive_queue_size_max << std::endl;
	out << "passive_transfer_size_max=" << config.passive_transfer_size_max << std::endl;
	out << "allreduce_buf_size=" << config.allreduce_buf_size << std::endl;
	out << "allreduce_elem_max=" << config.allreduce_elem_max << std::endl;
	out << "build_infrastructure=" << config.build_infrastructure << std::endl;
}
// allocates and returns a printable string representing the state of the gaspi_config structure
void print_gaspi_config(char ** out){
	std::stringstream sstr("");
	print_gaspi_config(sstr);
	std::string str = sstr.str();
	*out = (char * ) malloc(str.size()+1); ASSERT(out != nullptr);
	strcpy(*out, str.c_str());
}


inline void wait_for_queue_entries (gaspi_queue_id_t* queue, unsigned long wanted_entries)
{
	gaspi_number_t queue_size_max;
	gaspi_number_t queue_size;
	gaspi_number_t queue_num;

	SUCCESS_OR_DIE (gaspi_queue_size_max (&queue_size_max));
	SUCCESS_OR_DIE (gaspi_queue_size (*queue, &queue_size));
	SUCCESS_OR_DIE (gaspi_queue_num (&queue_num));

	if (! (queue_size + wanted_entries <= queue_size_max))
	{
		*queue = (*queue + 1) % queue_num;
		SUCCESS_OR_DIE (gaspi_wait (*queue, DLB_GASPI_TIMEOUT));
	}
}

void wait_for_queue_entries_for_write_notify (gaspi_queue_id_t* queue_id)		{	wait_for_queue_entries (queue_id, 2);	}
void wait_for_queue_entries_for_notify (gaspi_queue_id_t* queue_id)			    {	wait_for_queue_entries (queue_id, 1);	}

void wait_for_flush_queues ()
{
	gaspi_number_t queue_num;
	SUCCESS_OR_DIE (gaspi_queue_num (&queue_num));

	gaspi_queue_id_t queue = 0;
	while( queue < queue_num )
	{
		SUCCESS_OR_DIE (gaspi_wait (queue, DLB_GASPI_TIMEOUT));
		++queue;
	}
}

void wait_or_die( gaspi_segment_id_t segment_id, gaspi_notification_id_t notification_id, gaspi_notification_t expected	)
{
	gaspi_notification_id_t id;
	SUCCESS_OR_DIE(gaspi_notify_waitsome (segment_id, notification_id, 1, &id, DLB_GASPI_TIMEOUT));
	ASSERT (id == notification_id);

	gaspi_notification_t value;
	SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, id, &value));
	ASSERT (value == expected);
}

gaspi_return_t global_try_lock(const gaspi_segment_id_t seg, const gaspi_offset_t off, const gaspi_rank_t rank_loc, const gaspi_timeout_t timeout )
{
	gaspi_rank_t iProc;
	SUCCESS_OR_DIE (gaspi_proc_rank (&iProc));
	gaspi_atomic_value_t old_value;
	if(gaspi_atomic_compare_swap( seg, off, rank_loc, VAL_UNLOCKED, iProc, &old_value, timeout) == GASPI_ERROR)
	{
		return GASPI_ERROR;
	}
	return (old_value == VAL_UNLOCKED) ? GASPI_SUCCESS : GASPI_ERROR;
}

gaspi_return_t global_unlock ( const gaspi_segment_id_t seg, const gaspi_offset_t off, const gaspi_rank_t rank_loc, const gaspi_timeout_t timeout)
{
	gaspi_rank_t iProc;
	SUCCESS_OR_DIE (gaspi_proc_rank (&iProc));
	gaspi_atomic_value_t current_value;
	SUCCESS_OR_DIE ( gaspi_atomic_compare_swap(seg, off, rank_loc, iProc, VAL_UNLOCKED, &current_value, timeout));
	return GASPI_SUCCESS;
}
